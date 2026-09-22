#include "vfs.h"
#include "memory.h"

namespace seal
{
    /*
        filebuffer
    */
    FileBuffer FileBuffer::fromString(StringView content, IAllocator* alloc)
    {
        FileBuffer buffer;
        if (!alloc) return buffer;

        buffer.allocator = alloc;
        buffer.size = content.size();

        const ssize request = static_cast<ssize>(buffer.size ? buffer.size : 1);
        buffer.data = static_cast<unsigned char*>(alloc->allocate(request, 1));
        if (!buffer.data)
        {
            buffer.size = 0;
            return buffer;
        }

        if (buffer.size > 0) memcpy(buffer.data, content.data(), static_cast<ssize>(buffer.size));
        return buffer;
    }

    String FileBuffer::toString() const
    {
        if (!isValid() || size == 0) return String(allocator);
        return String(reinterpret_cast<const char*>(data), size, allocator);
    }

    /*
        vfs
    */
    VirtualFileSystem::VirtualFileSystem(IAllocator* alloc)
        : _allocator(alloc ? alloc : getStringAllocator()), _mounts(_allocator)
    {
    }

    void VirtualFileSystem::mount(StringView virtualPath, SharedPtr<IFileProvider> provider, int priority)
    {
        if (!provider) return;
        String path = trimPathSeparators(normalizePath(virtualPath), _allocator);

        if (!_mounts.push_back(MountPoint(static_cast<String&&>(path), provider, priority))) return;

        usize i = _mounts.size() - 1;
        while (i > 0)
        {
            MountPoint& prev = _mounts[i - 1];
            MountPoint& curr = _mounts[i];
            const bool prev_comes_after =
                (prev.priority < curr.priority) ||
                (prev.priority == curr.priority && prev.pathDepth < curr.pathDepth);
            if (!prev_comes_after) break;

            MountPoint tmp = static_cast<MountPoint&&>(_mounts[i - 1]);
            _mounts[i - 1] = static_cast<MountPoint&&>(_mounts[i]);
            _mounts[i] = static_cast<MountPoint&&>(tmp);
            --i;
        }
    }

    bool VirtualFileSystem::unmount(StringView virtualPath)
    {
        String normalizedPath = trimPathSeparators(normalizePath(virtualPath), _allocator);

        for (usize i = 0; i < _mounts.size(); ++i)
        {
            if (_mounts[i].virtualPath == normalizedPath)
            {
                _mounts.erase(i);
                return true;
            }
        }
        return false;
    }

    bool VirtualFileSystem::unmount(IFileProvider* provider)
    {
        if (!provider) return false;

        for (usize i = 0; i < _mounts.size(); ++i)
        {
            if (_mounts[i].provider.get() == provider)
            {
                _mounts.erase(i);
                return true;
            }
        }
        return false;
    }

    void VirtualFileSystem::unmountAll()
    {
        _mounts.clear();
    }

    bool VirtualFileSystem::exists(StringView path) const
    {
        String searchPath = trimPathSeparators(normalizePath(path), _allocator);

        for (usize i = 0; i < _mounts.size(); ++i)
        {
            String relativePath;
            if (getRelativePath(_mounts[i], searchPath, relativePath))
            {
                if (_mounts[i].provider->exists(relativePath)) return true;
            }
        }
        return false;
    }

    VFSResult VirtualFileSystem::readFile(StringView path, FileBuffer& outBuffer) const
    {
        String searchPath = trimPathSeparators(normalizePath(path), _allocator);

        for (usize i = 0; i < _mounts.size(); ++i)
        {
            String relativePath;
            if (getRelativePath(_mounts[i], searchPath, relativePath))
            {
                if (_mounts[i].provider->exists(relativePath))
                    return _mounts[i].provider->readFile(relativePath, outBuffer);
            }
        }
        return VFSResult::FileNotFound;
    }

    VFSResult VirtualFileSystem::writeFile(StringView path, const FileBuffer& buffer)
    {
        if (!buffer.isValid()) return VFSResult::Unknown;

        String relativePath;
        MountPoint* mount = findBestMount(path, relativePath);

        if (!mount) return VFSResult::InvalidPath;
        return mount->provider->writeFile(relativePath, buffer);
    }

    VFSResult VirtualFileSystem::deleteFile(StringView path)
    {
        String searchPath = trimPathSeparators(normalizePath(path), _allocator);

        for (usize i = 0; i < _mounts.size(); ++i)
        {
            String relativePath;
            if (getRelativePath(_mounts[i], searchPath, relativePath))
            {
                if (_mounts[i].provider->exists(relativePath))
                    return _mounts[i].provider->deleteFile(relativePath);
            }
        }
        return VFSResult::FileNotFound;
    }

