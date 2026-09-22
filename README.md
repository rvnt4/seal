<h1 align="center">
    <b>🦭seal🦭</b>
</h1>

<p align="center">
    A <b>modular and tiny C++20</b> powered library collection of random stuff I've needed for other projects.
</p>

## Table of Contents
* [Installation & Integration](#installation--integration)

## Installation & Integration

### 1. Requirements
* **Compiler**: A C++20 compliant compiler.
* **Build System**: CMake 4.1.0 or higher.

### 2. Integration via CMake
#### Option A: Using FetchContent (Recommended)
You can pull the library directly into your project from the git repository. Add this to your CMakeLists.txt:

```cmake
include(FetchContent)

FetchContent_Declare(
    seal
    GIT_REPOSITORY https://github.com/rvnt4/seal
    GIT_TAG main
)

# Enable the modules you want before making it available
set(SEAL_LOG ON)
set(SEAL_ASSERT OFF)

FetchContent_MakeAvailable(seal)

target_link_libraries(your_project PRIVATE seal)
```

#### Option B: Local Path
If you have the library downloaded locally:
```cmake
# Enable the modules you want before adding it
set(SEAL_LOG ON)
set(SEAL_ASSERT OFF)

add_subdirectory(path/to/seal)

target_link_libraries(your_project PRIVATE seal)
```

## License

[MIT](https://choosealicense.com/licenses/mit/)