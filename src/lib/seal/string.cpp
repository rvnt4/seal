#include "string.h"

using namespace seal;

/*
    shared alloc
*/
static IAllocator* g_strAllocator = nullptr;

void seal::setStringAllocator(IAllocator* alloc) noexcept
{
    g_strAllocator = alloc;
}

IAllocator* seal::getStringAllocator() noexcept
{
    return g_strAllocator;
}

/*
    string impl
*/
String::String() noexcept : alloc_(g_strAllocator)
{
    repr_.sso[0] = '\0';
}

String::String(IAllocator* alloc) noexcept : alloc_(alloc ? alloc : g_strAllocator)
{
    repr_.sso[0] = '\0';
}

String::String(const char* s) noexcept : String(s, s ? seal::str_len(s) : 0) {}

String::String(const char* s, usz len) noexcept : alloc_(g_strAllocator)
{
    repr_.sso[0] = '\0';
    if (s && len) (void)append(s, len);
}

String::String(const char* s, usz len, IAllocator* alloc) noexcept : alloc_(alloc ? alloc : g_strAllocator)
{
    repr_.sso[0] = '\0';
    if (s && len) (void)append(s, len);
}

String::String(const String& other) noexcept : alloc_(other.alloc_ ? other.alloc_ : g_strAllocator)
{
    repr_.sso[0] = '\0';
    if (other.len_) (void)append(other.c_str(), other.len_);
}

String::String(String&& other) noexcept : alloc_(other.alloc_)
{
    len_ = other.len_;
    is_heap_ = other.is_heap_;
    if (is_heap_)
        repr_.heap = other.repr_.heap;
    else
        mem_copy(repr_.sso, other.repr_.sso, len_ + 1);

    other.len_ = 0;
    other.is_heap_ = false;
    other.repr_.sso[0] = '\0';
    other.alloc_ = g_strAllocator;
}

String& String::operator=(const String& other) noexcept
{
    if (this == &other) return *this;

    clear();
    if (!alloc_) alloc_ = other.alloc_ ? other.alloc_ : g_strAllocator;
    if (other.len_) (void)append(other.c_str(), other.len_);

    return *this;
}

String& String::operator=(String&& other) noexcept
{
    if (this == &other) return *this;

    free_heap();

    alloc_ = other.alloc_;
    len_ = other.len_;
    is_heap_ = other.is_heap_;

    if (is_heap_)
        repr_.heap = other.repr_.heap;
    else
        mem_copy(repr_.sso, other.repr_.sso, len_ + 1);

    other.len_ = 0;
    other.is_heap_ = false;
    other.repr_.sso[0] = '\0';
    other.alloc_ = g_strAllocator;

    return *this;
}

String::~String() noexcept
{
    free_heap();
}

void String::free_heap() noexcept
{
    if (is_heap_ && repr_.heap.data && alloc_) alloc_->deallocate(repr_.heap.data);
    repr_.heap.data = nullptr;
    repr_.heap.cap = 0;
    is_heap_ = false;
}

bool String::grow_to(usz needed) noexcept
{
    if (needed <= capacity()) return true;
    if (!alloc_) return false;
    if (needed > static_cast<usz>(-1) - 1) return false;

    usz new_cap = capacity() ? capacity() * 2 : 32;
    if (new_cap <= capacity()) new_cap = needed;
    if (new_cap < needed) new_cap = needed;

    char* new_buf = static_cast<char*>(alloc_->allocate(static_cast<ssz>(new_cap + 1), 1));
    if (!new_buf) return false;

    mem_copy(new_buf, c_str(), len_ + 1);
    free_heap();

    repr_.heap.data = new_buf;
    repr_.heap.cap = new_cap;
    is_heap_ = true;
    return true;
}

bool String::reserve(usz new_cap) noexcept
{
    return grow_to(new_cap);
}

bool String::resize(usz new_len, char fill) noexcept
{
    if (new_len > len_)
    {
        if (!grow_to(new_len)) return false;
        char* buf = data_mut();
        for (usz i = len_; i < new_len; ++i)
            buf[i] = fill;
    }

    len_ = new_len;
    data_mut()[len_] = '\0';
    return true;
}

bool String::append(const char* s, usz len) noexcept
{
    if (len == 0) return true;
    if (!s) return false;
    usz needed = len_ + len;
    if (needed < len_) return false;
    if (!grow_to(needed)) return false;
    char* buf = data_mut();
    mem_copy(buf + len_, s, len);
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

bool String::at(usz idx, char& out) const noexcept
{
    if (idx >= len_) return false;
    out = c_str()[idx];
    return true;
}

usz String::find(const char* needle, usz start) const noexcept
{
    if (!needle) return npos;

    usz nlen = seal::str_len(needle);

    if (nlen == 0) return start <= len_ ? start : npos;
    if (start >= len_) return npos;

    const char* hay = c_str();

    for (usz i = start; i + nlen <= len_; ++i)
        if (mem_cmp(hay + i, needle, nlen) == 0) return i;

    return npos;
}

String String::substr(usz pos, usz len) const noexcept
{
    if (pos > len_) pos = len_;

    usz avail = len_ - pos;
    usz take = (len == npos || len > avail) ? avail : len;

    return String(c_str() + pos, take, alloc_);
}

int String::compare(const String& other) const noexcept
{
    usz min_len = len_ < other.len_ ? len_ : other.len_;
    int r = min_len ? mem_cmp(c_str(), other.c_str(), min_len) : 0;

    if (r != 0) return r;
    if (len_ < other.len_) return -1;
    if (len_ > other.len_) return 1;

    return 0;
}

void String::swap(String& other) noexcept
{
    Repr tmp_repr = repr_;
    usz tmp_len = len_;
    bool tmp_heap = is_heap_;
    IAllocator* tmp_alloc = alloc_;

    repr_ = other.repr_;
    len_ = other.len_;
    is_heap_ = other.is_heap_;
    alloc_ = other.alloc_;

    other.repr_ = tmp_repr;
    other.len_ = tmp_len;
    other.is_heap_ = tmp_heap;
    other.alloc_ = tmp_alloc;
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

    String operator+(const String& a, const String& b) noexcept
    {
        String res(a.allocator());
        if (res.reserve(a.size() + b.size()))
        {
            (void)res.append(a);
            (void)res.append(b);
        }
        return res;
    }

    String operator+(const String& a, const char* b) noexcept
    {
        String res(a.allocator());
        usz blen = seal::str_len(b);
        if (res.reserve(a.size() + blen))
        {
            (void)res.append(a);
            (void)res.append(b, blen);
        }
        return res;
    }

    String operator+(const char* a, const String& b) noexcept
    {
        String res(b.allocator());
        usz alen = seal::str_len(a);
        if (res.reserve(alen + b.size()))
        {
            (void)res.append(a, alen);
            (void)res.append(b);
        }
        return res;
    }
} // namespace seal

/*
    stringview impl
*/
usz StringView::find(StringView needle, usz start) const noexcept
{
    if (needle._size == 0) return start <= _size ? start : npos;
    if (start > _size) return npos;
    if (needle._size > _size - start) return npos;

    for (usz i = start; i <= _size - needle._size; ++i)
        if (seal::mem_cmp(_data + i, needle._data, needle._size) == 0) return i;

    return npos;
}

StringView StringView::substr(usz pos, usz count) const noexcept
{
    if (pos >= _size) return {};
    usz rcount = (count < _size - pos) ? count : _size - pos;
    return StringView(_data + pos, rcount);
}
