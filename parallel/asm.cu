#include "asm.h"

#define MAX_QUERY_LENGTH 1500
#define SHARED_SIZE 4000

#define BLOCK_NUM 60
#define THREAD_NUM 512

#define TEST_SEQ 39

#define INF 1e10

#define dist(x,y) (fabs(x-y))

__constant__ float query_gpu[MAX_QUERY_LENGTH];

float* Asm(float *query,int qline,int seq_num,float *all_seq,int all_length,int *seq_length,int *accu_length,float r)
{
    float t1,t2;
    float *all_seq_gpu;
    int *seq_length_gpu;
    int *accu_length_gpu;

    if(qline<=MAX_QUERY_LENGTH)
    {
        cutilSafeCall(cudaMemcpyToSymbol(query_gpu,query,qline*sizeof(float)));
    }
    else//limit the length of query to 2K
    {
        cutilSafeCall(cudaMemcpyToSymbol(query_gpu,query,MAX_QUERY_LENGTH*sizeof(float)));
        qline=MAX_QUERY_LENGTH;
    }
    //printf("query length is %d\n",qline);


    cutilSafeCall(cudaMalloc((void **)&all_seq_gpu,sizeof(float)*all_length));
    cutilSafeCall(cudaMemcpy(all_seq_gpu,all_seq,all_length*sizeof(float),cudaMemcpyHostToDevice));

    cutilSafeCall(cudaMalloc((void **)&seq_length_gpu,sizeof(int)*seq_num));
    cutilSafeCall(cudaMemcpy(seq_length_gpu,seq_length,seq_num*sizeof(int),cudaMemcpyHostToDevice));

    cutilSafeCall(cudaMalloc((void **)&accu_length_gpu,sizeof(int)*seq_num));
    cutilSafeCall(cudaMemcpy(accu_length_gpu,accu_length,seq_num*sizeof(int),cudaMemcpyHostToDevice));

    //malloc space for the variables used in asm
    float *cost_gpu;
    int *start_gpu;
    int *x_gpu;
    int *y_gpu;
    cutilSafeCall(cudaMalloc((void **)&cost_gpu,sizeof(float)*qline*seq_num*3));
    cutilSafeCall(cudaMalloc((void **)&start_gpu,sizeof(int)*qline*seq_num*3));
    cutilSafeCall(cudaMalloc((void **)&x_gpu,sizeof(int)*qline*seq_num*3));
    cutilSafeCall(cudaMalloc((void **)&y_gpu,sizeof(int)*qline*seq_num*3));

    int *left_bound_gpu,*right_bound_gpu;
    cutilSafeCall(cudaMalloc((void **)&left_bound_gpu,sizeof(int)*qline));
    cutilSafeCall(cudaMalloc((void **)&right_bound_gpu,sizeof(int)*qline));


    float *small_gpu;
    float *small;
    cutilSafeCall(cudaMalloc((void **)&small_gpu,sizeof(float)*seq_num));
    small=(float*)malloc(sizeof(float)*seq_num);

    bound<<<BLOCK_NUM,THREAD_NUM/4>>>(qline,r,left_bound_gpu,right_bound_gpu);

    /*
    float *dist,*dist_gpu;
    dist=(float*)malloc(sizeof(float)*seq_length[TEST_SEQ]);
    cutilSafeCall(cudaMalloc((void **)&dist_gpu,sizeof(float)*seq_length[TEST_SEQ]));

    float *diagonal,*diagonal_gpu;
    diagonal=(float*)malloc(sizeof(float)*qline);
    cutilSafeCall(cudaMalloc((void **)&diagonal_gpu,sizeof(float)*qline));
    */

    t1=clock();
    _asm<<<seq_num,THREAD_NUM,sizeof(int)*qline*2>>>(qline,all_seq_gpu,seq_length_gpu,
            accu_length_gpu,cost_gpu,start_gpu,x_gpu,y_gpu,
            left_bound_gpu,right_bound_gpu,small_gpu);
    cudaThreadSynchronize();
    t2=clock();
    //printf("cal time is %lf\n",(t2-t1)/CLOCKS_PER_SEC);

    cutilSafeCall(cudaMemcpy(small,small_gpu,sizeof(float)*seq_num,cudaMemcpyDeviceToHost));

    for(int i=0;i<seq_num;i++)
    {
        //printf("%d:%lf\n",i+1,small[i]);
    }

    /*
    cutilSafeCall(cudaMemcpy(dist,dist_gpu,sizeof(float)*seq_length[TEST_SEQ],cudaMemcpyDeviceToHost));

    for(int i=0;i<seq_length[TEST_SEQ];i++)
    {
        printf("%d:%f\n",i+1,dist[i]);
    }

    cutilSafeCall(cudaMemcpy(diagonal,diagonal_gpu,sizeof(float)*qline,cudaMemcpyDeviceToHost));

    printf("diagonal is:\n");
    for(int i=0;i<qline;i++)
    {
        printf("%d:%f\n",i+1,diagonal[i]);
    }
    */

    return small;
}

