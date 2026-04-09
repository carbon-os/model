#include <model/client.hpp>
#include <cstdlib>
#include <iostream>

int main() {
    const char* key = std::getenv("GEMINI_API_KEY");
    if (!key) { std::cerr << "GEMINI_API_KEY not set\n"; return 1; }

    auto client = model::gemini("gemini-3.1-flash-lite-preview", key);

    auto res = client.chat("What is the capital of France?");
    if (!res.ok()) { std::cerr << "Error: " << res.error() << "\n"; return 1; }

    std::cout << res.text() << "\n";
    std::cout << "Tokens used: " << res.tokens().total << "\n";
}