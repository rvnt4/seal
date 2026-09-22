#include "tests.h"

#include <seal/memory.h>

class MemTest : public ITest
{
    public:
        MemTest() : ITest() {};

        virtual void run() override
        {
            /*
                mem helper test
            */
            this->logInfo("testing memory helpers");

            char buffer[10] = {0};
            seal::memset(buffer, 'A', 5);
            for (int i = 0; i < 5; i++)
            {
                if (buffer[i] != 'A')
                {
                    this->logError("memset failed: invalid char");
                    return;
                }
            }

            if (buffer[5] != '\0')
            {
                this->logError("memset failed: missing null terminator");
                return;
            }

            char src[5] = "test";
            char dest[5] = {0};
            seal::memcpy(dest, src, 5);
            if (seal::memcmp(src, dest, 5) != 0)
            {
                this->logError("memcpy/memcmp failed: mismatch?");
                return;
            }

            char overlap[10] = "abcdefghi";
            seal::memmove(overlap + 1, overlap, 8);
            if (seal::memcmp(overlap + 1, "abcdefgh", 8) != 0)
            {
                this->logError("memmove failed on overlapping range");
                return;
            }

            const char* found = static_cast<const char*>(seal::memchr(src, 's', 4));
            if (found == nullptr || *found != 's')
            {
                this->logError("memchr failed: unexpected found value");
                return;
            }

            /*
                mem align functions test
            */
            this->logInfo("testing memory alignment functions");

            if (!seal::isPowerOfTwo(8) || seal::isPowerOfTwo(5) || seal::isPowerOfTwo(0) || seal::isPowerOfTwo(-2))
            {
                this->logError("isPowerOfTwo failed");
                return;
            }

            if (seal::alignUp(5, 4) != 8 || seal::alignDown(5, 4) != 4)
            {
                this->logError("alignUp/alignDown failed");
                return;
            }

            /*
                arena allocator test
            */
            this->logInfo("testing ArenaAllocator");

            constexpr seal::ssize ArenaCapacity = 512;
            seal::ArenaAllocator<ArenaCapacity> arena;

            if (arena.getAllocatedSize() != 0 || arena.getFreeSize() != ArenaCapacity)
            {
                this->logError("ArenaAllocator initial state failed");
                return;
            }

            void* a1 = arena.allocate(64, 8);
            if (!a1 || arena.getAllocatedSize() < 64)
            {
                this->logError("ArenaAllocator allocation failed");
                return;
            }

            arena.reset();
            if (arena.getAllocatedSize() != 0 || arena.getFreeSize() != ArenaCapacity)
            {
                this->logError("ArenaAllocator reset failed");
                return;
            }

            /*
                dyn heap alloc test
            */
            this->logInfo("testing DynamicHeapAllocator");

            seal::DynamicHeapAllocator heap;

            void* h1 = heap.allocate(64, 8);
            if (!h1 || heap.getAllocatedSize() < 64)
            {
                this->logError("DynamicHeapAllocator allocation failed");
                return;
            }

            void* h2 = heap.allocate(128, 64);
            if (!h2 || (reinterpret_cast<seal::sealptr>(h2) % 64) != 0)
            {
                this->logError("DynamicHeapAllocator alignment failed");
                return;
            }

            void* h3 = heap.reallocate(h1, 256, 8);
            if (!h3)
            {
                this->logError("DynamicHeapAllocator reallocate failed");
                return;
            }

            heap.deallocate(h2);
            heap.deallocate(h3);

            this->logInfo("all memory tests passed successfully");
        }

        virtual const char* getName() override { return "Memory test"; }
};

static MemTest g_MemTest;
