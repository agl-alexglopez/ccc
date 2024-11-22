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
- `util` - General helpers for the `tests` and `samples`. A `str_view` helper library is here to make `argc` and `argv` handling easier in the test runner and samples. Utilities for containers should not go here. If containers need to share utilities, they should find the header in `src`. The `util` folder is removed from releases.

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

Now, I am able to run the `dsan` and `rsan` sanitizer builds. Those configurations are hidden by default because `-fanalyzer` and sanitizer flags can be highly dependent on compilers, so it is best to get them set up based on one's own environment.

## Workflow

Now that tooling is set up, the workflow is roughly as follows.

- Checkout a branch and start working on changes.
- When ready or almost ready, open a draft pr so CI can start running checks.
- Before completion run the following tools (I am working on getting more of this into CI so it does not always have to happen locally).
    - Run `make tidy` and fix any issues from `clang-tidy`.
    - Run `make clean && cmake --preset=dsan && cmake --build build -j8 --target ccc tests samples`. Replace the `-j8` flag with the number of cores on your system. This runs GCC's `-fanalyzer` and supplementary sanitizer flags. GCC's `-fanalyzer` will flag issues at compile time. Sanitizers requires you run actual programs so it can observe undefined behavior, buffer overflow, out of bounds memory access, etc. Run the tests `make dtest` and any samples your changes affect.
    - Run `make clean && cmake --preset=rsan && cmake --build build -j8 --target ccc tests samples`. Replace the `-j8` flag with the number of cores on your system. This is the same as the previous step just in release mode. Sometimes the compiler can optimize in such a way to create different issues the sanitizer can catch. Run `make rtest`.
- Mark the pr as ready for review when all CI checks pass and tools show no errors locally.

## Targets

- `ccc` - The core C Container Collection library.
    - `cmake --preset=[PRESET HERE] && cmake --build build -j[NUM THREADS] --target ccc`
- `tests` - The tests.
    - `cmake --preset=[PRESET HERE] && cmake --build build -j[NUM THREADS] --target tests`
- `samples` - The samples.
    - `cmake --preset=[PRESET HERE] && cmake --build build -j[NUM THREADS] --target samples`

## Style

Formatting should be taken care of by the tools. Clang tidy will settle some small style matters. Defer to clang tidy suggestions when it makes them. Here are more general guidelines.

### Validate Parameters

Because this is a 3rd party library, we are responsible for avoiding undefined behavior and mysterious crashes from the user's perspective. This means checking any inputs to the user facing interface functions as non-NULL and valid for the operations we must complete.

Every function has some way to return early. In the worst case we must return `NULL` early. In functions that let us return a result or status, we can return the type of error we encountered to the user and there are mechanisms in `types.h` for viewing or logging more detailed error messages. This library does not use `errno` style reporting and it does not crash the user program if compiled in release mode with any assert style mechanism. Using `assert` is only acceptable in this library code where internal invariants of the data structure are being checked and documentation for all functions must be written as if those asserts do not exist (they do not exist in releases). Asserts help in the development and testing stage rather than the user-facing releases.

If an error can be checked and reported to the user in a meaningful way, there should be a direct if branch and return in the code rather than an assert. See the Entry Interface implementations in any of the associative containers for examples of this reporting style. There can still be improvements made to the current code in this regard.

### Prefer Early Returns and Reduce Nesting

Here is an examples from `buffer.h` (may not be in sync with current code).

```c
ccc_result
ccc_buf_alloc(ccc_buffer *const buf, size_t const capacity,
              ccc_alloc_fn *const fn)
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

For example, a flat priority queue is a binary heap that operates by swapping elements. To swap elements we need a temporary space the size of one of the elements in the heap. To avoid dynamically allocating such a space--we might not have permission to do so--the heap simply saves the last slot in the array for swapping.

Other containers must find other solutions. The ordered map for example is a node based container that offers the `insert` function.

```c
[[nodiscard]] ccc_entry ccc_om_insert(ccc_ordered_map *om,
                                      ccc_omap_elem *key_val_handle,
                                      ccc_omap_elem *tmp);
```

The insert function promises to return the old entry in the map if one existed and is now being replaced by the new key value. If the map has no allocation permission it cannot create space for this swap to occur. Therefore, it asks the user to decide where this space should come from by providing space for one more of their type. However, containers try to avoid passing these space requirements on to the user when possible.

Variable length arrays are prohibited because they could cause hard to find bugs if the array caused a stack overflow in our library code for the user.

## To Do

At least the following would need to happen before `v1.0`.

- A better hash map implementation. I think Abseil's hash table from Google is a good baseline. Many more interesting developments have occurred recently as well. The hash table would need to be able to accommodate the non-allocating principles mentioned previously and it would be a bonus if it was pointer stable under non-resizing conditions. Currently the hash table is a Robin Hood hash table that uses linear probing and swaps elements in the table on insert and erase. It caches hash values for faster distance calculations, resizing, and callback function avoidance. However, we can do much better.
- More tests. I added a decent suite of tests to each container with most of the focus on the associative containers, but more thorough testing should be added throughout.
