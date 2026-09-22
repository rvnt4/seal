#pragma once

#include "string.h"

namespace seal
{
    inline void formatArg(String& out, const char* val)
    {
        if (val) (void)out.append(val, StringView(val).size());
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
        if (val < 0.0)
        {
            (void)out.push_back('-');
            val = -val;
        }
        
        long long int_part = static_cast<long long>(val);
        formatArg(out, static_cast<unsigned long long>(int_part));
        (void)out.push_back('.');
        
        double frac = val - static_cast<double>(int_part);
        for (int i = 0; i < 6; ++i)
        {
            frac *= 10.0;
            int digit = static_cast<int>(frac);
            (void)out.push_back(static_cast<char>('0' + digit));
            frac -= digit;
        }
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

    template <typename... Args> inline String format(StringView fmt, const Args&... args)
    {
        String out;
        formatImpl(out, fmt, args...);
        return out;
    }
} // namespace seal
