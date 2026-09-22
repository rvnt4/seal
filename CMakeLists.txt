# ------------------------------
# cmake project
# ------------------------------
cmake_minimum_required(VERSION 4.1.0)
project(seal LANGUAGES CXX)

# ------------------------------
# options
# ------------------------------
option(SEAL_TEST "Build test project" OFF)
# ------------------------------
# sources
# ------------------------------
set(SEAL_SOURCES "")
set(SEAL_TEST_SOURCES "src/test/main.cpp" "src/test/tests.h")

if (SEAL_MEM)
    list(APPEND SEAL_SOURCES "src/lib/seal/memory.cpp" "src/lib/seal/memory.h")
    list(APPEND SEAL_TEST_SOURCES "src/test/tests/memory.cpp")
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