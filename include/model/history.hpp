#pragma once
#include <string>
#include <vector>

namespace model {

struct Message {
    std::string role;     // "system" | "user" | "assistant" | "tool"
    std::string content;
    std::string tool_call_id;   // non-empty for role == "tool"
};

class History {
public:
    History() = default;

    void system   (std::string content);
    void user     (std::string content);
    void assistant(std::string content);
    void tool_result(std::string tool_call_id, std::string content);

    const std::vector<Message>& messages() const { return messages_; }

    void save(const std::string& path) const;
    static History load(const std::string& path);

    void clear() { messages_.clear(); }

private:
    std::vector<Message> messages_;

    void push(std::string role, std::string content,
              std::string tool_call_id = {});
};

} // namespace model