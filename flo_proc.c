#include "flo_proc.h"
#include <math.h>

double euclidian_norm(int8_t *input, int idx_u, int idx_v) // should be float more strictly, but close enough
{
    return sqrt(input[idx_u] * input[idx_u] + input[idx_v] * input[idx_v]);
}

int32_t flow_error(int8_t *input, uint8_t *output, int width, int height)
{
    // sum left and right half separately, subtract
    // save euclidian norm to output
    uint32_t sum_left = 0;
    uint32_t sum_right = 0;
    
    for (int y = 0; y < height ; y++)
    {
        for (int x = 0; x < width/2 ; x++)
        {
            int idx_left_u = y * width + x;
            int idx_left_v = idx_left_u + width * height;
            // printf("idx_left_u: %d\n", idx_left_u);
            // printf("idx_left_v: %d\n", idx_left_v);
            double left_mag = euclidian_norm(input, idx_left_u, idx_left_v)/2;
            // printf("Left magnitude is %ld\n", (int)left_mag);
            // printf("%d\n", input[idx_left_u]);
            // printf("%lf\n", left_mag);
            output[idx_left_u] = left_mag;
            
            int idx_right_u = y * width + x + width/2;
            int idx_right_v = idx_right_u + width * height;
            // printf("idx_right_u: %d\n", idx_right_u);
            // printf("idx_right_v: %d\n", idx_right_v);
            double right_mag = euclidian_norm(input, idx_right_u, idx_right_v)/2;
            // printf("RIGHT - Saving %d to index %d\n", (int)right_mag, idx_right_u);
            output[idx_right_u] = right_mag;
            
            sum_left += left_mag;
            sum_right += right_mag;           
        }
    }

    int32_t diff = sum_right - sum_left;
    return diff;
}