__global__ static void bound(int qline,float r,int *left_bound_gpu,int *right_bound_gpu)
{
    int tid=threadIdx.x;
    int bid=blockIdx.x;

    int tsize=blockDim.x;
    int bsize=gridDim.x;

    float warp_width=qline*r;

    for(int i=tid+tsize*bid;i<qline;i+=bsize*tsize)
    {
        left_bound_gpu[i]=(int)((i+1)*0.2f+warp_width); 
        right_bound_gpu[i]=(int)((i+1)*0.25f+warp_width); 
    }
}

__device__ inline static float valid_pos(int left_bound,int right_bound, int x,int y,int kind)
{
    switch(kind)
    {
        case 1:
            y++;
            break;
        case 2:
            x++;
            break;
        default:
            break;
    }
    if(x>y&&x-y<=left_bound) return 0;
    else if(x<y&&y-x<=right_bound) return 0;
    else if(x==y) return 0;
    else return INF;
}

__device__ inline static float min(float a,float b,int *index)
{
    if(a<b)
    {
        *index=1;
        return a;
    }
    else
    {
        *index=2;
        return b;
    }
}

__global__ static void _asm(int qline,float *all_seq_gpu,
        int *seq_length_gpu,int *accu_length_gpu,
        float *cost_gpu,int *start_gpu,int *x_gpu,int *y_gpu,
        int *left_bound_gpu,int *right_bound_gpu,float *small_gpu)
{
    int tid=threadIdx.x;
    int bid=blockIdx.x;   
    int tsize=blockDim.x;

    __shared__ int seq_start;
    __shared__ int seq_length;

    extern __shared__ int left_right_bound[];

    __shared__ int out_start;//mark the start position of the cal row of every sequence
    __shared__ int inner_start;//mark the position of pprev,prev and cur

    __shared__ int pprev_start;
    __shared__ int prev_start;
    __shared__ int cur_start;

    __shared__ int start;
    __shared__ int end;

    __shared__ float small;
    //__shared__ int start_pos,end_pos;
    __shared__ float scale_factor;

    if(tid==0)
    {
        seq_start=accu_length_gpu[bid];
        seq_length=seq_length_gpu[bid];
        
        out_start=bid*qline*3;//mark the start position of the cal row 
        inner_start=0;

        pprev_start=0;
        prev_start=(inner_start+1)*qline;
        cur_start=(inner_start+2)*qline;
    }

    __syncthreads();

    for(int i=tid;i<3*qline;i+=tsize)
    {
        cost_gpu[out_start+i]=INF;
        start_gpu[out_start+i]=0;
    }
    __syncthreads();

    for(int i=tid;i<qline;i+=tsize)
    {
        left_right_bound[i]=left_bound_gpu[i];
        left_right_bound[qline+i]=right_bound_gpu[i]; 
    }

    __syncthreads();

    if(tid==0)
    {
        cost_gpu[out_start+pprev_start+qline-1]=dist(query_gpu[0],all_seq_gpu[seq_start]);
        cost_gpu[out_start+prev_start+qline-2]=dist(query_gpu[1],all_seq_gpu[seq_start])+cost_gpu[out_start+pprev_start+qline-1];
        cost_gpu[out_start+prev_start+qline-1]=dist(query_gpu[0],all_seq_gpu[seq_start+1]);

        start_gpu[out_start+pprev_start+qline-1]=1;
        start_gpu[out_start+prev_start+qline-2]=1;
        start_gpu[out_start+prev_start+qline-1]=2;

        x_gpu[out_start+pprev_start+qline-1]=1;
        y_gpu[out_start+pprev_start+qline-1]=1;

        x_gpu[out_start+prev_start+qline-2]=2;
        y_gpu[out_start+prev_start+qline-2]=1;

        x_gpu[out_start+prev_start+qline-1]=1;
        y_gpu[out_start+prev_start+qline-1]=1;

        start=qline-3;
        end=qline-2;

        small=INF;
    }

    __syncthreads();


    for(int i=3;i<qline;i++)
    {
        for(int j=start+tid;j<qline-1;j+=tsize)
        {
            float v1=valid_pos(left_right_bound[i-1-(j-start)],left_right_bound[qline+i-1-(j-start)],x_gpu[out_start+prev_start+j],y_gpu[out_start+prev_start+j],1); 
            float v2=valid_pos(left_right_bound[i-1-(j-start)],left_right_bound[qline+i-1-(j-start)],x_gpu[out_start+prev_start+j+1],y_gpu[out_start+prev_start+j+1],2); 

            int index1,index2;
            float small1=min(cost_gpu[out_start+prev_start+j]+v1,cost_gpu[out_start+prev_start+j+1]+v2,&index1);
            float small2=min(small1,cost_gpu[out_start+pprev_start+j+1],&index2);

            cost_gpu[out_start+cur_start+j]=dist(query_gpu[i-1-(j-start)],all_seq_gpu[seq_start+j-start])+small2;

            if(index2==2)
            {
                start_gpu[out_start+cur_start+j]=start_gpu[out_start+pprev_start+j+1];
                x_gpu[out_start+cur_start+j]=x_gpu[out_start+pprev_start+j+1]+1;
                y_gpu[out_start+cur_start+j]=y_gpu[out_start+pprev_start+j+1]+1;
            }
            else if(index1==2)
            {
                start_gpu[out_start+cur_start+j]=start_gpu[out_start+prev_start+j+1];
                x_gpu[out_start+cur_start+j]=x_gpu[out_start+prev_start+j+1]+1;
                y_gpu[out_start+cur_start+j]=y_gpu[out_start+prev_start+j+1];
            }
            else
            {
                start_gpu[out_start+cur_start+j]=start_gpu[out_start+prev_start+j];
                x_gpu[out_start+cur_start+j]=x_gpu[out_start+prev_start+j];
                y_gpu[out_start+cur_start+j]=y_gpu[out_start+prev_start+j]+1;
            }
        } 

        if(tid==0)
        {
            cost_gpu[out_start+cur_start+qline-1]=dist(query_gpu[0],all_seq_gpu[seq_start+i-1]);
            start_gpu[out_start+cur_start+qline-1]=i;
            x_gpu[out_start+cur_start+qline-1]=1;
            y_gpu[out_start+cur_start+qline-1]=1;

            inner_start++;
            pprev_start=(inner_start%3)*qline;
            prev_start=((inner_start+1)%3)*qline;
            cur_start=((inner_start+2)%3)*qline;

            start--;
        }

        /*
        __syncthreads();
        if(i==3&&bid==TEST_SEQ)
        {
            for(int k=tid;k<qline;k+=tsize)
            {
                diagonal_gpu[k]=cost_gpu[out_start+prev_start+k];
            }
        }
        */

        __syncthreads();
    }


    for(int i=qline;i<=seq_length;i++)
    {
        for(int j=tid;j<qline-1;j+=tsize)
        {
            float v1=valid_pos(left_right_bound[qline-1-j],left_right_bound[2*qline-1-j],x_gpu[out_start+prev_start+j],y_gpu[out_start+prev_start+j],1); 
            float v2=valid_pos(left_right_bound[qline-1-j],left_right_bound[2*qline-1-j],x_gpu[out_start+prev_start+j+1],y_gpu[out_start+prev_start+j+1],2); 

            int index1,index2;
            float small1=min(cost_gpu[out_start+prev_start+j]+v1,cost_gpu[out_start+prev_start+j+1]+v2,&index1);
            float small2=min(small1,cost_gpu[out_start+pprev_start+j+1],&index2);

            cost_gpu[out_start+cur_start+j]=dist(query_gpu[qline-1-j],all_seq_gpu[seq_start+i-qline+j])+small2;
            if(index2==2)
            {
                start_gpu[out_start+cur_start+j]=start_gpu[out_start+pprev_start+j+1];
                x_gpu[out_start+cur_start+j]=x_gpu[out_start+pprev_start+j+1]+1;
                y_gpu[out_start+cur_start+j]=y_gpu[out_start+pprev_start+j+1]+1;
            }
            else if(index1==2)
            {
                start_gpu[out_start+cur_start+j]=start_gpu[out_start+prev_start+j+1];
                x_gpu[out_start+cur_start+j]=x_gpu[out_start+prev_start+j+1]+1;
                y_gpu[out_start+cur_start+j]=y_gpu[out_start+prev_start+j+1];
            }
            else
            {
                start_gpu[out_start+cur_start+j]=start_gpu[out_start+prev_start+j];
                x_gpu[out_start+cur_start+j]=x_gpu[out_start+prev_start+j];
                y_gpu[out_start+cur_start+j]=y_gpu[out_start+prev_start+j]+1;
            }
        }

        if(tid==0)
        {
            cost_gpu[out_start+cur_start+qline-1]=dist(query_gpu[0],all_seq_gpu[seq_start+i-1]);
            start_gpu[out_start+cur_start+qline-1]=i;
            x_gpu[out_start+cur_start+qline-1]=1;
            y_gpu[out_start+cur_start+qline-1]=1;

            if(cost_gpu[out_start+cur_start]<small)
            {
                scale_factor=(i-qline+2-start_gpu[out_start+cur_start])*1.0f/qline;
                if(scale_factor>=0.8f&&scale_factor<=1.25f)
                {
                    small=cost_gpu[out_start+cur_start];
                    //start_pos=cur_start_gpu[out_start];
                    //end_pos=i-qline+1;
                }
            }

            /*
            if(bid==TEST_SEQ)
            {
                dist_gpu[i-qline]=cost_gpu[out_start+cur_start];
            }
            */

            inner_start++;
            pprev_start=(inner_start%3)*qline;
            prev_start=((inner_start+1)%3)*qline;
            cur_start=((inner_start+2)%3)*qline;
        }

        __syncthreads();
    }

    for(int i=seq_length+1;i<qline+seq_length;i++)
    {
        for(int j=tid;j<=end;j+=tsize)
        {
            float v1=valid_pos(left_right_bound[qline-1-j],left_right_bound[2*qline-1-j],x_gpu[out_start+prev_start+j],y_gpu[out_start+prev_start+j],1); 
            float v2=valid_pos(left_right_bound[qline-1-j],left_right_bound[2*qline-1-j],x_gpu[out_start+prev_start+j+1],y_gpu[out_start+prev_start+j+1],2); 

            int index1,index2;
            float small1=min(cost_gpu[out_start+prev_start+j]+v1,cost_gpu[out_start+prev_start+j+1]+v2,&index1);
            float small2=min(small1,cost_gpu[out_start+pprev_start+j+1],&index2);

            cost_gpu[out_start+cur_start+j]=dist(query_gpu[qline-1-j],all_seq_gpu[seq_start+i-qline+j])+small2;
            if(index2==2)
            {
                start_gpu[out_start+cur_start+j]=start_gpu[out_start+pprev_start+j+1];
                x_gpu[out_start+cur_start+j]=x_gpu[out_start+pprev_start+j+1]+1;
                y_gpu[out_start+cur_start+j]=y_gpu[out_start+pprev_start+j+1]+1;
            }
            else if(index1==2)
            {
                start_gpu[out_start+cur_start+j]=start_gpu[out_start+prev_start+j+1];
                x_gpu[out_start+cur_start+j]=x_gpu[out_start+prev_start+j+1]+1;
                y_gpu[out_start+cur_start+j]=y_gpu[out_start+prev_start+j+1];
            }
            else
            {
                start_gpu[out_start+cur_start+j]=start_gpu[out_start+prev_start+j];
                x_gpu[out_start+cur_start+j]=x_gpu[out_start+prev_start+j];
                y_gpu[out_start+cur_start+j]=y_gpu[out_start+prev_start+j]+1;
            }
        }

        if(tid==0)
        {

            if(cost_gpu[out_start+cur_start]<small)
            {
                scale_factor=(i-qline+2-start_gpu[out_start+cur_start])*1.0f/qline;
                if(scale_factor>=0.8f&&scale_factor<=1.25f)
                {
                    small=cost_gpu[out_start+cur_start];
                    //start_pos=cur_start_gpu[out_start];
                    //end_pos=i-qline+1;
                }
            }

            /*
            if(bid==TEST_SEQ)
            {
                dist_gpu[i-qline]=cost_gpu[out_start+cur_start];
            }
            */

            inner_start++;
            pprev_start=(inner_start%3)*qline;
            prev_start=((inner_start+1)%3)*qline;
            cur_start=((inner_start+2)%3)*qline;

            end--;
        }

        __syncthreads();
    }

    if(tid==0)
    {
        small_gpu[bid]=small;
    }
}
