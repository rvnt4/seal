#pragma once

#if defined(_MSC_VER)
    #define SEALLIB_DEBUGBREAK() __debugbreak()
    #define SEALLIB_ASSUME(cond) __assume(cond)
#elif defined(__GNUC__) || defined(__clang__)
    #define SEALLIB_DEBUGBREAK() __builtin_trap()
    #define SEALLIB_ASSUME(cond)                                                                                       \
        do                                                                                                             \
        {                                                                                                              \
            if (!(cond)) __builtin_unreachable();                                                                      \
        } while (0)
#else
    #define SEALLIB_DEBUGBREAK()                                                                                       \
        do                                                                                                             \
        {                                                                                                              \
        } while (0)
    #define SEALLIB_ASSUME(cond)                                                                                       \
        do                                                                                                             \
        {                                                                                                              \
        } while (0)
#endif

#ifdef _DEBUG
    #define ASSERT(cond, msg)                                                                                          \
        do                                                                                                             \
        {                                                                                                              \
            if (!(cond))                                                                                               \
            {                                                                                                          \
                seallib::displayError(msg, __FILE__, __LINE__);                                                        \
                SEALLIB_DEBUGBREAK();                                                                                  \
                for (;;)                                                                                               \
                {                                                                                                      \
                } /* infinite loop to prevent execution continuation */                                                \
            }                                                                                                          \
            SEALLIB_ASSUME(cond);                                                                                      \
        } while (0)
#else
    #define ASSERT(cond, msg) SEALLIB_ASSUME(cond)
#endif

#define PANIC(msg)                                                                                                     \
    do                                                                                                                 \
    {                                                                                                                  \
        seallib::displayError(msg, __FILE__, __LINE__);                                                                \
        SEALLIB_DEBUGBREAK();                                                                                          \
        for (;;)                                                                                                       \
        {                                                                                                              \
        } /* infinite loop to prevent execution continuation */                                                        \
    } while (0)

namespace seallib
{
    void displayError(const char* msg, const char* file, int line);
}
