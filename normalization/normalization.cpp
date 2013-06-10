#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <iostream>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>

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

void normalization_max(char *file,char *save_dir)
{
    char new_file[100];
    FILE *fp;

    double max=0;
    double v;
    double *sequence;
    int def_len,len=0;

    char *source_file;

    //distinguish if the source is a pure file name or a file name with directory
    if((source_file=strrchr(file,'/'))!=0)
    {
        source_file++;
    }
    else
    {
        source_file=file;
    }

    strcpy(new_file,save_dir);
    strcat(new_file,"/");
    strcat(new_file,source_file);
    printf("new file is %s\n",new_file);

    if(!access(new_file,0))//check if the new created file exists
    {
        printf("file already exists\n");
    }

    if((fp=fopen(file,"r"))==NULL)
    {
        perror("open file failed!\n");
        return;
    }

    fscanf(fp,"%d",&def_len);

    sequence=(double *)malloc(sizeof(double)*def_len);

    while(fscanf(fp,"%lf",&v)!=EOF&&v==0);//drop the first zero elements

    do
    {
        sequence[len++]=v;
        if(max<v)
        {
            max=v;
        }
    }while(fscanf(fp,"%lf",&v)!=EOF);

    fclose(fp);

    //remove the back zero
    for(int i=len-1;i>=0;i--)
    {
        if(sequence[i]!=0)
            break;
        len--;
    }

    //wirte to new file
    if((fp=fopen(new_file,"w"))==NULL)
    {
        perror("open new file failed!");
        return;       
    }

    fprintf(fp,"%d\r\n",len);
    for(int i=0;i<len;i++)
    {
        sequence[i]/=max;
        fprintf(fp,"%lf\r\n",sequence[i]);
    }

    fclose(fp);
    free(sequence);
}

void normalization_mean(char *file,char *save_dir,int ignore,int downsample)
{
    char new_file[100];
    FILE *fp;

    double sum=0,mean;
    double v;
    double *sequence;
    int def_len,len=0,non_zero=0;

    char *source_file;

    //distinguish if the source is a pure file name or a file name with directory
    if((source_file=strrchr(file,'/'))!=0)
    {
        source_file++;
    }
    else
    {
        source_file=file;
    }

    strcpy(new_file,save_dir);
    strcat(new_file,"/");
    strcat(new_file,source_file);

    printf("new file is %s\n",new_file);

    if(!access(new_file,0))//check if the new created file exists
    {
        printf("file already exists\n");
    }

    if((fp=fopen(file,"r"))==NULL)
    {
        perror("open file failed!\n");
        return;
    }

    fscanf(fp,"%d",&def_len);

    sequence=(double *)malloc(sizeof(double)*def_len);

    while(fscanf(fp,"%lf",&v)!=EOF&&v==0);//drop the first zero elements

    do
    {
        sequence[len++]=v;
        if(ignore)
        {
            if(v!=0)//can be ignored for different stratogies
            {
                sum+=v;
                non_zero++;
            }
        }
        else
        {
            sum+=v;
            non_zero++;
        }
        //fseek(fp,sizeof(double)*(downsample-1),SEEK_CUR);
        for(int i=1;i<downsample;i++)
        {
            fscanf(fp,"%lf",&v);
        }
    }while(fscanf(fp,"%lf",&v)!=EOF);

    fclose(fp);

    for(int i=len-1;i>=0;i--)
    {
        if(sequence[i]!=0)
            break;
        len--;
    }

    mean=sum/non_zero;

    //wirte to new file
    if((fp=fopen(new_file,"w"))==NULL)
    {
        perror("open new file failed!");
        return;       
    }

    fprintf(fp,"%d\r\n",len);
    for(int i=0;i<len;i++)
    {
        sequence[i]/=mean;
        fprintf(fp,"%lf\r\n",sequence[i]);
    }

    fclose(fp);
    free(sequence);
}

