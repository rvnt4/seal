#pragma once

#include "types.h"

namespace seal
{
    struct placement_t
    {
    };
} // namespace seal
inline void* operator new(seal::usz size, void* ptr, seal::placement_t) noexcept
{
    (void)size;
    return ptr;
}
inline void operator delete(void*, void*, seal::placement_t) noexcept {}

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
    inline void* mem_set(void* dest, int value, ssz count)
    {
        if (count <= 0) return dest;
        auto* ptr = static_cast<unsigned char*>(dest);
        const auto val = static_cast<unsigned char>(value);
        for (ssz i = 0; i < count; ++i)
            ptr[i] = val;
        return dest;
    }

    inline void* mem_copy(void* dest, const void* src, ssz count)
    {
        if (count <= 0) return dest;
        auto* d = static_cast<unsigned char*>(dest);
        const auto* s = static_cast<const unsigned char*>(src);
        for (ssz i = 0; i < count; ++i)
            d[i] = s[i];
        return dest;
    }

    inline void* mem_move(void* dest, const void* src, ssz count)
    {
        if (dest == src || count <= 0) return dest;
        auto* d = static_cast<unsigned char*>(dest);
        const auto* s = static_cast<const unsigned char*>(src);

        const sealptr d_addr = reinterpret_cast<sealptr>(d);
        const sealptr src_addr = reinterpret_cast<sealptr>(s);

        if (d_addr < src_addr)
        {
            for (ssz i = 0; i < count; ++i)
                d[i] = s[i];
        }
        else
        {
            for (ssz i = count; i > 0; --i)
                d[i - 1] = s[i - 1];
        }
        return dest;
    }

    inline int mem_cmp(const void* ptr1, const void* ptr2, ssz count)
    {
        const auto* s1 = static_cast<const unsigned char*>(ptr1);
        const auto* s2 = static_cast<const unsigned char*>(ptr2);
        for (ssz i = 0; i < count; ++i)
        {
            if (s1[i] != s2[i]) return static_cast<int>(s1[i]) - static_cast<int>(s2[i]);
        }
        return 0;
    }

    inline const void* mem_chr(const void* ptr, int ch, ssz count)
    {
        if (!ptr || count <= 0) return nullptr;
        const auto* p = static_cast<const unsigned char*>(ptr);
        const auto c = static_cast<unsigned char>(ch);
        for (ssz i = 0; i < count; ++i)
        {
            if (p[i] == c) return p + i;
        }
        return nullptr;
    }

    inline void* mem_chr(void* ptr, int ch, ssz count)
    {
        return const_cast<void*>(mem_chr(static_cast<const void*>(ptr), ch, count));
    }

    inline usz str_len(const char* s) noexcept
    {
        if (!s) return 0;
        usz n = 0;
        while (s[n] != '\0')
            ++n;
        return n;
    }

    /*
        alignment functions
    */
    inline constexpr bool isPowerOfTwo(ssz value)
    {
        return value > 0 && !(value & (value - 1));
    }

    inline constexpr ssz alignUp(ssz value, ssz alignment)
    {
        if (alignment <= 1) return value;
        return (value + alignment - 1) & ~(alignment - 1);
    }

    inline constexpr ssz alignDown(ssz value, ssz alignment)
    {
        if (alignment <= 1) return value;
        return value & ~(alignment - 1);
    }

    inline sealptr alignPtrUp(sealptr ptr, ssz alignment)
    {
        if (alignment <= 1) return ptr;
        const sealptr mask = static_cast<sealptr>(alignment) - 1;
        return (ptr + mask) & ~mask;
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

            virtual void* allocate(ssz size, ssz alignment = 8) = 0;
            virtual void* reallocate(void* ptr, ssz newSize, ssz alignment = 8) = 0;
            virtual void deallocate(void* ptr) = 0;

            virtual ssz getAllocatedSize() const = 0;
            virtual ssz getFreeSize() const = 0;

            virtual const char* getAllocatorName() const = 0;
    };

    /*
        arena allocator
    */
    template <ssz Capacity> class ArenaAllocator : public IAllocator
    {
        public:
            ArenaAllocator() : _offset(0)
            {
                for (ssz i = 0; i < Capacity; ++i)
                    _heap[i] = 0;
            }

            ~ArenaAllocator() override = default;

            ArenaAllocator(const ArenaAllocator&) = delete;
            ArenaAllocator& operator=(const ArenaAllocator&) = delete;

            ArenaAllocator(ArenaAllocator&&) = delete;
            ArenaAllocator& operator=(ArenaAllocator&&) = delete;

            void* allocate(ssz size, ssz alignment = 8) override
            {
                if (size <= 0) return nullptr;

                const sealptr current_ptr = reinterpret_cast<sealptr>(_heap + _offset);
                const sealptr aligned_ptr = alignPtrUp(current_ptr, alignment);
                const ssz shift = static_cast<ssz>(aligned_ptr - current_ptr);

                if (shift < 0 || _offset + shift + size > Capacity) return nullptr; // out of mem

                _offset += shift + size;
                return reinterpret_cast<void*>(aligned_ptr);
            }

            void* reallocate(void* ptr, ssz newSize, ssz alignment = 8) override
            {
                if (!ptr) return allocate(newSize, alignment);
                if (newSize <= 0) return nullptr;

                void* newPtr = allocate(newSize, alignment);
                if (newPtr)
                {
                    ssz offset = static_cast<ssz>(static_cast<unsigned char*>(ptr) - _heap);
                    ssz maxCopy = Capacity - offset;
                    if (maxCopy < 0) maxCopy = 0;
                    ssz copySize = (newSize < maxCopy) ? newSize : maxCopy;
                    if (copySize > 0) seal::mem_copy(newPtr, ptr, copySize);
                }
                return newPtr;
            }

            void deallocate(void* ptr) override { (void)ptr; }
            void reset() { _offset = 0; }

            ssz getAllocatedSize() const override { return _offset; }
            ssz getFreeSize() const override { return Capacity - _offset; }

            const char* getAllocatorName() const override { return "ArenaAllocator"; }

        private:
            unsigned char _heap[Capacity];
            ssz _offset;
    };

    class DynamicHeapAllocator : public IAllocator
    {
        private:
            struct BlockHeader
            {
                    ssz size;          // total block size including header
                    ssz requestedSize; // exact bytes requested by user
                    bool isFree;
                    BlockHeader* next;     // next physical block in memory page
                    BlockHeader* prev;     // previous physical block in memory page
                    BlockHeader* nextFree; // free-list pointer
                    BlockHeader* prevFree; // free-list pointer
            };

            struct PageChunk
            {
                    ssz size;
                    PageChunk* next;
            };

            static constexpr ssz MIN_CHUNK_SIZE = 64 * 1024; // default page request: 64 KB
            static constexpr ssz HEADER_SIZE = sizeof(BlockHeader);

            BlockHeader* _freeListHead = nullptr;
            PageChunk* _chunkListHead = nullptr;
            ssz _totalAllocatedBytes = 0;
            ssz _totalCapacityBytes = 0;

        public:
            DynamicHeapAllocator() = default;
            ~DynamicHeapAllocator() override;

            DynamicHeapAllocator(const DynamicHeapAllocator&) = delete;
            DynamicHeapAllocator& operator=(const DynamicHeapAllocator&) = delete;

            DynamicHeapAllocator(DynamicHeapAllocator&& other) noexcept;

            void* allocate(ssz size, ssz alignment = 8) override;
            void* reallocate(void* ptr, ssz newSize, ssz alignment = 8) override;
            void deallocate(void* ptr) override;

            ssz getAllocatedSize() const override { return _totalAllocatedBytes; }
            ssz getFreeSize() const override { return _totalCapacityBytes - _totalAllocatedBytes; }
            const char* getAllocatorName() const override { return "DynamicHeapAllocator"; }

        private:
            BlockHeader* findBestFit(ssz size);

            void splitBlock(BlockHeader* block, ssz size);
            void coalesce(BlockHeader* block);
            void insertFreeBlock(BlockHeader* block);
            void removeFreeBlock(BlockHeader* block);
    };

    /*
        shared ptr
    */
    template <typename T> class SharedPtr
    {
        private:
            struct ControlBlock
            {
                    T* ptr;    // typed pointer used for destruction
                    void* raw; // original allocation address used for deallocation
                    usz ref_count;
                    IAllocator* alloc;
            };

            ControlBlock* _cb = nullptr;

            void destroyBlock() noexcept
            {
                if (!_cb) return;
                IAllocator* alloc = _cb->alloc;
                if (_cb->ptr) _cb->ptr->~T();
                if (_cb->raw) alloc->deallocate(_cb->raw);
                alloc->deallocate(_cb);
            }

        public:
            constexpr SharedPtr() noexcept = default;

            explicit SharedPtr(T* ptr, IAllocator* alloc) noexcept : SharedPtr(ptr, static_cast<void*>(ptr), alloc) {}

            SharedPtr(T* ptr, void* rawPtr, IAllocator* alloc) noexcept
            {
                if (!ptr || !alloc) return;
                void* raw = rawPtr ? rawPtr : static_cast<void*>(ptr);
                _cb = static_cast<ControlBlock*>(alloc->allocate(sizeof(ControlBlock), alignof(ControlBlock)));
                if (_cb)
                {
                    _cb->ptr = ptr;
                    _cb->raw = raw;
                    _cb->ref_count = 1;
                    _cb->alloc = alloc;
                }
                else
                {
                    ptr->~T();
                    alloc->deallocate(raw);
                }
            }

            ~SharedPtr() noexcept { release(); }

            SharedPtr(const SharedPtr& other) noexcept : _cb(other._cb)
            {
                if (_cb) _cb->ref_count++;
            }

            SharedPtr& operator=(const SharedPtr& other) noexcept
            {
                if (this != &other)
                {
                    release();
                    _cb = other._cb;
                    if (_cb) _cb->ref_count++;
                }
                return *this;
            }

            SharedPtr(SharedPtr&& other) noexcept : _cb(other._cb) { other._cb = nullptr; }

            SharedPtr& operator=(SharedPtr&& other) noexcept
            {
                if (this != &other)
                {
                    release();
                    _cb = other._cb;
                    other._cb = nullptr;
                }
                return *this;
            }

            T* get() const noexcept { return _cb ? _cb->ptr : nullptr; }
            T* operator->() const noexcept { return get(); }
            T& operator*() const noexcept { return *get(); }
            explicit operator bool() const noexcept { return get() != nullptr; }

            usz use_count() const noexcept { return _cb ? _cb->ref_count : 0; }

            void reset() noexcept { release(); }

            template <typename U> bool operator==(const SharedPtr<U>& other) const noexcept { return get() == other.get(); }
            template <typename U> bool operator!=(const SharedPtr<U>& other) const noexcept { return get() != other.get(); }
            bool operator==(decltype(nullptr)) const noexcept { return get() == nullptr; }
            bool operator!=(decltype(nullptr)) const noexcept { return get() != nullptr; }

        private:
            void release() noexcept
            {
                if (_cb)
                {
                    if (--_cb->ref_count == 0) destroyBlock();
                    _cb = nullptr;
                }
            }
    };

    template <typename T, typename... Args>
    SharedPtr<T> makeShared(IAllocator* alloc, Args&&... args) noexcept
    {
        if (!alloc) return SharedPtr<T>();
        void* raw = alloc->allocate(sizeof(T), alignof(T));
        if (!raw) return SharedPtr<T>();
        T* obj = ::new (raw, seal::placement_t{}) T(static_cast<Args&&>(args)...);
        return SharedPtr<T>(obj, raw, alloc);
    }
} // namespace seal
