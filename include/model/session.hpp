#pragma once
#include "chat.hpp"
#include "client.hpp"
#include <string>

namespace model {

struct SessionOptions {
    std::string system;     // system prompt (optional)
    std::string log_file;   // path to JSONL auto-save on destruction
};

class Session {
public:
    Session(const Client& client, SessionOptions opts = {});
    ~Session();  // flushes History to log_file if set

    // No copy — session owns its lifecycle
    Session(const Session&)            = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&)                 = default;

    Response send(const std::string& message);

    const History& history() const;

private:
    Chat          chat_;
    SessionOptions opts_;
};

} // namespace model