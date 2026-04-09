#pragma once
#include "provider.hpp"
#include <fetch/fetch.hpp>

namespace model::detail {

class AnthropicProvider final : public IProvider {
public:
    AnthropicProvider(std::string model, std::string api_key, model::Config cfg);

    Response chat  (const History& history,
                    const std::vector<Tool>& tools) override;

    void     stream(const History& history,
                    const std::vector<Tool>& tools,
                    StreamCallback cb)              override;

private:
    std::string   model_;
    std::string   api_key_;
    model::Config cfg_;
    fetch::Client http_;

    nlohmann::json build_body(const History& history,
                              const std::vector<Tool>& tools,
                              bool stream = false) const;

    Response parse_response(const fetch::Response& r) const;
};

} // namespace model::detail