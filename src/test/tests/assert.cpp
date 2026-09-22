#include "tests.h"

#include <seal/assert.h>

class AssertTest : public ITest
{
    public:
        AssertTest() : ITest() {};

        virtual void run() override
        {
            this->logInfo("testing ASSERT with satisfied conditions");

            ASSERT(true, "this must never fire");
            ASSERT(1 + 1 == 2, "arithmetic must hold");

            int value = 42;
            ASSERT(value == 42, "value check");

            int* ptr = &value;
            ASSERT(ptr != nullptr, "pointer check");

            this->logInfo("all assert tests passed successfully");
        }

        virtual const char* getName() override { return "Assert test"; }
};

static AssertTest g_AssertTest;
