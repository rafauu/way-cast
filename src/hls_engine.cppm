export module hls_engine;
import std;

export namespace HlsCast {
    struct Segment {
        int id;
        std::vector<std::uint8_t> data;
    };

    class HlsManager {
        std::map<int, std::vector<std::uint8_t>> m_segments;
        std::string m_playlist;
        std::mutex m_mutex;

    public:
        void add_segment(Segment seg)
        {
            std::println ("add_segment");
            std::lock_guard lock(m_mutex);
            m_segments[seg.id] = std::move(seg.data);
            if (m_segments.size() > 5) m_segments.erase(m_segments.begin());

            m_playlist = std::format(
                "#EXTM3U\n#EXT-X-VERSION:3\n#EXT-X-TARGETDURATION:2\n"
                "#EXT-X-MEDIA-SEQUENCE:{}\n",
                m_segments.empty() ? 0 : m_segments.begin()->first);

            for (auto const& [id, _] : m_segments) {
                m_playlist += std::format("#EXTINF:2.0,\nsegment_{}.ts\n", id);
            }
        }

        std::expected<std::string, bool> get_playlist() {
            std::println ("get_playlist");
            std::lock_guard lock(m_mutex);
            if (m_playlist.empty()) return std::unexpected(false);
            return m_playlist;
        }

        std::expected<std::vector<std::uint8_t>, bool> get_segment(int id) {
            std::println ("get_segment");
            std::lock_guard lock(m_mutex);
            if (!m_segments.contains(id)) return std::unexpected(false);
            return m_segments.at(id);
        }
    };
}
