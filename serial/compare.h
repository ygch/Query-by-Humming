#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <iostream>

typedef struct
{
    double dis;
    char name[256];
}distance;

int com(const void *d1,const void *d2)
{
    const distance *dis1=(distance*)d1;
    const distance *dis2=(distance*)d2;

    return (dis1->dis>dis2->dis)?1:-1;
}

