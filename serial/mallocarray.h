#include <stdio.h>
#include <stdlib.h>

template<class T>
void Array2D(int m,int n,T** &a)
{
    a=new T*[m];
    for(int i=0;i<m;i++)
    {
        a[i]=new T[n];
    }
}

template<class T>
void free_Array2D(T** &a,int m)
{
	for(int i=0;i<m;i++)
    {
        delete [] a[i];
    }
    delete [] a;
    a=0;
}
