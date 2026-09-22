#pragma once

namespace seal
{
    /*
        ptr type
    */
#if defined(__SIZEOF_POINTER__)
    #if __SIZEOF_POINTER__ == 8
    typedef unsigned long long sealptr;
    #elif __SIZEOF_POINTER__ == 4
    typedef unsigned int sealptr;
    #endif
#elif defined(_WIN64)
    typedef unsigned long long sealptr;
#elif defined(_WIN32)
    typedef unsigned int sealptr;
#elif defined(__x86_64__) || defined(__aarch64__)
    typedef unsigned long sealptr;
#else
    typedef unsigned long sealptr;
#endif

    /*
        generic mem fns
    */
    inline void* memset(void* dest, int value, size_t count)
    {
        auto* ptr = static_cast<unsigned char*>(dest);
        const auto val = static_cast<unsigned char>(value);
        for (size_t i = 0; i < count; ++i)
            ptr[i] = val;
        return dest;
    }

    inline void* memcpy(void* dest, const void* src, size_t count)
    {
        auto* d = static_cast<unsigned char*>(dest);
        const auto* s = static_cast<const unsigned char*>(src);
        for (size_t i = 0; i < count; ++i)
            d[i] = s[i];
        return dest;
    }

    inline void* memmove(void* dest, const void* src, size_t count)
    {
        auto* d = static_cast<unsigned char*>(dest);
        const auto* s = static_cast<const unsigned char*>(src);

        if (d == s || count == 0) return dest;

        if (d < s)
        {
            for (size_t i = 0; i < count; ++i)
                d[i] = s[i];
        }
        else
        {
            for (size_t i = count; i > 0; --i)
                d[i - 1] = s[i - 1];
        }
        return dest;
    }

    inline int memcmp(const void* ptr1, const void* ptr2, size_t count)
    {
        const auto* s1 = static_cast<const unsigned char*>(ptr1);
        const auto* s2 = static_cast<const unsigned char*>(ptr2);
        for (size_t i = 0; i < count; ++i)
        {
            if (s1[i] != s2[i]) return static_cast<int>(s1[i]) - static_cast<int>(s2[i]);
        }
        return 0;
    }

    inline const void* memchr(const void* ptr, int ch, size_t count)
    {
        const auto* p = static_cast<const unsigned char*>(ptr);
        const auto c = static_cast<unsigned char>(ch);
        for (size_t i = 0; i < count; ++i)
        {
            if (p[i] == c) return p + i;
        }
        return nullptr;
    }

    inline void* memchr(void* ptr, int ch, size_t count)
    {
        return const_cast<void*>(memchr(static_cast<const void*>(ptr), ch, count));
    }

    /*
        alignment functions
    */
    inline constexpr bool isPowerOfTwo(size_t value)
    {
        return value && !(value & (value - 1));
    }

    inline constexpr size_t alignUp(size_t value, size_t alignment)
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    inline constexpr size_t alignDown(size_t value, size_t alignment)
    {
        return value & ~(alignment - 1);
    }

    inline sealptr alignPtrUp(sealptr ptr, size_t alignment)
    {
        return (ptr + alignment - 1) & ~(alignment - 1);
    }

    /*
        allocator base interface
    */
    enum class AllocatorResult
    {
        Success = 0,
        OutOfMemory,
        InvalidPointer,
        Unknown
    };

    inline const char* allocatorResultToString(AllocatorResult result)
    {
        switch (result)
        {
            case AllocatorResult::Success:
                return "Success";
            case AllocatorResult::OutOfMemory:
                return "OutOfMemory";
            case AllocatorResult::InvalidPointer:
                return "InvalidPointer";
            default:
                return "Unknown";
        }
    }

    class IAllocator
    {
        public:
            virtual ~IAllocator() = default;

            virtual void* allocate(size_t size, size_t alignment = 8) = 0;
            virtual void* reallocate(void* ptr, size_t newSize, size_t alignment = 8) = 0;
            virtual void deallocate(void* ptr) = 0;

            virtual size_t getAllocatedSize() const = 0;
            virtual size_t getFreeSize() const = 0;

