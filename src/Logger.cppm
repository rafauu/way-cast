module;

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

export module Logger;

import std;

namespace
{
    inline quill::Logger* g_logger = nullptr;
}

export class Logger
{
public:
    enum class Level
    {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Critical
    };

    Logger() = delete;

    class Session
    {
    public:
        Session()
        {
            quill::Backend::start();

            auto sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>(
                "waycast_console"
            );

            g_logger = quill::Frontend::create_or_get_logger(
                "waycast",
                {std::move(sink)}
            );

            g_logger->set_log_level(quill::LogLevel::Debug);

            Logger::info("Logger initialized");
        }

        ~Session()
        {
            if (g_logger)
            {
                Logger::info("Logger shutting down");
            }

            quill::Backend::stop();
            g_logger = nullptr;
        }

        Session(const Session&) = delete;
        Session& operator=(const Session&) = delete;
        Session(Session&&) = delete;
        Session& operator=(Session&&) = delete;
    };

    template<typename... Args>
    static void trace(std::format_string<Args...> fmt, Args&&... args)
    {
        log(Level::Trace, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void debug(std::format_string<Args...> fmt, Args&&... args)
    {
        log(Level::Debug, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void info(std::format_string<Args...> fmt, Args&&... args)
    {
        log(Level::Info, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void warning(std::format_string<Args...> fmt, Args&&... args)
    {
        log(Level::Warning, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void error(std::format_string<Args...> fmt, Args&&... args)
    {
        log(Level::Error, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void critical(std::format_string<Args...> fmt, Args&&... args)
    {
        log(Level::Critical, std::format(fmt, std::forward<Args>(args)...));
    }

private:
    static void log(Level Level, std::string message)
    {
        if (not g_logger)
        {
            return;
        }

        switch (Level)
        {
            case Level::Trace:
                LOG_TRACE_L3(g_logger, "{}", message);
                break;
            case Level::Debug:
                LOG_DEBUG(g_logger, "{}", message);
                break;
            case Level::Info:
                LOG_INFO(g_logger, "{}", message);
                break;
            case Level::Warning:
                LOG_WARNING(g_logger, "{}", message);
                break;
            case Level::Error:
                LOG_ERROR(g_logger, "{}", message);
                break;
            case Level::Critical:
                LOG_CRITICAL(g_logger, "{}", message);
                break;
        }
    }
};
