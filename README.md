<h1 align="center">
    <b>🦭seal🦭</b>
</h1>

<p align="center">
    A <b>modular and tiny CRT / std free C++20</b> powered library collection of random stuff I've needed for other projects.
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

---

## Build Options
Toggle features by setting these variables to ``ON`` or ``OFF`` in your CMake configuration.
| Option | Description | Default |
| :--- | :--- | :--- |
| SEAL_ASSERT | Enable Assertion utility | OFF |
| SEAL_EVENTS | Enable Event Dispatcher | OFF |
| SEAL_LOG | Enable Logging Framework | OFF |
| SEAL_VFS | Enable Virtual File System | OFF |
| SEAL_TEST | Build the test project | OFF |

---

## Modules

### 1. Logging
The Log module uses a sink architecture. You create a Logger, attach an ILogSink, and log messages using std::format syntax.

#### Example
```C++
#include <seal/log.h>
#include <seal/memory.h>
#include <seal/string.h>
#include <iostream>
#include <string_view>

using namespace seal;

class ConsoleSink : public ILogSink {
public:
    void receiveLog(LogType type, StringView loggerName, StringView message) override {
        std::cout << getLogTypeColor(type) << "[" << getLogTypeName(type) << "] "
                  << "[" << std::string_view(loggerName.data(), loggerName.size()) << "] " 
                  << std::string_view(message.data(), message.size()) << "\x1b[0m" << std::endl;
    }
};

int main() {
    DynamicHeapAllocator allocator;
    setStringAllocator(&allocator);

    Logger logger("MainApp", &allocator);

    void* mem = allocator.allocate(sizeof(ConsoleSink), alignof(ConsoleSink));
    ConsoleSink* sink = new (mem, placement_t{}) ConsoleSink();
    SharedPtr<ILogSink> sharedSink(sink, &allocator);

    logger.addSink(sharedSink);

    logger.info("Application started with version {}", 1.0);
    logger.error("Failed to load config: {}", "file_not_found.json");

    return 0;
}
```

### 2. Assert
The assert module provides diagnostic macros made to trap critical logic failures.

#### Macros and their behavior
| Macro | Debug Mode (`_DEBUG`) | Release Mode |
| :--- | :--- | :--- |
| `ASSERT(cond, msg)` | Evaluates `cond`. If `false`, logs the error location, pops up an error box, drops a debugger breakpoint trap, and calls `std::exit`. | Evaluates to a compiler optimization hint informing the optimizer that `cond` is always `true`. |
| `PANIC(msg)` | Logs the error, displays an error box, breaks execution, and terminates. | Logs the error, displays an error box, breaks execution, and terminates. **Always halts execution.** |
#### Example
```c++
#include <seal/assert.h>

void processSystemData(void* dataPtr, int size) {
    // 1. Defensively check states that should literally be impossible in a valid run
    ASSERT(dataPtr != nullptr, "Managed data pointer cannot be null during processing pipe");
    
    // 2. Asserts behave as optimizer hints in Release mode. 
    // The compiler now optimizes this function knowing 'size' can never be less than zero.
    ASSERT(size >= 0, "Buffer stream size constraint violation");

    if (size == 0) {
        return; // Valid empty state
    }

    // Processing logic...
}

void processNetworkPacket(int packetId) {
    switch (packetId) {
        case 1: /* Handle read */ break;
        case 2: /* Handle write */ break;
        default:
            // 3. Use PANIC for unrecoverable structural states that must crash 
            // the software immediately in both Debug and Release builds.
            PANIC("Critical Error: Received completely unhandled or corrupted packet opcode");
            break;
    }
}

int main() {
    // Fails in debug: displays an error window detailing the file and line number
    processSystemData(nullptr, -1); 

    return 0;
}
```

