#include "mtf.h"

void MTF_Encode(uint8_t *in, uint8_t *out, size_t n) {
    uint8_t list[256];

    for (int i = 0; i < 256; i++)
        list[i] = i;

    for (size_t i = 0; i < n; i++) {
        int pos = 0;
        while (list[pos] != in[i]) pos++;

        out[i] = pos;

        for (int j = pos; j > 0; j--)
            list[j] = list[j-1];

        list[0] = in[i];
    }
}

void MTF_Decode(uint8_t *in, uint8_t *out, size_t n) {
    uint8_t list[256];

    for (int i = 0; i < 256; i++)
        list[i] = i;

    for (size_t i = 0; i < n; i++) {
        uint8_t x = list[in[i]];
        out[i] = x;

        for (int j = in[i]; j > 0; j--)
            list[j] = list[j-1];

        list[0] = x;
    }
}
