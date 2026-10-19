module;

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogFunctions.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

export module logger;

import std;


static quill::Logger* loggerInstance = nullptr;

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
    if (not loggerInstance)
    {
        return;
    }

    switch (level)
    {
        case Level::Trace:
            quill::tracel3(loggerInstance, "{}", message);
            break;
        case Level::Debug:
            quill::debug(loggerInstance, "{}", message);
            break;
        case Level::Info:
            quill::info(loggerInstance, "{}", message);
            break;
        case Level::Warning:
            quill::warning(loggerInstance, "{}", message);
            break;
        case Level::Error:
            quill::error(loggerInstance, "{}", message);
            break;
        case Level::Critical:
            quill::critical(loggerInstance, "{}", message);
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

        loggerInstance = quill::Frontend::create_or_get_logger(
            "waycast",
            quill::Frontend::create_or_get_sink<quill::ConsoleSink>(
                "waycast_console"
            ),
            quill::PatternFormatterOptions{
                "%(time) [%(thread_id)] LOG_%(log_level:<9) %(message)"
            }
        );

        loggerInstance->set_log_level(quill::LogLevel::Debug);

        logger::info("Logger initialized");
    }

    ~Session()
    {
        if (loggerInstance)
        {
            logger::info("Logger shutting down");
        }

        quill::Backend::stop();
        loggerInstance = nullptr;
    }

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&) = delete;
    Session& operator=(Session&&) = delete;
};

}
