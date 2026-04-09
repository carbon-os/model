#pragma once
#include <nlohmann/json.hpp>
#include <cassert>
#include <string>
#include <vector>

namespace model {

struct Tokens {
    int prompt     = 0;
    int completion = 0;
    int total      = 0;
};

struct ToolCall {
    std::string    id;
    std::string    name;
    nlohmann::json arguments;
};

class Response {
public:
    // Construct a successful response
    Response(std::string text,
             std::string finish_reason,
             std::string model_name,
             Tokens      tokens)
        : text_         (std::move(text))
        , finish_reason_(std::move(finish_reason))
        , model_        (std::move(model_name))
        , tokens_       (tokens)
        , ok_           (true)
    {}

    // Construct an error response
    static Response make_error(std::string message) {
        Response r;
        r.ok_    = false;
        r.error_ = std::move(message);
        r.finish_reason_ = "error";
        return r;
    }

    bool               ok()            const { return ok_; }
    const std::string& text()          const { return text_; }
    const std::string& error()         const { return error_; }
    const std::string& finish_reason() const { return finish_reason_; }
    const std::string& model()         const { return model_; }
    Tokens             tokens()        const { return tokens_; }

    bool has_tool_call() const { return !tool_calls_.empty(); }

    const ToolCall& tool_call() const {
        assert(tool_calls_.size() == 1 && "use tool_calls() when there are multiple");
        return tool_calls_.front();
    }

    const std::vector<ToolCall>& tool_calls() const { return tool_calls_; }

    // Used internally by providers to attach tool calls
    void set_tool_calls(std::vector<ToolCall> calls) {
        tool_calls_ = std::move(calls);
    }

private:
    Response() = default;

    bool        ok_            = false;
    std::string text_;
    std::string error_;
    std::string finish_reason_;
    std::string model_;
    Tokens      tokens_;
    std::vector<ToolCall> tool_calls_;
};

} // namespace model