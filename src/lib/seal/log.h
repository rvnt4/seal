#pragma once

#include "fmt.h"
#include "vector.h"

namespace seal
{
    enum class LogType
    {
        VERBOSE = 0,
        INFO,
        WARNING,
        ERROR
    };

    inline constexpr usize logTypeCount = 4;

    inline constexpr StringView logTypeNames[]{"VERBOSE", "INFO", "WARNING", "ERROR"};
    inline constexpr StringView logTypeColors[]{"\x1b[34;40m", "\x1b[32;40m", "\x1b[30;43m", "\x1b[97;41m"};

    inline constexpr StringView getLogTypeName(LogType type)
    {
        const usize idx = static_cast<usize>(type);
        return idx < logTypeCount ? logTypeNames[idx] : StringView("UNKNOWN");
    }

    inline constexpr StringView getLogTypeColor(LogType type)
    {
        const usize idx = static_cast<usize>(type);
        return idx < logTypeCount ? logTypeColors[idx] : StringView("\x1b[0m");
    }

    class ILogSink
    {
        public:
            virtual ~ILogSink() = default;
            virtual void receiveLog(LogType type, StringView loggerName, StringView message) = 0;
    };

    class Logger
    {
        public:
            explicit Logger(StringView name, IAllocator* alloc = nullptr)
                : _name(name.data(), name.size(), alloc), _sinks(alloc), _alloc(alloc)
            {
            }

            template <typename... Args> void verbose(StringView fmt, const Args&... args)
            {
                write(LogType::VERBOSE, format(_alloc, fmt, args...));
            }

            template <typename... Args> void info(StringView fmt, const Args&... args)
            {
                write(LogType::INFO, format(_alloc, fmt, args...));
            }

            template <typename... Args> void warning(StringView fmt, const Args&... args)
            {
                write(LogType::WARNING, format(_alloc, fmt, args...));
            }

            template <typename... Args> void error(StringView fmt, const Args&... args)
            {
                write(LogType::ERROR, format(_alloc, fmt, args...));
            }

            [[nodiscard]] StringView getName() const { return _name; }

            [[nodiscard]] bool addSink(SharedPtr<ILogSink> sink) { return _sinks.push_back(static_cast<SharedPtr<ILogSink>&&>(sink)); }

            [[nodiscard]] usize sinkCount() const { return _sinks.size(); }

        protected:
            void write(LogType type, const String& message)
            {
                for (usize i = 0; i < _sinks.size(); ++i)
                {
                    if (_sinks[i])
                    {
                        _sinks[i]->receiveLog(type, _name, StringView(message));
                    }
                }
            }

        private:
            String _name;
            Vector<SharedPtr<ILogSink>> _sinks;
            IAllocator* _alloc;
    };
} // namespace seal
