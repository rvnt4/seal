#include "tests.h"

#include <seal/memory.h>
#include <seal/string.h>

class StringTest : public ITest
{
    public:
        StringTest() : ITest() {};

        virtual void run() override
        {
            seal::DynamicHeapAllocator heapAllocator;
            seal::setStringAllocator(&heapAllocator);

            /*
                construction and sso
            */
            this->logInfo("testing String construction and SSO");
            seal::String s1;
            if (!s1.empty() || s1.size() != 0 || !s1.is_sso())
            {
                this->logError("default constructor failed");
                return;
            }

            seal::String s2("Hello, Seal!");
            if (s2.size() != 12 || !s2.is_sso() || seal::memcmp(s2.c_str(), "Hello, Seal!", 13) != 0)
            {
                this->logError("short string constructor or SSO failed");
                return;
            }

            const char* longText = "This is a very long string that definitely exceeds twenty-two characters.";
            seal::String s3(longText);
            if (s3.is_sso() || s3.size() <= seal::String::sso_capacity)
            {
                this->logError("long string failed to trigger heap allocation");
                return;
            }

            /*
                copy and move
            */
            this->logInfo("testing String copy and move semantics");
            seal::String s4(s2);
            if (s4.size() != s2.size() || s4.compare(s2) != 0)
            {
                this->logError("copy constructor failed");
                return;
            }

            seal::String s5(std::move(s4));
            if (s5.compare(s2) != 0)
            {
                this->logError("move constructor failed");
                return;
            }

            seal::String s6;
            s6 = s3;
            if (s6.compare(s3) != 0)
            {
                this->logError("copy assignment failed");
                return;
            }

            seal::String s7;
            s7 = std::move(s6);
            if (s7.compare(s3) != 0)
            {
                this->logError("move assignment failed");
                return;
            }

            /*
                modification and mutation
            */
            this->logInfo("testing String mutations (push_back, append, resize)");

            seal::String s8;
            (void)s8.push_back('A');
            (void)s8.push_back('B');
            if (s8.size() != 2 || s8[0] != 'A' || s8[1] != 'B')
            {
                this->logError("push_back or operator[] failed");
                return;
            }

            s8.pop_back();
            if (s8.size() != 1 || s8[0] != 'A')
            {
                this->logError("pop_back failed");
                return;
            }

            char checkedChar = '\0';
            if (!s8.at(0, checkedChar) || checkedChar != 'A' || s8.at(5, checkedChar))
            {
                this->logError("at() bounds checking failed");
                return;
            }

            (void)s8.append("CD", 2);
            if (s8.compare("ACD") != 0)
            {
                this->logError("append failed");
                return;
            }

            s8.clear();
            if (!s8.empty() || s8.size() != 0)
            {
                this->logError("clear failed");
                return;
            }

            /*
                search and substr
            */
            this->logInfo("testing String find and substr");

            seal::String s9("Hello World");
            seal::usize idx = s9.find("World", 0);
            if (idx != 6)
            {
                this->logError("find() failed");
                return;
            }

            seal::String sub = s9.substr(0, 5);
            if (sub.compare("Hello") != 0)
            {
                this->logError("substr() failed");
                return;
            }

            /*
                comparison and operators
            */
            this->logInfo("testing String comparisons and relational operators");

            seal::String a("Apple");
            seal::String b("Banana");
            seal::String c("Apple");

            if (!(a == c) || (a == b) || !(a != b) || !(a < b))
            {
                this->logError("relational operators (==, !=, <) failed");
                return;
            }

            if (a.compare(b) >= 0 || b.compare(a) <= 0 || a.compare(c) != 0)
            {
                this->logError("compare() method failed");
                return;
            }

            /*
                allocator lifetime safety
            */
            this->logInfo("testing String allocator lifetime safety");
            {
                seal::String heapStr("This string is definitely longer than the small string optimization buffer.");
                if (heapStr.is_sso())
                {
                    this->logError("expected a heap-backed string");
                    return;
                }

                seal::setStringAllocator(nullptr);
            }
            seal::setStringAllocator(&heapAllocator);

            if (seal::getStringAllocator() != &heapAllocator)
            {
                this->logError("setStringAllocator/getStringAllocator mismatch");
                return;
            }

            this->logInfo("all string tests passed successfully");
        }

        virtual const char* getName() override { return "String test"; }
};

static StringTest g_StringTest;
