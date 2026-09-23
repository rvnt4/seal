#include "tests.h"

#include <seal/memory.h>
#include <seal/vfs.h>
#include <seal/string.h>

class MockProvider : public seal::IFileProvider
{
    public:
        seal::IAllocator* alloc;
        seal::String providerName;
        seal::Vector<seal::String> files;
        seal::String lastWritten;

        MockProvider(seal::StringView name, seal::IAllocator* a)
            : alloc(a), providerName(name.data(), name.size()), files(a)
        {
        }

        bool exists(seal::StringView path) const override
        {
            for (seal::usize i = 0; i < files.size(); ++i)
                if (files[i] == seal::String(path.data(), path.size())) return true;
            return false;
        }

        seal::VFSResult readFile(seal::StringView path, seal::FileBuffer& outBuffer) const override
        {
            if (!exists(path)) return seal::VFSResult::FileNotFound;
            outBuffer = seal::FileBuffer::fromString(seal::StringView("dummy_data"), alloc);
            return seal::VFSResult::Success;
        }

        seal::VFSResult writeFile(seal::StringView path, const seal::FileBuffer& buffer) override
        {
            if (!exists(path)) files.push_back(seal::String(path.data(), path.size()));
            lastWritten = buffer.toString();
            return seal::VFSResult::Success;
        }

        seal::VFSResult deleteFile(seal::StringView path) override
        {
            for (seal::usize i = 0; i < files.size(); ++i)
            {
                if (files[i] == seal::String(path.data(), path.size()))
                {
                    files.erase(i);
                    return seal::VFSResult::Success;
                }
            }
            return seal::VFSResult::FileNotFound;
        }

        seal::VFSResult createDirectory(seal::StringView /*path*/) override { return seal::VFSResult::Success; }

        seal::Vector<seal::String> listDirectory(seal::StringView /*path*/, seal::IAllocator* a) const override
        {
            seal::Vector<seal::String> res(a);
            for (seal::usize i = 0; i < files.size(); ++i)
                res.push_back(files[i]);
            return res;
        }

        seal::StringView getProviderName() const override { return seal::StringView(providerName); }
};

class VFSTest : public ITest
{
    public:
        VFSTest() : ITest() {};

        virtual void run() override
        {
            seal::DynamicHeapAllocator heapAllocator;
            seal::setStringAllocator(&heapAllocator);

            seal::VirtualFileSystem vfs(&heapAllocator);

            void* mem1 = heapAllocator.allocate(sizeof(MockProvider), alignof(MockProvider));
            MockProvider* prov1 = new (mem1, seal::placement_t{}) MockProvider("MemoryProvider", &heapAllocator);
            seal::SharedPtr<seal::IFileProvider> p1(prov1, &heapAllocator);

            void* mem2 = heapAllocator.allocate(sizeof(MockProvider), alignof(MockProvider));
            MockProvider* prov2 = new (mem2, seal::placement_t{}) MockProvider("MemoryProvider", &heapAllocator);
            seal::SharedPtr<seal::IFileProvider> p2(prov2, &heapAllocator);

            /*
                mnt test
            */
            vfs.mount(seal::StringView("scripts"), p1);
            vfs.mount(seal::StringView("config/network"), p2);

            /*
                write / read tests
            */
            logInfo("Writing to different mount points");
            vfs.writeFile(seal::StringView("scripts/main.lua"),
                          seal::FileBuffer::fromString(seal::StringView("print('hello')"), &heapAllocator));
            vfs.writeFile(seal::StringView("config/network/settings.json"),
                          seal::FileBuffer::fromString(seal::StringView("{ \"ip\": \"127.0.0.1\" }"), &heapAllocator));

            logInfo("Reading from mount points");
            seal::FileBuffer b1, b2;
            if (vfs.readFile(seal::StringView("scripts/main.lua"), b1) == seal::VFSResult::Success)
                logInfo("Read Scripts: {}", "dummy_data");

            /*
                existence and crosstalk
            */
            if (!vfs.exists(seal::StringView("scripts/main.lua"))) logError("scripts/main.lua is missing?");
            if (vfs.exists(seal::StringView("scripts/settings.json")))
                logError("wrong provider, settings.json should be inaccessible");
            logInfo("Path isolation verified");

            /*
                priority / overlapping mounts
            */
            void* mem3 = heapAllocator.allocate(sizeof(MockProvider), alignof(MockProvider));
            MockProvider* overrideProv = new (mem3, seal::placement_t{}) MockProvider("MemoryProvider", &heapAllocator);
            seal::SharedPtr<seal::IFileProvider> p3(overrideProv, &heapAllocator);

            p3->writeFile(seal::StringView("patch.txt"),
                          seal::FileBuffer::fromString(seal::StringView("Hotfix Data"), &heapAllocator));

            vfs.mount(seal::StringView("scripts"), p3, 10); // higher priority

            seal::FileBuffer b3;
            if (vfs.readFile(seal::StringView("scripts/patch.txt"), b3) == seal::VFSResult::Success)
                logInfo("Priority mount successful: {}", "dummy_data");

            /*
                directory listing
            */
            logInfo("Listing 'scripts' folder");

            auto list = vfs.listDirectory(seal::StringView("scripts"));
            for (seal::usize i = 0; i < list.size(); ++i)
                logInfo("\t* {}", list[i].c_str());

            /*
                deletion
            */
            seal::VFSResult delRes = vfs.deleteFile(seal::StringView("scripts/main.lua"));
            logInfo("Deletion result: {}", seal::vfsResultToString(delRes).data());
            if (vfs.exists(seal::StringView("scripts/main.lua")))
                logError("Failed to delete file, scrips/main.lua still accessible");

            /*
                empty file support
            */
            logInfo("Testing empty file buffer");
            seal::FileBuffer emptyBuf = seal::FileBuffer::fromString(seal::StringView(""), &heapAllocator);
            if (!emptyBuf.isValid() || emptyBuf.size != 0)
            {
                logError("empty FileBuffer should be valid with size 0");
            }

            /*
                unmounting
            */
            vfs.unmount(seal::StringView("scripts"));
            if (!vfs.exists(seal::StringView("scripts/patch.txt"))) logInfo("Unmount successful");
        }

        virtual const char* getName() override { return "VFSTest"; }
};

static VFSTest g_vfsTest;
