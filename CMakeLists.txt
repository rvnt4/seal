# ------------------------------
# cmake project
# ------------------------------
cmake_minimum_required(VERSION 4.1.0)
project(seal LANGUAGES CXX)

# ------------------------------
# options
# ------------------------------
option(SEAL_MEM "Use memory library" OFF)
option(SEAL_STRING "Use string library" OFF)
option(SEAL_VECTOR "Use vector library" OFF)
option(SEAL_FMT "Use format library" OFF)
option(SEAL_LOG "Use log library" OFF)
option(SEAL_VFS "Use virtual file system" OFF)
option(SEAL_EVENTS "Use events library" OFF)
option(SEAL_ASSERT "Use assertion utility" OFF)

option(SEAL_TEST "Build test project" OFF)

# log depends on fmt and vec
if (SEAL_LOG)
    set(SEAL_FMT ON CACHE BOOL "" FORCE)
    set(SEAL_VECTOR ON CACHE BOOL "" FORCE)
endif()

# fmt depends on str
if (SEAL_FMT)
    set(SEAL_STRING ON CACHE BOOL "" FORCE)
endif()

# str depends on mem
if (SEAL_STRING)
    set(SEAL_MEM ON CACHE BOOL "" FORCE)
endif() 

# vec depends on mem
if (SEAL_VECTOR)
    set(SEAL_MEM ON CACHE BOOL "" FORCE)
endif() 

# vfs depends on str, vec, mem
if (SEAL_VFS)
    set(SEAL_STRING ON CACHE BOOL "" FORCE)
    set(SEAL_VECTOR ON CACHE BOOL "" FORCE)
    set(SEAL_MEM ON CACHE BOOL "" FORCE)
endif()

# events depends on vec, mem
if (SEAL_EVENTS)
    set(SEAL_VECTOR ON CACHE BOOL "" FORCE)
    set(SEAL_MEM ON CACHE BOOL "" FORCE)
endif()

# all libs for test
if (SEAL_TEST)
    set(ALL_LIBS SEAL_MEM SEAL_STRING SEAL_VECTOR SEAL_FMT SEAL_LOG SEAL_VFS SEAL_EVENTS SEAL_ASSERT)
    foreach(lib ${ALL_LIBS})
        set(${lib} ON CACHE BOOL "" FORCE)
    endforeach()
endif()

# ------------------------------
# sources
# ------------------------------
set(SEAL_SOURCES "src/lib/seal/types.h")
set(SEAL_TEST_SOURCES "src/test/main.cpp" "src/test/tests.h")

if (SEAL_MEM)
    list(APPEND SEAL_SOURCES "src/lib/seal/memory.cpp" "src/lib/seal/memory.h")
    list(APPEND SEAL_TEST_SOURCES "src/test/tests/memory.cpp")
endif()

if (SEAL_STRING)
    list(APPEND SEAL_SOURCES "src/lib/seal/string.cpp" "src/lib/seal/string.h")
    list(APPEND SEAL_TEST_SOURCES "src/test/tests/string.cpp")
endif()

if (SEAL_VECTOR)
    list(APPEND SEAL_SOURCES "src/lib/seal/vector.h")
    list(APPEND SEAL_TEST_SOURCES "src/test/tests/vector.cpp")
endif()

if (SEAL_FMT)
    list(APPEND SEAL_SOURCES "src/lib/seal/fmt.h")
    list(APPEND SEAL_TEST_SOURCES "src/test/tests/fmt.cpp")
endif()

if (SEAL_LOG)
    list(APPEND SEAL_SOURCES "src/lib/seal/log.h")
    list(APPEND SEAL_TEST_SOURCES "src/test/tests/log.cpp")
endif()

if (SEAL_VFS)
    list(APPEND SEAL_SOURCES "src/lib/seal/vfs.cpp" "src/lib/seal/vfs.h")
    list(APPEND SEAL_TEST_SOURCES "src/test/tests/vfs.cpp")
endif()

if (SEAL_EVENTS)
    list(APPEND SEAL_SOURCES "src/lib/seal/events.h")
    list(APPEND SEAL_TEST_SOURCES "src/test/tests/events.cpp")
endif()

if (SEAL_ASSERT)
    list(APPEND SEAL_SOURCES "src/lib/seal/assert.cpp" "src/lib/seal/assert.h")
    list(APPEND SEAL_TEST_SOURCES "src/test/tests/assert.cpp")
endif()

# ------------------------------
# lib
# ------------------------------
# is header only detection
set(SEAL_HAS_SOURCES FALSE)
foreach(src ${SEAL_SOURCES})
    if(src MATCHES "\\.cpp$")
        set(SEAL_HAS_SOURCES TRUE)
        break()
    endif()
endforeach()

if (SEAL_HAS_SOURCES)
    add_library(seal STATIC ${SEAL_SOURCES})
else()
    add_library(seal INTERFACE)
    target_sources(seal INTERFACE ${SEAL_SOURCES})
endif()

add_library(seal::seal ALIAS seal)

if(SEAL_HAS_SOURCES)
    set(SEAL_SCOPE PUBLIC)
else()
    set(SEAL_SCOPE INTERFACE)
endif()

# config
set_target_properties(seal PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(seal ${SEAL_SCOPE} src/lib)
target_compile_features(seal ${SEAL_SCOPE} cxx_std_20)
set_target_properties(seal PROPERTIES CXX_EXTENSIONS OFF)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/lib" FILES ${SEAL_SOURCES})

# ------------------------------
# test
# ------------------------------
if (SEAL_TEST)
    add_executable(seal-test ${SEAL_TEST_SOURCES})
    target_include_directories(seal-test PRIVATE "src/test")
    target_link_libraries(seal-test PRIVATE seal)
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/test" FILES ${SEAL_TEST_SOURCES})
endif()