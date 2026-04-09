#include <model/client.hpp>
#include <model/chat.hpp>
#include <model/tools.hpp>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <iostream>

using json = nlohmann::json;

static std::string fake_weather(const std::string& location) {
    return "{ \"location\": \"" + location + "\", "
           "\"temperature\": 22, \"unit\": \"celsius\", "
           "\"condition\": \"sunny\" }";
}

int main() {
    const char* key = std::getenv("GEMINI_API_KEY");
    if (!key) { std::cerr << "GEMINI_API_KEY not set\n"; return 1; }

    auto client = model::gemini("gemini-3.1-flash-lite-preview", key);

    model::Tool weather_tool {
        .name        = "get_weather",
        .description = "Returns current weather for a given location.",
        .parameters  = json::parse(R"({
            "type": "object",
            "properties": {
                "location": {
                    "type": "string",
                    "description": "City and country, e.g. Paris, France"
                }
            },
            "required": ["location"]
        })")
    };

    model::Chat chat(client);
    chat.tools({ weather_tool });

    auto res = chat.send("What is the weather like in Tokyo right now?");
    if (!res.ok()) { std::cerr << "Error: " << res.error() << "\n"; return 1; }

    if (res.has_tool_call()) {
        const auto& call = res.tool_call();
        std::cout << "Model called tool: " << call.name << "\n";
        std::cout << "Arguments: " << call.arguments.dump(2) << "\n";

        std::string location = call.arguments.value("location", "Unknown");
        auto final_res = chat.submit_tool_result(call.id, fake_weather(location));
        if (!final_res.ok()) { std::cerr << "Error: " << final_res.error() << "\n"; return 1; }

        std::cout << "Final response: " << final_res.text() << "\n";
    } else {
        std::cout << res.text() << "\n";
    }
}