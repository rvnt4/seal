#pragma once

#include <vector>
#include <iostream>
#include <format>

/*
	test implementation interface
*/
class ITest
{
    public:
        ITest();
        virtual ~ITest() = default;

    protected:
        virtual void run() = 0;
        virtual const char* getName() = 0;

        /*
            shamelessly yoinked from log lib
        */
        template <typename... Args> void logVerbose(std::format_string<Args...> fmt, Args&&... args)
        {
            writeLog("\x1b[34;40m", std::vformat(fmt.get(), std::make_format_args(args...)));
        }

        template <typename... Args> void logInfo(std::format_string<Args...> fmt, Args&&... args)
        {
            writeLog("\x1b[32;40m", std::vformat(fmt.get(), std::make_format_args(args...)));
        }

        template <typename... Args> void logWarning(std::format_string<Args...> fmt, Args&&... args)
        {
            writeLog("\x1b[30;43m", std::vformat(fmt.get(), std::make_format_args(args...)));
        }

        template <typename... Args> void logError(std::format_string<Args...> fmt, Args&&... args)
        {
            writeLog("\x1b[97;41m", std::vformat(fmt.get(), std::make_format_args(args...)));
        }

        friend class TestRunner;

    private:
        void writeLog(const char* color, const std::string& message)
        {
            std::cout << color << "[" << getName() << "]" << "\x1b[0m " << message << std::endl;
        }
};

/*
	test runner singleton class
*/
class TestRunner
{
    public:
        void runTests()
        {
            for (auto& test : _tests)
            {
                test->logInfo("------- Init -------");
                test->run();
                test->logInfo("-------- End --------");
            }
        }

        static TestRunner& get()
        {
            static TestRunner instance;
            return instance;
        }

    protected:
        void registerTest(ITest* test) { _tests.push_back(test); }
        friend class ITest;

    private:
        TestRunner() {};
        TestRunner(const TestRunner&) = delete;
        TestRunner& operator=(const TestRunner&) = delete;

        std::vector<ITest*> _tests;
};

/*
    test cctor
*/
inline ITest::ITest()
{
    TestRunner::get().registerTest(this);
}
