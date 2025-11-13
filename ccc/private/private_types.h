/** @cond
Copyright 2025 Alexander G. Lopez

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
@endcond */
#ifndef CCC_PRIVATE_TYPES_H
#define CCC_PRIVATE_TYPES_H

/** @cond */
#include <stddef.h>
#include <stdint.h>
/** @endcond */

/** @private */
enum CCC_Entry_status : uint8_t
{
    CCC_ENTRY_VACANT = 0,
    CCC_ENTRY_OCCUPIED = 0x1,
    CCC_ENTRY_INSERT_ERROR = 0x2,
    CCC_ENTRY_ARGUMENT_ERROR = 0x4,
    CCC_ENTRY_NO_UNWRAP = 0x8,
};

/** @private */
struct CCC_Entry
{
    void *type;
    enum CCC_Entry_status status;
};

/** @private */
union CCC_Entry_wrap
{
    struct CCC_Entry private;
};

/** @private */
struct CCC_Handle
{
    size_t index;
    enum CCC_Entry_status status;
};

/** @private */
union CCC_Handle_wrap
{
    struct CCC_Handle private;
};

/** @private */
struct CCC_Range
{
    union
    {
        void *begin;
        void *reverse_begin;
    };
    union
    {
        void *end;
        void *reverse_end;
    };
};

/** @private */
union CCC_Range_wrap
{
    struct CCC_Range private;
};

/** @private */
union CCC_Range_reverse_wrap
{
    struct CCC_Range private;
};

#endif /* CCC_PRIVATE_TYPES_H */
