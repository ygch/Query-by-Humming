#include "asm.h"
#include "mallocarray.h"
#include "util.h"

#define dist(x,y) (fabs(x-y))
#define dist2(x,y) ((x-y)*(x-y))

#define INF 1e10

void bound(int qline,int qpos,double r,int *left,int *right)
{
    *left=(int)(qpos*0.2+qline*r);
    *right=(int)(qpos*0.25+qline*r);
}

double valid_pos(int left_bound,int right_bound,int x,int y,int kind)
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

double naive_asm(double *query,int qline,double *sequence,int sline,double r)
{
    double **cost;
    int **start;
    int **x;
    int **y;
    int warp_thresh=(int)(2*qline*r);

    Array2D(qline+1,sline+1,cost);//the initialized value is zero
    Array2D(qline+1,sline+1,start);
    Array2D(qline+1,sline+1,x);
    Array2D(qline+1,sline+1,y);

    for(int i=0;i<=sline;i++)
    {
        start[0][i]=i+1;
        cost[0][i]=0;
        y[0][i]=0;//x and y are all zero
    }

    for(int i=1;i<=qline;i++)
    {
        cost[i][0]=INF;
    }

    for(int i=1;i<=qline;i++)
    {
        int left_bound;
        int right_bound;

        bound(qline,i,r,&left_bound,&right_bound);
        //printf("left bound is %d,right bound is %d\n",left_bound,right_bound);
        for(int j=1;j<=sline;j++)
        {
            //use global constraint to check the validation of the anti-diaginol elements
            double v1=valid_pos(left_bound,right_bound,x[i][j-1],y[i][j-1],1);
            double v2=valid_pos(left_bound,right_bound,x[i-1][j],y[i-1][j],2);

            int index1,index2;
            double small1=min(cost[i][j-1]+v1,cost[i-1][j]+v2,&index1);
            double small2=min(small1,cost[i-1][j-1],&index2);

            cost[i][j]=dist(query[i-1],sequence[j-1])+small2;
            if(index2==2)
            {
                start[i][j]=start[i-1][j-1];
                x[i][j]=x[i-1][j-1]+1;
                y[i][j]=y[i-1][j-1]+1;
            }
            else if(index1==2)
            {
                start[i][j]=start[i-1][j];
                x[i][j]=x[i-1][j]+1;
                y[i][j]=y[i-1][j];
            }
            else
            {
                start[i][j]=start[i][j-1];
                x[i][j]=x[i][j-1];
                y[i][j]=y[i][j-1]+1;
            }
        }
    }

    double small=INF;
    int start_pos,end_pos;
    for(int i=(int)0.8*qline;i<=sline;i++)
    {
        double scale_factor=(i-start[qline][i]+1)*1.0/qline;
        //printf("scale_factor is %lf\n",scale_factor);
        if(cost[qline][i]<small&&(scale_factor>=0.8&&scale_factor<=1.25))
        {
            small=cost[qline][i];
            start_pos=start[qline][i];
            end_pos=i;
        }
    }

    /*
    for(int i=qline;i>=0;i--)
    {
        for(int j=0;j<=10;j++)
        {
            printf("%0.2lf(%d)\t",cost[i][j],start[i][j]);
        }
        printf("\n");
    }

    for(int i=qline;i>=0;i--)
    {
        for(int j=0;j<=sline;j++)
        {
            printf("(%d,%d) ",x[i][j],y[i][j]);
        }
        printf("\n");
    }
    */

    printf("start:%d,end:%d,scale factor is %lf,distance:%lf\n",start_pos,end_pos,(end_pos-start_pos+1)*1.0/qline,small);

    free_Array2D(cost,qline+1);
    free_Array2D(start,qline+1);
    free_Array2D(x,qline+1);
    free_Array2D(y,qline+1);

    return small;
}

