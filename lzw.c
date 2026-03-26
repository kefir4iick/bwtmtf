#include "lzw.h"
#include "bitstream.h"
#include "bwt.h"
#include "mtf.h"
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define MAX_DICT 65536
#define HASH_SIZE 131071

typedef struct {
    int prefix;
    uint8_t c;
    int code;
} DictEntry;

static DictEntry dict[MAX_DICT];
static int hash_table[HASH_SIZE];

static int Hash(int prefix, uint8_t c) {
    return (prefix * 257 + c) % HASH_SIZE;
}

static int Find(int prefix, uint8_t c) {
    int h = Hash(prefix, c);
    while (hash_table[h] != -1) {
        int idx = hash_table[h];
        if (dict[idx].prefix == prefix && dict[idx].c == c)
            return idx;
        h = (h + 1) % HASH_SIZE;
    }
    return -1;
}

static void Insert(int prefix, uint8_t c, int code) {
    int h = Hash(prefix, c);
    while (hash_table[h] != -1)
        h = (h + 1) % HASH_SIZE;

    hash_table[h] = code;
    dict[code].prefix = prefix;
    dict[code].c = c;
    dict[code].code = code;
}


void LZWEncode(int in_fd, int out_fd, int max_bits, int reset) {
    int max_dict = 1 << max_bits;

    for (int i = 0; i < HASH_SIZE; i++)
        hash_table[i] = -1;

    for (int i = 0; i < 256; i++) {
        dict[i].prefix = -1;
        dict[i].c = i;
    }

    int dict_size = 256;

    off_t size = lseek(in_fd, 0, SEEK_END);
    lseek(in_fd, 0, SEEK_SET);

    BitStream *bs = BitStreamOpenFD(out_fd, 1);

    if (size <= 0) {
        WriteUInt64(bs, max_bits, 8);
        WriteUInt64(bs, reset, 1);
        WriteUInt64(bs, 0, 64);
        WriteUInt64(bs, 0, 1); 
        BitStreamClose(bs);
        return;
    }

    uint8_t *buf = malloc(size);
    if (read(in_fd, buf, size) != size) {
        perror("read");
        exit(1);
    }

    BWTResult bwt = BWT_Transform(buf, size);

    uint8_t *mtf = malloc(size);
    MTF_Encode(bwt.data, mtf, size);

    WriteUInt64(bs, max_bits, 8);
    WriteUInt64(bs, reset, 1);
    WriteUInt64(bs, size, 64);
    WriteUInt64(bs, 1, 1); 
    WriteUInt64(bs, bwt.primary, 64);

    size_t pos = 0;
    uint8_t c = mtf[pos++];
    int prefix = c;

    while (pos < size) {
        c = mtf[pos++];
        int found = Find(prefix, c);

        if (found != -1) {
            prefix = found;
        } else {
            WriteUInt64(bs, prefix, max_bits);

            if (dict_size < max_dict) {
                Insert(prefix, c, dict_size++);
            } else if (reset) {
                dict_size = 256;
                for (int i = 0; i < HASH_SIZE; i++)
                    hash_table[i] = -1;
            }

            prefix = c;
        }
    }

    WriteUInt64(bs, prefix, max_bits);

    BitStreamFlush(bs);
    BitStreamClose(bs);

    free(buf);
    free(bwt.data);
    free(mtf);
}


static uint8_t stack[MAX_DICT];

static int DecodeString(int code, uint8_t *buf) {
    int i = 0;
    while (code != -1) {
        buf[i++] = dict[code].c;
        code = dict[code].prefix;
    }
    return i;
}

void LZWDecode(int in_fd, int out_fd) {
    BitStream *bs = BitStreamOpenFD(in_fd, 0);

    int max_bits = ReadUInt64(bs, 8);
    int reset = ReadUInt64(bs, 1);
    uint64_t size = ReadUInt64(bs, 64);

    int use_bwt = ReadUInt64(bs, 1);
    uint64_t primary = 0;

    if (use_bwt)
        primary = ReadUInt64(bs, 64);

    int max_dict = 1 << max_bits;

    for (int i = 0; i < 256; i++) {
        dict[i].prefix = -1;
        dict[i].c = i;
    }

    int dict_size = 256;

    if (size == 0) {
        BitStreamClose(bs);
        return;
    }

    uint8_t *mtf = malloc(size);

    int prev = ReadUInt64(bs, max_bits);
    uint8_t first_char = dict[prev].c;

    mtf[0] = first_char;
    uint64_t written = 1;

    while (written < size) {
        int code = ReadUInt64(bs, max_bits);

        int len;

        if (code < dict_size) {
            len = DecodeString(code, stack);
        } else {
            stack[0] = first_char;
            len = DecodeString(prev, stack + 1) + 1;
        }

        for (int i = len - 1; i >= 0 && written < size; i--) {
            mtf[written++] = stack[i];
        }

        first_char = stack[len - 1];

        if (dict_size < max_dict) {
            dict[dict_size].prefix = prev;
            dict[dict_size].c = first_char;
            dict_size++;
        } else if (reset) {
            dict_size = 256;
        }

        prev = code;
    }

    BitStreamClose(bs);

    if (use_bwt) {
        uint8_t *bwt = malloc(size);
        MTF_Decode(mtf, bwt, size);

        uint8_t *orig = malloc(size);
        BWT_Inverse(bwt, size, primary, orig);

        if (write(out_fd, orig, size) != size) {
            perror("write");
            exit(1);
        }

        free(bwt);
        free(orig);
    } else {
        if (write(out_fd, mtf, size) != size) {
            perror("write");
            exit(1);
        }
    }

    free(mtf);
}
