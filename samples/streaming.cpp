#include <model/client.hpp>
#include <cstdlib>
#include <iostream>

int main() {
    const char* key = std::getenv("GEMINI_API_KEY");
    if (!key) { std::cerr << "GEMINI_API_KEY not set\n"; return 1; }

    auto client = model::gemini("gemini-3.1-flash-lite-preview", key);

    std::cout << "Response: ";
    client.stream(
        "Tell me a short story about a robot who learns to paint.",
        [](std::string_view chunk) {
            std::cout << chunk << std::flush;
        }
    );
    std::cout << "\n";
}