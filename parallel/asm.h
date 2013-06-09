#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <iostream>

#include <cutil_inline.h>

float* Asm(float *query,int qline,int seq_num,float *all_seq,int all_length,int *seq_length,int *accu_length,float r);

__global__ static void bound(int qline,float r,int *left_bound_gpu,int *right_bound_gpu);

__device__ inline static float valid_pos(int left_bound,int right_bound, int x,int y,int kind);

__device__ inline static float min(float a,float b,int *index);

__global__ static void _asm(int qline,float *all_seq_gpu,
                            int *seq_length_gpu,int *accu_length_gpu,
                            float *cost_gpu,int *start_gpu,int *x_gpu,int *y_gpu,
                            int *left_bound_gpu,int *right_bound_gpu,float *small_gpu);
