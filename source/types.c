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

/** @internal */
static char const *const result_messages[CCC_PRIVATE_RESULT_COUNT] = {
    [CCC_RESULT_OK] = "",
    [CCC_RESULT_FAIL]
    = "An operation ran on a container but the desired result could not be "
      "returned, meaning no valid value can be given to the user.",
    [CCC_RESULT_NO_ALLOCATION_FUNCTION]
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
CCC_entry_occupied(CCC_Entry const *const entry)
{
    if (!entry)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (entry->private.status & CCC_ENTRY_OCCUPIED) != 0;
}

CCC_Tribool
CCC_entry_insert_error(CCC_Entry const *const entry)
{
    if (!entry)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (entry->private.status & CCC_ENTRY_INSERT_ERROR) != 0;
}

CCC_Tribool
CCC_entry_input_error(CCC_Entry const *const entry)
{
    if (!entry)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (entry->private.status & CCC_ENTRY_ARGUMENT_ERROR) != 0;
}

void *
CCC_entry_unwrap(CCC_Entry const *const entry)
{
    if (!entry)
    {
        return NULL;
    }
    return entry->private.status & CCC_ENTRY_NO_UNWRAP ? NULL
                                                       : entry->private.type;
}

CCC_Tribool
CCC_handle_occupied(CCC_Handle const *const handle)
{
    if (!handle)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (handle->private.status & CCC_ENTRY_OCCUPIED) != 0;
}

CCC_Tribool
CCC_handle_insert_error(CCC_Handle const *const handle)
{
    if (!handle)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (handle->private.status & CCC_ENTRY_INSERT_ERROR) != 0;
}

CCC_Tribool
CCC_handle_input_error(CCC_Handle const *const handle)
{
    if (!handle)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (handle->private.status & CCC_ENTRY_ARGUMENT_ERROR) != 0;
}

CCC_Handle_index
CCC_handle_unwrap(CCC_Handle const *const handle)
{
    if (!handle)
    {
        return 0;
    }
    return handle->private.status & CCC_ENTRY_NO_UNWRAP ? 0
                                                        : handle->private.index;
}

void *
CCC_range_begin(CCC_Range const *const range)
{
    return range ? range->private.begin : NULL;
}

void *
CCC_range_end(CCC_Range const *const range)
{
    return range ? range->private.end : NULL;
}

void *
CCC_range_reverse_begin(CCC_Range_reverse const *const range)
{
    return range ? range->private.reverse_begin : NULL;
}

void *
CCC_range_reverse_end(CCC_Range_reverse const *const range)
{
    return range ? range->private.reverse_end : NULL;
}

CCC_Handle_index
CCC_handle_range_begin(CCC_Handle_range const *const range)
{
    return range ? range->private.begin : 0;
}

CCC_Handle_index
CCC_handle_range_end(CCC_Handle_range const *const range)
{
    return range ? range->private.end : 0;
}

CCC_Handle_index
CCC_handle_range_reverse_begin(CCC_Handle_range_reverse const *const range)
{
    return range ? range->private.reverse_begin : 0;
}

CCC_Handle_index
CCC_handle_range_reverse_end(CCC_Handle_range_reverse const *const range)
{
    return range ? range->private.reverse_end : 0;
}

char const *
CCC_result_message(CCC_Result const result)
{
    if (result >= CCC_PRIVATE_RESULT_COUNT)
    {
        return "error: invalid result provided no message exists";
    }
    return result_messages[result];
}

CCC_Entry_status
CCC_get_entry_status(CCC_Entry const *entry)
{
    if (!entry)
    {
        return CCC_ENTRY_ARGUMENT_ERROR;
    }
    return entry->private.status;
}

CCC_Handle_status
CCC_get_handle_status(CCC_Handle const *handle)
{
    if (!handle)
    {
        return CCC_ENTRY_ARGUMENT_ERROR;
    }
    return handle->private.status;
}

char const *
CCC_handle_status_message(CCC_Handle_status const status)
{
    return CCC_entry_status_message(status);
}

char const *
CCC_entry_status_message(CCC_Entry_status const status)
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
        case CCC_ENTRY_ARGUMENT_ERROR:
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
