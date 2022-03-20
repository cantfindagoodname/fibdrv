#ifndef __SSTRING_H__
#define __SSTRING_H__

#ifndef SWAP
#define SWAP(a, b, type) \
    do {                 \
        type *__x = (a); \
        type *__y = (b); \
        *__x ^= *__y;    \
        *__y ^= *__x;    \
        *__x ^= *__y;    \
    } while (0)
#endif

typedef struct sstring_t sstring_t;

void str_free(sstring_t *self);
sstring_t *str_new(void);
sstring_t str_assign(void *copy, size_t size);
void str_add(const sstring_t *str1, const sstring_t *str2, sstring_t *out);
void str_sub(const sstring_t *str1, const sstring_t *str2, sstring_t *out);
void str_mul(const sstring_t *multiplicand,
             const sstring_t *multiplier,
             sstring_t *out);
void str_reverse(sstring_t *str);
void str_resize(sstring_t *str, size_t size);

struct sstring_t {
    char *value;
    size_t size : 58;
    size_t capacity : 6;
};

#endif
