# Contributing

Not sure if this library will be used but might as well add this just in case. People can also ask questions in issues or the Discussions page.

## Organization

Here is a description of the current repository organization.

- `ccc` - The user-facing headers for each container. Documentation generation comes from these headers.
    - `ccc/impl` - The type definitions for all containers. The macros used for initialization and more efficient insertion/construction can be found here. They are separated here to make the headers in `ccc` cleaner and more readable.
- `cmake` - Helper CMake files for installing the `ccc` library.
- `docs` - Folder for generating Doxygen Documentation locally and hosting online on the `gh-pages` branch.
- `etc` - The tools folder to make running `clang-format` and `clang-tidy` more convenient.
- `samples` - The sample programs used to demonstrate library capabilities.
- `src` - The implementations for the containers listed in the `ccc/` headers.
- `tests` - Testing code. The `run_tests.c` test runner. Also the `checkers.h` testing framework is well-documented for writing test cases.
    - `tests/[container]` - The folder for the container specific tests.
- `utility` - General helpers for the `tests` and `samples`. A `str_view` helper library is here to make `argc` and `argv` handling easier in the test runner and samples. Utilities for containers should not go here. If containers need to share utilities, they should find the header in `src`. The `utility` folder is removed from releases.

## Tooling

Required tools:

- [pre-commit](https://pre-commit.com/) - Run `pre-commit install` once the repo is forked/cloned. This will ensure pointless formatting changes don't enter the commit history.
- clangd - This is helpful for your editor if you have a `LSP` that can auto-format on save. This repo has both a `.clang-tidy` and `.clang-format` file that help `LSP`'s and other tools with warnings while you write.
- gcc-14+ - GCC 14 onward added excellent `-fanalyzer` capabilities. There are GCC presets in `CMakePresets.json` to run `-fanalyzer` and numerous sanitizers. These work best with newer GCC versions.
- .editorconfig - Settles cross platform issues like line endings and editor concerns like tabs vs spaces. Ensure your editor supports .editorconfig.
- .clang-format - This is needed less so now that `pre-commit` helps with formatting.
- .clang-tidy - Clang tidy should be run often on any changes to the code to catch obvious errors.

Add a `CMakeUserPresets.json` file so that you can run the sanitizer presets found in `CMakePresets.json`. Here is my setup as a sample.

```json
{
    "version": 3,
    "cmakeMinimumRequired": {
        "major": 3,
        "minor": 23,
        "patch": 0
    },
    "configurePresets": [
        {
            "name": "deb",
            "inherits": ["default-deb"],
            "cacheVariables": {
                "CMAKE_C_COMPILER": "gcc-14.2"
            }
        },
        {
            "name": "rel",
            "inherits": ["default-rel"],
            "cacheVariables": {
                "CMAKE_C_COMPILER": "gcc-14.2"
            }
        },
        {
            "name": "cdeb",
            "inherits": ["default-deb"],
            "generator": "Ninja",
            "cacheVariables": {
                "CMAKE_C_COMPILER": "clang"
            }
        },
        {
            "name": "crel",
            "inherits": ["default-rel"],
            "generator": "Ninja",
            "cacheVariables": {
                "CMAKE_C_COMPILER": "clang"
            }
        },
        {
            "name": "dsan",
            "inherits": ["gcc-dsan"],
            "cacheVariables": {
                "CMAKE_C_COMPILER": "gcc-14.2"
            }
        },
        {
            "name": "rsan",
            "inherits": ["gcc-rsan"],
            "cacheVariables": {
                "CMAKE_C_COMPILER": "gcc-14.2"
            }
        }
    ]
}
```

Now, I am able to run the `dsan` and `rsan` sanitizer builds. There is a default sanitizer preset that is provided but setting up a custom preset with the most recent GCC compiler, preferably 14+, provides the most robust `-fanalyzer` and sanitizer diagnostics. GCC has been improving these diagnostic tools rapidly since version 14.

## Workflow

Now that tooling is set up, the workflow is roughly as follows.

- Checkout a branch and start working on changes.
- When ready or almost ready, open a draft pr so CI can start running checks.
- Before completion run the following tools. Most run remotely on the PR as well but feedback is faster locally.
    - Run `make tidy` on debug and release builds and fix any issues from `clang-tidy`. This will also run as an action on any pull request in case you forget, but it only runs if it detects the most recent commit has changed a `.c` or `.h` file. It is also easier to quickly fix feedback locally.
    - Run `make clean && cmake --preset=dsan && cmake --build build -j8 --target ccc tests samples`. Replace the `-j8` flag with the number of cores on your system. This runs GCC's `-fanalyzer` and supplementary sanitizer flags. GCC's `-fanalyzer` will flag issues at compile time. Sanitizers requires you run actual programs so it can observe undefined behavior, buffer overflow, out of bounds memory access, etc. Run the tests `make dtest` and any samples your changes affect. Tests will run the PR remotely in case you forget, but feedback is faster locally.
    - Run `make clean && cmake --preset=rsan && cmake --build build -j8 --target ccc tests samples`. Replace the `-j8` flag with the number of cores on your system. This is the same as the previous step just in release mode. Sometimes the compiler can optimize in such a way to create different issues the sanitizer can catch. Run `make rtest`. Tests will run the PR remotely in case you forget, but feedback is faster locally.
- Mark the pr as ready for review when all CI checks pass and tools show no errors locally.

## Targets

- `ccc` - The core C Container Collection library.
    - `cmake --preset=[PRESET HERE] && cmake --build build -j[NUM THREADS] --target ccc`
- `tests` - The tests.
    - `cmake --preset=[PRESET HERE] && cmake --build build -j[NUM THREADS] --target tests`
    - `make tests` to build.
    - `make dtest` to run in debug build.
    - `make rtest` to run in release build.
    - `./build/debug/bin/tests/[TEST_FILE_NAME]` to run a specific container test in debug mode.
    - `./build/bin/tests/[TEST_FILE_NAME]` to run a specific container test in release mode.
- `samples` - The samples.
    - `cmake --preset=[PRESET HERE] && cmake --build build -j[NUM THREADS] --target samples`
    - `./build/debug/bin/[SAMPLE_NAME]` to run a sample in debug mode.
    - `./build/bin/[SAMPLE_NAME]` to run a sample in release mode.

## Style

Formatting should be taken care of by the tools. Clang tidy will settle some small style matters. Defer to clang tidy suggestions when it makes them. Here are more general guidelines.

### Naming

We present as clean an interface to the user as possible. Because we don't have C++ private fields, and because we cannot make our types opaque pointers, we must use other strategies to hide our implementation. All types must be complete. Complete means all fields of all types are visible to the user at compile time. This allows them to store our types on the stack or data segment which is a core goal of this library.

Given these constraints we use the following naming conventions.

- All types intentionally available to the user in a container `.h` file are `typedef`. For example, the flat hash map is `typedef struct ccc_fhmap ccc_flat_hash_map`. This is to discourage the user from thinking too hard about what the underlying types are because opaque pointers are not available.
- All types internal to the developers shall not use `typedef`. Internally, we want to know exactly what every type is no matter how long the name. Implementation helper functions that operate within the interface functions should not use the user facing `typedef`. Use the true underlying type name so the code carries as much information as possible while reading.
- To aid in the prior point, all functions internal to an implementation `src/*.c` file should be marked `static`. This will also help signal that you should be using the full type name in parameters and inside that function, not the user facing `typedef`.
- The implementation of macros is always completed in the `ccc/impl/` directory, and wrapper macros are provided in the `ccc/*.h` headers. The user should not be distracted with macro details.
- The `impl_` and `ccc_impl_` prefix is used in macros in the `ccc/impl/*.h` headers.

The exceptions to the user and developer naming split are as follows.

- When the internal implementation uses types from `types.h` that have been `typedef`.
    - For example, it improves code clarity to use the same `typedef` available to users for our function pointer types.
- When we `typedef` platform specific types such as integers. For example, in the bit set, we work with many different integer widths. The integers communicate nothing about their intent or purpose in the overall code so we `typedef` for clarity of variables and function arguments.

The reason for this naming split, and for the few exceptions to the rules, is to maintain the following principle.

> [!IMPORTANT]
> Code for the user simplifies and minimizes implementation details. Code for the developer specifies and maximizes implementation details.

### Validate Parameters

Because this is a 3rd party library, we are responsible for avoiding undefined behavior and mysterious crashes from the user's perspective. This means checking any inputs to the user facing interface functions as non-NULL and valid for the operations we must complete.

Every function has some way to return early. In the worst case we must return `NULL` early. In functions that let us return a result or status, we can return the type of error we encountered to the user and there are mechanisms in `types.h` for viewing or logging more detailed error messages. This library does not use `errno` style reporting and it does not crash the user program if compiled in release mode with any assert style mechanism. We also do not use an optional error reporting parameter to all functions. While this is a valid approach, it would detract from one of the design goals of this library: detailed initialization leads to cleaner container operations. Cluttering every container function call site with NULL parameters or allowing a container to provide a function that ignores errors is currently not a design priority.

Using `assert` is only acceptable in this library code where internal invariants of the data structure are being checked and documentation for all functions must be written as if those asserts do not exist (they do not exist in releases). Asserts help in the development and testing stage rather than the user-facing releases.

If an error can be checked and reported to the user in a meaningful way, there should be a direct if branch and return in the code rather than an assert. See the Entry Interface implementations in any of the associative containers for examples of this reporting style. There can still be improvements made to the current code in this regard.

### Prefer Early Returns and Reduce Nesting

Here is an examples from `buffer.h` (may not be in sync with current code).

```c
ccc_result
ccc_buf_alloc(ccc_buffer *const buf, size_t const capacity,
              ccc_any_alloc_fn *const fn)
{
    if (!buf)
    {
        return CCC_INPUT_ERR;
    }
    if (!fn)
    {
        return CCC_NO_ALLOC;
    }
    void *const new_mem = fn(buf->mem_, buf->elem_sz_ * capacity, buf->aux_);
    if (capacity && !new_mem)
    {
        return CCC_MEM_ERR;
    }
    buf->mem_ = new_mem;
    buf->capacity_ = capacity;
    return CCC_OK;
}
```

Early returns make it easier to find and reason about the happy path. The only exception is statement expressions in the `ccc/impl` folder headers. Early returns are not possible in statement expressions so the code structure becomes slightly more complex.

### Prohibited Dynamic Allocation

Variable length arrays are strictly prohibited. All containers must be designed to accommodate a non-allocating mode. This means the user can initialize the container to have no allocation permissions. However, for some containers this poses a challenge.

For example, a flat priority queue is a binary heap that operates by swapping elements. To swap elements we need a temporary space the size of one of the elements in the heap. We are not a header only template-style library so we must find space across an interface boundary. To ensure the user can store exactly as many elements as they wish in the flat array all signatures for functions that require swapping internally ask for a swap slot. For example here is the pop operation signature.

```c
ccc_result ccc_fpq_pop(ccc_flat_priority_queue *fpq, void *tmp);

```

Other containers may do the same or be able to avoid pushing this space requirement to the user. Here is the ordered map swap entry operation that requires a swap slot.

```c
[[nodiscard]] ccc_entry ccc_om_swap_entry(ccc_ordered_map *om,
                                          ccc_omap_elem *key_val_output,
                                          ccc_omap_elem *tmp);
```

For the equivalent handle version of this container the space requirement is handled internally.

```c
[[nodiscard]] ccc_handle ccc_hom_swap_handle(ccc_handle_ordered_map *hom,
                                             void *key_val_output);
```

The handle version of the container is required to preserve the 0th slot in the array as the nil node so it is able to swap when needed with this extra slot.

Variable length arrays are prohibited because they could cause hard to find bugs if the array caused a stack overflow in our library code for the user.

## To Do

At least the following would need to happen before `v1.0`.

- More tests. I added a decent suite of tests to each container with most of the focus on the associative containers, but more thorough testing should be added throughout.
    - A "bad user" test file should be added for every container in order to learn to what extent we can protect the user from their mistakes. If we cannot protect the user, the documentation must be obvious and clear regarding the critical error they can make.
- Now that a much more efficient hash table has been implemented, an adaptation of Rust's Hashbrown Hash Table, it is time to start narrowing down changes and lock in interfaces for v1.0. Suggestions are welcome for this phase of refactoring.
