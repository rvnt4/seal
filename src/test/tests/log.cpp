#include "tests.h"

#include <seal/memory.h>
#include <seal/string.h>
#include <seal/log.h>

class MockSink : public seal::ILogSink
{
    public:
        seal::LogType lastType = seal::LogType::INFO;
        seal::String lastLoggerName;
        seal::String lastMessage;
        int callCount = 0;

        void receiveLog(seal::LogType type, seal::StringView loggerName, seal::StringView message) override
        {
            lastType = type;
            lastLoggerName = seal::String(loggerName.data(), loggerName.size());
            lastMessage = seal::String(message.data(), message.size());
            callCount++;
        }

        void reset()
        {
            callCount = 0;
            lastLoggerName.clear();
            lastMessage.clear();
        }
};

class LoggerTest : public ITest
{
    public:
        LoggerTest() : ITest() {};

        virtual void run() override
        {
            seal::DynamicHeapAllocator heapAllocator;
            seal::setStringAllocator(&heapAllocator);

            /*
                name and helper fn test
            */
            this->logInfo("testing Logger metadata and enum helpers");

            seal::Logger logger("CoreSystem", &heapAllocator);
            if (logger.getName() != seal::StringView("CoreSystem"))
            {
                this->logError("logger name mismatch");
                return;
            }

            if (seal::getLogTypeName(seal::LogType::WARNING) != seal::StringView("WARNING"))
            {
                this->logError("getLogTypeName failed");
                return;
            }

            /*
                sink attachment and routing test
            */
            this->logInfo("testing Logger sink attachment and message routing");
            void* mockSinkMem = heapAllocator.allocate(sizeof(MockSink), alignof(MockSink));
            MockSink* mockSink = new (mockSinkMem) MockSink();
            seal::SharedPtr<seal::ILogSink> sink(mockSink, &heapAllocator);
            (void)logger.addSink(sink);

            logger.info("Example log {}", 1337);

            if (mockSink->callCount != 1)
            {
                this->logError("sink did not receive the log message");
                return;
            }

            if (mockSink->lastType != seal::LogType::INFO)
            {
                this->logError("log type mismatch (expected INFO)");
                return;
            }

            if (mockSink->lastLoggerName != seal::String("CoreSystem"))
            {
                this->logError("logger name passed to sink mismatch");
                return;
            }

            if (mockSink->lastMessage != seal::String("Example log 1337"))
            {
                this->logError("formatted log message content mismatch");
                return;
            }

            /*
                log level test
            */
            this->logInfo("testing various log levels (verbose, warning, error)");

            mockSink->reset();

            logger.verbose("Debug value: {}", 3.14);
            logger.warning("Low memory warning: {}% used", 92);
            logger.error("Fatal error code: {}", 500);

            if (mockSink->callCount != 3)
            {
                this->logError("multiple log levels call count mismatch");
                return;
            }

            if (mockSink->lastType != seal::LogType::ERROR || mockSink->lastMessage != seal::String("Fatal error code: 500"))
            {
                this->logError("last log level (ERROR) message mismatch");
                return;
            }

            this->logInfo("all logger tests passed successfully");
        }
        virtual const char* getName() override { return "Logger test"; }
};

static LoggerTest g_LoggerTest;
