#ifndef CCC_IMPL_TYPES_H
#define CCC_IMPL_TYPES_H

#include <stdint.h>

struct ccc_entry_
{
    void *e_;
    uint8_t stats_;
};

struct ccc_range_
{
    void *begin_;
    void *end_;
};

struct ccc_rrange_
{
    void *rbegin_;
    void *end_;
};

#endif /* CCC_IMPL_TYPES_H */
