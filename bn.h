#ifndef __BN_H__
#define __BN_H__

#include <linux/slab.h>
#include <linux/types.h>

#include "utils.h"

#define MAX_LEN 8
#define MASK ((uint32_t)(1e8)) /* 32-bit up to 1e10 (-2) for carry */

typedef struct bn {
    size_t size;
    uint32_t arr[]; /* Flexible array */
} bn_t;

void display(const bn_t *);
void assign(bn_t **, uint32_t);
void add(const bn_t *, const bn_t *, bn_t **);
void sub(const bn_t *, const bn_t *, bn_t **);
void mul(const bn_t *, const bn_t *, bn_t **);
void sl(const bn_t *, bn_t **);
void sr(const bn_t *, bn_t **);

#endif
