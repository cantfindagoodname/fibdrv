#include "bn.h"

void assign(bn_t **selfp, uint32_t n)
{
    *selfp = krealloc(*selfp, sizeof(bn_t) + sizeof(uint32_t), GFP_KERNEL);
    (*selfp)->size = 1;
    (*selfp)->arr[0] = n;
}

__attribute__((nonnull())) void display(const bn_t *self)
{
    printk(KERN_INFO "Value : ");
    printk(KERN_INFO "%u", *(self->arr));
    for (size_t i = 1; i < self->size; ++i) {
        printk(KERN_INFO "%0*u", MAX_LEN, self->arr[i]);
    }
    printk(KERN_INFO "\n");
}

__attribute__((nonnull(1, 2))) void add(const bn_t *n1,
                                        const bn_t *n2,
                                        bn_t **out)
{
    if (n1->size < n2->size)
        SWAP((uintptr_t *) &n1, (uintptr_t *) &n2, uintptr_t);

    autofree uint32_t *arr =
        kcalloc(sizeof(uint32_t), n1->size + 1, GFP_KERNEL);
    int i, sum, carry = 0;

    for (i = 0; i < n2->size; ++i) {
        sum = n2->arr[n2->size - i - 1] + n1->arr[n1->size - i - 1] + carry;
        arr[i] = sum % MASK;
        carry = sum / MASK;
    }

    for (; i < n1->size; ++i) {
        sum = n1->arr[n1->size - i - 1] + carry;
        arr[i] = sum % MASK;
        carry = sum / MASK;
    }

    if (carry)
        arr[i++] = carry;

    *out = krealloc(*out, sizeof(bn_t) + i * sizeof(uint32_t), GFP_KERNEL);
    (*out)->size = i;
    for (int index = 0; index < i; ++index)
        (*out)->arr[index] = arr[i - index - 1];
}

__attribute__((nonnull(1, 2))) void sub(const bn_t *n1,
                                        const bn_t *n2,
                                        bn_t **out)
{
    /* Only calculate difference in two numbers */
    if (n1->size < n2->size ||
        (n1->size == n2->size && n1->arr[0] < n2->arr[0]))
        SWAP((uintptr_t *) &n1, (uintptr_t *) &n2, uintptr_t);
    autofree uint32_t *arr =
        kcalloc(sizeof(uint32_t), n1->size + 1, GFP_KERNEL);
    int i, diff, borrow = 0;

    for (i = 0; i < n2->size; ++i) {
        diff = n1->arr[n1->size - i - 1] - n2->arr[n2->size - i - 1] - borrow;
        borrow = diff < 0;
        arr[i] = (diff + (MASK & -borrow));
    }

    for (; i < n1->size; ++i) {
        diff = n1->arr[n1->size - i - 1] - borrow;
        borrow = diff < 0;
        arr[i] = (diff + (MASK & -borrow));
    }

    *out = krealloc(*out, sizeof(bn_t) + i * sizeof(uint32_t), GFP_KERNEL);
    (*out)->size = i;
    for (int index = 0; index < i; ++index)
        (*out)->arr[index] = arr[i - index - 1];
}

__attribute__((nonnull(1, 2))) void mul(const bn_t *n1,
                                        const bn_t *n2,
                                        bn_t **out)
{
    autofree bn_t *result = NULL;
    autofree bn_t *multiplicand = NULL, *multiplier = NULL;
    assign(&result, 0);
    assign(&multiplicand, 0);
    assign(&multiplier, 0);
    add(multiplicand, n1, &multiplicand);
    add(multiplier, n2, &multiplier);

    for (; multiplier->arr[0] != 0; sr(multiplier, &multiplier)) {
        for (int i = 0; i < multiplier->arr[multiplier->size - 1] % 10; ++i)
            add(result, multiplicand, &result);
        sl(multiplicand, &multiplicand);
    }

    *out = krealloc(*out, sizeof(bn_t) + result->size * sizeof(uint32_t),
                    GFP_KERNEL);
    (*out)->size = result->size;
    for (int i = 0; i < result->size; ++i)
        (*out)->arr[i] = result->arr[i];
}

void sl(const bn_t *n, bn_t **out)
{
    autofree uint32_t *arr = kcalloc(sizeof(uint32_t), n->size + 1, GFP_KERNEL);
    int i, carry = 0;
    for (i = n->size - 1; i >= 0; --i) {
        arr[i + 1] = (n->arr[i] * 10) % MASK + carry;
        carry = (n->arr[i] * 10) / MASK;
    }
    if (carry)
        arr[0] = carry;

    size_t sz = n->size; /* Memory may be invalid after realloc */
    *out = krealloc(*out, sizeof(bn_t) + (sz + (carry > 0)) * sizeof(uint32_t),
                    GFP_KERNEL);
    (*out)->size = sz + (carry > 0);
    for (int index = 0; index < (*out)->size; ++index)
        (*out)->arr[index] = arr[index + (carry <= 0)];
}

void sr(const bn_t *n, bn_t **out)
{
    autofree uint32_t *arr = kcalloc(sizeof(uint32_t), n->size, GFP_KERNEL);
    int i, carry = 0;
    for (i = 0; i < n->size; ++i) {
        arr[i] = n->arr[i] / 10 + ((carry * MASK) / 10);
        carry = n->arr[i] % 10;
    }

    *out = krealloc(*out,
                    sizeof(bn_t) + (i - (i != 1 && !arr[0])) * sizeof(uint32_t),
                    GFP_KERNEL);
    (*out)->size = i - (i != 1 && !arr[0]);
    for (int index = (i != 1 && !arr[0]); index < i; ++index)
        (*out)->arr[index - (i != 1 && !arr[0])] = arr[index];
}
