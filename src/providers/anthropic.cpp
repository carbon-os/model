#include "anthropic.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace model::detail {

// ── constructor ─────────────────────────────────────────────────────────────

AnthropicProvider::AnthropicProvider(std::string model,
                                     std::string api_key,
                                     model::Config cfg)
    : model_  (std::move(model))
    , api_key_(std::move(api_key))
    , cfg_    (cfg)
    , http_   (fetch::ClientOptions{
          .base_url = "https://api.anthropic.com",
          .headers  = {
              {"x-api-key",         api_key_},
              {"anthropic-version", "2023-06-01"},
              {"Content-Type",      "application/json"},
          },
          .timeout = std::chrono::milliseconds(cfg_.timeout_ms),
      })
{}

// ── helpers ──────────────────────────────────────────────────────────────────

json AnthropicProvider::build_body(const History& history,
                                   const std::vector<Tool>& tools,
                                   bool do_stream) const
{
    // Anthropic separates system prompt from the messages array
    json messages = json::array();
    std::string system_prompt;

    for (const auto& m : history.messages()) {
        if (m.role == "system") {
            system_prompt = m.content;
            continue;
        }
        if (m.role == "tool") {
            messages.push_back({
                {"role", "user"},
                {"content", json::array({
                    {
                        {"type",          "tool_result"},
                        {"tool_use_id",   m.tool_call_id},
                        {"content",       m.content},
                    }
                })}
            });
            continue;
        }
        messages.push_back({ {"role", m.role}, {"content", m.content} });
    }

    json body = {
        {"model",      model_},
        {"messages",   messages},
        {"max_tokens", cfg_.max_tokens},
        {"temperature",cfg_.temperature},
    };

    if (!system_prompt.empty()) body["system"] = system_prompt;
    if (do_stream)              body["stream"] = true;

    if (!tools.empty()) {
        json tarr = json::array();
        for (const auto& t : tools) {
            tarr.push_back({
                {"name",         t.name},
                {"description",  t.description},
                {"input_schema", t.parameters},
            });
        }
        body["tools"] = tarr;
    }

    return body;
}

Response AnthropicProvider::parse_response(const fetch::Response& r) const {
    if (!r.ok())
        return Response::make_error("HTTP " + std::to_string(r.status())
                                    + ": " + r.text());
    try {
        auto j = json::parse(r.text());

        Tokens tok {
            j["usage"]["input_tokens"] .get<int>(),
            j["usage"]["output_tokens"].get<int>(),
            j["usage"]["input_tokens"].get<int>()
                + j["usage"]["output_tokens"].get<int>(),
        };

        std::string finish = j["stop_reason"].get<std::string>();
        std::string text;
        std::vector<ToolCall> calls;

        for (auto& block : j["content"]) {
            std::string type = block["type"];
            if (type == "text") {
                text += block["text"].get<std::string>();
            } else if (type == "tool_use") {
                calls.push_back({
                    block["id"]   .get<std::string>(),
                    block["name"] .get<std::string>(),
                    block["input"],
                });
            }
        }

        Response res(text, finish, j["model"].get<std::string>(), tok);
        if (!calls.empty()) res.set_tool_calls(std::move(calls));
        return res;

    } catch (const std::exception& e) {
        return Response::make_error(std::string("parse error: ") + e.what());
    }
}

// ── public interface ─────────────────────────────────────────────────────────

Response AnthropicProvider::chat(const History& history,
                                  const std::vector<Tool>& tools)
{
    auto r = http_.post("/v1/messages", build_body(history, tools).dump());
    return parse_response(r);
}

void AnthropicProvider::stream(const History& history,
                                const std::vector<Tool>& tools,
                                StreamCallback cb)
{
    std::string body = build_body(history, tools, /*stream=*/true).dump();

    http_.sse("/v1/messages",
        [&cb](const fetch::ServerEvent& ev) -> bool {
            if (ev.event == "message_stop") return false;
            if (ev.event != "content_block_delta") return true;
            try {
                auto j = json::parse(ev.data);
                if (j["delta"]["type"] == "text_delta") {
                    cb(j["delta"]["text"].get<std::string>());
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