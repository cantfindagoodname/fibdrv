#include <linux/slab.h>
#include <linux/string.h>

#include "sstring.h"

void str_free(sstring_t *self)
{
    kfree(self->value);
    self->value = NULL;
    kfree(self);
}

sstring_t *str_new()
{
    sstring_t *ret = kmalloc(sizeof(sstring_t), GFP_KERNEL);
    ret->value = NULL;
    ret->size = 0;
    ret->capacity = 0;
    /*
    if (ret == NULL){
        fprintf(stderr, "malloc fail");
        exit(1);
    }
    */
    return ret;
}

sstring_t str_assign(void *str, size_t size)
{
    size &= 0x03FFFFFFFFFFFFFF;
    size_t capacity = 64 - __builtin_clzll(size);
    char *value = NULL;
    value = krealloc(value, (1 << capacity) + 1, GFP_KERNEL);
    /*
    if (value == NULL){
        fprintf(stderr, "realloc fail");
        exit(1);
    }
    */
    memcpy(value, str, size);
    value[size] = '\0';
    return (sstring_t){.value = value, .size = size, .capacity = capacity};
}

void str_add(const sstring_t *str1, const sstring_t *str2, sstring_t *out)
{
    if (str1->size < str2->size) {
        SWAP((size_t *) &str1, (size_t *) &str2, size_t);
    }

    char *str_buf = kmalloc(str1->size + 2, GFP_KERNEL);
    int i, sum, carry = 0;

    for (i = 0; i < str2->size; ++i) {
        sum = (str1->value[str1->size - i - 1] & 0x0F) +
              (str2->value[str2->size - i - 1] & 0x0F) + carry;
        str_buf[i] = (sum % 10) | 0x30;
        carry = sum / 10;
    }

    for (; i < str1->size; ++i) {
        sum = (str1->value[str1->size - i - 1] & 0x0F) + carry;
        str_buf[i] = (sum % 10) | 0x30;
        carry = sum / 10;
    }

    if (carry)
        str_buf[i++] = carry | 0x30;
    str_buf[i] = '\0';

    if (out->value)
        kfree(out->value);
    *out = str_assign(str_buf, i);
    str_reverse(out);
    kfree(str_buf);
}

void str_sub(const sstring_t *str1, const sstring_t *str2, sstring_t *out)
{
    if (str1->size < str2->size ||
        (str1->size == str2->size && str1->value[0] < str2->value[0]))
        SWAP((size_t *) &str1, (size_t *) &str2, size_t);

    char *str_buf = kmalloc(str1->size + 1, GFP_KERNEL);
    int i, sum, borrow = 0;

    for (i = 0; i < str2->size; ++i) {
        sum = (str1->value[str1->size - i - 1] & 0x0F) -
              (str2->value[str2->size - i - 1] & 0x0F) - borrow;
        borrow = sum < 0;
        str_buf[i] = (sum + (10 & -borrow)) | 0x30;
    }

    for (; i < str1->size; ++i) {
        sum = (str1->value[str1->size - i - 1] & 0x0F) - borrow;
        borrow = sum < 0;
        str_buf[i] = (sum + (10 & -borrow)) | 0x30;
    }

    str_buf[i] = '\0';

    if (out->value)
        kfree(out->value);
    *out = str_assign(str_buf, i);
    str_reverse(out);
    kfree(str_buf);
}

void str_reverse(sstring_t *str)
{
    for (int i = 0; i < (str->size >> 1); ++i)
        SWAP(&str->value[i], &str->value[str->size - i - 1], char);
}

void str_resize(sstring_t *str, size_t size)
{
    size &= 0x03FFFFFFFFFFFFFF;
    size_t capacity = 64 - __builtin_clzll(size);
    str->value = krealloc(str->value, (1 << capacity) + 1, GFP_KERNEL);
    /*
    if (str->value == NULL){
        fprintf(stderr, "realloc fail");
        exit(1);
    }
    */
    str->size = size;
    str->capacity = capacity;
}

void str_mul(const sstring_t *multiplicand,
             const sstring_t *multiplier,
             sstring_t *out)
{
    int i = multiplier->size - 1;
    for (int dig = 0; i >= 0; --i, ++dig) {
        sstring_t *addNum = str_new();
        *addNum = str_assign("0", 1);
        for (int ldigit = multiplier->value[i] & 0x0F; ldigit > 0; --ldigit)
            str_add(multiplicand, addNum, addNum);

        int temp = addNum->size;
        str_resize(addNum, addNum->size + dig);
        char *cp = &(addNum->value[temp]);
        for (int t = 0; t < dig; ++t)
            *cp++ = '0';
        *cp = '\0';
        str_add(out, addNum, out);
        str_free(addNum);
    }
}
