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
#ifndef CCC_IMPL_TYPES_H
#define CCC_IMPL_TYPES_H

/** @cond */
#include <stddef.h>
#include <stdint.h>
/** @endcond */

/** @private */
enum CCC_entry_status : uint8_t
{
    CCC_ENTRY_VACANT = 0,
    CCC_ENTRY_OCCUPIED = 0x1,
    CCC_ENTRY_INSERT_ERROR = 0x2,
    CCC_ENTRY_ARG_ERROR = 0x4,
    CCC_ENTRY_NO_UNWRAP = 0x8,
};

/** @private */
struct CCC_ent
{
    void *e;
    enum CCC_entry_status stats;
};

/** @private */
union CCC_entry
{
    struct CCC_ent impl;
};

/** @private */
struct CCC_handl
{
    size_t i;
    enum CCC_entry_status stats;
};

/** @private */
union CCC_handle
{
    struct CCC_handl impl;
};

/** @private */
struct CCC_range_u
{
    union
    {
        void *begin;
        void *rbegin;
    };
    union
    {
        void *end;
        void *rend;
    };
};

/** @private */
union CCC_range
{
    struct CCC_range_u impl;
};

/** @private */
union CCC_rrange
{
    struct CCC_range_u impl;
};

#endif /* CCC_IMPL_TYPES_H */
