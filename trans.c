/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

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
    if(M == 32 && N == 32)
    {
        int i, j, k;//variables for loop
        int c1, c2, c3, c4, c5, c6, c7, c8;//a cache line
        for (i = 0; i < 32; i += 8)
        {
            for (j = 0; j < 32; j += 8)
            {
                for (k = i; k < (i + 8); k++)
                {
                    //store a cache line to avoid diagonal eviction
                    c1 = A[k][j];
                    c2 = A[k][j + 1];
                    c3 = A[k][j + 2];
                    c4 = A[k][j + 3];
                    c5 = A[k][j + 4];
                    c6 = A[k][j + 5];
                    c7 = A[k][j + 6];
                    c8 = A[k][j + 7];
                    B[j][k] = c1;
                    B[j + 1][k] = c2;
                    B[j + 2][k] = c3;
                    B[j + 3][k] = c4;
                    B[j + 4][k] = c5;
                    B[j + 5][k] = c6;
                    B[j + 6][k] = c7;
                    B[j + 7][k] = c8;
                }
            }
        }
    }
    if(M == 64 && N == 64)
    {
        int i, j, m, n;//variables for loop
        int c1, c2, c3, c4, c5, c6, c7, c8;//a cache line
        for (i = 0; i < 64; i += 8)
        {
            for (j = 0; j < 64; j += 8)
            {
                //for less evictions, we have to detour a little bit so that we take full use of A[i][j]~A[i][j+7](a cache line)
                for (m = i; m < i + 4; m++)
                {
                    c1 = A[m][j];
                    c2 = A[m][j + 1];
                    c3 = A[m][j + 2];
                    c4 = A[m][j + 3];
                    c5 = A[m][j + 4];
                    c6 = A[m][j + 5];
                    c7 = A[m][j + 6];
                    c8 = A[m][j + 7];
                    B[j][m] = c1;
                    B[j + 1][m] = c2;
                    B[j + 2][m] = c3;
                    B[j + 3][m] = c4;
                    B[j][m + 4] = c5;
                    B[j + 1][m + 4] = c6;
                    B[j + 2][m + 4] = c7;
                    B[j + 3][m + 4] = c8;
                }
                //copy the "detour" part to the right part
                for (n = j; n < j + 4; n++)
                {
                    c1 = A[i + 4][n];
                    c2 = A[i + 5][n];
                    c3 = A[i + 6][n];
                    c4 = A[i + 7][n];
                    c5 = B[n][i + 4];
                    c6 = B[n][i + 5];
                    c7 = B[n][i + 6];
                    c8 = B[n][i + 7];

                    B[n][i + 4] = c1;
                    B[n][i + 5] = c2;
                    B[n][i + 6] = c3;
                    B[n][i + 7] = c4;
                    B[n + 4][i] = c5;
                    B[n + 4][i + 1] = c6;
                    B[n + 4][i + 2] = c7;
                    B[n + 4][i + 3] = c8;
                }
                for (m = i + 4; m < i + 8; m++)
                {
                    c1 = A[m][j + 4];
                    c2 = A[m][j + 5];
                    c3 = A[m][j + 6];
                    c4 = A[m][j + 7];

                    B[j + 4][m] = c1;
                    B[j + 5][m] = c2;
                    B[j + 6][m] = c3;
                    B[j + 7][m] = c4;
                }
            }
        }
    }
    if(M == 61 && N == 67)
    {
        int i, j, m, n;//variables for loop
        for (m = 0; m < 67; m += 17)
        {
            for (n = 0; n < 61; n += 4)
            {
                for (i = m; i < (m < 50 ? (m + 17) : 67); i++)
                {
                    for (j = n; j < (n < 57 ? (n + 4) : 61); j++)
                    {
                        B[j][i] = A[i][j];
                    }
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

