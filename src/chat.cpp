#include "model/chat.hpp"

namespace model {

Chat::Chat(const Client& client)
    : client_(client)
{}

Chat::Chat(const Client& client, History history)
    : client_(client)
    , history_(std::move(history))
{}

void Chat::system(std::string prompt) {
    history_.system(std::move(prompt));
}

void Chat::tools(std::vector<Tool> tools) {
    tools_ = std::move(tools);
}

void Chat::append_response(const Response& res) {
    if (res.ok() && !res.text().empty()) {
        history_.assistant(res.text());
    }
}

Response Chat::send(const std::string& message) {
    history_.user(message);
    auto res = client_.chat(history_, tools_);
    append_response(res);
    return res;
}

void Chat::send_stream(const std::string& message, StreamCallback cb) {
    history_.user(message);
    std::string accumulated;
    client_.stream(
        history_,
        [&](std::string_view chunk) {
            accumulated += chunk;
            cb(chunk);
        },
        tools_
    );
    if (!accumulated.empty()) history_.assistant(std::move(accumulated));
}

Response Chat::submit_tool_result(const std::string& tool_call_id,
                                   const std::string& result)
{
    auto res = client_.submit_tool_result(history_, tool_call_id, result, tools_);
    append_response(res);
    return res;
}

void Chat::submit_tool_result_async(const std::string& tool_call_id,
                                     const std::string& result)
{
    pending_results_.emplace_back(tool_call_id, result);
}

Response Chat::flush_tool_results() {
    for (auto& [id, result] : pending_results_) {
        history_.tool_result(id, result);
    }
    pending_results_.clear();
    auto res = client_.chat(history_, tools_);
    append_response(res);
    return res;
}

} // namespace model