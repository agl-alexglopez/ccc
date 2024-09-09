#include "types.h"

bool
ccc_entry_occupied(ccc_entry const e)
{
    return e.status & CCC_ENTRY_OCCUPIED;
}

bool
ccc_entry_error(ccc_entry const e)
{
    return e.status == CCC_ENTRY_ERROR;
}

void *
ccc_entry_unwrap(ccc_entry const e)
{
    return e.entry;
}

void *
ccc_begin_range(ccc_range const *const r)
{
    return r->begin;
}

void *
ccc_end_range(ccc_range const *const r)
{
    return r->end;
}

void *
ccc_begin_rrange(ccc_rrange const *const r)
{
    return r->rbegin;
}

void *
ccc_end_rrange(ccc_rrange const *const r)
{
    return r->end;
}
