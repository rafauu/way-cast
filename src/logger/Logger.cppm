module;

#include <format>
#include <string>

export module logger;


namespace logger
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

void logImpl(Level level, std::string message);

export {

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
    Session();
    ~Session();

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&) = delete;
    Session& operator=(Session&&) = delete;
};

}
}
