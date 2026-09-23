#include "tests.h"
#include <seal/fmt.h>
#include <seal/memory.h>
#include <cmath>

class FormatTest : public ITest
{
    public:
        FormatTest() : ITest() {};

        virtual void run() override
        {
            seal::DynamicHeapAllocator heapAllocator;
            seal::setStringAllocator(&heapAllocator);

            /*
                basic str fmt and strview fmt
            */
            this->logInfo("testing format with strings and string views");
            seal::String s1 = seal::format("Hello, {}!", "World");
            if (s1 != seal::String("Hello, World!")) return this->logError("format const char* failed");

            seal::StringView sv("Seal Engine");
            seal::String s2 = seal::format("Framework: {}", sv);
            if (s2 != seal::String("Framework: Seal Engine")) return this->logError("format StringView failed");

            /*
                char and bool fmt
            */
            this->logInfo("testing format with bool and char");
            seal::String s4 = seal::format("Char: {}, BoolTrue: {}, BoolFalse: {}", 'Z', true, false);
            if (s4 != seal::String("Char: Z, BoolTrue: true, BoolFalse: false"))
                return this->logError("format bool/char failed");

            /*
                int fmt
            */
            this->logInfo("testing format with integers");
            seal::String s5 = seal::format("Zero: {}, Pos: {}, Neg: {}", 0, 42, -12345);
            if (s5 != seal::String("Zero: 0, Pos: 42, Neg: -12345"))
                return this->logError("format basic integers failed");

            seal::String s6 = seal::format("MinInt: {}", -2147483648LL);
            if (s6 != seal::String("MinInt: -2147483648"))
                return this->logError("format minimum signed integer failed");

            /*
                escaped braces
            */
            this->logInfo("testing escaped braces");
            seal::String sEsc = seal::format("{{Hello}} {0}!", "World");
            if (sEsc != seal::String("{Hello} World!")) return this->logError("format escaped braces failed");

            /*
                explicit positional indexing
            */
            this->logInfo("testing explicit indexing");
            seal::String sIdx = seal::format("{1} {0} {1}", "Zero", "One");
            if (sIdx != seal::String("One Zero One")) return this->logError("format explicit indexing failed");

            /*
                alignment and padding
            */
            this->logInfo("testing alignment and padding");
            seal::String sAlignRight = seal::format("{:>5}", 42);
            if (sAlignRight != seal::String("   42")) return this->logError("format right alignment failed");

            seal::String sAlignLeft = seal::format("{:<5}", 42);
            if (sAlignLeft != seal::String("42   ")) return this->logError("format left alignment failed");

            seal::String sAlignCenter = seal::format("{:^5}", 42);
            if (sAlignCenter != seal::String(" 42  ")) return this->logError("format center alignment failed");

            seal::String sPadZero = seal::format("{:05}", 42);
            if (sPadZero != seal::String("00042")) return this->logError("format zero padding failed");

            seal::String sPadNeg = seal::format("{:05}", -42);
            if (sPadNeg != seal::String("-0042")) return this->logError("format zero padding for negatives failed");

            /*
                floating point
            */
            this->logInfo("testing floating point formatting");
            seal::String sFloat1 = seal::format("Val: {}", 3.141592);
            if (sFloat1 != seal::String("Val: 3.141592")) return this->logError("format basic double failed");

            double nanValue = std::numeric_limits<double>::quiet_NaN();
            seal::String sFloatNan = seal::format("NaN: {}", nanValue);
            if (sFloatNan != seal::String("NaN: nan")) return this->logError("format NaN failed");

            seal::String sFloatInf = seal::format("Inf: {}", 1e300 * 1e300);
            if (sFloatInf != seal::String("Inf: inf")) return this->logError("format Inf failed");

            seal::String sFloatPrec = seal::format("{:.2}", 3.14159);
            if (sFloatPrec != seal::String("3.14"))
                return this->logError("format float precision failed"); // test truncation not rounding

            /*
                hexadecimal
            */
            this->logInfo("testing hexadecimal format");
            seal::String sHexLower = seal::format("{:x}", 255);
            if (sHexLower != seal::String("ff")) return this->logError("format hex lowercase failed");

            seal::String sHexUpper = seal::format("{:X}", 255);
            if (sHexUpper != seal::String("FF")) return this->logError("format hex uppercase failed");

            seal::String sHexZero = seal::format("{:X}", 0);
            if (sHexZero != seal::String("0")) return this->logError("format hex with 0 failed");

            seal::String sHex64 = seal::format("0x{:X}", 0xDEADBEEFCAFEULL);
            if (sHex64 != seal::String("0xDEADBEEFCAFE")) return this->logError("format 64-bit hex failed");

            seal::String sHexPad = seal::format("{:08X}", 0x1A2B);
            if (sHexPad != seal::String("00001A2B")) return this->logError("format hex padding failed");

            /*
                pointers
            */
            this->logInfo("testing pointer formatting");
            seal::String sPtrNull = seal::format("Null: {}", nullptr);
            if (sPtrNull != seal::String("Null: nullptr")) return this->logError("format nullptr failed");

            int dummyVal = 0;
            seal::String sPtr = seal::format("{}", static_cast<void*>(&dummyVal));
            if (sPtr.size() < 3 || sPtr[0] != '0' || sPtr[1] != 'x')
                return this->logError("format void* failed to include 0x prefix");

            this->logInfo("all format tests passed successfully");
        }

        virtual const char* getName() override { return "Format test"; }
};
static FormatTest g_FormatTest;
