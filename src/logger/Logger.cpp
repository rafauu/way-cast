module;

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogFunctions.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

module logger;


static quill::Logger* loggerInstance = nullptr;

namespace logger
{

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

Session::Session()
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

Session::~Session()
{
    if (loggerInstance)
    {
        logger::info("Logger shutting down");
    }

    quill::Backend::stop();
    loggerInstance = nullptr;
}

}
