# Building and Installation

Currently, this library utilizes some features that many compilers support such as gcc, clang, and AppleClang, but support is not ready for Windows.

## Manual Install Quick Start

1. Use the provided defaults
2. Build the library
3. Install the library
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

If you want to simply write the following command in your `CMakeLists.txt`,

```cmake
find_package(ccc)
```

specify that this library shall be installed to a location CMake recognizes by default. For example, my preferred location is as follows:

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

## Include the Library

Once CMake can find the package, link against it and include the container header.

The `CMakeLists.txt` file.

```cmake
add_executable(my_exe my_exe.c)
target_link_libraries(my_exe ccc::ccc)
```

The C code.

```.c
#include "ccc/flat_hash_map.h"
```

## Alternative Builds

You may wish to use a different compiler and tool chain than what your system default specifies. Review the `CMakePrests.json` file for different compilers.

```zsh
make gcc-rel [OPTIONAL/INSTALL/PATH]
make install
```

Use Clang to compile the library.

```zsh
make clang-rel [OPTIONAL/INSTALL/PATH]
make install
```

## Without Make

If your system does not support Makefiles or the `make` command here are the CMake commands one can run that will do the same.

```zsh
# Configure the project cmake files.
# Replace this preset with your own if you'd like.
cmake --preset=clang-rel -DCMAKE_INSTALL_PREFIX=[DESIRED/INSTALL/LOCATION]
cmake --build build
cmake --build build --target install
```

## User Presets

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
cmake --build build
cmake --build build --target install
```