void normalization_std_mean(char *file,char *save_dir,int ignore)
{
    char new_file[100];
    FILE *fp;

    double sum=0,mean,std=0;
    double v;
    double *sequence;
    int def_len,len=0,non_zero=0;

    char *source_file;

    //distinguish if the source is a pure file name or a file name with directory
    if((source_file=strrchr(file,'/'))!=0)
    {
        source_file++;
    }
    else
    {
        source_file=file;
    }

    strcpy(new_file,save_dir);
    strcat(new_file,"/");
    strcat(new_file,source_file);

    printf("new file is %s\n",new_file);

    if(!access(new_file,0))//check if the new created file exists
    {
        printf("file already exists\n");
    }

    if((fp=fopen(file,"r"))==NULL)
    {
        perror("open file failed!\n");
        return;
    }

    fscanf(fp,"%d",&def_len);

    sequence=(double *)malloc(sizeof(double)*def_len);

    while(fscanf(fp,"%lf",&v)!=EOF&&v==0);//drop the first zero elements

    do
    {
        sequence[len++]=v;
        if(ignore)
        {
            if(v!=0)//can be ignored for different stratogies
            {
                sum+=v;
                std+=v*v;
                non_zero++;
            }
        }
        else
        {
            sum+=v;
            std+=v*v;
            non_zero++;
        }
    }while(fscanf(fp,"%lf",&v)!=EOF);

    fclose(fp);

    for(int i=len-1;i>=0;i--)
    {
        if(sequence[i]!=0)
            break;
        len--;
    }

    mean=sum/non_zero;
    std=sqrt(std/non_zero-mean*mean);

    //wirte to new file
    if((fp=fopen(new_file,"w"))==NULL)
    {
        perror("open new file failed!");
        return;       
    }

    fprintf(fp,"%d\r\n",len);
    for(int i=0;i<len;i++)
    {
        sequence[i]=(sequence[i]-mean)/std;
        fprintf(fp,"%lf\r\n",sequence[i]);
    }

    fclose(fp);
    free(sequence);
}

void normalization_log_mean(char *file,char *save_dir)
{
    char new_file[100];
    FILE *fp;

    double sum=0,mean;
    double v;
    double *sequence;
    int def_len,len=0,non_zero=0;

    char *source_file;

    //distinguish if the source is a pure file name or a file name with directory
    if((source_file=strrchr(file,'/'))!=0)
    {
        source_file++;
    }
    else
    {
        source_file=file;
    }

    strcpy(new_file,save_dir);
    strcat(new_file,"/");
    strcat(new_file,source_file);

    printf("new file is %s\n",new_file);

    if(!access(new_file,0))//check if the new created file exists
    {
        printf("file already exists\n");
    }

    if((fp=fopen(file,"r"))==NULL)
    {
        perror("open file failed!\n");
        return;
    }

    fscanf(fp,"%d",&def_len);

    sequence=(double *)malloc(sizeof(double)*def_len);

    while(fscanf(fp,"%lf",&v)!=EOF&&v==0);//drop the first zero elements

    do
    {
        if(v!=0)//can be ignored for different stratogies
        {
            sequence[len++]=log2(v);
            sum+=log2(v);
            non_zero++;
        }
        else
        {
            sequence[len++]=0;
        }
    }while(fscanf(fp,"%lf",&v)!=EOF);

    fclose(fp);

    for(int i=len-1;i>=0;i--)
    {
        if(sequence[i]!=0)
            break;
        len--;
    }

    mean=sum/non_zero;

    //wirte to new file
    if((fp=fopen(new_file,"w"))==NULL)
    {
        perror("open new file failed!");
        return;       
    }

    fprintf(fp,"%d\r\n",len);
    for(int i=0;i<len;i++)
    {
        sequence[i]/=mean;
        fprintf(fp,"%lf\r\n",sequence[i]);
    }

    fclose(fp);
    free(sequence);
}


int main(int argc, char **argv)
{
    if(argc!=6)
    {
        perror("parameter:./normalization source_file dest_directory norm_choice ignore downsample");
        return -1;
    }

    if(isreg(argv[1])&&istxt(argv[1]))
    {
        switch (atoi(argv[3]))
        {
            case 1:
                normalization_max(argv[1],argv[2]);
                break;
            case 2:
                normalization_mean(argv[1],argv[2],atoi(argv[4]),atoi(argv[5]));
                break;
            case 3:
                normalization_std_mean(argv[1],argv[2],atoi(argv[4]));
                break;
            case 4:
                normalization_log_mean(argv[1],argv[2]);
                break;
        }
    }
    else
    {
        DIR *dp;
        struct dirent *entry;
        struct stat statbuf;

        if((dp=opendir(argv[1]))==NULL)
        {
            fprintf(stderr,"cannot open directory: %s\n",argv[1]);
        }

        chdir(argv[1]);

        while((entry=readdir(dp))!=NULL)
        {
            lstat(entry->d_name,&statbuf);
            if(S_ISREG(statbuf.st_mode)&&istxt(entry->d_name))
            {
                printf("normalize %s file\n",entry->d_name);
                switch (atoi(argv[3]))
                {
                    case 1:
                        normalization_max(entry->d_name,argv[2]);
                        break;
                    case 2:
                        normalization_mean(entry->d_name,argv[2],atoi(argv[4]),atoi(argv[5]));
                        break;
                    case 3:
                        normalization_std_mean(entry->d_name,argv[2],atoi(argv[4]));
                        break;
                    case 4:
                        normalization_log_mean(entry->d_name,argv[2]);
                        break;
                }
                printf("\n");
            }
        }
        closedir(dp);
    }
}
