#include "types.h"

bool
ccc_entry_occupied(ccc_entry const *const e)
{
    return e->impl_.stats_ & CCC_ENTRY_OCCUPIED;
}

bool
ccc_entry_error(ccc_entry const *const e)
{
    return e->impl_.stats_ == CCC_ENTRY_ERROR;
}

void *
ccc_entry_unwrap(ccc_entry const *const e)
{
    return e->impl_.e_;
}

void *
ccc_begin_range(ccc_range const *const r)
{
    return r->impl_.begin_;
}

void *
ccc_end_range(ccc_range const *const r)
{
    return r->impl_.end_;
}

void *
ccc_begin_rrange(ccc_rrange const *const r)
{
    return r->impl_.rbegin_;
}

void *
ccc_end_rrange(ccc_rrange const *const r)
{
    return r->impl_.end_;
}
