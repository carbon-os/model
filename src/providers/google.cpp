#include "google.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace model::detail {

// ── constructor ──────────────────────────────────────────────────────────────

GoogleProvider::GoogleProvider(std::string model,
                               std::string api_key,
                               model::Config cfg)
    : model_(std::move(model))
    , cfg_  (cfg)
    , http_ (fetch::ClientOptions{
          .base_url = "https://generativelanguage.googleapis.com",
          .headers  = {
              {"Content-Type",   "application/json"},
              {"x-goog-api-key", std::move(api_key)},
          },
          .timeout  = std::chrono::milliseconds(cfg_.timeout_ms),
      })
{}

// ── helpers ──────────────────────────────────────────────────────────────────

std::string GoogleProvider::chat_endpoint() const {
    return "/v1beta/models/" + model_ + ":generateContent";
}

std::string GoogleProvider::stream_endpoint() const {
    return "/v1beta/models/" + model_ + ":streamGenerateContent?alt=sse";
}

json GoogleProvider::build_body(const History& history,
                                const std::vector<Tool>& tools) const
{
    json contents     = json::array();
    json system_parts = json::array();

    for (const auto& m : history.messages()) {
        if (m.role == "system") {
            system_parts.push_back({ {"text", m.content} });
            continue;
        }

        if (m.role == "tool") {
            contents.push_back({
                {"role", "user"},
                {"parts", json::array({
                    {{"functionResponse", {
                        {"name",     m.tool_call_id},
                        {"response", { {"content", m.content} }},
                    }}}
                })}
            });
            continue;
        }

        // Gemini uses "model" not "assistant"
        std::string role = (m.role == "assistant") ? "model" : m.role;
        contents.push_back({
            {"role",  role},
            {"parts", json::array({ {{"text", m.content}} })},
        });
    }

    json body = {
        {"contents",         contents},
        {"generationConfig", {
            {"temperature",     cfg_.temperature},
            {"maxOutputTokens", cfg_.max_tokens},
        }},
    };

    if (!system_parts.empty())
        body["systemInstruction"] = { {"parts", system_parts} };

    if (!tools.empty()) {
        json fns = json::array();
        for (const auto& t : tools)
            fns.push_back({
                {"name",        t.name},
                {"description", t.description},
                {"parameters",  t.parameters},
            });
        body["tools"] = json::array({ {{"functionDeclarations", fns}} });
    }

    return body;
}

Response GoogleProvider::parse_response(const fetch::Response& r) const {
    if (!r.ok())
        return Response::make_error("HTTP " + std::to_string(r.status())
                                    + ": " + r.text());
    try {
        auto j = json::parse(r.text());

        Tokens tok;
        if (j.contains("usageMetadata")) {
            tok.prompt     = j["usageMetadata"].value("promptTokenCount",     0);
            tok.completion = j["usageMetadata"].value("candidatesTokenCount", 0);
            tok.total      = j["usageMetadata"].value("totalTokenCount",      0);
        }

        auto& candidate = j["candidates"][0];
        std::string finish = candidate.value("finishReason", "STOP");
        std::string text;
        std::vector<ToolCall> calls;

        for (auto& part : candidate["content"]["parts"]) {
            if (part.contains("text")) {
                text += part["text"].get<std::string>();
            } else if (part.contains("functionCall")) {
                auto& fc = part["functionCall"];
                calls.push_back({
                    fc["name"].get<std::string>(),
                    fc["name"].get<std::string>(),
                    fc["args"],
                });
            }
        }

        Response res(text, finish, model_, tok);
        if (!calls.empty()) res.set_tool_calls(std::move(calls));
        return res;

    } catch (const std::exception& e) {
        return Response::make_error(std::string("parse error: ") + e.what());
    }
}

// ── public interface ─────────────────────────────────────────────────────────

Response GoogleProvider::chat(const History& history,
                               const std::vector<Tool>& tools)
{
    auto r = http_.post(chat_endpoint(), build_body(history, tools).dump());
    return parse_response(r);
}

void GoogleProvider::stream(const History& history,
                             const std::vector<Tool>& tools,
                             StreamCallback cb)
{
    std::string body = build_body(history, tools).dump();

    http_.sse(stream_endpoint(),
        [&cb](const fetch::ServerEvent& ev) -> bool {
            if (ev.data.empty()) return true;
            try {
                auto j = json::parse(ev.data);
                auto& candidate = j["candidates"][0];

                for (auto& part : candidate["content"]["parts"])
                    if (part.contains("text")) cb(part["text"].get<std::string>());

                // Any non-empty finishReason means the stream is done
                std::string finish = candidate.value("finishReason", "");
                if (!finish.empty() && finish != "FINISH_REASON_UNSPECIFIED")
                    return false;

            } catch (...) {}
            return true;
        },
        fetch::Options{
            .method = "POST",
            .headers = {},
            .body    = std::move(body),
        }
    );
}

} // namespace model::detail