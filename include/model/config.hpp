#pragma once

namespace model {

struct Config {
    float temperature = 1.0f;
    int   max_tokens  = 1024;
    int   timeout_ms  = 30'000;
};

} // namespace model