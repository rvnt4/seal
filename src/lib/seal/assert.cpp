#include "assert.h"

#if defined(_WIN32)
    // from: WinUser.h / WinBase.h
    #ifndef MB_ICONHAND
        #define MB_ICONHAND 0x00000010L
        #define MB_ICONERROR MB_ICONHAND
        #define MB_SYSTEMMODAL 0x00001000L
        #define MB_OK 0x00000000L
    #endif

    #define STD_ERROR_HANDLE ((unsigned long)-12)

extern "C"
{
    __declspec(dllimport) void* __stdcall GetModuleHandleA(const char* lpModuleName);
    __declspec(dllimport) void* __stdcall LoadLibraryA(const char* lpLibFileName);

    typedef void*(__stdcall* SeallibFarProc)();
    __declspec(dllimport) SeallibFarProc __stdcall GetProcAddress(void* hModule, const char* lpProcName);

    __declspec(dllimport) void* __stdcall GetStdHandle(unsigned long nStdHandle);
    __declspec(dllimport) int __stdcall WriteFile(void* hFile, const void* lpBuffer,
                                                  unsigned long nNumberOfBytesToWrite,
                                                  unsigned long* lpNumberOfBytesWritten, void* lpOverlapped);
    __declspec(noreturn) void __stdcall ExitProcess(unsigned int uExitCode);
}
#elif defined(__unix__) || defined(__APPLE__) || defined(__linux__)
    #include <unistd.h>
#endif

namespace
{
    void appendString(char* buffer, unsigned int& offset, const char* str, unsigned int maxLen)
    {
        if (!str || maxLen == 0) return;
        while (*str && offset < maxLen - 1)
        {
            buffer[offset++] = *str++;
        }
        buffer[offset] = '\0';
    }

    void appendInt(char* buffer, unsigned int& offset, int value, unsigned int maxLen)
    {
        if (maxLen == 0) return;

        if (value == 0)
        {
            if (offset < maxLen - 1) buffer[offset++] = '0';
            buffer[offset] = '\0';
            return;
        }

        char temp[32];
        int tempIdx = 0;
        const bool isNeg = value < 0;
        unsigned int magnitude = isNeg ? 0u - static_cast<unsigned int>(value) : static_cast<unsigned int>(value);

        while (magnitude > 0 && tempIdx < 31)
        {
            temp[tempIdx++] = '0' + static_cast<char>(magnitude % 10);
            magnitude /= 10;
        }

        if (isNeg && tempIdx < 31) temp[tempIdx++] = '-';

        while (tempIdx > 0 && offset < maxLen - 1)
            buffer[offset++] = temp[--tempIdx];
        buffer[offset] = '\0';
    }

    void writeStderr(const char* text, unsigned int length)
    {
#if defined(_WIN32)
        void* hStdErr = GetStdHandle(STD_ERROR_HANDLE);
        if (hStdErr && hStdErr != (void*)-1)
        {
            unsigned long written = 0;
            WriteFile(hStdErr, text, length, &written, nullptr);
        }
#elif defined(__unix__) || defined(__APPLE__) || defined(__linux__)
        (void)write(2, text, length);
#else
        (void)text;
        (void)length;
#endif
    }
} // namespace

void seal::displayError(const char* msg, const char* file, int line)
{
    char fullError[2048];
    unsigned int offset = 0;

    appendString(fullError, offset, "[FATAL ERROR] ", 2048);
    appendString(fullError, offset, msg, 2048);
    appendString(fullError, offset, "\n\nFile: ", 2048);
    appendString(fullError, offset, file, 2048);
    appendString(fullError, offset, "\nLine: ", 2048);
    appendInt(fullError, offset, line, 2048);
    appendString(fullError, offset, "\n", 2048);

    writeStderr(fullError, offset);

#if defined(_WIN32)
    typedef int(__stdcall * fnMessageBoxA)(void*, const char*, const char*, unsigned int);
    void* hUser32 = GetModuleHandleA("user32.dll");
    if (!hUser32) hUser32 = LoadLibraryA("user32.dll");

    if (hUser32)
    {
        auto msgBoxA = reinterpret_cast<fnMessageBoxA>(GetProcAddress(hUser32, "MessageBoxA"));
        if (msgBoxA) msgBoxA(nullptr, fullError, "Fatal Error Occurred", MB_ICONERROR | MB_OK | MB_SYSTEMMODAL);
    }
#endif
}

[[noreturn]] void seal::fatalExit()
{
#if defined(_WIN32)
    ExitProcess(1);
    __assume(0); // unreachable
#elif defined(__GNUC__) || defined(__clang__)
    __builtin_trap();
#else
    // last resort volatile access that keeps the loop observable so it cannot be removed
    volatile int sink = 0;
    for (;;)
    {
        (void)sink;
    }
#endif
}
