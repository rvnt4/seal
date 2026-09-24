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
        constexpr double absMax = 1.7976931348623157e308;

        if (val != val)
        {
            (void)out.append("nan", 3);
            return;
        }
        if (val > absMax)
        {
            (void)out.append("inf", 3);
            return;
        }
        if (val < -absMax)
        {
            (void)out.append("-inf", 4);
            return;
        }

        static_assert(sizeof(double) == sizeof(unsigned long long),
                      "Double size mismatch for IEEE 754 bitcast"); // should i keep this actually idk

        unsigned long long bits = 0;
        seal::mem_copy(&bits, &val, sizeof(bits));
        const bool isNeg = (bits >> 63) != 0;
        if (isNeg)
        {
            (void)out.push_back('-');
            val = -val;
        }

        constexpr double exactLimit = 9007199254740992.0;
        if (val < exactLimit)
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

    /*
        format specification
    */
    struct FormatSpec
    {
            char fill = ' ';
            char align = '\0'; // '<' left, '>' right, '^' center, '\0' default
            int width = 0;
            int precision = -1; // -1 = not specified
            char type = '\0';
    };

    template <typename T>
    inline void formatArgWithSpec(String& out, const T& val, const FormatSpec& /*spec*/)
    {
        formatArg(out, val);
    }

    inline void formatArgWithSpec(String& out, unsigned long long val, const FormatSpec& spec)
    {
        if (spec.type == 'x' || spec.type == 'X')
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
                const unsigned d = static_cast<unsigned>(val & 0xF);
                buf[i++] = (d < 10) ? static_cast<char>('0' + d) : 
                           (spec.type == 'X' ? static_cast<char>('A' + (d - 10)) : static_cast<char>('a' + (d - 10)));
                val >>= 4;
            }
            for (usize j = 0; j < i / 2; ++j)
            {
                char tmp = buf[j];
                buf[j] = buf[i - 1 - j];
                buf[i - 1 - j] = tmp;
            }
            (void)out.append(buf, i);
            return;
        }
        formatArg(out, val);
    }

    inline void formatArgWithSpec(String& out, long long val, const FormatSpec& spec)
    {
        if (spec.type == 'x' || spec.type == 'X')
        {
            formatArgWithSpec(out, static_cast<unsigned long long>(val), spec);
            return;
        }
        formatArg(out, val);
    }

    inline void formatArgWithSpec(String& out, int val, const FormatSpec& spec) { formatArgWithSpec(out, static_cast<long long>(val), spec); }
    inline void formatArgWithSpec(String& out, unsigned int val, const FormatSpec& spec) { formatArgWithSpec(out, static_cast<unsigned long long>(val), spec); }
    inline void formatArgWithSpec(String& out, long val, const FormatSpec& spec) { formatArgWithSpec(out, static_cast<long long>(val), spec); }
    inline void formatArgWithSpec(String& out, unsigned long val, const FormatSpec& spec) { formatArgWithSpec(out, static_cast<unsigned long long>(val), spec); }

    /*
        type erased argument wrapper for positional access
    */
    struct FormatArg
    {
            const void* data;
            void (*write)(String&, const void*, const FormatSpec&);
    };

    template <typename T> inline FormatArg makeFormatArg(const T& val)
    {
        return FormatArg{&val, [](String& out, const void* ptr, const FormatSpec& spec) { formatArgWithSpec(out, *static_cast<const T*>(ptr), spec); }};
    }

    /*
        format spec parsing helpers
    */
    inline bool fmtIsAlign(char c)
    {
        return c == '<' || c == '>' || c == '^';
    }

    inline int fmtParseDigits(const char* s, usize len, usize& pos)
    {
        int val = 0;
        while (pos < len && s[pos] >= '0' && s[pos] <= '9')
        {
            val = val * 10 + (s[pos] - '0');
            ++pos;
        }
        return val;
    }

    /*
        parse a format spec string (the part after ':' inside a replacement field)

        grammar:
            [[fill]align][0][width][.precision]
            fill        = any char except '{' or '}'
            align       = '<' | '>' | '^'
            width       = digit+
            precision   = digit+
    */
    inline FormatSpec fmtParseSpec(const char* s, usize len)
    {
        FormatSpec spec;
        usize pos = 0;

        if (len >= 2 && fmtIsAlign(s[1]))
        {
            spec.fill = s[0];
            spec.align = s[1];
            pos = 2;
        }
        else if (len >= 1 && fmtIsAlign(s[0]))
        {
            spec.align = s[0];
            pos = 1;
        }

        if (pos < len && s[pos] == '0' && spec.align == '\0' && spec.fill == ' ')
        {
            spec.fill = '0';
            spec.align = '>';
            ++pos;
        }

        if (pos < len && s[pos] >= '0' && s[pos] <= '9') spec.width = fmtParseDigits(s, len, pos);

        if (pos < len && s[pos] == '.')
        {
            ++pos;
            spec.precision = fmtParseDigits(s, len, pos);
        }

        if (pos < len)
        {
            char c = s[pos];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
            {
                spec.type = c;
                ++pos;
            }
        }

        return spec;
    }

    inline void fmtApplyPrecision(String& temp, int precision)
    {
        if (precision < 0) return;

        usize dotPos = StringView::npos;
        bool isNumber = true;

        for (usize i = 0; i < temp.size(); ++i)
        {
            if (temp[i] == '.')
                dotPos = i;
            else if (temp[i] < '0' || temp[i] > '9')
            {
                if (i == 0 && temp[i] == '-') continue;
                isNumber = false;
            }
        }

        if (dotPos != StringView::npos)
        {
            const usize fracStart = dotPos + 1;
            const usize fracLen = temp.size() - fracStart;
            usize keepEnd = (precision == 0) ? dotPos : dotPos + 1 + static_cast<usize>(precision);

            if (fracLen > static_cast<usize>(precision))
            {
                bool carry = false;
                if (keepEnd < temp.size() && temp[keepEnd] >= '5')
                {
                    carry = true;
                    for (int i = static_cast<int>(keepEnd) - 1; i >= 0 && carry; --i)
                    {
                        if (temp[i] == '.') continue;
                        if (temp[i] == '-') break;

                        if (temp[i] == '9')
                        {
                            temp[i] = '0';
                        }
                        else
                        {
                            temp[i]++;
                            carry = false;
                        }
                    }
                }

                (void)temp.resize(keepEnd);

                if (carry)
                {
                    (void)temp.push_back('0');
                    usize shiftStart = (temp[0] == '-') ? 1 : 0;
                    for (usize j = temp.size() - 1; j > shiftStart; --j)
                        temp[j] = temp[j - 1];
                    temp[shiftStart] = '1';
                }
            }
            else
            {
                const usize needed = static_cast<usize>(precision) - fracLen;
                for (usize i = 0; i < needed; ++i)
                    (void)temp.push_back('0');
            }
        }
        else if (!isNumber)
        {
            // Truncate strings, but skip integers!
            if (temp.size() > static_cast<usize>(precision)) (void)temp.resize(static_cast<usize>(precision));
        }
    }

    /*
        apply width / fill / alignment padding
    */
    inline void fmtApplyPadding(String& out, StringView content, const FormatSpec& spec)
    {
        if (spec.width <= 0 || content.size() >= static_cast<usize>(spec.width))
        {
            (void)out.append(content.data(), content.size());
            return;
        }

        usize padTotal = static_cast<usize>(spec.width) - content.size();
        const char align = (spec.align != '\0') ? spec.align : '<';

        bool is_neg_zero_pad = (content.size() > 0 && content.data()[0] == '-' && spec.fill == '0' && align == '>');
        if (is_neg_zero_pad)
        {
            (void)out.push_back('-');
            content = StringView(content.data() + 1, content.size() - 1);
        }

        if (align == '>')
        {
            for (usize i = 0; i < padTotal; ++i)
                (void)out.push_back(spec.fill);
            (void)out.append(content.data(), content.size());
        }
        else if (align == '<')
        {
            (void)out.append(content.data(), content.size());
            for (usize i = 0; i < padTotal; ++i)
                (void)out.push_back(spec.fill);
        }
        else
        {
            const usize padLeft = padTotal / 2;
            const usize padRight = padTotal - padLeft;
            for (usize i = 0; i < padLeft; ++i)
                (void)out.push_back(spec.fill);
            (void)out.append(content.data(), content.size());
            for (usize i = 0; i < padRight; ++i)
                (void)out.push_back(spec.fill);
        }
    }

    inline void formatCore(String& out, StringView fmt, const FormatArg* args, usize argCount)
    {
        const char* s = fmt.data();
        const usize len = fmt.size();
        usize i = 0;
        usize autoIdx = 0;

        while (i < len)
        {
            // escaped braces
            if (s[i] == '{' && i + 1 < len && s[i + 1] == '{')
            {
                (void)out.push_back('{');
                i += 2;
                continue;
            }
            if (s[i] == '}' && i + 1 < len && s[i + 1] == '}')
            {
                (void)out.push_back('}');
                i += 2;
                continue;
            }

            // replacement field
            if (s[i] == '{')
            {
                ++i;

                usize end = i;
                while (end < len && s[end] != '}')
                    ++end;

                if (end >= len)
                {
                    (void)out.push_back('{');
                    continue;
                }

                const char* fieldStart = s + i;
                const usize fieldLen = end - i;

                usize fpos = 0;
                usize argIdx = autoIdx;
                bool hasExplicitIndex = false;

                if (fpos < fieldLen && fieldStart[fpos] >= '0' && fieldStart[fpos] <= '9')
                {
                    usize saved = fpos;
                    int parsed = fmtParseDigits(fieldStart, fieldLen, fpos);
                    if (fpos == fieldLen || fieldStart[fpos] == ':')
                    {
                        argIdx = static_cast<usize>(parsed);
                        hasExplicitIndex = true;
                    }
                    else
                    {
                        fpos = saved;
                    }
                }

                if (!hasExplicitIndex) ++autoIdx;

                FormatSpec spec;
                if (fpos < fieldLen && fieldStart[fpos] == ':')
                {
                    ++fpos;
                    spec = fmtParseSpec(fieldStart + fpos, fieldLen - fpos);
                }

                if (argIdx < argCount)
                {
                    String temp(out.allocator());
                    args[argIdx].write(temp, args[argIdx].data, spec);
                    fmtApplyPrecision(temp, spec.precision);
                    fmtApplyPadding(out, StringView(temp), spec);
                }
                else
                {
                    (void)out.push_back('{');
                    (void)out.append(fieldStart, fieldLen);
                    (void)out.push_back('}');
                }

                i = end + 1;
                continue;
            }

            (void)out.push_back(s[i]);
            ++i;
        }
    }

    template <typename... Args> inline String format(IAllocator* alloc, StringView fmt, const Args&... args)
    {
        String out(alloc);
        if constexpr (sizeof...(Args) == 0)
        {
            formatCore(out, fmt, nullptr, 0);
        }
        else
        {
            FormatArg argArray[] = {makeFormatArg(args)...};
            formatCore(out, fmt, argArray, sizeof...(Args));
        }
        return out;
    }

    template <typename... Args> inline String format(StringView fmt, const Args&... args)
    {
        return format(nullptr, fmt, args...);
    }
} // namespace seal
