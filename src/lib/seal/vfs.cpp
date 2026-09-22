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
        buffer.data = static_cast<unsigned char*>(alloc->allocate(buffer.size));
        if (buffer.data && buffer.size > 0)
        {
            memcpy(buffer.data, content.data(), buffer.size);
        }
        return buffer;
    }

    String FileBuffer::toString() const
    {
        if (!isValid() || size == 0) return String();
        return String(reinterpret_cast<const char*>(data), size);
    }

    /*
        vfs
    */
    VirtualFileSystem::VirtualFileSystem(IAllocator* alloc) : _allocator(alloc), _mounts(alloc) {}

    void VirtualFileSystem::mount(StringView virtualPath, SharedPtr<IFileProvider> provider, int priority)
    {
        if (!provider) return;
        String path = trimPathSeparators(normalizePath(virtualPath), _allocator);

        _mounts.push_back(MountPoint(static_cast<String&&>(path), provider, priority));

        for (usize i = 1; i < _mounts.size(); ++i)
        {
            MountPoint key = static_cast<MountPoint&&>(_mounts[i]);
            usize j = i;
            while (j > 0 && (_mounts[j - 1].priority < key.priority || 
                            (_mounts[j - 1].priority == key.priority && _mounts[j - 1].pathDepth < key.pathDepth)))
            {
                _mounts[j] = static_cast<MountPoint&&>(_mounts[j - 1]);
                j--;
            }
            _mounts[j] = static_cast<MountPoint&&>(key);
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
        if (path.empty()) return String("/");

        String result;
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

        if (parts.empty()) return String("/");

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
            if (searchPath == _mounts[i].virtualPath)
            {
                relativePath = String("/");
                return &_mounts[i];
            }

            if (_mounts[i].virtualPath == String("/"))
            {
                relativePath = searchPath;
                return &_mounts[i];
            }

            String prefix = _mounts[i].virtualPath;
            (void)prefix.push_back('/');
            
            if (searchPath.size() > prefix.size() && StringView(searchPath).substr(0, prefix.size()) == StringView(prefix))
            {
                relativePath = searchPath.substr(prefix.size());
                return &_mounts[i];
            }
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
        
        if (start == path.size()) return String("/");

        usize end = path.size() - 1;
        while (end > start && isPathSeparator(path.data()[end]))
        {
            end--;
        }
        
        return String(path.data() + start, end - start + 1);
    }

    bool VirtualFileSystem::getRelativePath(const MountPoint& mount, StringView searchPath,
                                            String& outRelative) const
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
        String prefix = mount.virtualPath;
        (void)prefix.push_back('/');
        if (searchPath.size() >= prefix.size() && searchPath.substr(0, prefix.size()) == StringView(prefix))
        {
            outRelative = String(searchPath.data() + prefix.size(), searchPath.size() - prefix.size());
            return true;
        }
        return false;
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
