/** Copyright 2025 Alexander G. Lopez

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */
#include <stddef.h>

#include "impl/impl_types.h"
#include "types.h"

/** @private */
static char const *const result_msgs[CCC_RESULT_SIZE] = {
    [CCC_RESULT_OK] = "",
    [CCC_RESULT_FAIL]
    = "An operation ran on a container but the desired result could not be "
      "returned, meaning no valid value can be given to the user.",
    [CCC_RESULT_NO_ALLOC]
    = "A container performed an operation requiring new allocation of "
      "memory, but no allocation function was provided upon initialization.",
    [CCC_RESULT_MEM_ERROR]
    = "A container performed an operation requiring new allocation of memory, "
      "but the allocator function provided on initialization failed.",
    [CCC_RESULT_ARG_ERROR]
    = "A container function received bad arguments such as NULL pointers, out "
      "of range values, or arguments that cannot be processed in the context "
      "of an operation.",
};

/*============================   Interface    ===============================*/

ccc_tribool
ccc_entry_occupied(ccc_entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl.stats & CCC_ENTRY_OCCUPIED) != 0;
}

ccc_tribool
ccc_entry_insert_error(ccc_entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl.stats & CCC_ENTRY_INSERT_ERROR) != 0;
}

ccc_tribool
ccc_entry_input_error(ccc_entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl.stats & CCC_ENTRY_ARG_ERROR) != 0;
}

void *
ccc_entry_unwrap(ccc_entry const *const e)
{
    if (!e)
    {
        return NULL;
    }
    return e->impl.stats & CCC_ENTRY_NO_UNWRAP ? NULL : e->impl.e;
}

ccc_tribool
ccc_handle_occupied(ccc_handle const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl.stats & CCC_ENTRY_OCCUPIED) != 0;
}

ccc_tribool
ccc_handle_insert_error(ccc_handle const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl.stats & CCC_ENTRY_INSERT_ERROR) != 0;
}

ccc_tribool
ccc_handle_input_error(ccc_handle const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl.stats & CCC_ENTRY_ARG_ERROR) != 0;
}

ccc_handle_i
ccc_handle_unwrap(ccc_handle const *const e)
{
    if (!e)
    {
        return 0;
    }
    return e->impl.stats & CCC_ENTRY_NO_UNWRAP ? 0 : e->impl.i;
}

void *
ccc_begin_range(ccc_range const *const r)
{
    return r ? r->impl.begin : NULL;
}

void *
ccc_end_range(ccc_range const *const r)
{
    return r ? r->impl.end : NULL;
}

void *
ccc_rbegin_rrange(ccc_rrange const *const r)
{
    return r ? r->impl.rbegin : NULL;
}

void *
ccc_rend_rrange(ccc_rrange const *const r)
{
    return r ? r->impl.rend : NULL;
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
        return CCC_ENTRY_ARG_ERROR;
    }
    return e->impl.stats;
}

ccc_handle_status
ccc_get_handle_status(ccc_handle const *e)
{
    if (!e)
    {
        return CCC_ENTRY_ARG_ERROR;
    }
    return e->impl.stats;
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
        case CCC_ENTRY_VACANT:
            return "vacant with no errors";
            break;
        case CCC_ENTRY_OCCUPIED:
            return "occupied and non-NULL";
            break;
        case CCC_ENTRY_INSERT_ERROR:
            return "insert error has occurred or will occur on insert "
                   "attempted";
            break;
        case CCC_ENTRY_ARG_ERROR:
            return "could not proceed due to bad arguments to a function";
            break;
        case CCC_ENTRY_NO_UNWRAP:
            return "unwrap prohibited in order to protect container";
            break;
        default:
            return "error: encountered an unknown combination of flags";
            break;
    }
}
