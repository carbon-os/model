#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace model {

struct Tool {
    std::string    name;
    std::string    description;
    nlohmann::json parameters;   // JSON Schema object
};

// ToolCall is defined in response.hpp; re-declared here for convenience headers
// that only need the type without pulling in the full Response.
// In practice, just include response.hpp.

} // namespace model