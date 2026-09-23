#include "tests.h"

#include <seal/memory.h>
#include <seal/string.h>
#include <seal/events.h>

namespace
{
    struct StringSinkCtx
    {
        seal::Vector<seal::String>* out;

        static void record(void* ctx, seal::String value)
        {
            static_cast<StringSinkCtx*>(ctx)->out->push_back(static_cast<seal::String&&>(value));
        }
    };

    struct SelfRemoveCtx
    {
        seal::Event<int>* ev;
        seal::Event<int>::ConnectionId id;
        int calls;

        static void cb(void* ctx, int)
        {
            SelfRemoveCtx* self = static_cast<SelfRemoveCtx*>(ctx);
            self->calls++;
            self->ev->removeListener(self->id);
        }
    };
} // namespace

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
            id2; // unused

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

            /*
                non-trivial arguments must reach every listener intact
            */
            this->logInfo("testing Event with non-trivial arguments and multiple listeners");
            {
                seal::setStringAllocator(&heapAllocator);
                seal::Vector<seal::String> received(&heapAllocator);
                StringSinkCtx sinkA{&received};
                StringSinkCtx sinkB{&received};

                seal::Event<seal::String> strEvent(&heapAllocator);
                if (strEvent.addListener(seal::Delegate<seal::String>(&sinkA, StringSinkCtx::record)) ==
                    seal::Event<seal::String>::InvalidConnection)
                {
                    this->logError("addListener failed for non-trivial event");
                    return;
                }
                strEvent.addListener(seal::Delegate<seal::String>(&sinkB, StringSinkCtx::record));

                strEvent.run(seal::String("payload longer than the SSO buffer size"));

                if (received.size() != 2 || received[0].size() == 0 || received[0] != received[1])
                {
                    this->logError("Event moved arguments between listeners");
                    return;
                }
            }

            /*
                listeners may safely remove themselves during dispatch
            */
            this->logInfo("testing listener self-removal during dispatch");
            {
                seal::Event<int> selfRemove(&heapAllocator);
                SelfRemoveCtx sctx{&selfRemove, seal::Event<int>::InvalidConnection, 0};
                sctx.id = selfRemove.addListener(seal::Delegate<int>(&sctx, SelfRemoveCtx::cb));
                selfRemove.run(1);

                if (sctx.calls != 1 || selfRemove.listenerCount() != 0)
                {
                    this->logError("self-removal during dispatch failed");
                    return;
                }
            }

            this->logInfo("all event tests passed successfully");
        }

        virtual const char* getName() override { return "Event test"; }
};

int EventTest::freeFunctionCalls = 0;
int EventTest::freeFunctionValue = 0;

static EventTest g_EventTest;
