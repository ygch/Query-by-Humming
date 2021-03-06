#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <iostream>

void bound(int qline,int qpos,double r,int *left,int *right);

double valid_pos(int left_bound,int right_bound,int x,int y,int kind);

double naive_asm(double *query,int qline,double *sequence,int sline,double r);

double Asm(double *query,int qline,double *sequence,int sline,double r);
