#pragma once

#include "string.h"

namespace seal
{
    inline void formatArg(String& out, const char* val)
    {
        if (!val)
        {
            (void)out.append("(null)", 6);
            return;
        }
        (void)out.append(val, StringView(val).size());
    }

    inline void formatArg(String& out, char* val)
    {
        formatArg(out, static_cast<const char*>(val));
    }

    inline void formatArg(String& out, StringView sv)
    {
        (void)out.append(sv.data(), sv.size());
    }

    inline void formatArg(String& out, const String& s)
    {
        (void)out.append(s.c_str(), s.size());
    }

    inline void formatArg(String& out, char c)
    {
        (void)out.push_back(c);
    }

    inline void formatArg(String& out, bool b)
    {
        (void)out.append(b ? "true" : "false", b ? 4 : 5);
    }

    inline void formatArg(String& out, long long val)
    {
        if (val == 0)
        {
            (void)out.push_back('0');
            return;
        }

        char buf[32];
        usize i = 0;
        bool is_neg = false;

        unsigned long long uval = 0;
        if (val < 0)
        {
            is_neg = true;
            uval = 0ULL - static_cast<unsigned long long>(val);
        }
        else
            uval = static_cast<unsigned long long>(val);

        while (uval > 0)
        {
            buf[i++] = '0' + static_cast<char>(uval % 10);
            uval /= 10;
        }
        if (is_neg) buf[i++] = '-';

        for (usize j = 0; j < i / 2; ++j)
        {
            char tmp = buf[j];
            buf[j] = buf[i - 1 - j];
            buf[i - 1 - j] = tmp;
        }
        (void)out.append(buf, i);
    }

    inline void formatArg(String& out, unsigned long long val)
    {
        if (val == 0)
        {
            (void)out.push_back('0');
            return;
        }

        char buf[32];
        usize i = 0;

        while (val > 0)
        {
            buf[i++] = '0' + static_cast<char>(val % 10);
            val /= 10;
        }

        for (usize j = 0; j < i / 2; ++j)
        {
            char tmp = buf[j];
            buf[j] = buf[i - 1 - j];
            buf[i - 1 - j] = tmp;
        }
        (void)out.append(buf, i);
    }

    inline void formatArg(String& out, int val)
    {
        formatArg(out, static_cast<long long>(val));
    }

    inline void formatArg(String& out, unsigned int val)
    {
        formatArg(out, static_cast<unsigned long long>(val));
    }

    inline void formatArg(String& out, long val)
    {
        formatArg(out, static_cast<long long>(val));
    }

    inline void formatArg(String& out, unsigned long val)
    {
        formatArg(out, static_cast<unsigned long long>(val));
    }

    inline void formatArg(String& out, double val)
    {
        constexpr double abs_max = 1.7976931348623157e308;

        if (val != val)
        {
            (void)out.append("nan", 3);
            return;
        }
        if (val > abs_max)
        {
            (void)out.append("inf", 3);
            return;
        }
        if (val < -abs_max)
        {
            (void)out.append("-inf", 4);
            return;
        }

        unsigned long long bits = 0;
        seal::memcpy(&bits, &val, sizeof(bits));
        const bool is_neg = (bits >> 63) != 0;
        if (is_neg)
        {
            (void)out.push_back('-');
            val = -val;
        }

        constexpr double exact_limit = 9007199254740992.0;
        if (val < exact_limit)
        {
            unsigned long long ip = static_cast<unsigned long long>(val);
            unsigned long long fr = static_cast<unsigned long long>((val - static_cast<double>(ip)) * 1000000.0 + 0.5);
            if (fr >= 1000000ULL)
            {
                fr = 0;
                ip += 1;
            }

            char fbuf[6];
            for (int k = 5; k >= 0; --k)
            {
                fbuf[k] = static_cast<char>('0' + (fr % 10));
                fr /= 10;
            }

            formatArg(out, ip);
            (void)out.push_back('.');
            (void)out.append(fbuf, 6);
            return;
        }

        double ipd = val;
        if (ipd >= 1.0)
        {
            double scale = 1.0;
            while (ipd / scale >= 10.0)
                scale *= 10.0;

            while (scale >= 1.0)
            {
                int d = static_cast<int>(ipd / scale);
                if (d > 9) d = 9;
                if (d < 0) d = 0;
                (void)out.push_back(static_cast<char>('0' + d));
                ipd -= static_cast<double>(d) * scale;
                scale /= 10.0;
            }
        }
        else
        {
            (void)out.push_back('0');
        }

        (void)out.push_back('.');
        (void)out.append("000000", 6);
    }

    inline void formatArg(String& out, long double val)
    {
        formatArg(out, static_cast<double>(val));
    }

    inline void formatArg(String& out, const void* val)
    {
        if (!val)
        {
            (void)out.append("nullptr", 7);
            return;
        }

        (void)out.append("0x", 2);
        sealptr v = reinterpret_cast<sealptr>(val);
        char buf[2 * sizeof(sealptr)];
        usize i = 0;
        do
        {
            const unsigned d = static_cast<unsigned>(v & 0xF);
            buf[i++] = (d < 10) ? static_cast<char>('0' + d) : static_cast<char>('a' + (d - 10));
            v >>= 4;
        } while (v);

        while (i)
            (void)out.push_back(buf[--i]);
    }

    inline void formatArg(String& out, void* val)
    {
        formatArg(out, static_cast<const void*>(val));
    }

    inline void formatArg(String& out, decltype(nullptr))
    {
        formatArg(out, static_cast<const void*>(nullptr));
    }

    inline void formatImpl(String& out, StringView fmt)
    {
        (void)out.append(fmt.data(), fmt.size());
    }

    template <typename T, typename... Args>
    inline void formatImpl(String& out, StringView fmt, const T& first, const Args&... rest)
    {
        usize pos = fmt.find("{}");

        if (pos == StringView::npos)
        {
            (void)out.append(fmt.data(), fmt.size());
            return;
        }

        (void)out.append(fmt.data(), pos);

        formatArg(out, first);
        formatImpl(out, fmt.substr(pos + 2), rest...);
    }

    template <typename... Args> inline String format(IAllocator* alloc, StringView fmt, const Args&... args)
    {
        String out(alloc);
        formatImpl(out, fmt, args...);
        return out;
    }

    template <typename... Args> inline String format(StringView fmt, const Args&... args)
    {
        return format(nullptr, fmt, args...);
    }
} // namespace seal
