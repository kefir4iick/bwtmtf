#ifndef BWT_H
#define BWT_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *data;
    size_t n;
    size_t primary;
} BWTResult;

BWTResult BWT_Transform(uint8_t *input, size_t n);
void BWT_Inverse(uint8_t *L, size_t n, size_t primary, uint8_t *out);

#endif
