#include "tests.h"

int main()
{
    TestRunner& runner = TestRunner::get();
    runner.runTests();

    if (runner.failureCount() > 0)
    {
        std::cout << "\x1b[97;41m" << runner.failureCount() << " test(s) failed" << "\x1b[0m" << std::endl;
        return 1;
    }

    std::cout << "\x1b[32;40m" << "all tests passed" << "\x1b[0m" << std::endl;
    return 0;
}
