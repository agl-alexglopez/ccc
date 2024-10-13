#include "types.h"
#include "impl_types.h"

static char const *const result_msgs[CCC_RESULTS_SIZE] = {
    [CCC_OK] = "",
    [CCC_NO_ALLOC]
    = "A container performed an operation requiring new allocation of "
      "memory, but no allocation function was provided upon initialization.",
    [CCC_MEM_ERR]
    = "A container performed an operation requiring new allocation of memory, "
      "but the allocator function provided on initialization failed.",
    [CCC_INPUT_ERR]
    = "A container function received bad arguments such as NULL pointers or "
      "arguments that cannot be processed in the context of an operation.",
};

/*============================   Interface    ===============================*/

char const *
ccc_result_msg(ccc_result const res)
{
    return result_msgs[res];
}

bool
ccc_entry_occupied(ccc_entry const *const e)
{
    return e ? e->impl_.stats_ & CCC_ENTRY_OCCUPIED : false;
}

bool
ccc_entry_insert_error(ccc_entry const *const e)
{
    return e ? e->impl_.stats_ & CCC_ENTRY_INSERT_ERROR : false;
}

void *
ccc_entry_unwrap(ccc_entry const *const e)
{
    return e ? e->impl_.e_ : NULL;
}

void *
ccc_begin_range(ccc_range const *const r)
{
    return r ? r->impl_.begin_ : NULL;
}

void *
ccc_end_range(ccc_range const *const r)
{
    return r ? r->impl_.end_ : NULL;
}

void *
ccc_rbegin_rrange(ccc_rrange const *const r)
{
    return r ? r->impl_.rbegin_ : NULL;
}

void *
ccc_rend_rrange(ccc_rrange const *const r)
{
    return r ? r->impl_.rend_ : NULL;
}
