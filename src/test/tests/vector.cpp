#include "tests.h"

#include <seal/vector.h>
#include <seal/string.h>

class VectorTest : public ITest
{
    public:
        VectorTest() : ITest() {};

        virtual void run() override
        {
            seal::DynamicHeapAllocator heapAllocator;

            /*
                prim types and growth test
            */
            this->logInfo("testing Vector with primitive types and dynamic growth");
            seal::Vector<int> vec(&heapAllocator);
            if (vec.size() != 0)
            {
                this->logError("initial vector size is not zero");
                return;
            }

            for (int i = 0; i < 50; ++i)
            {
                if (!vec.push_back(i))
                {
                    this->logError("push_back failed during growth loop");
                    return;
                }
            }

            if (vec.size() != 50)
            {
                this->logError("vector size mismatch after push_back loop");
                return;
            }

            int count = 0;
            for (int val : vec)
            {
                if (val != count++)
                {
                    this->logError("vector iteration value mismatch");
                    return;
                }
            }

            /*
                reserve and clear test
            */
            this->logInfo("testing Vector reserve and clear");

            if (!vec.reserve(100))
            {
                this->logError("reserve(100) failed");
                return;
            }

            if (vec.size() != 50)
            {
                this->logError("reserve altered vector size unexpectedly");
                return;
            }

            vec.clear();
            if (vec.size() != 0)
            {
                this->logError("clear failed to reset vector size");
                return;
            }

            /*
                non trivial types and lifetime
            */
            this->logInfo("testing Vector with non-trivial types (seal::String)");

            seal::setStringAllocator(&heapAllocator);
            seal::Vector<seal::String> strVec(&heapAllocator);

            strVec.push_back(seal::String("Short"));
            strVec.push_back(seal::String("A much longer string that bypasses SSO allocation"));

            if (strVec.size() != 2)
            {
                this->logError("string vector size mismatch");
                return;
            }

            if (strVec.begin()[0] != seal::String("Short") ||
                strVec.begin()[1] != seal::String("A much longer string that bypasses SSO allocation"))
            {
                this->logError("string vector content mismatch");
                return;
            }

            if (!strVec.reserve(32))
            {
                this->logError("string vector reserve failed");
                return;
            }

            if (strVec.begin()[0] != seal::String("Short") ||
                strVec.begin()[1] != seal::String("A much longer string that bypasses SSO allocation"))
            {
                this->logError("string vector content corrupt after reserve relocation");
                return;
            }

            /*
                move semantics
            */
            this->logInfo("testing Vector move construction and move assignment");

            seal::Vector<seal::String> movedVec(static_cast<seal::Vector<seal::String>&&>(strVec));
            if (strVec.size() != 0 || movedVec.size() != 2)
            {
                this->logError("vector move constructor failed");
                return;
            }

            seal::Vector<seal::String> assignedVec(&heapAllocator);
            assignedVec = static_cast<seal::Vector<seal::String>&&>(movedVec);
            if (movedVec.size() != 0 || assignedVec.size() != 2)
            {
                this->logError("vector move assignment failed");
                return;
            }

            this->logInfo("all vector tests passed successfully");
        }
        virtual const char* getName() override { return "Vector test"; }
};

static VectorTest g_VectorTest;
