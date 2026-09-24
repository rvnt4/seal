#include "memory.h"



/*
    os helpers
*/
#if defined(_KERNEL_MODE)
    #include <ntddk.h>
#elif defined(_WIN32)
extern "C"
{
    __declspec(dllimport) void* __stdcall VirtualAlloc(void* lpAddress, seal::ssz dwSize, unsigned long flAllocationType,
                                                       unsigned long flProtect);
    __declspec(dllimport) int __stdcall VirtualFree(void* lpAddress, seal::ssz dwSize, unsigned long dwFreeType);
}
#elif defined(__unix__) || defined(__APPLE__) || defined(__linux__)
    #include <sys/mman.h>
#endif

using namespace seal;

namespace
{
    void* allocatePages(ssz bytes)
    {
#if defined(_KERNEL_MODE)
        return ExAllocatePool2(POOL_FLAG_NON_PAGED, bytes, 'lseS');
#elif defined(_WIN32)
        // MEM_COMMIT | MEM_RESERVE = 0x1000 | 0x2000, PAGE_READWRITE = 0x04
        return VirtualAlloc(nullptr, bytes, 0x3000, 0x04);
#elif defined(__unix__) || defined(__APPLE__) || defined(__linux__)
        void* ptr = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        return (ptr == MAP_FAILED) ? nullptr : ptr;
#else
        (void)bytes;
        return nullptr;
#endif
    }

    void freePages(void* ptr, ssz bytes)
    {
        if (!ptr) return;
#if defined(_KERNEL_MODE)
        (void)bytes;
        ExFreePoolWithTag(ptr, 'lseS');
#elif defined(_WIN32)
        (void)bytes;
        // MEM_RELEASE = 0x8000
        VirtualFree(ptr, 0, 0x8000);
#elif defined(__unix__) || defined(__APPLE__) || defined(__linux__)
        munmap(ptr, bytes);
#else
        (void)bytes;
#endif
    }

    ssz normalizeAlignment(ssz alignment)
    {
        const ssz minAlign = static_cast<ssz>(alignof(void*));
        if (alignment <= minAlign) return minAlign;
        if (isPowerOfTwo(alignment)) return alignment;

        ssz p = minAlign;
        while (p < alignment)
            p <<= 1;
        return p;
    }
} // namespace

/*
    dynamic heap allocator
*/
DynamicHeapAllocator::DynamicHeapAllocator(DynamicHeapAllocator&& other) noexcept
    : _freeListHead(other._freeListHead), _chunkListHead(other._chunkListHead),
      _totalAllocatedBytes(other._totalAllocatedBytes), _totalCapacityBytes(other._totalCapacityBytes)
{
    other._freeListHead = nullptr;
    other._chunkListHead = nullptr;
    other._totalAllocatedBytes = 0;
    other._totalCapacityBytes = 0;
}

DynamicHeapAllocator::~DynamicHeapAllocator()
{
    PageChunk* chunk = _chunkListHead;
    while (chunk)
    {
        PageChunk* next = chunk->next;
        freePages(chunk, chunk->size);
        chunk = next;
    }
}

void* DynamicHeapAllocator::allocate(ssz size, ssz alignment)
{
    if (size <= 0) return nullptr;
    if (size > (static_cast<ssz>(1) << 60)) return nullptr;

    const ssz eff_align = normalizeAlignment(alignment);
    const ssz payload_size = alignUp(size, eff_align);
    const ssz reserve = HEADER_SIZE + payload_size + eff_align + static_cast<ssz>(sizeof(void*));

    BlockHeader* block = findBestFit(reserve);

    if (!block)
    {
        ssz req_size = reserve + static_cast<ssz>(sizeof(PageChunk));
        if (req_size < MIN_CHUNK_SIZE) req_size = MIN_CHUNK_SIZE;

        void* raw = allocatePages(req_size);
        if (!raw) return nullptr;

        PageChunk* chunk = static_cast<PageChunk*>(raw);
        chunk->size = req_size;
        chunk->next = _chunkListHead;
        _chunkListHead = chunk;
        _totalCapacityBytes += req_size;

        unsigned char* block_start = static_cast<unsigned char*>(raw) + sizeof(PageChunk);
        block = reinterpret_cast<BlockHeader*>(block_start);
        block->size = req_size - static_cast<ssz>(sizeof(PageChunk));
        block->requestedSize = 0;
        block->isFree = true;
        block->next = nullptr;
        block->prev = nullptr;
        block->nextFree = nullptr;
        block->prevFree = nullptr;

        insertFreeBlock(block);
    }

    unsigned char* block_bytes = reinterpret_cast<unsigned char*>(block);
    unsigned char* base = block_bytes + HEADER_SIZE;
    unsigned char* data = reinterpret_cast<unsigned char*>(alignPtrUp(reinterpret_cast<sealptr>(base), eff_align));
    if (data - base < static_cast<ssz>(sizeof(void*))) data += eff_align;

    *reinterpret_cast<BlockHeader**>(data - sizeof(void*)) = block;

    const ssz used = static_cast<ssz>(data - block_bytes) + payload_size;
    splitBlock(block, used);
    removeFreeBlock(block);

    block->isFree = false;
    block->requestedSize = size;
    _totalAllocatedBytes += block->size;

    return data;
}

