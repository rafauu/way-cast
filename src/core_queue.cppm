module;
#include <readerwriterqueue/readerwriterqueue.h>
#include <vector>
#include <cstdint>

export module core_queue;

export namespace HlsCast::Core {
    using DataPacket = std::vector<std::uint8_t>;
    using FrameQueue = moodycamel::ReaderWriterQueue<DataPacket>;
}