double Asm(double *query,int qline,double *sequence,int sline,double r)
{
    double *pprev,*prev,*cur;
    int *pprev_start,*prev_start,*cur_start;
    int *pprev_x,*prev_x,*cur_x;
    int *pprev_y,*prev_y,*cur_y;
    int *left_bound,*right_bound;

    pprev=(double *)malloc(sizeof(double)*qline);
    prev=(double *)malloc(sizeof(double)*qline);
    cur=(double *)malloc(sizeof(double)*qline);

    pprev_start=(int *)malloc(sizeof(int)*qline);
    prev_start=(int *)malloc(sizeof(int)*qline);
    cur_start=(int *)malloc(sizeof(int)*qline);

    pprev_x=(int *)malloc(sizeof(int)*qline);
    prev_x=(int *)malloc(sizeof(int)*qline);
    cur_x=(int *)malloc(sizeof(int)*qline);

    pprev_y=(int *)malloc(sizeof(int)*qline);
    prev_y=(int *)malloc(sizeof(int)*qline);
    cur_y=(int *)malloc(sizeof(int)*qline);

    left_bound=(int *)malloc(sizeof(int)*qline);
    right_bound=(int *)malloc(sizeof(int)*qline);

    for(int i=0;i<qline-1;i++)
    {
        pprev[i]=INF;
        prev[i]=INF;
        cur[i]=INF;//should be inilized as they may exchange their roles circularly
    }

    pprev[qline-1]=dist(query[0],sequence[0]);
    prev[qline-2]=dist(query[1],sequence[0])+pprev[qline-1];
    prev[qline-1]=dist(query[0],sequence[1]);

    pprev_start[qline-1]=1;//start position from 1
    prev_start[qline-2]=1;
    prev_start[qline-1]=2;

    pprev_x[qline-1]=1;
    pprev_y[qline-1]=1;

    prev_x[qline-2]=2;
    prev_y[qline-2]=1;

    prev_x[qline-1]=1;
    prev_y[qline-1]=1;
    
    for(int i=0;i<qline;i++)
    {
        bound(qline,i+1,r,&left_bound[i],&right_bound[i]);
        //printf("%d:(%d,%d)\n",i+1,left_bound[i],right_bound[i]);
    }

    int start=qline-3;
    for(int i=3;i<qline;i++)//line from 1 to n+m-1
    {
        for(int j=start;j<qline-1;j++)//the index of calculated row starts from 0
        {
            //use global constraint to check the validation of the anti-diaginol elements
            double v1=valid_pos(left_bound[i-1-(j-start)],right_bound[i-1-(j-start)],prev_x[j],prev_y[j],1);
            double v2=valid_pos(left_bound[i-1-(j-start)],right_bound[i-1-(j-start)],prev_x[j+1],prev_y[j+1],2);
          
            int index1,index2;
            double small1=min(prev[j]+v1,prev[j+1]+v2,&index1);
            double small2=min(small1,pprev[j+1],&index2);

            cur[j]=dist(query[i-1-(j-start)],sequence[j-start])+small2;
            //printf("(%d,%d):v1 is %lf,v2 is %lf,p1(%d,%d),p2:(%d,%d),dis:%lf,index1:%d,index2:%d\n",i-(j-start),j-start+1,v1,v2,prev_x[j],prev_y[j],prev_x[j+1],prev_y[j+1],cur[j],index1,index2);
            if(index2==2)
            {
                cur_start[j]=pprev_start[j+1];
                cur_x[j]=pprev_x[j+1]+1;
                cur_y[j]=pprev_y[j+1]+1;
            }
            else if(index1==2)
            {
                cur_start[j]=prev_start[j+1];
                cur_x[j]=prev_x[j+1]+1;
                cur_y[j]=prev_y[j+1];
            }
            else
            {
                cur_start[j]=prev_start[j];
                cur_x[j]=prev_x[j];
                cur_y[j]=prev_y[j]+1;
            }
        }
        cur[qline-1]=dist(query[0],sequence[i-1]);
        cur_start[qline-1]=i;
        cur_x[qline-1]=1;
        cur_y[qline-1]=1;

        double *temp=pprev;
        pprev=prev;
        prev=cur;
        cur=temp;

        int *stemp=pprev_start;
        pprev_start=prev_start;
        prev_start=cur_start;
        cur_start=stemp;

        int* xtemp=pprev_x;
        pprev_x=prev_x;
        prev_x=cur_x;
        cur_x=xtemp;

        int* ytemp=pprev_y;
        pprev_y=prev_y;
        prev_y=cur_y;
        cur_y=ytemp;

        start--;
    }

    double small=INF;
    int start_pos,end_pos;
    double scale_factor;
    for(int i=qline;i<=sline;i++)
    {
        for(int j=0;j<qline-1;j++)//the index of calculated row starts from 0
        {
            //use global constraint to check the validation of the anti-diaginol elements
            double v1=valid_pos(left_bound[qline-1-j],right_bound[qline-1-j],prev_x[j],prev_y[j],1);
            double v2=valid_pos(left_bound[qline-1-j],right_bound[qline-1-j],prev_x[j+1],prev_y[j+1],2);


            int index1,index2;
            double small1=min(prev[j]+v1,prev[j+1]+v2,&index1);
            double small2=min(small1,pprev[j+1],&index2);

            cur[j]=dist(query[qline-1-j],sequence[i-qline+j])+small2;
            if(index2==2)
            {
                cur_start[j]=pprev_start[j+1];
                cur_x[j]=pprev_x[j+1]+1;
                cur_y[j]=pprev_y[j+1]+1;
            }
            else if(index1==2)
            {
                cur_start[j]=prev_start[j+1];
                cur_x[j]=prev_x[j+1]+1;
                cur_y[j]=prev_y[j+1];
            }
            else
            {
                cur_start[j]=prev_start[j];
                cur_x[j]=prev_x[j];
                cur_y[j]=prev_y[j]+1;
            }
        }
        cur[qline-1]=dist(query[0],sequence[i-1]);
        cur_start[qline-1]=i;
        cur_x[qline-1]=1;
        cur_y[qline-1]=1;

        if((i-qline+1)>=(int)0.8*qline)
        {
            scale_factor=(i-qline+2-cur_start[0])*1.0/qline;
            if(cur[0]<small&&scale_factor>=0.8&&scale_factor<=1.25)
            {
                small=cur[0];
                start_pos=cur_start[0];
                end_pos=i-qline+1;
            }
        }

        double *temp=pprev;
        pprev=prev;
        prev=cur;
        cur=temp;

        int *stemp=pprev_start;
        pprev_start=prev_start;
        prev_start=cur_start;
        cur_start=stemp;

        int* xtemp=pprev_x;
        pprev_x=prev_x;
        prev_x=cur_x;
        cur_x=xtemp;

        int* ytemp=pprev_y;
        pprev_y=prev_y;
        prev_y=cur_y;
        cur_y=ytemp;
    }

    int end=qline-2;
    for(int i=sline+1;i<qline+sline;i++)
    {
        for(int j=0;j<=end;j++)
        {
            //use global constraint to check the validation of the anti-diaginol elements
            double v1=valid_pos(left_bound[qline-1-j],right_bound[qline-1-j],prev_x[j],prev_y[j],1);
            double v2=valid_pos(left_bound[qline-1-j],right_bound[qline-1-j],prev_x[j+1],prev_y[j+1],2);

         

            int index1,index2;
            double small1=min(prev[j]+v1,prev[j+1]+v2,&index1);
            double small2=min(small1,pprev[j+1],&index2);

            cur[j]=dist(query[qline-1-j],sequence[i-qline+j])+small2;
            if(index2==2)
            {
                cur_start[j]=pprev_start[j+1];
                cur_x[j]=pprev_x[j+1]+1;
                cur_y[j]=pprev_y[j+1]+1;
            }
            else if(index1==2)
            {
                cur_start[j]=prev_start[j+1];
                cur_x[j]=prev_x[j+1]+1;
                cur_y[j]=prev_y[j+1];
            }
            else
            {
                cur_start[j]=prev_start[j];
                cur_x[j]=prev_x[j];
                cur_y[j]=prev_y[j]+1;
            }
        }

        if((i-qline+1)>=(int)0.8*qline)
        {
            scale_factor=(i-qline+2-cur_start[0])*1.0/qline;
            if(cur[0]<small&&scale_factor>=0.8&&scale_factor<=1.25)
            {
                small=cur[0];
                start_pos=cur_start[0];
                end_pos=i-qline+1;
            }
        }

        double *temp=pprev;
        pprev=prev;
        prev=cur;
        cur=temp;

        int *stemp=pprev_start;
        pprev_start=prev_start;
        prev_start=cur_start;
        cur_start=stemp;

        int* xtemp=pprev_x;
        pprev_x=prev_x;
        prev_x=cur_x;
        cur_x=xtemp;

        int* ytemp=pprev_y;
        pprev_y=prev_y;
        prev_y=cur_y;
        cur_y=ytemp;

        end--;
    }

    //printf("start:%d,end:%d,scale factor is %lf,distance:%lf\n",start_pos,end_pos,(end_pos-start_pos+1)*1.0/qline,small);

    free(pprev);
    free(prev);
    free(cur);
    free(pprev_start);
    free(prev_start);
    free(cur_start);
    free(pprev_x);
    free(prev_x);
    free(cur_x);
    free(pprev_y);
    free(prev_y);
    free(cur_y);

    free(left_bound);
    free(right_bound);

    return small;
}