void* DynamicHeapAllocator::reallocate(void* ptr, ssz newSize, ssz alignment)
{
    if (!ptr) return allocate(newSize, alignment);

    if (newSize <= 0)
    {
        deallocate(ptr);
        return nullptr;
    }

    const ssz eff_align = normalizeAlignment(alignment);
    unsigned char* data = static_cast<unsigned char*>(ptr);
    BlockHeader* block = *reinterpret_cast<BlockHeader**>(data - sizeof(void*));

    const ssz overhead = static_cast<ssz>(data - reinterpret_cast<unsigned char*>(block));
    const ssz required = overhead + alignUp(newSize, eff_align);

    if (block->size >= required)
    {
        const ssz old_size = block->size;
        splitBlock(block, required);
        _totalAllocatedBytes -= (old_size - block->size);
        block->requestedSize = newSize;
        return ptr;
    }

    void* newPtr = allocate(newSize, eff_align);
    if (!newPtr) return nullptr;

    ssz copySize = (newSize < block->requestedSize) ? newSize : block->requestedSize;
    if (copySize > 0) seal::mem_copy(newPtr, ptr, copySize);

    deallocate(ptr);
    return newPtr;
}

void DynamicHeapAllocator::deallocate(void* ptr)
{
    if (!ptr) return;

    unsigned char* data = static_cast<unsigned char*>(ptr);
    BlockHeader* block = *reinterpret_cast<BlockHeader**>(data - sizeof(void*));

    if (block->isFree) return;

    block->isFree = true;
    block->requestedSize = 0;
    _totalAllocatedBytes -= block->size;

    insertFreeBlock(block);
    coalesce(block);
}

DynamicHeapAllocator::BlockHeader* DynamicHeapAllocator::findBestFit(ssz size)
{
    BlockHeader* best = nullptr;
    BlockHeader* curr = _freeListHead;

    while (curr)
    {
        if (curr->size >= size)
        {
            if (!best || curr->size < best->size)
            {
                best = curr;
            }
        }
        curr = curr->nextFree;
    }
    return best;
}

void DynamicHeapAllocator::splitBlock(BlockHeader* block, ssz size)
{
    ssz min_split_size = HEADER_SIZE + 8;

    if (block->size >= size + min_split_size)
    {
        unsigned char* block_ptr = reinterpret_cast<unsigned char*>(block);
        BlockHeader* new_block = reinterpret_cast<BlockHeader*>(block_ptr + size);

        new_block->size = block->size - size;
        new_block->requestedSize = 0;
        new_block->isFree = true;

        new_block->next = block->next;
        new_block->prev = block;
        if (new_block->next) new_block->next->prev = new_block;

        block->next = new_block;
        block->size = size;

        insertFreeBlock(new_block);
    }
}

void DynamicHeapAllocator::coalesce(BlockHeader* block)
{
    if (block->next && block->next->isFree)
    {
        BlockHeader* next_block = block->next;
        removeFreeBlock(next_block);

        block->size += next_block->size;
        block->next = next_block->next;
        if (block->next) block->next->prev = block;
    }

    if (block->prev && block->prev->isFree)
    {
        BlockHeader* prev_block = block->prev;
        removeFreeBlock(block);

        prev_block->size += block->size;
        prev_block->next = block->next;
        if (prev_block->next) prev_block->next->prev = prev_block;
    }
}

void DynamicHeapAllocator::insertFreeBlock(BlockHeader* block)
{
    block->nextFree = _freeListHead;
    block->prevFree = nullptr;
    if (_freeListHead)
    {
        _freeListHead->prevFree = block;
    }
    _freeListHead = block;
}

void DynamicHeapAllocator::removeFreeBlock(BlockHeader* block)
{
    if (block->prevFree)
        block->prevFree->nextFree = block->nextFree;
    else
        _freeListHead = block->nextFree;

    if (block->nextFree) block->nextFree->prevFree = block->prevFree;

    block->prevFree = nullptr;
    block->nextFree = nullptr;
}
