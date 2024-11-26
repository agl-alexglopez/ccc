# Building and Installation

Currently, this library utilizes some features that many compilers support such as gcc, clang, and AppleClang, but support is not ready for Windows.

## Fetch Content Install

This approach will allow CMake to build the collection from source as part of your project. The collection does not have external dependencies, besides the standard library, so this may be viable for you. This is helpful if you want the ability to build the library in release or debug mode along with your project and possibly step through it with a debugger during a debug build. If you would rather link to the release build library file see the next section for the manual install.

To avoid including tests, samples, and other extraneous files when fetching content download a release.

```cmake
include(FetchContent)
FetchContent_Declare(
  ccc
  URL https://github.com/agl-alexglopez/ccc/releases/download/v[MAJOR.MINOR.PATCH]/ccc-v[MAJOR.MINOR.PATCH].zip
  #DOWNLOAD_EXTRACT_TIMESTAMP FALSE # CMake may raise a warning to set this. If so, uncomment and set.
)
FetchContent_MakeAvailable(ccc)
```

To link against the library in the project use the `ccc` namespace.

```cmake
add_executable(main main.c)
target_link_libraries(main ccc::ccc)
```

Here is a concrete example with an arbitrary release that is likely out of date. Replace this version with the newest version on the releases page.

```cmake
include(FetchContent)
FetchContent_Declare(
  ccc
  URL https://github.com/agl-alexglopez/ccc/releases/download/v0.7.0/ccc-v0.7.0.zip
  #DOWNLOAD_EXTRACT_TIMESTAMP FALSE # CMake may raise a warning to set this. If so, uncomment and set.
)
FetchContent_MakeAvailable(ccc)
add_executable(main main.c)
target_link_libraries(main ccc::ccc)
```

Now, the C Container Collection is part of your project build, allowing you to configure as you see fit. For a more traditional approach read the manual install section below.

## Manual Install Quick Start

1. Use the provided defaults.
2. Build the library.
3. Install the library.
4. Include the library.

To complete steps 1-3 with one command try the following if your system supports `make`.

```zsh
make clang-ccc [OPTIONAL/INSTALL/PATH]
```

Or build and install with gcc. However, a newer gcc than the default on many systems may be required. For example, I compile this library with gcc-14.2 as of the time of writing. To set up a `CMakeUserPresets.json` file to use a newer gcc, see [User Presets](#user-presets).

```zsh
make gcc-ccc [OPTIONAL/INSTALL/PATH]
```

This will use CMake and your default compiler to build and install the library in release mode. By default, this library does not touch your system paths and it is installed in the `install/` directory of this folder. This is best for testing the library out while pointing `cmake` to the install location. Then, deleting the `install/` folder deletes any trace of this library from your system.

Then, in your `CMakeLists.txt`:

```cmake
find_package(ccc HINTS "~/path/to/ccc-v[VERSION]/install")
```

You can simply write the following command in your `CMakeLists.txt`.

```cmake
find_package(ccc)
```

To do so, specify that this library shall be installed to a location CMake recognizes by default. For example, my preferred location is as follows:

```zsh
make clang-ccc ~/.local
```

Then the installation looks like this.

```txt
install
├── include
│   └── ccc
│       ├── buffer.h
│       ├── double_ended_priority_queue.h
│       ├── doubly_linked_list.h
│       ├── flat_hash_map.h
│       └── ...continues
└── lib
    ├── cmake
    │   └── ccc
    │       ├── cccConfig.cmake
    │       ├── cccConfigVersion.cmake
    │       ├── cccTargets.cmake
    │       └── cccTargets-release.cmake
    └── libccc_release.a
```

Now to delete the library if needed, simply find all folders and files with the `*ccc*` string somewhere within them and delete. You can also check the `build/install_manifest.txt` to confirm the locations of any files installed with this library.

### Include the Library

Once CMake can find the package, link against it and include the container header.

The `CMakeLists.txt` file.

```cmake
add_executable(main main.c)
target_link_libraries(main ccc::ccc)
```

The C code.

```.c
#include "ccc/flat_hash_map.h"
```

### Without Make

If your system does not support Makefiles or the `make` command here are the CMake commands one can run that will do the same.

```zsh
# Configure the project cmake files.
# Replace this preset with your own if you'd like.
cmake --preset=clang-rel -DCMAKE_INSTALL_PREFIX=[DESIRED/INSTALL/LOCATION]
cmake --build build --target install
```

### User Presets

If you do not like the default presets, create a `CMakeUserPresets.json` in this folder and place your preferred configuration in that file. Here is my preferred configuration to get you started. I like to use the newest version of gcc that I have installed.

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
            "inherits": "default-deb",
            "cacheVariables": {
                "CMAKE_C_COMPILER": "gcc-14.2"
            }
        },
        {
            "name": "rel",
            "inherits": "default-rel",
            "cacheVariables": {
                "CMAKE_C_COMPILER": "gcc-14.2"
            }
        },
        {
            "name": "cdeb",
            "inherits": "default-deb",
            "generator": "Ninja",
            "cacheVariables": {
                "CMAKE_C_COMPILER": "clang"
            }
        },
        {
            "name": "crel",
            "inherits": "default-rel",
            "generator": "Ninja",
            "cacheVariables": {
                "CMAKE_C_COMPILER": "clang"
            }
        }
    ]
}
```

Then your preset can be invoked as follows:

```zsh
# Your preferred preset with the same other steps as before.
cmake --preset=rel -DCMAKE_INSTALL_PREFIX=[DESIRED/INSTALL/LOCATION]
cmake --build build --target install
```

## Generate Documentation

Documentation is available [HERE](https://agl-alexglopez.github.io/ccc/). However, if you want to build the documentation locally you will need `doxygen` and `graphviz` installed. Then run:

```zsh
doxygen Doxyfile
```

This will generate documentation in the `docs` folder. To view the docs in your local browser, double click the `docs/html/index.html` file.
