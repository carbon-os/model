#pragma once
#include "client.hpp"
#include "history.hpp"
#include "response.hpp"
#include "tools.hpp"
#include <string>
#include <vector>

namespace model {

class Chat {
public:
    explicit Chat(const Client& client);
    Chat(const Client& client, History history);

    void system(std::string prompt);

    // Register tools for every turn in this Chat
    void tools(std::vector<Tool> tools);

    Response send(const std::string& message);

    void send_stream(const std::string& message, StreamCallback cb);

    // After a tool_call response, submit one result and get the next response
    Response submit_tool_result(const std::string& tool_call_id,
                                const std::string& result);

    // When the model issued multiple tool calls, submit all results then flush
    void submit_tool_result_async(const std::string& tool_call_id,
                                  const std::string& result);
    Response flush_tool_results();

    const History& history() const { return history_; }

private:
    const Client&      client_;
    History            history_;
    std::vector<Tool>  tools_;

    // Pending tool results waiting for flush
    std::vector<std::pair<std::string, std::string>> pending_results_;

    void append_response(const Response& res);
};

} // namespace model