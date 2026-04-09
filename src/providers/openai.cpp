#include "openai.hpp"
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;

namespace model::detail {

// ── constructor ─────────────────────────────────────────────────────────────

OpenAIProvider::OpenAIProvider(std::string model,
                               std::string api_key,
                               model::Config cfg)
    : model_  (std::move(model))
    , api_key_(std::move(api_key))
    , cfg_    (cfg)
    , http_   (fetch::ClientOptions{
          .base_url = "https://api.openai.com",
          .headers  = {
              {"Authorization", "Bearer " + api_key_},
              {"Content-Type",  "application/json"},
          },
          .timeout = std::chrono::milliseconds(cfg_.timeout_ms),
      })
{}

// ── helpers ──────────────────────────────────────────────────────────────────

static json messages_to_json(const History& history) {
    json arr = json::array();
    for (const auto& m : history.messages()) {
        json obj = { {"role", m.role}, {"content", m.content} };
        if (!m.tool_call_id.empty()) {
            obj["tool_call_id"] = m.tool_call_id;
        }
        arr.push_back(std::move(obj));
    }
    return arr;
}

static json tools_to_json(const std::vector<Tool>& tools) {
    json arr = json::array();
    for (const auto& t : tools) {
        arr.push_back({
            {"type", "function"},
            {"function", {
                {"name",        t.name},
                {"description", t.description},
                {"parameters",  t.parameters},
            }},
        });
    }
    return arr;
}

json OpenAIProvider::build_body(const History& history,
                                const std::vector<Tool>& tools,
                                bool do_stream) const
{
    json body = {
        {"model",       model_},
        {"messages",    messages_to_json(history)},
        {"temperature", cfg_.temperature},
        {"max_tokens",  cfg_.max_tokens},
    };
    if (!tools.empty())   body["tools"]  = tools_to_json(tools);
    if (do_stream)        body["stream"] = true;
    return body;
}

Response OpenAIProvider::parse_response(const fetch::Response& r) const {
    if (!r.ok()) {
        return Response::make_error("HTTP " + std::to_string(r.status())
                                    + ": " + r.text());
    }
    try {
        auto j = json::parse(r.text());

        Tokens tok {
            j["usage"]["prompt_tokens"]    .get<int>(),
            j["usage"]["completion_tokens"].get<int>(),
            j["usage"]["total_tokens"]     .get<int>(),
        };

        auto& choice  = j["choices"][0];
        auto& message = choice["message"];
        std::string finish = choice["finish_reason"].get<std::string>();

        Response res(
            message.value("content", ""),
            finish,
            j["model"].get<std::string>(),
            tok
        );

        // Tool calls
        if (message.contains("tool_calls")) {
            std::vector<ToolCall> calls;
            for (auto& tc : message["tool_calls"]) {
                calls.push_back({
                    tc["id"].get<std::string>(),
                    tc["function"]["name"].get<std::string>(),
                    json::parse(tc["function"]["arguments"].get<std::string>()),
                });
            }
            res.set_tool_calls(std::move(calls));
        }
        return res;

    } catch (const std::exception& e) {
        return Response::make_error(std::string("parse error: ") + e.what());
    }
}

// ── public interface ─────────────────────────────────────────────────────────

Response OpenAIProvider::chat(const History& history,
                               const std::vector<Tool>& tools)
{
    auto r = http_.post("/v1/chat/completions", build_body(history, tools).dump());
    return parse_response(r);
}

void OpenAIProvider::stream(const History& history,
                             const std::vector<Tool>& tools,
                             StreamCallback cb)
{
    std::string body = build_body(history, tools, /*stream=*/true).dump();
    http_.sse("/v1/chat/completions",
        [&cb](const fetch::ServerEvent& ev) -> bool {
            if (ev.data == "[DONE]") return false;
            try {
                auto j = json::parse(ev.data);
                auto delta = j["choices"][0]["delta"];
                if (delta.contains("content") && !delta["content"].is_null()) {
                    cb(delta["content"].get<std::string>());
                }
            } catch (...) {}
            return true;
        },
        fetch::Options{
            .method  = "POST",
            .headers = {{"Content-Type", "application/json"}},
            .body    = std::move(body),
        }
    );
}

} // namespace model::detail