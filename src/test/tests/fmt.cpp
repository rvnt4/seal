#include "tests.h"

#include <seal/fmt.h>
#include <seal/memory.h>

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
            if (s1 != seal::String("Hello, World!"))
            {
                this->logError("format const char* failed");
                return;
            }

            seal::StringView sv("Seal Engine");
            seal::String s2 = seal::format("Framework: {}", sv);
            if (s2 != seal::String("Framework: Seal Engine"))
            {
                this->logError("format StringView failed");
                return;
            }

            seal::String existingStr("Dynamic String");
            seal::String s3 = seal::format("Value = {}", existingStr);
            if (s3 != seal::String("Value = Dynamic String"))
            {
                this->logError("format String object failed");
                return;
            }

            /*
                char and bool fmt
            */
            this->logInfo("testing format with bool and char");

            seal::String s4 = seal::format("Char: {}, BoolTrue: {}, BoolFalse: {}", 'Z', true, false);
            if (s4 != seal::String("Char: Z, BoolTrue: true, BoolFalse: false"))
            {
                this->logError("format bool/char failed");
                return;
            }

            /*
                int fmt
            */
            this->logInfo("testing format with integers");

            seal::String s5 = seal::format("Zero: {}, Pos: {}, Neg: {}", 0, 42, -12345);
            if (s5 != seal::String("Zero: 0, Pos: 42, Neg: -12345"))
            {
                this->logError("format basic integers failed");
                return;
            }

            seal::String s6 = seal::format("MinInt: {}", -2147483648LL);
            if (s6 != seal::String("MinInt: -2147483648"))
            {
                this->logError("format minimum signed integer failed");
                return;
            }

            /*
                multiple placeholders and mixed types
            */
            this->logInfo("testing format with mixed argument types");

            seal::String s7 = seal::format("{} + {} = {} (Status: {})", 10, 20, 30, true);
            if (s7 != seal::String("10 + 20 = 30 (Status: true)"))
            {
                this->logError("format mixed types failed");
                return;
            }

            /*
                edge cases
            */
            this->logInfo("testing format edge cases");

            seal::String s8 = seal::format("No placeholders here!");
            if (s8 != seal::String("No placeholders here!"))
            {
                this->logError("format with no placeholders failed");
                return;
            }

            seal::String s9 = seal::format("Arg1: {}, Arg2: {}", "First");
            if (s9 != seal::String("Arg1: First, Arg2: {}"))
            {
                this->logError("format with unmatched placeholder failed");
                return;
            }

            this->logInfo("all format tests passed successfully");
        }

        virtual const char* getName() override { return "Format test"; }
};

static FormatTest g_FormatTest;
