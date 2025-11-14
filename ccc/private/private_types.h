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

/** @internal The basic statuses possible when interacting with entries and
handles. Handles are just an index based version of entries. Not only can
we clearly understand the enum itself in a debugger, but we may provide more
detailed strings to the user by taking these values as input to helper
functions. */
enum CCC_Entry_status : uint8_t
{
    /** @internal The entry has no value and is ready for new insert. */
    CCC_ENTRY_VACANT = 0,
    /** @internal The entry has a value. */
    CCC_ENTRY_OCCUPIED = 0x1,
    /** @internal Insert errors only occur for vacant entries. This means we
        cannot insert a new value. No slot is available. */
    CCC_ENTRY_INSERT_ERROR = 0x2,
    /** @internal Some input to the function returning an entry is bad. */
    CCC_ENTRY_ARGUMENT_ERROR = 0x4,
    /** @internal Lesser used annotation on a vacant entry. Important not to
        look at the vacant entry for some associative containers to preserve
        data structure invariants. */
    CCC_ENTRY_NO_UNWRAP = 0x8,
};

/** @internal An entry is inspired by Rust's Entry API. Entries provide elegant
and efficient ways to interact with associative containers. Instead of
searching for elements and then inserting or removing based on the result of a
search, we can obtain either the present element or a vacant slot where the
element belongs. This allows us to act how we wish with this reference and no
second insert, search, or remove operation is needed. */
struct CCC_Entry
{
    /** @internal The user type that belongs at this container location. */
    void *type;
    /** @internal A status to help us decide how to act with the entry. */
    enum CCC_Entry_status status;
};

/** @internal A helper type so that we may return entries by compound literal
reference to other functions for API consistency. The user interacts with this
type in the API as follows but we typedef it so they don't care.

```
CCC_Entry *entry = &(CCC_Entry){function_returns_wrapper(arguments).private};
```

We can then wrap that returned compound literal reference in a macro. Union is
used as the wrapping type as a reminder that this type should serve no other
purpose and add no additional fields. */
union CCC_Entry_wrap
{
    /** @internal Helper to return the compound literal reference. */
    struct CCC_Entry private;
};

/** @internal A handle is the same as an entry but it uses an index into a
contiguous region of storage rather than a pointer. */
struct CCC_Handle
{
    /** @internal The index into the contiguous region of memory. */
    size_t index;
    /** @internal A status to help us decide how to act with the entry. */
    enum CCC_Entry_status status;
};

/** @internal A helper type so that we may return handles by compound literal
reference to other functions for API consistency. The user interacts with this
type in the API as follows but we typedef it so they don't care.

```
CCC_Handle *handle = &(CCC_Handle){function_returns_wrapper(arguments).private};
```

We can then wrap that returned compound literal reference in a macro. Union is
used as the wrapping type as a reminder that this type should serve no other
purpose and add no additional fields. */
union CCC_Handle_wrap
{
    /** @internal Helper to return the compound literal reference. */
    struct CCC_Handle private;
};

/** @internal The single type used to handle both forward and reverse iteration
through containers. The concept is the exact same due to how we always return
the full user type during iteration, regardless of the container being intrusive
or not. So we use a union for some clarity for the implementer.

We will also wrap this in two different types so that the user has some help
ensuring they pass the correct direction range to the correct function. */
struct CCC_Range
{
    union
    {
        /** @internal Start of forward iteration for a container. */
        void *begin;
        /** @internal Start of reverse iteration for a container. */
        void *reverse_begin;
    };
    union
    {
        /** @internal End of forward iteration for a container. */
        void *end;
        /** @internal End of reverse iteration for a container. */
        void *reverse_end;
    };
};

/** @internal A helper type so that we may return ranges by compound literal
reference to other functions for API consistency. The user interacts with this
type in the API as follows but we typedef it so they don't care.

```
CCC_Range *range
    = &(CCC_Range){function_returns_wrapper(arguments).private};
```

We can then wrap that returned compound literal reference in a macro. Union is
used as the wrapping type as a reminder that this type should serve no other
purpose and add no additional fields. */
union CCC_Range_wrap
{
    /** @internal Helper to return the compound literal reference. */
    struct CCC_Range private;
};

/** @internal A helper type so that we may return ranges by compound literal
reference to other functions for API consistency. The user interacts with this
type in the API as follows but we typedef it so they don't care.

```
CCC_Range_reverse *range
    = &(CCC_Range){function_returns_wrapper(arguments).private};
```

We can then wrap that returned compound literal reference in a macro. Union is
used as the wrapping type as a reminder that this type should serve no other
purpose and add no additional fields. */
union CCC_Range_reverse_wrap
{
    /** @internal Helper to return the compound literal reference. */
    struct CCC_Range private;
};

#endif /* CCC_PRIVATE_TYPES_H */
