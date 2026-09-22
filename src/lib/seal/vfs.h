#pragma once

#include "string.h"
#include "vector.h"
#include "memory.h"

namespace seal
{
    struct FileBuffer
    {
            unsigned char* data = nullptr;
            usize size = 0;
            IAllocator* allocator = nullptr;

            FileBuffer() = default;
            FileBuffer(unsigned char* d, usize s, IAllocator* alloc) : data(d), size(s), allocator(alloc) {}

            FileBuffer(const FileBuffer&) = delete;
            FileBuffer& operator=(const FileBuffer&) = delete;

            FileBuffer(FileBuffer&& other) noexcept : data(other.data), size(other.size), allocator(other.allocator)
            {
                other.data = nullptr;
                other.size = 0;
            }
            
            FileBuffer& operator=(FileBuffer&& other) noexcept
            {
                if (this != &other)
                {
                    if (data && allocator) allocator->deallocate(data);
                    data = other.data;
                    size = other.size;
                    allocator = other.allocator;
                    other.data = nullptr;
                    other.size = 0;
                }
                return *this;
            }

            ~FileBuffer()
            {
                if (data && allocator) allocator->deallocate(data);
            }

            bool isValid() const { return data != nullptr; }

            static FileBuffer fromString(StringView content, IAllocator* alloc);
            String toString() const;
    };

    enum class VFSResult
    {
        Success = 0,
        FileNotFound,
        AccessDenied,
        ProviderError,
        InvalidPath,
        AlreadyExists,
        NotADirectory,
        Unknown
    };

    class IFileProvider
    {
        public:
            virtual ~IFileProvider() = default;

            virtual bool exists(StringView path) const = 0;

            virtual VFSResult readFile(StringView path, FileBuffer& outBuffer) const = 0;
            virtual VFSResult writeFile(StringView path, const FileBuffer& buffer) = 0;

            virtual VFSResult deleteFile(StringView path) = 0;
            virtual VFSResult createDirectory(StringView path) = 0;

            virtual Vector<String> listDirectory(StringView path, IAllocator* alloc) const = 0;

            virtual StringView getProviderName() const { return "UnknownProvider"; }
    };

    class VirtualFileSystem
    {
        public:
            VirtualFileSystem(IAllocator* alloc);
            ~VirtualFileSystem() = default;

            VirtualFileSystem(const VirtualFileSystem&) = delete;
            VirtualFileSystem& operator=(const VirtualFileSystem&) = delete;

            VirtualFileSystem(VirtualFileSystem&&) noexcept = default;
            VirtualFileSystem& operator=(VirtualFileSystem&&) noexcept = default;

            void mount(StringView virtualPath, SharedPtr<IFileProvider> provider, int priority = 0);
            bool unmount(StringView virtualPath);
            bool unmount(IFileProvider* provider);
            void unmountAll();

            bool exists(StringView path) const;
            VFSResult readFile(StringView path, FileBuffer& outBuffer) const;
            VFSResult writeFile(StringView path, const FileBuffer& buffer);
            VFSResult deleteFile(StringView path);

            VFSResult createDirectory(StringView path);
            Vector<String> listDirectory(StringView path) const;

            String normalizePath(StringView path) const;

        private:
            struct MountPoint
            {
                    String virtualPath;
                    SharedPtr<IFileProvider> provider;
                    int priority;
                    usize pathDepth;

                    MountPoint() = default;
                    MountPoint(String path, SharedPtr<IFileProvider> prov, int prio)
                        : virtualPath(static_cast<String&&>(path)), provider(prov), priority(prio),
                          pathDepth(countPathDepth(virtualPath))
                    {
                    }

                    static usize countPathDepth(StringView path);
            };

            IAllocator* _allocator;
            Vector<MountPoint> _mounts;

            const MountPoint* findBestMount(StringView path, String& relativePath) const;
            MountPoint* findBestMount(StringView path, String& relativePath);

            static bool isPathSeparator(char c);
            static String trimPathSeparators(StringView path, IAllocator* alloc);

            bool getRelativePath(const MountPoint& mount, StringView searchPath, String& outRelative) const;
    };

    inline StringView vfsResultToString(VFSResult result)
    {
        switch (result)
        {
            case VFSResult::Success: return "Success";
            case VFSResult::FileNotFound: return "FileNotFound";
            case VFSResult::AccessDenied: return "AccessDenied";
            case VFSResult::ProviderError: return "ProviderError";
            case VFSResult::InvalidPath: return "InvalidPath";
            case VFSResult::AlreadyExists: return "AlreadyExists";
            case VFSResult::NotADirectory: return "NotADirectory";
            default: return "Unknown";
        }
    }
} // namespace seal
