#include "model/client.hpp"
#include "providers/openai.hpp"
#include "providers/anthropic.hpp"
#include "providers/google.hpp"

namespace model {

Client::Client(std::shared_ptr<detail::IProvider> provider)
    : provider_(std::move(provider))
{}

Response Client::chat(const std::string& prompt,
                      const std::vector<Tool>& tools) const
{
    History h;
    h.user(prompt);
    return provider_->chat(h, tools);
}

Response Client::chat(const History& history,
                      const std::vector<Tool>& tools) const
{
    return provider_->chat(history, tools);
}

void Client::stream(const std::string& prompt,
                    StreamCallback cb,
                    const std::vector<Tool>& tools) const
{
    History h;
    h.user(prompt);
    provider_->stream(h, tools, std::move(cb));
}

void Client::stream(const History& history,
                    StreamCallback cb,
                    const std::vector<Tool>& tools) const
{
    provider_->stream(history, tools, std::move(cb));
}

Response Client::submit_tool_result(History& history,
                                    const std::string& tool_call_id,
                                    const std::string& result,
                                    const std::vector<Tool>& tools) const
{
    history.tool_result(tool_call_id, result);
    return provider_->chat(history, tools);
}

// ── Factory functions ─────────────────────────────────────────────────────────

Client openai(std::string model, std::string api_key, Config cfg) {
    return Client(std::make_shared<detail::OpenAIProvider>(
        std::move(model), std::move(api_key), cfg));
}

Client claude(std::string model, std::string api_key, Config cfg) {
    return Client(std::make_shared<detail::AnthropicProvider>(
        std::move(model), std::move(api_key), cfg));
}

Client gemini(std::string model, std::string api_key, Config cfg) {
    return Client(std::make_shared<detail::GoogleProvider>(
        std::move(model), std::move(api_key), cfg));
}

} // namespace model