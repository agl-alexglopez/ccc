#include "types.h"
#include "impl/impl_types.h"

#include <stddef.h>

/** @private */
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

bool
ccc_entry_input_error(ccc_entry const *const e)
{
    return e ? e->impl_.stats_ & CCC_ENTRY_INPUT_ERROR : false;
}

void *
ccc_entry_unwrap(ccc_entry const *const e)
{
    if (!e)
    {
        return NULL;
    }
    return e->impl_.stats_ & CCC_ENTRY_NO_UNWRAP ? NULL : e->impl_.e_;
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

char const *
ccc_result_msg(ccc_result const res)
{
    if (res >= CCC_RESULTS_SIZE)
    {
        return "error: invalid result provided no message exists";
    }
    return result_msgs[res];
}

ccc_entry_status
ccc_get_entry_status(ccc_entry const *e)
{
    if (!e)
    {
        return CCC_ENTRY_INPUT_ERROR;
    }
    return e->impl_.stats_;
}

char const *
ccc_entry_status_msg(ccc_entry_status const status)
{
    switch (status)
    {
    case CCC_ENTRY_VACANT:
        return "entry is Vacant with no errors";
        break;
    case CCC_ENTRY_OCCUPIED:
        return "entry is Occupied and non-NULL";
        break;
    case CCC_ENTRY_INSERT_ERROR:
        return "entry should have been inserted but encountered an error";
        break;
    case CCC_ENTRY_SEARCH_ERROR:
        return "entry encountered an error while searching for a key";
        break;
    case CCC_ENTRY_DELETE_ERROR:
        return "entry encountered an error while trying to delete";
        break;
    case CCC_ENTRY_INPUT_ERROR:
        return "entry could not be produced due to bad input to function";
        break;
    case CCC_ENTRY_NO_UNWRAP:
        return "entry shall not be unwrapped by user to protect container";
        break;
    default:
        return "error: entry encountered an unknown combination of flags";
        break;
    }
}
