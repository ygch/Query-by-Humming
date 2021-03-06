#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <iostream>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include "spring.h"
#include "util.h"
#include "asm.h"
#include "compare.h"

double* get_sequence(char *name,int *l)
{
    FILE *fp;
    int line;
    double *sequence;
    if((fp=fopen(name,"r"))==NULL)
    {
        fprintf(stderr,"Open file failed!\n");
        return NULL;
    }
    fscanf(fp,"%d",&line);
    sequence=(double *)malloc(sizeof(double)*line);
    for(int i=0;i<line;i++)
    {
        if(fscanf(fp,"%lf",&sequence[i])==EOF)
        {
            *l=i;
            fclose(fp);
            return sequence;
        }
    }
    fclose(fp);

    *l=line;
    return sequence;
}

void print_sequence(double *seq,int line)
{
    printf("line is %d,seq is:\n");
    for(int i=0;i<line;i++)
    {
        printf("%lf\n",seq[i]);
    }
}

int isreg(char *filename)
{
    char *temp;

    temp=strrchr(filename,(int)'.');
    if(temp!=NULL) return 1;
    else return 0;
}

int istxt(char *filename)
{
    char *temp,*temp2;

    temp=strstr(filename,".txt");
    temp2=strstr(filename,".txt~");//drop the backup file
    if(temp!=NULL&&temp2==NULL) return 1;
    else return 0;
}

int main(int argc, char **argv)
{
    double *query,*sequence;
    int qline,sline;
    double t1,t2;
    double r;
    int left_bound,right_bound;


    if(argc!=4) error(2);

    r=atof(argv[3]);

    query=get_sequence(argv[1],&qline);
    if(query==NULL)
    {
        perror("get query error!\n");
        return -1;
    }

    if(isreg(argv[2])&&istxt(argv[2]))
    {
        t1=clock();
        sequence=get_sequence(argv[2],&sline);
        if(sequence==NULL)
        {
            perror("get query error!\n");
            return -1;
        }
        t2=clock();

        if(qline>sline)
        {
            double *temp=query;
            query=sequence;
            sequence=temp;

            int ltemp=qline;
            qline=sline;
            sline=ltemp;
        }
        printf("query length is %d, sequence length is %d,time of read sequence is %lf\n",qline,sline,(t2-t1)/CLOCKS_PER_SEC);

        t1=clock();
        //naive_dtw(query,qline,sequence,sline);
        t2=clock();

        printf("naive exection time is %lf\n",(t2-t1)/CLOCKS_PER_SEC);

        t1=clock();
        //dtw(query,qline,sequence,sline);
        t2=clock();

        printf("modified exection time is %lf\n",(t2-t1)/CLOCKS_PER_SEC);


        t1=clock();
        //naive_asm(query,qline,sequence,sline,r);
        t2=clock();

        printf("naive asm time is %lf\n",(t2-t1)/CLOCKS_PER_SEC);

        t1=clock();
        Asm(query,qline,sequence,sline,r);
        t2=clock();

        printf("Asm time is %lf\n",(t2-t1)/CLOCKS_PER_SEC);

        free(sequence);
    }
    else
    {
        DIR *dp;
        struct dirent *entry;
        struct stat statbuf;
        distance dist[400];
        double d;
        int scaned_file=0;
        double start,end;


        if((dp=opendir(argv[2]))==NULL)
        {
            fprintf(stderr,"cannot open directory: %s\n",argv[2]);
        }

        chdir(argv[2]);

        start=clock();
        while((entry=readdir(dp))!=NULL)
        {
            bool exchanged=false;

            lstat(entry->d_name,&statbuf);
            if(S_ISREG(statbuf.st_mode)&&istxt(entry->d_name))
            {
                //printf("compare with %s file\n",entry->d_name);
                t1=clock();
                sequence=get_sequence(entry->d_name,&sline);
                if(sequence==NULL)
                {
                    perror("get query error!\n");
                    return -1;
                }
                t2=clock();

                if(qline>sline)
                {
                    double *temp=query;
                    query=sequence;
                    sequence=temp;

                    int ltemp=qline;
                    qline=sline;
                    sline=ltemp;

                    exchanged=true;
                }
                //printf("query length is %d, sequence length is %d,time of read sequence is %lf\n",qline,sline,(t2-t1)/CLOCKS_PER_SEC);

                /*
                t1=clock();
                naive_dtw(query,qline,sequence,sline);
                t2=clock();

                printf("naive exection time is %lf\n",(t2-t1)/CLOCKS_PER_SEC);

                t1=clock();
                dtw(query,qline,sequence,sline);
                t2=clock();

                printf("modified exection time is %lf\n",(t2-t1)/CLOCKS_PER_SEC);

                t1=clock();
                naive_asm(query,qline,sequence,sline,r);
                t2=clock();

                printf("naive asm time is %lf\n",(t2-t1)/CLOCKS_PER_SEC);
                */

                t1=clock();
                d=Asm(query,qline,sequence,sline,r);
                t2=clock();

                //printf("Asm time is %lf\n",(t2-t1)/CLOCKS_PER_SEC);

                dist[scaned_file].dis=d;
                strcpy(dist[scaned_file++].name,entry->d_name);

                //printf("\n");
                if(exchanged)
                {
                    free(query);
                    query=sequence;
                    qline=sline;
                }
                else
                {
                    free(sequence);
                }
            }
        }
        closedir(dp);

        end=clock();

        printf("asm time of all the sequence is %lf\n",(end-start)/CLOCKS_PER_SEC);

        for(int i=0;i<scaned_file;i++)
        {
            //printf("(%d) file %s: %lf\n",i+1,dist[i].name,dist[i].dis);
        }    

        //printf("compare with %d file\n",scaned_file);
        qsort(dist,scaned_file,sizeof(distance),com);

        int rank;
        char * query_name=strrchr(argv[1],'/');
        query_name++;
        printf("query file is %s\n",query_name);

        for(int i=0;i<scaned_file;i++)
        {
            printf("(%d) file %s: %lf\n",i+1,dist[i].name,dist[i].dis);
            if(strcmp(query_name,dist[i].name)==0)
            {
                rank=i+1;
            }
        }

        printf("search file: %s, rank is %d\n\n",query_name,rank);
    }
    free(query);

    return 0;
}
