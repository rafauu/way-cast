module;

#include <memory>
#include <string>

export module HttpWebServer;

import GstPipelineManager;
import std;

namespace web {

export class HttpWebServer {
public:
    HttpWebServer(std::string address, unsigned short port, std::shared_ptr<GstPipelineManager> pipeline_mgr);
    ~HttpWebServer();

    HttpWebServer(const HttpWebServer&) = delete;
    HttpWebServer& operator=(const HttpWebServer&) = delete;
    HttpWebServer(HttpWebServer&&) noexcept;
    HttpWebServer& operator=(HttpWebServer&&) noexcept;

    void start(std::size_t threads);
    void stop();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace web
