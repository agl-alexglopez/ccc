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

/* Make more modern nullptr_t and nullptr available on mac available  */
#if !defined(__STDC__) || !defined(__STDC_VERSION__)                           \
    || (__STDC_VERSION__ < 202000L)                                            \
    || (!defined(__has_builtin) && !__has_builtin(__is_identifier)             \
        && !__is_identifier(nullptr))
#    define nullptr (void *)0
typedef void *nullptr_t;
#endif
/** @endcond */

/** @private */
enum ccc_entry_status : uint8_t
{
    CCC_ENTRY_VACANT = 0,
    CCC_ENTRY_OCCUPIED = 0x1,
    CCC_ENTRY_INSERT_ERROR = 0x2,
    CCC_ENTRY_ARG_ERROR = 0x4,
    CCC_ENTRY_NO_UNWRAP = 0x8,
};

/** @private */
struct ccc_ent
{
    void *e;
    enum ccc_entry_status stats;
};

/** @private */
union ccc_entry
{
    struct ccc_ent impl;
};

/** @private */
struct ccc_handl
{
    size_t i;
    enum ccc_entry_status stats;
};

/** @private */
union ccc_handle
{
    struct ccc_handl impl;
};

/** @private */
struct ccc_range_u
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
union ccc_range
{
    struct ccc_range_u impl;
};

/** @private */
union ccc_rrange
{
    struct ccc_range_u impl;
};

#endif /* CCC_IMPL_TYPES_H */
