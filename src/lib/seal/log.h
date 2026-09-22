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

    inline constexpr StringView logTypeNames[]{"VERBOSE", "INFO", "WARNING", "ERROR"};
    inline constexpr StringView logTypeColors[]{"\x1b[34;40m", "\x1b[32;40m", "\x1b[30;43m", "\x1b[97;41m"};

    inline constexpr StringView getLogTypeName(LogType type)
    {
        return logTypeNames[static_cast<usize>(type)];
    }

    inline constexpr StringView getLogTypeColor(LogType type)
    {
        return logTypeColors[static_cast<usize>(type)];
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
            explicit Logger(StringView name, IAllocator* alloc = nullptr) : _name(name.data(), name.size()), _sinks(alloc) {}

            template <typename... Args> void verbose(StringView fmt, const Args&... args)
            {
                write(LogType::VERBOSE, format(fmt, args...));
            }

            template <typename... Args> void info(StringView fmt, const Args&... args)
            {
                write(LogType::INFO, format(fmt, args...));
            }

            template <typename... Args> void warning(StringView fmt, const Args&... args)
            {
                write(LogType::WARNING, format(fmt, args...));
            }

            template <typename... Args> void error(StringView fmt, const Args&... args)
            {
                write(LogType::ERROR, format(fmt, args...));
            }

            [[nodiscard]] StringView getName() const { return _name; }

            void addSink(SharedPtr<ILogSink> sink) { _sinks.push_back(sink); }

        protected:
            void write(LogType type, const String& message)
            {
                for (auto* sink = _sinks.begin(); sink != _sinks.end(); ++sink)
                {
                    if (*sink)
                    {
                        (*sink)->receiveLog(type, _name, StringView(message));
                    }
                }
            }

        private:
            String _name;
            Vector<SharedPtr<ILogSink>> _sinks;
    };
} // namespace seal
