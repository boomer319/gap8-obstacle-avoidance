#ifndef __FLO_PROC_H__
#define __FLO_PROC_H__

#include "pmsis.h"

typedef struct {
    char *srcBuffer;     // pointer to the input vector
    char *resBuffer;     // pointer to the output vector
    uint32_t width;      // image width
    uint32_t height;     // image height
    uint32_t nPE;        // number of cores
    uint32_t grayscale;        // grayscale if one
} plp_example_kernel_instance_i32;

double euclidian_norm(int8_t *input, int idx_u, int idx_v);
int32_t flow_error(int8_t *input, unsigned char* output, int width, int height);

#endif