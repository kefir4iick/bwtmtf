#include "bwt.h"
#include <stdlib.h>
#include <string.h>

static int *rank_arr;
static int *tmp;
static uint8_t *text;
static size_t N;

static int cmp_sa(const void *a, const void *b) {
    int i = *(int*)a;
    int j = *(int*)b;

    if (rank_arr[i] != rank_arr[j])
        return rank_arr[i] - rank_arr[j];

    int ri = (i + tmp[0] < N) ? rank_arr[i + tmp[0]] : -1;
    int rj = (j + tmp[0] < N) ? rank_arr[j + tmp[0]] : -1;

    return ri - rj;
}

BWTResult BWT_Transform(uint8_t *input, size_t n) {
    int *sa = malloc(n * sizeof(int));
    rank_arr = malloc(n * sizeof(int));
    tmp = malloc(n * sizeof(int));

    text = input;
    N = n;

    for (size_t i = 0; i < n; i++) {
        sa[i] = i;
        rank_arr[i] = input[i];
    }

    for (tmp[0] = 1; tmp[0] < n; tmp[0] *= 2) {
        qsort(sa, n, sizeof(int), cmp_sa);

        tmp[sa[0]] = 0;
        for (size_t i = 1; i < n; i++) {
            tmp[sa[i]] = tmp[sa[i-1]] +
                (cmp_sa(&sa[i-1], &sa[i]) < 0);
        }

        for (size_t i = 0; i < n; i++)
            rank_arr[i] = tmp[i];
    }

    uint8_t *L = malloc(n);
    size_t primary = 0;

    for (size_t i = 0; i < n; i++) {
        if (sa[i] == 0)
            primary = i;

        L[i] = input[(sa[i] + n - 1) % n];
    }

    free(sa);
    free(rank_arr);
    free(tmp);

    BWTResult res = {L, n, primary};
    return res;
}





void BWT_Inverse(uint8_t *L, size_t n, size_t primary, uint8_t *out) {
    int count[256] = {0};
    int *occ = malloc(n * sizeof(int));

    for (size_t i = 0; i < n; i++) {
        occ[i] = count[L[i]];
        count[L[i]]++;
    }

    int sum = 0;
    for (int i = 0; i < 256; i++) {
        int t = count[i];
        count[i] = sum;
        sum += t;
    }

    size_t idx = primary;

    for (ssize_t i = n - 1; i >= 0; i--) {
        out[i] = L[idx];
        idx = count[L[idx]] + occ[idx];
    }

    free(occ);
}
