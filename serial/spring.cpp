#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <iostream>

#include "spring.h"
#include "mallocarray.h"
#include "util.h"

#define dist(x,y) (fabs(x-y))
#define dist2(x,y) ((x-y)*(x-y))

#define INF 1e20  

double naive_dtw(double *query,int qline,double *sequence,int sline)
{
    double **cost;
    int **start;

    Array2D(qline+1,sline+1,cost);//the initialized value is zero
    Array2D(qline+1,sline+1,start);

    for(int i=0;i<=sline;i++)
    {
        start[0][i]=i+1;
        cost[0][i]=0;
    }

    for(int i=1;i<=qline;i++)
    {
        cost[i][0]=INF;
    }

    for(int i=1;i<=qline;i++)
    {
        for(int j=1;j<=sline;j++)
        {
            int index1,index2;
            double small1=min(cost[i-1][j],cost[i][j-1],&index1);//when i=1, the cost is calculated using cost[i-1][j],if we use the formula, we must be sure
                                                                 // we use the right position to calculate the cost
            double small2=min(small1,cost[i-1][j-1],&index2);

            cost[i][j]=dist(query[i-1],sequence[j-1])+small2;
            if(index2==2)
                start[i][j]=start[i-1][j-1];
            else if(index1==1)
                start[i][j]=start[i-1][j];
            else
                start[i][j]=start[i][j-1];
        }
    }

    double small=INF;
    int start_pos,end_pos;
    for(int i=1;i<=sline;i++)
    {
        if(cost[qline][i]<small)
        {
            small=cost[qline][i];
            start_pos=start[qline][i];
            end_pos=i;
        }
    }

    /*
    for(int i=qline;i>=0;i--)
    {
        for(int j=0;j<=sline;j++)
        {
            printf("%0.1lf(%d) ",cost[i][j],start[i][j]);
        }
        printf("\n");
    }
    */

    printf("start:%d,end:%d,distance:%lf\n",start_pos,end_pos,small);

    free_Array2D(cost,qline+1);
    free_Array2D(start,qline+1);

    return small;
}

double dtw(double *query,int qline,double *sequence,int sline)
{
    double *pprev,*prev,*cur;
    int *pprev_start,*prev_start,*cur_start;

    pprev=(double *)malloc(sizeof(double)*qline);
    prev=(double *)malloc(sizeof(double)*qline);
    cur=(double *)malloc(sizeof(double)*qline);

    pprev_start=(int *)malloc(sizeof(int)*qline);
    prev_start=(int *)malloc(sizeof(int)*qline);
    cur_start=(int *)malloc(sizeof(int)*qline);

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
    
    int start=qline-3;
    for(int i=3;i<qline;i++)//line from 1 to n+m-1
    {
        for(int j=start;j<qline-1;j++)//the index of calculated row starts from 0
        {
            int index1,index2;
            double small1=min(prev[j],prev[j+1],&index1);
            double small2=min(small1,pprev[j+1],&index2);

            cur[j]=dist(query[i-1-(j-start)],sequence[j-start])+small2;
            if(index2==2)
                cur_start[j]=pprev_start[j+1];
            else if(index1==2)
                cur_start[j]=prev_start[j+1];
            else
                cur_start[j]=prev_start[j];
        }
        cur[qline-1]=dist(query[0],sequence[i-1]);
        cur_start[qline-1]=i;

        double *temp=pprev;
        pprev=prev;
        prev=cur;
        cur=temp;

        int *stemp=pprev_start;
        pprev_start=prev_start;
        prev_start=cur_start;
        cur_start=stemp;

        start--;
    }

    double small=INF;
    int start_pos,end_pos;
    for(int i=qline;i<=sline;i++)
    {
        for(int j=0;j<qline-1;j++)//the index of calculated row starts from 0
        {
            int index1,index2;
            double small1=min(prev[j],prev[j+1],&index1);
            double small2=min(small1,pprev[j+1],&index2);

            cur[j]=dist(query[qline-1-j],sequence[i-qline+j])+small2;
            if(index2==2)
                cur_start[j]=pprev_start[j+1];
            else if(index1==2)
                cur_start[j]=prev_start[j+1];
            else
                cur_start[j]=prev_start[j];
        }
        cur[qline-1]=dist(query[0],sequence[i-1]);
        cur_start[qline-1]=i;

        if(cur[0]<small)
        {
            small=cur[0];
            start_pos=cur_start[0];
            end_pos=i-qline+1;
        }

        double *temp=pprev;
        pprev=prev;
        prev=cur;
        cur=temp;

        int *stemp=pprev_start;
        pprev_start=prev_start;
        prev_start=cur_start;
        cur_start=stemp;
    }
    int end=qline-2;
    for(int i=sline+1;i<qline+sline;i++)
    {
        for(int j=0;j<=end;j++)
        {
            int index1,index2;
            double small1=min(prev[j],prev[j+1],&index1);
            double small2=min(small1,pprev[j+1],&index2);

            cur[j]=dist(query[qline-1-j],sequence[i-qline+j])+small2;
            if(index2==2)
                cur_start[j]=pprev_start[j+1];
            else if(index1==2)
                cur_start[j]=prev_start[j+1];
            else
                cur_start[j]=prev_start[j];
        }

        if(cur[0]<small)
        {
            small=cur[0];
            start_pos=cur_start[0];
            end_pos=i-qline+1;
        }

        double *temp=pprev;
        pprev=prev;
        prev=cur;
        cur=temp;

        int *stemp=pprev_start;
        pprev_start=prev_start;
        prev_start=cur_start;
        cur_start=stemp;

        end--;
    }

    printf("start:%d,end:%d,distance:%lf\n",start_pos,end_pos,small);

    free(pprev);
    free(prev);
    free(cur);
    free(pprev_start);
    free(prev_start);
    free(cur_start);

    return small;
}

