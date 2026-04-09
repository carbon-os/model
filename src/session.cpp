#include "model/session.hpp"

namespace model {

Session::Session(const Client& client, SessionOptions opts)
    : chat_(client)
    , opts_(std::move(opts))
{
    if (!opts_.system.empty()) chat_.system(opts_.system);
}

Session::~Session() {
    if (!opts_.log_file.empty()) {
        try { chat_.history().save(opts_.log_file); }
        catch (...) {}    // destructor must not throw
    }
}

Response Session::send(const std::string& message) {
    return chat_.send(message);
}

const History& Session::history() const {
    return chat_.history();
}

} // namespace model