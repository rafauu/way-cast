export module media_processor;
import std;
import core_queue;
import hls_engine;

export namespace HlsCast::Media
{
    class Processor
    {
        Core::FrameQueue& m_queue;
        HlsManager& m_hls;
    public:
        Processor(Core::FrameQueue& q, HlsManager& h) : m_queue(q), m_hls(h) {}

        void start(std::stop_token st) {
            std::vector<std::uint8_t> buffer;
            int id = 0;
            auto last = std::chrono::steady_clock::now();

            while (not st.stop_requested())
            {
                Core::DataPacket chunk;
                if (m_queue.try_dequeue(chunk))
                {
                    buffer.insert(buffer.end(), chunk.begin(), chunk.end());
                    auto now = std::chrono::steady_clock::now();
                    if (now - last >= std::chrono::seconds(3)) {
                        m_hls.add_segment({id++, std::move(buffer)});
                        buffer.clear();
                        last = now;
                    }
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        }
    };
}
