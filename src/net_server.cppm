module;

#include <stop_token>
#include <cstdint>

export module net_server;

import hls_engine;

export namespace HlsCast::Network
{

class Server
{
  public:
    explicit Server (HlsCast::HlsManager& hls, std::uint16_t port)
        : m_hls {hls}, m_port {port} {}

    void run (std::stop_token st);

  private:
    HlsCast::HlsManager& m_hls;
    std::uint16_t m_port;
};

}
