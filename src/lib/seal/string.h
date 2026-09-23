#pragma once

#include "types.h"
#include "memory.h"

namespace seal
{
    void setStringAllocator(IAllocator* alloc) noexcept;
    IAllocator* getStringAllocator() noexcept;

    class String
    {
        public:
            static constexpr usize npos = static_cast<usize>(-1);
            static constexpr usize sso_capacity = 22;

            String() noexcept;
            explicit String(IAllocator* alloc) noexcept;
            String(const char* s) noexcept;
            String(const char* s, usize len) noexcept;
            String(const char* s, usize len, IAllocator* alloc) noexcept;
            String(const String& other) noexcept;
            String(String&& other) noexcept;
            String& operator=(const String& other) noexcept;
            String& operator=(String&& other) noexcept;

            ~String() noexcept;

            [[nodiscard]] bool reserve(usize new_cap) noexcept;
            [[nodiscard]] bool resize(usize new_len, char fill = '\0') noexcept;
            [[nodiscard]] bool append(const char* s, usize len) noexcept;
            [[nodiscard]] bool append(const String& other) noexcept;
            [[nodiscard]] bool push_back(char c) noexcept;
            void pop_back() noexcept;
            void clear() noexcept;

            char& operator[](usize idx) noexcept { return data_mut()[idx]; }
            const char& operator[](usize idx) const noexcept { return c_str()[idx]; }
            bool at(usize idx, char& out) const noexcept;

            const char* c_str() const noexcept { return is_heap_ ? repr_.heap.data : repr_.sso; }
            const char* data() const noexcept { return c_str(); }
            usize size() const noexcept { return len_; }
            usize length() const noexcept { return len_; }
            usize capacity() const noexcept { return is_heap_ ? repr_.heap.cap : sso_capacity; }
            bool empty() const noexcept { return len_ == 0; }
            bool is_sso() const noexcept { return !is_heap_; }
            IAllocator* allocator() const noexcept { return alloc_; }

            usize find(const char* needle, usize start = 0) const noexcept;
            String substr(usize pos, usize len = npos) const noexcept;
            int compare(const String& other) const noexcept;

            void swap(String& other) noexcept;

        private:
            union Repr {
                    struct
                    {
                            char* data;
                            usize cap;
                    } heap;
                    char sso[sso_capacity + 1];
            };
            Repr repr_;
            usize len_ = 0;
            bool is_heap_ = false;
            IAllocator* alloc_ = nullptr;

            char* data_mut() noexcept { return is_heap_ ? repr_.heap.data : repr_.sso; }
            bool grow_to(usize needed) noexcept;
            void free_heap() noexcept;
    };

    bool operator==(const String& a, const String& b) noexcept;
    bool operator!=(const String& a, const String& b) noexcept;
    bool operator<(const String& a, const String& b) noexcept;

    /*
        stringview
    */
    class StringView
    {
        public:
            static constexpr usize npos = static_cast<usize>(-1);

            constexpr StringView() noexcept = default;
            constexpr StringView(const char* s) noexcept : _data(s), _size(strLen(s)) {}
            constexpr StringView(const char* s, usize len) noexcept : _data(s), _size(len) {}
            StringView(const String& s) noexcept : _data(s.c_str()), _size(s.size()) {}

            [[nodiscard]] constexpr const char* data() const noexcept { return _data; }
            [[nodiscard]] constexpr usize size() const noexcept { return _size; }
            [[nodiscard]] constexpr bool empty() const noexcept { return _size == 0; }

            [[nodiscard]] usize find(StringView needle, usize start = 0) const noexcept;
            [[nodiscard]] StringView substr(usize pos, usize count = npos) const noexcept;

            [[nodiscard]] constexpr bool starts_with(StringView prefix) const noexcept
            {
                if (prefix._size > _size) return false;
                for (usize i = 0; i < prefix._size; ++i)
                    if (_data[i] != prefix._data[i]) return false;
                return true;
            }

            [[nodiscard]] constexpr bool ends_with(StringView suffix) const noexcept
            {
                if (suffix._size > _size) return false;
                const usize offset = _size - suffix._size;
                for (usize i = 0; i < suffix._size; ++i)
                    if (_data[offset + i] != suffix._data[i]) return false;
                return true;
            }

        private:
            static constexpr usize strLen(const char* s) noexcept
            {
                if (!s) return 0;
                usize len = 0;
                while (s[len])
                    ++len;
                return len;
            }

            const char* _data = nullptr;
            usize _size = 0;
    };

    inline constexpr bool operator==(StringView a, StringView b) noexcept
    {
        if (a.size() != b.size()) return false;
        for (usize i = 0; i < a.size(); ++i)
        {
            if (a.data()[i] != b.data()[i]) return false;
        }
        return true;
    }

    inline constexpr bool operator!=(StringView a, StringView b) noexcept
    {
        return !(a == b);
    }

    inline constexpr bool operator<(StringView a, StringView b) noexcept
    {
        const usize n = a.size() < b.size() ? a.size() : b.size();
        for (usize i = 0; i < n; ++i)
        {
            if (a.data()[i] != b.data()[i]) return a.data()[i] < b.data()[i];
        }
        return a.size() < b.size();
    }
} // namespace seal
