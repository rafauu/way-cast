module;

#include <cstdint>
#include <optional>

export module media.Win32;


export namespace media
{

class Win32
{
public:
    struct SessionData
    {
        std::uint64_t monitorHandle{0};
    };

    static std::optional<SessionData> requestScreenCapture();
};

}
