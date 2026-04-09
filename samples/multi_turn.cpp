#include <model/client.hpp>
#include <model/chat.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

int main() {
    const char* key = std::getenv("GEMINI_API_KEY");
    if (!key) { std::cerr << "GEMINI_API_KEY not set\n"; return 1; }

    auto client = model::gemini("gemini-3.1-flash-lite-preview", key);
    model::Chat chat(client);

    chat.system("You are a helpful assistant. Be concise.");

    std::string input;
    while (true) {
        std::cout << "You: ";
        if (!std::getline(std::cin, input) || input == "/quit") break;

        auto res = chat.send(input);
        if (!res.ok()) { std::cerr << "Error: " << res.error() << "\n"; continue; }

        std::cout << "Assistant: " << res.text() << "\n\n";
    }

    std::cout << "Conversation had "
              << chat.history().messages().size()
              << " messages total.\n";
}