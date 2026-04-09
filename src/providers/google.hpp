#pragma once
#include "provider.hpp"
#include <fetch/fetch.hpp>

namespace model::detail {

class GoogleProvider final : public IProvider {
public:
    GoogleProvider(std::string model, std::string api_key, model::Config cfg);

    Response chat  (const History& history,
                    const std::vector<Tool>& tools) override;

    void     stream(const History& history,
                    const std::vector<Tool>& tools,
                    StreamCallback cb)              override;

private:
    std::string   model_;
    model::Config cfg_;
    fetch::Client http_;

    nlohmann::json build_body(const History& history,
                              const std::vector<Tool>& tools) const;

    Response parse_response(const fetch::Response& r) const;

    std::string chat_endpoint()   const;
    std::string stream_endpoint() const;
};

} // namespace model::detail