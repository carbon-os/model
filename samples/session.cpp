#include <model/client.hpp>
#include <model/session.hpp>
#include <model/history.hpp>
#include <cstdlib>
#include <filesystem>
#include <iostream>

static constexpr auto LOG = "session_log.jsonl";

int main() {
    const char* key = std::getenv("GEMINI_API_KEY");
    if (!key) { std::cerr << "GEMINI_API_KEY not set\n"; return 1; }

    if (std::filesystem::exists(LOG)) {
        std::cout << "=== Previous session ===\n";
        auto old = model::History::load(LOG);
        for (const auto& m : old.messages()) {
            std::cout << "[" << m.role << "]: " << m.content << "\n";
        }
        std::cout << "========================\n\n";
    }

    auto client = model::gemini("gemini-3.1-flash-lite-preview", key);

    model::Session session(client, {
        .system   = "You are a helpful assistant.",
        .log_file = LOG,
    });

    auto r1 = session.send("Hi! My name is Alex.");
    std::cout << "Assistant: " << r1.text() << "\n\n";

    auto r2 = session.send("What's my name?");
    std::cout << "Assistant: " << r2.text() << "\n\n";

    std::cout << "Session ending — history will be saved to " << LOG << "\n";
}