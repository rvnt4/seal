#pragma once

/*
    break into the debugger when available or terminates through seal::fatalExit().
*/
#if defined(_MSC_VER)
    #define SEAL_DEBUGBREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
    #define SEAL_DEBUGBREAK() __builtin_trap()
#else
    #define SEAL_DEBUGBREAK() ((void)0)
#endif

/*
    tell the optimizer a condition is always true.
*/
#if defined(_MSC_VER)
    #define SEAL_ASSUME(cond) __assume(cond)
#elif defined(__GNUC__) || defined(__clang__)
    #define SEAL_ASSUME(cond)                                                                                          \
        do                                                                                                             \
        {                                                                                                              \
            if (!(cond)) __builtin_unreachable();                                                                      \
        } while (0)
#else
    #define SEAL_ASSUME(cond) ((void)0)
#endif

namespace seal
{
    void displayError(const char* msg, const char* file, int line);
    [[noreturn]] void fatalExit();
} // namespace seal

/*
    Shared fatal path for ASSERT and PANIC.
*/
#define SEAL_FATAL(msg)                                                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        seal::displayError((msg), __FILE__, __LINE__);                                                                 \
        SEAL_DEBUGBREAK();                                                                                             \
        seal::fatalExit();                                                                                             \
    } while (0)

#if !defined(NDEBUG)
    #define ASSERT(cond, msg)                                                                                          \
        do                                                                                                             \
        {                                                                                                              \
            if (!(cond))                                                                                               \
            {                                                                                                          \
                SEAL_FATAL(msg);                                                                                       \
            }                                                                                                          \
            SEAL_ASSUME(cond);                                                                                         \
        } while (0)
#else
    #define ASSERT(cond, msg) SEAL_ASSUME(cond)
#endif

/*
    PANIC always halts execution, in debug and release builds alike.
*/
#define PANIC(msg) SEAL_FATAL(msg)
