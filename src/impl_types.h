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
    union {
        void *begin_;
        void *rbegin_;
    };
    union {
        void *end_;
        void *rend_;
    };
};

#endif /* CCC_IMPL_TYPES_H */
