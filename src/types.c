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

#include "private/private_types.h"
#include "types.h"

/** @private */
static char const *const result_msgs[CCC_PRIVATE_RESULT_COUNT] = {
    [CCC_RESULT_OK] = "",
    [CCC_RESULT_FAIL]
    = "An operation ran on a container but the desired result could not be "
      "returned, meaning no valid value can be given to the user.",
    [CCC_RESULT_NO_ALLOC]
    = "A container performed an operation requiring new allocation of "
      "memory, but no allocation function was provided upon initialization.",
    [CCC_RESULT_ALLOCATOR_ERROR]
    = "A container performed an operation requiring new allocation of memory, "
      "but the allocator function provided on initialization failed.",
    [CCC_RESULT_ARGUMENT_ERROR]
    = "A container function received bad arguments such as NULL pointers, out "
      "of range values, or arguments that cannot be processed in the context "
      "of an operation.",
};

/*============================   Interface    ===============================*/

CCC_Tribool
CCC_entry_occupied(CCC_Entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl.stats & CCC_ENTRY_OCCUPIED) != 0;
}

CCC_Tribool
CCC_entry_insert_error(CCC_Entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl.stats & CCC_ENTRY_INSERT_ERROR) != 0;
}

CCC_Tribool
CCC_entry_input_error(CCC_Entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl.stats & CCC_ENTRY_ARG_ERROR) != 0;
}

void *
CCC_entry_unwrap(CCC_Entry const *const e)
{
    if (!e)
    {
        return NULL;
    }
    return e->impl.stats & CCC_ENTRY_NO_UNWRAP ? NULL : e->impl.e;
}

CCC_Tribool
CCC_handle_occupied(CCC_Handle const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl.stats & CCC_ENTRY_OCCUPIED) != 0;
}

CCC_Tribool
CCC_handle_insert_error(CCC_Handle const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl.stats & CCC_ENTRY_INSERT_ERROR) != 0;
}

CCC_Tribool
CCC_handle_input_error(CCC_Handle const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl.stats & CCC_ENTRY_ARG_ERROR) != 0;
}

CCC_Handle_index
CCC_handle_unwrap(CCC_Handle const *const e)
{
    if (!e)
    {
        return 0;
    }
    return e->impl.stats & CCC_ENTRY_NO_UNWRAP ? 0 : e->impl.i;
}

void *
CCC_range_begin(CCC_Range const *const r)
{
    return r ? r->impl.begin : NULL;
}

void *
CCC_range_end(CCC_Range const *const r)
{
    return r ? r->impl.end : NULL;
}

void *
CCC_rrange_rbegin(CCC_Reverse_range const *const r)
{
    return r ? r->impl.rbegin : NULL;
}

void *
CCC_rrange_rend(CCC_Reverse_range const *const r)
{
    return r ? r->impl.rend : NULL;
}

char const *
CCC_Result_msg(CCC_Result const res)
{
    if (res >= CCC_PRIVATE_RESULT_COUNT)
    {
        return "error: invalid result provided no message exists";
    }
    return result_msgs[res];
}

CCC_Entry_status
CCC_get_entry_status(CCC_Entry const *e)
{
    if (!e)
    {
        return CCC_ENTRY_ARG_ERROR;
    }
    return e->impl.stats;
}

CCC_Handle_status
CCC_get_handle_status(CCC_Handle const *e)
{
    if (!e)
    {
        return CCC_ENTRY_ARG_ERROR;
    }
    return e->impl.stats;
}

char const *
CCC_handle_status_msg(CCC_Handle_status const status)
{
    return CCC_Entry_status_msg(status);
}

char const *
CCC_Entry_status_msg(CCC_Entry_status const status)
{
    switch (status)
    {
        case CCC_ENTRY_VACANT:
            return "vacant with no errors";
            break;
        case CCC_ENTRY_OCCUPIED:
            return "occupied and non-NULL with no errors";
            break;
        case CCC_ENTRY_INSERT_ERROR:
            return "insert error has occurred or will occur on next insert";
            break;
        case CCC_ENTRY_ARG_ERROR:
            return "could not proceed due to bad arguments to a function";
            break;
        case CCC_ENTRY_NO_UNWRAP:
            return "unwrap prohibited in order to protect container integrity";
            break;
        default:
            return "error: encountered an unknown combination of entry/handle "
                   "flags";
            break;
    }
}
