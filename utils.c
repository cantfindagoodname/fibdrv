#include "utils.h"

inline void free_stack(void *pt)
{
    kfree(*(void **) pt);
}
