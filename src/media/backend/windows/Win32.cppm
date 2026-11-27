module;

#include <cstdint>
#include <optional>

export module media.Backend;


namespace media
{

class Win32
{
public:
    struct SessionData
    {
        std::uint64_t monitorHandle{0};
    };

    [[nodiscard]] static std::optional<SessionData> requestScreenCapture();
};

export using Backend = Win32;

}
