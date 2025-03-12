#include <stddef.h>

#include "impl/impl_types.h"
#include "types.h"

/** @private */
static char const *const result_msgs[CCC_RESULT_SIZE] = {
    [CCC_RESULT_OK] = "",
    [CCC_RESULT_NO_ALLOC]
    = "A container performed an operation requiring new allocation of "
      "memory, but no allocation function was provided upon initialization.",
    [CCC_RESULT_MEM_ERR]
    = "A container performed an operation requiring new allocation of memory, "
      "but the allocator function provided on initialization failed.",
    [CCC_RESULT_ARG_ERROR]
    = "A container function received bad arguments such as NULL pointers or "
      "arguments that cannot be processed in the context of an operation.",
};

/*============================   Interface    ===============================*/

ccc_tribool
ccc_entry_occupied(ccc_entry const *const e)
{
    if (!e)
    {
        return CCC_BOOL_ERR;
    }
    return (e->impl_.stats_ & CCC_OCCUPIED) != 0;
}

ccc_tribool
ccc_entry_insert_error(ccc_entry const *const e)
{
    if (!e)
    {
        return CCC_BOOL_ERR;
    }
    return (e->impl_.stats_ & CCC_INSERT_ERROR) != 0;
}

ccc_tribool
ccc_entry_input_error(ccc_entry const *const e)
{
    if (!e)
    {
        return CCC_BOOL_ERR;
    }
    return (e->impl_.stats_ & CCC_ARG_ERROR) != 0;
}

void *
ccc_entry_unwrap(ccc_entry const *const e)
{
    if (!e)
    {
        return NULL;
    }
    return e->impl_.stats_ & CCC_NO_UNWRAP ? NULL : e->impl_.e_;
}

ccc_tribool
ccc_handle_occupied(ccc_handle const *const e)
{
    if (!e)
    {
        return CCC_BOOL_ERR;
    }
    return (e->impl_.stats_ & CCC_OCCUPIED) != 0;
}

ccc_tribool
ccc_handle_insert_error(ccc_handle const *const e)
{
    if (!e)
    {
        return CCC_BOOL_ERR;
    }
    return (e->impl_.stats_ & CCC_INSERT_ERROR) != 0;
}

ccc_tribool
ccc_handle_input_error(ccc_handle const *const e)
{
    if (!e)
    {
        return CCC_BOOL_ERR;
    }
    return (e->impl_.stats_ & CCC_ARG_ERROR) != 0;
}

ccc_handle_i
ccc_handle_unwrap(ccc_handle const *const e)
{
    if (!e)
    {
        return 0;
    }
    return e->impl_.stats_ & CCC_NO_UNWRAP ? 0 : e->impl_.i_;
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
    if (res >= CCC_RESULT_SIZE)
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
        return CCC_ARG_ERROR;
    }
    return e->impl_.stats_;
}

ccc_handle_status
ccc_get_handle_status(ccc_handle const *e)
{
    if (!e)
    {
        return CCC_ARG_ERROR;
    }
    return e->impl_.stats_;
}

char const *
ccc_handle_status_msg(ccc_handle_status const status)
{
    return ccc_entry_status_msg(status);
}

char const *
ccc_entry_status_msg(ccc_entry_status const status)
{
    switch (status)
    {
    case CCC_VACANT:
        return "vacant with no errors";
        break;
    case CCC_OCCUPIED:
        return "occupied and non-NULL";
        break;
    case CCC_INSERT_ERROR:
        return "insert error has or will occur when insert is attempted";
        break;
    case CCC_ARG_ERROR:
        return "could not proceed due to bad input to function";
        break;
    case CCC_NO_UNWRAP:
        return "unwrap denied in order to protect container";
        break;
    default:
        return "error: encountered an unknown combination of flags";
        break;
    }
}
