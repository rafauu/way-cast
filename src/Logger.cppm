module;

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogFunctions.h"
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

            g_logger = quill::Frontend::create_or_get_logger(
                "waycast",
                quill::Frontend::create_or_get_sink<quill::ConsoleSink>(
                    "waycast_console"
                ),
                quill::PatternFormatterOptions{
                    "%(time) [%(thread_id)] LOG_%(log_level:<9) %(message)"
                }
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
    static void log(Level level, std::string message)
    {
        if (not g_logger)
        {
            return;
        }

        switch (level)
        {
            case Level::Trace:
                quill::tracel3(g_logger, "{}", message);
                break;
            case Level::Debug:
                quill::debug(g_logger, "{}", message);
                break;
            case Level::Info:
                quill::info(g_logger, "{}", message);
                break;
            case Level::Warning:
                quill::warning(g_logger, "{}", message);
                break;
            case Level::Error:
                quill::error(g_logger, "{}", message);
                break;
            case Level::Critical:
                quill::critical(g_logger, "{}", message);
                break;
        }
    }
};
