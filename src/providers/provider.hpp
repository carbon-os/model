#pragma once
#include "model/config.hpp"
#include "model/history.hpp"
#include "model/response.hpp"
#include "model/tools.hpp"
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace model::detail {

using StreamCallback = std::function<void(std::string_view)>;

struct IProvider {
    virtual ~IProvider() = default;

    virtual Response chat(const History&            history,
                          const std::vector<Tool>&  tools) = 0;

    virtual void     stream(const History&           history,
                            const std::vector<Tool>& tools,
                            StreamCallback           cb)   = 0;
};

} // namespace model::detail