    VFSResult VirtualFileSystem::createDirectory(StringView path)
    {
        String relativePath;
        MountPoint* mount = findBestMount(path, relativePath);

        if (!mount) return VFSResult::InvalidPath;
        return mount->provider->createDirectory(relativePath);
    }

    Vector<String> VirtualFileSystem::listDirectory(StringView path) const
    {
        String searchPath = trimPathSeparators(normalizePath(path), _allocator);

        Vector<String> mergedFiles(_allocator);

        for (usize i = 0; i < _mounts.size(); ++i)
        {
            String relativePath;
            if (getRelativePath(_mounts[i], searchPath, relativePath))
            {
                Vector<String> files = _mounts[i].provider->listDirectory(relativePath, _allocator);
                for (usize f = 0; f < files.size(); ++f)
                {
                    bool found = false;
                    for (usize m = 0; m < mergedFiles.size(); ++m)
                    {
                        if (mergedFiles[m] == files[f])
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        mergedFiles.push_back(files[f]);
                    }
                }
            }
        }

        return mergedFiles;
    }

    String VirtualFileSystem::normalizePath(StringView path) const
    {
        if (path.empty()) return String("/", 1, _allocator);

        String result(_allocator);
        for (usize i = 0; i < path.size(); ++i)
        {
            char c = path.data()[i];
            if (isPathSeparator(c))
                (void)result.push_back('/');
            else
                (void)result.push_back(c);
        }

        Vector<String> parts(_allocator);
        usize pos = 0;
        while (pos < result.size())
        {
            usize next = result.find("/", pos);
            if (next == String::npos) next = result.size();

            String part = result.substr(pos, next - pos);
            if (part == String(".."))
            {
                if (!parts.empty()) parts.pop_back();
            }
            else if (part != String(".") && !part.empty())
            {
                parts.push_back(part);
            }

            pos = next + 1;
        }

        if (parts.empty()) return String("/", 1, _allocator);

        result.clear();
        for (usize i = 0; i < parts.size(); ++i)
        {
            (void)result.push_back('/');
            (void)result.append(parts[i]);
        }

        return result;
    }

    const VirtualFileSystem::MountPoint* VirtualFileSystem::findBestMount(StringView path,
                                                                          String& relativePath) const
    {
        String searchPath = trimPathSeparators(normalizePath(path), _allocator);

        for (usize i = 0; i < _mounts.size(); ++i)
        {
            if (matchesMount(_mounts[i], searchPath, relativePath)) return &_mounts[i];
        }
        return nullptr;
    }

    VirtualFileSystem::MountPoint* VirtualFileSystem::findBestMount(StringView path, String& relativePath)
    {
        const auto* res = const_cast<const VirtualFileSystem*>(this)->findBestMount(path, relativePath);
        return const_cast<MountPoint*>(res);
    }

    bool VirtualFileSystem::isPathSeparator(char c)
    {
        return c == '/' || c == '\\';
    }

    String VirtualFileSystem::trimPathSeparators(StringView path, IAllocator* alloc)
    {
        usize start = 0;
        while (start < path.size() && isPathSeparator(path.data()[start]))
        {
            start++;
        }

        if (start == path.size()) return String("/", 1, alloc);

        usize end = path.size() - 1;
        while (end > start && isPathSeparator(path.data()[end]))
        {
            end--;
        }

        return String(path.data() + start, end - start + 1, alloc);
    }

    bool VirtualFileSystem::matchesMount(const MountPoint& mount, StringView searchPath, String& outRelative)
    {
        if (searchPath == StringView(mount.virtualPath))
        {
            outRelative = String("/");
            return true;
        }
        if (mount.virtualPath == String("/"))
        {
            outRelative = String(searchPath.data(), searchPath.size());
            return true;
        }

        const StringView mountPath(mount.virtualPath);
        if (searchPath.size() <= mountPath.size()) return false;
        if (!StringView(searchPath).starts_with(mountPath)) return false;
        if (searchPath.data()[mountPath.size()] != '/') return false; // require a path boundary

        outRelative = String(searchPath.data() + mountPath.size() + 1, searchPath.size() - mountPath.size() - 1);
        return true;
    }

    bool VirtualFileSystem::getRelativePath(const MountPoint& mount, StringView searchPath,
                                            String& outRelative) const
    {
        return matchesMount(mount, searchPath, outRelative);
    }

    /*
        mountpoint
    */
    usize VirtualFileSystem::MountPoint::countPathDepth(StringView path)
    {
        if (path == StringView("/")) return 0;
        usize depth = 1;
        for (usize i = 0; i < path.size(); ++i)
            if (isPathSeparator(path.data()[i])) depth++;
        return depth;
    }
} // namespace seal
