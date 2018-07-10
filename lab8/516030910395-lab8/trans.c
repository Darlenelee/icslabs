/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 *
 * Student Name: Yijun Li
 * Student ID: 516030910395
 *
 */ 
#include <stdio.h>
#include "cachelab.h"
#define BSIZE 8
int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k, t, x1, x2,x3,x4,x5,x6,x7,x8;
    int n = (N/BSIZE) * BSIZE;
    int m = (M/BSIZE) * BSIZE;
    if(N!=64 || M!=64){
        // use 8x8 blocks to get better spatical locality
        for(j=0;j<m;j+=BSIZE){
            for(k=0;k<n;k++){
                x1 = A[k][j];
                x2 = A[k][j+1];
                x3 = A[k][j+2];
                x4 = A[k][j+3];
                x5 = A[k][j+4];
                x6 = A[k][j+5];
                x7 = A[k][j+6];
                x8 = A[k][j+7];

                B[j][k] = x1;
                B[j+1][k] = x2;
                B[j+2][k] = x3;
                B[j+3][k] = x4;
                B[j+4][k] = x5;
                B[j+5][k] = x6;
                B[j+6][k] = x7;
                B[j+7][k] = x8;
            }
        }
        // dealing with remainders
        for(i=n;i<N;i++){
            for(j=m;j<M;j++){
                t = A[i][j];
                B[j][i] = t;
            }
        }
        for(i=0;i<N;i++){
            for(j=m;j<M;j++){
                t = A[i][j];
                B[j][i] = t;
            }
        }
        for(i=n;i<N;i++){
            for(j=0;j<M;j++){
                t = A[i][j];
                B[j][i] = t;
            }
        }  
    }
    else{ // 64 64
        /* separate 8x8 into 4 parts
         *  [A11 A12]   [B11 B12]
         *  [A21 A22]   [B21 B22]
         *  first transpose A11->B11
         *  then A12->B12
         *  B12->B21
         *  A21->B12
         *  finally! A22->B22 
         */        
        for(i=0;i<N;i+=BSIZE){
            for(j=0;j<M;j+=BSIZE){
                // A11->B11 A12->B12 
                for(k=i;k<i+4;k++){
                    x1 = A[k][j];
                    x2 = A[k][j+1];
                    x3 = A[k][j+2];
                    x4 = A[k][j+3];
                    x5 = A[k][j+4];
                    x6 = A[k][j+5];
                    x7 = A[k][j+6];
                    x8 = A[k][j+7];

                    B[j][k] = x1;
                    B[j+1][k] = x2;
                    B[j+2][k] = x3;
                    B[j+3][k] = x4;
                    B[j][k+4] = x8;
                    B[j+1][k+4] = x7;
                    B[j+2][k+4] = x6;
                    B[j+3][k+4] = x5;
                }
                // B12->B21 A21->B12
                for(t=0;t<4;t++){
                    

                    x1 = A[i+4][j+3-t];
                    x2 = A[i+5][j+3-t];
                    x3 = A[i+6][j+3-t];
                    x4 = A[i+7][j+3-t];
                    x5 = A[i+4][j+4+t];
                    x6 = A[i+5][j+4+t];
                    x7 = A[i+6][j+4+t];
                    x8 = A[i+7][j+4+t];
                    B[j+4+t][i] = B[j+3-t][i+4];
                    B[j+4+t][i+1] = B[j+3-t][i+5];
                    B[j+4+t][i+2] = B[j+3-t][i+6];
                    B[j+4+t][i+3] = B[j+3-t][i+7];
                    B[j+3-t][i+4] = x1;
                    B[j+3-t][i+5] = x2;
                    B[j+3-t][i+6] = x3;
                    B[j+3-t][i+7] = x4;
                    B[j+4+t][i+4] = x5;
                    B[j+4+t][i+5] = x6;
                    B[j+4+t][i+6] = x7;
                    B[j+4+t][i+7] = x8;
                }
            }

        }
    }
    
     
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

