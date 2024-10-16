#ifndef CCC_IMPL_TYPES_H
#define CCC_IMPL_TYPES_H

#include <stdint.h>

enum ccc_entry_status_ : uint8_t
{
    CCC_ENTRY_VACANT = 0,
    CCC_ENTRY_OCCUPIED = 0x1,
    CCC_ENTRY_INSERT_ERROR = 0x2,
    CCC_ENTRY_SEARCH_ERROR = 0x4,
    CCC_ENTRY_CONTAINS_NULL = 0x8,
    CCC_ENTRY_DELETE_ERROR = 0x10,
    CCC_ENTRY_INPUT_ERROR = 0x20,
    CCC_ENTRY_OCCUPIED_CONTAINS_NULL
    = (CCC_ENTRY_OCCUPIED | CCC_ENTRY_CONTAINS_NULL),
    CCC_ENTRY_NULL_INSERT_ERROR
    = (CCC_ENTRY_INSERT_ERROR | CCC_ENTRY_CONTAINS_NULL),
};

struct ccc_entry_
{
    void *e_;
    enum ccc_entry_status_ stats_;
};

struct ccc_range_
{
    union
    {
        void *begin_;
        void *rbegin_;
    };
    union
    {
        void *end_;
        void *rend_;
    };
};

#endif /* CCC_IMPL_TYPES_H */