### 3. Events
The Events module provides a thread and type safe signal/slot mechanism. It allows you to define custom events with any number of parameters and subscribe to them using delegates.
#### Example
```cpp
#include <seal/events.h>
#include <seal/memory.h>
#include <seal/string.h>
#include <iostream>
#include <string_view>

using namespace seal;

void onLogin(void*, int id, String name) {
    std::cout << "User " << std::string_view(name.data(), name.size()) << " (ID: " << id << ") logged in!" << std::endl;
}

int main() {
    DynamicHeapAllocator allocator;
    setStringAllocator(&allocator);

    // Define an event that takes an int and a string
    Event<int, String> OnUserLogin(&allocator);

    // 1. Subscribe to the event via a free function Delegate
    Delegate<int, String> loginDelegate(nullptr, onLogin);
    auto connection = OnUserLogin.addListener(loginDelegate);

    // 2. Trigger the event
    OnUserLogin.run(42, String("SomeUser"));

    // 3. Remove a listener when no longer needed
    OnUserLogin.removeListener(connection);

    return 0;
}
```

### 4. VFS
The VFS (Virtual File System) module provides a unified interface for file operations by mapping virtual paths to user defined providers. 

#### Implementing a file provider
To create a new storage backend (e.g., a zip loader, encrypted volume, etc), you must inherit from `IFileProvider`.

#### Example provider implementation
```cpp
#include <seal/vfs.h>
#include <seal/memory.h>
#include <seal/string.h>

using namespace seal;

class MyCustomProvider : public IFileProvider {
public:
    // checks if file exists
    bool exists(StringView path) const override {
        /*
            implementation logic here
        */
        return true; 
    }

    // loads data from provider to outBuffer
    VFSResult readFile(StringView path, FileBuffer& outBuffer) const override {
        /* 
           1. locate the file in your system.
           2. if not found, return VFSResult::FileNotFound.
           3. outBuffer = FileBuffer::fromString(StringView("file_content_here"), allocator);
        */
        return VFSResult::Success;
    }

    // write data from a buffer to your provider
    VFSResult writeFile(StringView path, const FileBuffer& buffer) override {
        /* 
           VFS ensures 'buffer' is valid (data != nullptr) before calling this.
           return VFSResult::AccessDenied if your provider is read-only or if that path shouldn't be written to
        */
        return VFSResult::Success;
    }

    // generic file management operations
    VFSResult deleteFile(StringView path) override { return VFSResult::Success; }
    VFSResult createDirectory(StringView path) override { return VFSResult::Success; }

    // list all files and subdirectories within a given path
    Vector<String> listDirectory(StringView path, IAllocator* alloc) const override {
        // note: directories should ideally end with a '/' for clarity
        Vector<String> files(alloc);
        files.push_back(String("file1"));
        files.push_back(String("file2"));
        return files;
    }

    // optional: return a name for debugging/logging
    StringView getProviderName() const override { return StringView("MyCustomProvider"); }
};
```

#### Example provider usage
```c++
#include <seal/vfs.h>
#include <seal/memory.h>
#include <seal/string.h>

using namespace seal;

int main() {
    DynamicHeapAllocator allocator;
    setStringAllocator(&allocator);

    VirtualFileSystem vfs(&allocator);

    /*
        mount a custom provider to the virtual "assets" folder
        higher priority (10 in this case) means this mount is checked before others
    */
    void* mem = allocator.allocate(sizeof(MyCustomProvider), alignof(MyCustomProvider));
    MyCustomProvider* provider = new (mem, placement_t{}) MyCustomProvider();
    SharedPtr<IFileProvider> p(provider, &allocator);

    vfs.mount(StringView("/assets"), p, 10);

    // reading a file (Paths are automatically normalized to remove '../' etc.)
    FileBuffer buffer;
    if (vfs.readFile(StringView("/assets/somefile.bin"), buffer) == VFSResult::Success) {
        String rawData = buffer.toString();
    }

    return 0;
}
```

---

## Running tests

### Windows (PowerShell + Visual Studio)
A script is provided to automate generation and open the solution in Visual Studio:

./create-test.ps1

### Cross-Platform (CLI)
To build and run manually from the terminal:

```bash
mkdir build && cd build
cmake .. -DSEAL_TEST=ON
cmake --build .
./seal-test
```

## License

[MIT](https://choosealicense.com/licenses/mit/)