#include <iterator>
#include <logger.hpp>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace vkBasalt
{
    namespace
    {
        std::string_view GetFileName() noexcept
        {
            static constexpr std::string_view defaultValue{"stderr"};
            const char*                       envVar = std::getenv("VKBASALT_LOG_FILE");

            if (envVar == nullptr)
            {
                return defaultValue;
            }

            const std::string_view value{envVar};
            if (std::empty(value))
            {
                return defaultValue;
            }

            return value;
        }

        LogLevel getMinLogLevel() noexcept
        {
            constexpr static std::array<std::pair<std::string_view, LogLevel>, 6> logLevels = {{
                {"trace", LogLevel::Trace},
                {"debug", LogLevel::Debug},
                {"info", LogLevel::Info},
                {"warn", LogLevel::Warn},
                {"error", LogLevel::Error},
                {"none", LogLevel::None},
            }};

            const char* envVar = std::getenv("VKBASALT_LOG_LEVEL");
            if (envVar == nullptr || std::empty(std::string_view{envVar}))
            {
                return LogLevel::Info;
            }

            for (const auto& [str, num] : logLevels)
            {
                if (envVar == str)
                {
                    return num;
                }
            }

            return LogLevel::Info;
        }
    } // namespace

    Logger::Logger() noexcept
    try : m_minLevel{getMinLogLevel()}
    {
        if (m_minLevel != LogLevel::None)
        {
            auto filename = GetFileName();
            if (filename == "stderr")
            {
                m_outStream = std::addressof(std::cerr);
            }
            else if (filename == "stdout")
            {
                m_outStream = std::addressof(std::cout);
            }
            else
            {
                m_fileStream.open(std::data(filename));
                m_outStream = std::addressof(m_fileStream);
            }
            return;
        }
    }
    catch (const std::exception& ex)
    {
        std::ignore = std::fprintf(stderr, "exception on logger creation: %s", ex.what());
        std::exit(EXIT_FAILURE);
    }
    catch (...)
    {
        std::ignore = std::fprintf(stderr, "exception on logger creation: UNKNOWN");
        std::exit(EXIT_FAILURE);
    }

    Logger::~Logger() = default;

    void Logger::trace(const std::string& message)
    {
        s_instance.emitMsg(LogLevel::Trace, message);
    }

    void Logger::debug(const std::string& message)
    {
        s_instance.emitMsg(LogLevel::Debug, message);
    }

    void Logger::info(const std::string& message)
    {
        s_instance.emitMsg(LogLevel::Info, message);
    }

    void Logger::warn(const std::string& message)
    {
        s_instance.emitMsg(LogLevel::Warn, message);
    }

    void Logger::err(const std::string& message)
    {
        s_instance.emitMsg(LogLevel::Error, message);
    }

    void Logger::log(LogLevel level, const std::string& message)
    {
        s_instance.emitMsg(level, message);
    }

    void Logger::emitMsg(LogLevel level, const std::string& message)
    {
        if (level >= m_minLevel)
        {
            using namespace std::string_view_literals;

            const std::scoped_lock lock{m_mutex};

            constexpr static std::array s_prefixes{
                "vkBasalt trace: "sv, "vkBasalt debug: "sv, "vkBasalt info:  "sv, "vkBasalt warn:  "sv, "vkBasalt err:   "sv};

            const auto prefix = s_prefixes.at(std::to_underlying(level));

            std::stringstream stream(message);
            std::string       line;

            while (std::getline(stream, line, '\n'))
            {
                *m_outStream << prefix << line << '\n';
            }
            m_outStream->flush();
        }
    }

} // namespace vkBasalt
