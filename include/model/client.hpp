#pragma once
#include "config.hpp"
#include "response.hpp"
#include "history.hpp"
#include "tools.hpp"
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace model {

namespace detail { struct IProvider; }

using StreamCallback = std::function<void(std::string_view chunk)>;

class Client {
public:
    explicit Client(std::shared_ptr<detail::IProvider> provider);

    // Single-turn
    Response chat(const std::string& prompt,
                  const std::vector<Tool>& tools = {}) const;

    // Single-turn over an existing History
    Response chat(const History& history,
                  const std::vector<Tool>& tools = {}) const;

    // Single-turn streaming from a prompt
    void stream(const std::string& prompt,
                StreamCallback cb,
                const std::vector<Tool>& tools = {}) const;

    // Single-turn streaming over an existing History
    void stream(const History& history,
                StreamCallback cb,
                const std::vector<Tool>& tools = {}) const;

    // Submit a tool result and get the next response
    // (history must already contain the assistant tool_call turn)
    Response submit_tool_result(History& history,
                                const std::string& tool_call_id,
                                const std::string& result,
                                const std::vector<Tool>& tools = {}) const;

private:
    std::shared_ptr<detail::IProvider> provider_;
};

// --- Factory functions ---
Client openai (std::string model, std::string api_key, Config cfg = {});
Client claude (std::string model, std::string api_key, Config cfg = {});
Client gemini (std::string model, std::string api_key, Config cfg = {});

} // namespace model