module;

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogFunctions.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

export module logger;

import std;

namespace
{

inline quill::Logger* g_logger = nullptr;

}


export namespace logger
{

enum class Level
{
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

void logImpl(Level level, std::string message)
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

template<typename... Args>
void trace(std::format_string<Args...> fmt, Args&&... args)
{
    logImpl(Level::Trace, std::format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
void debug(std::format_string<Args...> fmt, Args&&... args)
{
    logImpl(Level::Debug, std::format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
void info(std::format_string<Args...> fmt, Args&&... args)
{
    logImpl(Level::Info, std::format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
void warning(std::format_string<Args...> fmt, Args&&... args)
{
    logImpl(Level::Warning, std::format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
void error(std::format_string<Args...> fmt, Args&&... args)
{
    logImpl(Level::Error, std::format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
void critical(std::format_string<Args...> fmt, Args&&... args)
{
    logImpl(Level::Critical, std::format(fmt, std::forward<Args>(args)...));
}

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

        logger::info("Logger initialized");
    }

    ~Session()
    {
        if (g_logger)
        {
            logger::info("Logger shutting down");
        }

        quill::Backend::stop();
        g_logger = nullptr;
    }

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&) = delete;
    Session& operator=(Session&&) = delete;
};

}
