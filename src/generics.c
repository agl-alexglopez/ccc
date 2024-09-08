#include "generics.h"
#include "types.h"

inline bool
ccc_entry_occupied(ccc_entry const e)
{
    return e.status & CCC_ENTRY_OCCUPIED;
}

inline bool
ccc_entry_error(ccc_entry const e)
{
    return e.status == CCC_ENTRY_ERROR;
}

inline void *
ccc_entry_unwrap(ccc_entry const e)
{
    return e.entry;
}

inline void *
ccc_begin_range(ccc_range const *const r)
{
    return r->begin;
}

inline void *
ccc_end_range(ccc_range const *const r)
{
    return r->end;
}

inline void *
ccc_begin_rrange(ccc_rrange const *const r)
{
    return r->rbegin;
}

inline void *
ccc_end_rrange(ccc_rrange const *const r)
{
    return r->end;
}
