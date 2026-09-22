#include "tests.h"

#include <seal/memory.h>
#include <seal/events.h>

class EventTest : public ITest
{
    public:
        EventTest() : ITest() {};

        static int freeFunctionCalls;
        static int freeFunctionValue;
        static void testFreeFunction(void*, int val)
        {
            freeFunctionCalls++;
            freeFunctionValue = val;
        }

        struct Context
        {
            int calls = 0;
            int lastVal = 0;

            static void method(void* ctx, int val)
            {
                Context* self = static_cast<Context*>(ctx);
                self->calls++;
                self->lastVal = val;
            }
        };

        virtual void run() override
        {
            seal::DynamicHeapAllocator heapAllocator;
            
            this->logInfo("testing Delegate free function");
            
            freeFunctionCalls = 0;
            seal::Delegate<int> d1(nullptr, testFreeFunction);
            d1(42);
            
            if (freeFunctionCalls != 1 || freeFunctionValue != 42)
            {
                this->logError("Delegate free function failed");
                return;
            }
            
            this->logInfo("testing Delegate context method");
            
            Context ctx;
            seal::Delegate<int> d2(&ctx, Context::method);
            d2(100);
            
            if (ctx.calls != 1 || ctx.lastVal != 100)
            {
                this->logError("Delegate context method failed");
                return;
            }

            this->logInfo("testing Event");
            seal::Event<int> ev(&heapAllocator);
            
            auto id1 = ev.addListener(d1);
            auto id2 = ev.addListener(d2);
            
            ev.run(200);
            
            if (freeFunctionCalls != 2 || freeFunctionValue != 200 || ctx.calls != 2 || ctx.lastVal != 200)
            {
                this->logError("Event run failed");
                return;
            }
            
            ev.removeListener(id1);
            ev.run(300);
            
            if (freeFunctionCalls != 2 || ctx.calls != 3 || ctx.lastVal != 300)
            {
                this->logError("Event removeListener failed");
                return;
            }
            
            ev.clear();
            ev.run(400);
            
            if (ctx.calls != 3)
            {
                this->logError("Event clear failed");
                return;
            }

            this->logInfo("all event tests passed successfully");
        }
        
        virtual const char* getName() override { return "Event test"; }
};

int EventTest::freeFunctionCalls = 0;
int EventTest::freeFunctionValue = 0;

static EventTest g_EventTest;
