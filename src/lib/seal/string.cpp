#include "string.h"

using namespace seal;

/*
    shared alloc
*/
IAllocator* g_strAllocator;
void seal::setStringAllocator(IAllocator* alloc) noexcept
{
    g_strAllocator = alloc;
}

/*
    string impl
*/
String::String() noexcept
{
    repr_.sso[0] = '\0';
}

String::String(const char* s) noexcept : String(s, s ? seal::strlen(s) : 0) {}

String::String(const char* s, usize len) noexcept
{
    repr_.sso[0] = '\0';
    if (s && len) (void)append(s, len);
}

String::String(const String& other) noexcept
{
    repr_.sso[0] = '\0';
    (void)append(other.c_str(), other.len_);
}

String::String(String&& other) noexcept : len_(other.len_), is_heap_(other.is_heap_)
{
    if (is_heap_)
        repr_.heap = other.repr_.heap;
    else
        memcpy(repr_.sso, other.repr_.sso, len_ + 1);

    other.len_ = 0;
    other.is_heap_ = false;
    other.repr_.sso[0] = '\0';
}

String& String::operator=(const String& other) noexcept
{
    if (this == &other) return *this;

    clear();
    (void)append(other.c_str(), other.len_);

    return *this;
}

String& String::operator=(String&& other) noexcept
{
    if (this == &other) return *this;

    free_heap();

    len_ = other.len_;
    is_heap_ = other.is_heap_;

    if (is_heap_)
        repr_.heap = other.repr_.heap;
    else
        memcpy(repr_.sso, other.repr_.sso, len_ + 1);

    other.len_ = 0;
    other.is_heap_ = false;
    other.repr_.sso[0] = '\0';

    return *this;
}

String::~String() noexcept
{
    free_heap();
}

void String::free_heap() noexcept
{
    if (is_heap_ && repr_.heap.data) g_strAllocator->deallocate(repr_.heap.data);
}

bool String::grow_to(usize needed) noexcept
{
    if (needed <= capacity()) return true;
    if (!g_strAllocator) return false;

    usize new_cap = capacity() ? capacity() * 2 : 32;
    if (new_cap < needed) new_cap = needed;

    char* new_buf = static_cast<char*>(g_strAllocator->allocate(new_cap + 1, 1));
    if (!new_buf) return false;

    memcpy(new_buf, c_str(), len_ + 1); // includes null terminator
    free_heap();

    repr_.heap.data = new_buf;
    repr_.heap.cap = new_cap;
    is_heap_ = true;
    return true;
}

bool String::reserve(usize new_cap) noexcept
{
    return grow_to(new_cap);
}

bool String::resize(usize new_len, char fill) noexcept
{
    if (new_len > len_)
    {
        if (!grow_to(new_len)) return false;
        char* buf = data_mut();
        for (usize i = len_; i < new_len; ++i)
            buf[i] = fill;
    }

    len_ = new_len;
    data_mut()[len_] = '\0';
    return true;
}

bool String::append(const char* s, usize len) noexcept
{
    if (len == 0) return true;
    usize needed = len_ + len;
    if (!grow_to(needed)) return false;
    char* buf = data_mut();
    memcpy(buf + len_, s, len);
    len_ = needed;
    buf[len_] = '\0';
    return true;
}

bool String::append(const String& other) noexcept
{
    return append(other.c_str(), other.len_);
}

bool String::push_back(char c) noexcept
{
    return append(&c, 1);
}

void String::pop_back() noexcept
{
    if (len_ == 0) return;
    --len_;
    data_mut()[len_] = '\0';
}

void String::clear() noexcept
{
    len_ = 0;
    data_mut()[0] = '\0';
}

bool String::at(usize idx, char& out) const noexcept
{
    if (idx >= len_) return false;
    out = c_str()[idx];
    return true;
}

usize String::find(const char* needle, usize start) const noexcept
{
    if (!needle) return npos;

    usize nlen = seal::strlen(needle);

    if (nlen == 0) return start <= len_ ? start : npos;
    if (start >= len_) return npos;

    const char* hay = c_str();

    for (usize i = start; i + nlen <= len_; ++i)
        if (memcmp(hay + i, needle, nlen) == 0) return i;

    return npos;
}

String String::substr(usize pos, usize len) const noexcept
{
    if (pos > len_) pos = len_;

    usize avail = len_ - pos;
    usize take = (len == npos || len > avail) ? avail : len;

    return String(c_str() + pos, take);
}

int String::compare(const String& other) const noexcept
{
    usize min_len = len_ < other.len_ ? len_ : other.len_;
    int r = min_len ? memcmp(c_str(), other.c_str(), min_len) : 0;

    if (r != 0) return r;
    if (len_ < other.len_) return -1;
    if (len_ > other.len_) return 1;

    return 0;
}

void String::swap(String& other) noexcept
{
    Repr tmp_repr = repr_;
    usize tmp_len = len_;
    bool tmp_heap = is_heap_;

    repr_ = other.repr_;
    len_ = other.len_;
    is_heap_ = other.is_heap_;

    other.repr_ = tmp_repr;
    other.len_ = tmp_len;
    other.is_heap_ = tmp_heap;
}

namespace seal
{
    bool operator==(const String& a, const String& b) noexcept
    {
        return a.compare(b) == 0;
    }

    bool operator!=(const String& a, const String& b) noexcept
    {
        return a.compare(b) != 0;
    }

    bool operator<(const String& a, const String& b) noexcept
    {
        return a.compare(b) < 0;
    }
} // namespace seal
