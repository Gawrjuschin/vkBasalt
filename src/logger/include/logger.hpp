#ifndef LOGGER_HPP_INCLUDED
#define LOGGER_HPP_INCLUDED

#include <cstdint>
#include <iostream>
#include <mutex>
#include <memory>
#include <string>

namespace vkBasalt
{

    enum class LogLevel : uint8_t
    {
        Trace = 0,
        Debug = 1,
        Info  = 2,
        Warn  = 3,
        Error = 4,
        None  = 5,
    };

    class Logger
    {
        Logger() noexcept;

    public:
        Logger(const Logger&)            = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&)                 = delete;
        Logger& operator=(Logger&&)      = delete;
        ~Logger();

        static void trace(const std::string& message);
        static void debug(const std::string& message);
        static void info(const std::string& message);
        static void warn(const std::string& message);
        static void err(const std::string& message);
        static void log(LogLevel level, const std::string& message);

        static LogLevel logLevel()
        {
            return s_instance.m_minLevel;
        }

    private:
        static Logger s_instance;

        const LogLevel m_minLevel;

        std::mutex m_mutex;

        std::unique_ptr<std::ostream, void (*)(std::ostream*)> m_outStream{nullptr, nullptr};

        void emitMsg(LogLevel level, const std::string& message);
    };

} // namespace vkBasalt

#endif // LOGGER_HPP_INCLUDED
