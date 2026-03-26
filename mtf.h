#ifndef MTF_H
#define MTF_H

#include <stddef.h>
#include <stdint.h>

void MTF_Encode(uint8_t *in, uint8_t *out, size_t n);
void MTF_Decode(uint8_t *in, uint8_t *out, size_t n);

#endif
