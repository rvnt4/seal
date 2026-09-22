#include "memory.h"

using namespace seal;

/*
    os helpers
*/
#if defined(_WIN32)
extern "C"
{
    __declspec(dllimport) void* __stdcall VirtualAlloc(void* lpAddress, ssize dwSize, unsigned long flAllocationType,
                                                       unsigned long flProtect);
    __declspec(dllimport) int __stdcall VirtualFree(void* lpAddress, ssize dwSize, unsigned long dwFreeType);
}
#elif defined(__unix__) || defined(__APPLE__) || defined(__linux__)
    #include <sys/mman.h>
#endif

inline void* allocatePages(ssize bytes)
{
#if defined(_WIN32)
    // MEM_COMMIT | MEM_RESERVE = 0x1000 | 0x2000, PAGE_READWRITE = 0x04
    return VirtualAlloc(nullptr, bytes, 0x3000, 0x04);
#elif defined(__unix__) || defined(__APPLE__) || defined(__linux__)
    void* ptr = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (ptr == MAP_FAILED) ? nullptr : ptr;
#else
    return nullptr;
#endif
}

inline void freePages(void* ptr, ssize bytes)
{
    if (!ptr) return;
#if defined(_WIN32)
    (void)bytes;
    // MEM_RELEASE = 0x8000
    VirtualFree(ptr, 0, 0x8000);
#elif defined(__unix__) || defined(__APPLE__) || defined(__linux__)
    munmap(ptr, bytes);
#endif
}

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

void* DynamicHeapAllocator::allocate(ssize size, ssize alignment)
{
    if (size == 0) return nullptr;

    ssize total_size = HEADER_SIZE + alignUp(size, alignment);

    BlockHeader* block = findBestFit(total_size);

    if (!block)
    {
        ssize req_size = total_size + sizeof(PageChunk);
        if (req_size < MIN_CHUNK_SIZE) req_size = MIN_CHUNK_SIZE;

        void* raw = allocatePages(req_size);
        if (!raw) return nullptr; // out of mem

        PageChunk* chunk = static_cast<PageChunk*>(raw);
        chunk->size = req_size;
        chunk->next = _chunkListHead;
        _chunkListHead = chunk;
        _totalCapacityBytes += req_size;

        unsigned char* block_start = static_cast<unsigned char*>(raw) + sizeof(PageChunk);
        block = reinterpret_cast<BlockHeader*>(block_start);
        block->size = req_size - sizeof(PageChunk);
        block->requestedSize = 0;
        block->isFree = true;
        block->next = nullptr;
        block->prev = nullptr;
        block->nextFree = nullptr;
        block->prevFree = nullptr;

        insertFreeBlock(block);
    }

    splitBlock(block, total_size);
    removeFreeBlock(block);

    block->isFree = false;
    block->requestedSize = size;
    _totalAllocatedBytes += block->size;

    unsigned char* block_ptr = reinterpret_cast<unsigned char*>(block);
    return reinterpret_cast<void*>(block_ptr + HEADER_SIZE);
}

void* DynamicHeapAllocator::reallocate(void* ptr, ssize newSize, ssize alignment)
{
    if (!ptr) return allocate(newSize, alignment);

    if (newSize == 0)
    {
        deallocate(ptr);
        return nullptr;
    }

    unsigned char* p = static_cast<unsigned char*>(ptr);
    BlockHeader* block = reinterpret_cast<BlockHeader*>(p - HEADER_SIZE);

    ssize required_size = HEADER_SIZE + alignUp(newSize, alignment);

    if (block->size >= required_size)
    {
        splitBlock(block, required_size);
        block->requestedSize = newSize;
        return ptr;
    }

    void* newPtr = allocate(newSize, alignment);
    if (!newPtr) return nullptr;
    
    ssize copySize = (newSize < block->requestedSize) ? newSize : block->requestedSize;
    seal::memcpy(newPtr, ptr, copySize);

    deallocate(ptr);
    return newPtr;
}

void DynamicHeapAllocator::deallocate(void* ptr)
{
    if (!ptr) return;

    unsigned char* p = static_cast<unsigned char*>(ptr);
    BlockHeader* block = reinterpret_cast<BlockHeader*>(p - HEADER_SIZE);

    block->isFree = true;
    _totalAllocatedBytes -= block->size;

    insertFreeBlock(block);
    coalesce(block);
}

DynamicHeapAllocator::BlockHeader* DynamicHeapAllocator::findBestFit(ssize size)
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

void DynamicHeapAllocator::splitBlock(BlockHeader* block, ssize size)
{
    ssize min_split_size = HEADER_SIZE + 8;

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