            virtual const char* getAllocatorName() const = 0;
    };

    /*
        arena allocator
    */
    template <size_t Capacity> class ArenaAllocator : public IAllocator
    {
        public:
            ArenaAllocator() : _offset(0)
            {
                for (size_t i = 0; i < Capacity; ++i)
                    _heap[i] = 0;
            }

            ~ArenaAllocator() override = default;

            ArenaAllocator(const ArenaAllocator&) = delete;
            ArenaAllocator& operator=(const ArenaAllocator&) = delete;

            ArenaAllocator(ArenaAllocator&&) noexcept = default;
            ArenaAllocator& operator=(ArenaAllocator&&) noexcept = default;

            void* allocate(size_t size, size_t alignment = 8) override
            {
                if (size == 0) return nullptr;

                sealptr current_ptr = reinterpret_cast<sealptr>(_heap + _offset);
                sealptr aligned_ptr = alignPtrUp(current_ptr, alignment);
                size_t shift = aligned_ptr - current_ptr;

                if (_offset + shift + size > Capacity) return nullptr; // out of mem

                _offset += shift + size;
                return reinterpret_cast<void*>(aligned_ptr);
            }

            virtual void* reallocate(void* ptr, size_t newSize, size_t alignment = 8) override
            {
                if (!ptr) return allocate(newSize, alignment);
                if (newSize == 0) return nullptr;

                void* newPtr = allocate(newSize, alignment);
                if (newPtr)
                {
                    size_t maxCopy = Capacity - (static_cast<unsigned char*>(ptr) - _heap);
                    size_t copySize = (newSize < maxCopy) ? newSize : maxCopy;
                    seal::memcpy(newPtr, ptr, copySize);
                }
                return newPtr;
            }

            void deallocate(void* ptr) override { ptr; }
            void reset() { _offset = 0; }

            size_t getAllocatedSize() const override { return _offset; }
            size_t getFreeSize() const override { return Capacity - _offset; }

            const char* getAllocatorName() const override { return "ArenaAllocator"; }

        private:
            unsigned char _heap[Capacity];
            size_t _offset;
    };

    class DynamicHeapAllocator : public IAllocator
    {
        private:
            struct BlockHeader
            {
                    size_t size;          // total block size including header
                    size_t requestedSize; // exact bytes requested by user
                    bool isFree;
                    BlockHeader* next;     // next physical block in memory page
                    BlockHeader* prev;     // previous physical block in memory page
                    BlockHeader* nextFree; // free-list pointer
                    BlockHeader* prevFree; // free-list pointer
            };

            struct PageChunk
            {
                    size_t size;
                    PageChunk* next;
            };

            static constexpr size_t MIN_CHUNK_SIZE = 64 * 1024; // default page request: 64 KB
            static constexpr size_t HEADER_SIZE = sizeof(BlockHeader);

            BlockHeader* _freeListHead = nullptr;
            PageChunk* _chunkListHead = nullptr;
            size_t _totalAllocatedBytes = 0;
            size_t _totalCapacityBytes = 0;

        public:
            DynamicHeapAllocator() = default;
            ~DynamicHeapAllocator() override;

            DynamicHeapAllocator(const DynamicHeapAllocator&) = delete;
            DynamicHeapAllocator& operator=(const DynamicHeapAllocator&) = delete;

            DynamicHeapAllocator(DynamicHeapAllocator&& other) noexcept;

            void* allocate(size_t size, size_t alignment = 8) override;
            void* reallocate(void* ptr, size_t newSize, size_t alignment = 8) override;
            void deallocate(void* ptr) override;

            size_t getAllocatedSize() const override { return _totalAllocatedBytes; }
            size_t getFreeSize() const override { return _totalCapacityBytes - _totalAllocatedBytes; }
            const char* getAllocatorName() const override { return "DynamicHeapAllocator"; }

        private:
            BlockHeader* findBestFit(size_t size);

            void splitBlock(BlockHeader* block, size_t size);
            void coalesce(BlockHeader* block);
            void insertFreeBlock(BlockHeader* block);
            void removeFreeBlock(BlockHeader* block);
    };
} // namespace seal
