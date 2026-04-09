# model

A modern C++17/20 HTTP client library for interfacing with frontier AI model APIs.
One interface. Any provider.

## Supported Providers

- OpenAI (GPT-4o, GPT-4-turbo, etc.)
- Anthropic (Claude Sonnet, Opus, Haiku, etc.)
- Google (Gemini 2.0 Flash, Gemini Pro, etc.)

---

## Table of Contents

- [Installation](#installation)
- [Project Layout](#project-layout)
- [Quick Start](#quick-start)
- [API Reference](#api-reference)
  - [Factory Functions](#factory-functions)
  - [Config](#config)
  - [Response](#response)
  - [Client](#client)
  - [Chat](#chat)
  - [History](#history)
  - [Session](#session)
  - [Streaming](#streaming)
  - [Tool Calling](#tool-calling)
  - [Error Handling](#error-handling)
- [Core Types](#core-types)
- [Design Goals](#design-goals)
- [Requirements](#requirements)
- [License](#license)

---

## Installation

### CMake (FetchContent)
```cmake
include(FetchContent)
FetchContent_Declare(
    model
    GIT_REPOSITORY https://github.com/yourname/model.git
    GIT_TAG        v1.0.0
)
FetchContent_MakeAvailable(model)
target_link_libraries(your_target PRIVATE model)
```

### vcpkg
```bash
vcpkg install model
```

### Conan
```bash
conan install model/1.0.0
```

---

## Project Layout

```
model/
│
├── CMakeLists.txt                  # top-level build, exposes model:: target
├── vcpkg.json                      # manifest — declares fetch, nlohmann-json deps
├── LICENSE
├── README.md
│
├── include/
│   └── model/
│       ├── client.hpp              # model::Client + factory fns (openai/claude/gemini)
│       ├── chat.hpp                # model::Chat (multi-turn, owns a History)
│       ├── history.hpp             # model::History (plain JSONL message log)
│       ├── session.hpp             # model::Session (RAII Chat + auto-save)
│       ├── config.hpp              # model::Config
│       ├── response.hpp            # model::Response, model::Tokens
│       └── tools.hpp               # model::Tool, model::ToolCall
│
├── src/
│   ├── client.cpp
│   ├── chat.cpp
│   ├── history.cpp
│   ├── session.cpp
│   ├── response.cpp
│   │
│   └── providers/                  # one translation unit per provider
│       ├── provider.hpp            # internal abstract base: IProvider
│       ├── openai.hpp  / .cpp      # OpenAI wire format
│       ├── anthropic.hpp / .cpp    # Anthropic wire format
│       └── google.hpp  / .cpp      # Google Gemini wire format
│
├── cmake/
│   └── modelConfig.cmake.in        # for cmake --install consumers
│
└── tests/
    ├── CMakeLists.txt
    ├── test_client.cpp
    ├── test_chat.cpp
    ├── test_history.cpp
    ├── test_session.cpp
    ├── test_tools.cpp
    └── mock_server.hpp             # tiny local SSE/HTTP mock using fetch primitives
```

### Dependencies

`fetch` and `nlohmann/json` are the only external dependencies. Both are declared
in `vcpkg.json` and acquired automatically:

```json
{
  "name": "model",
  "version": "1.0.0",
  "dependencies": [
    "fetch",
    "nlohmann-json"
  ]
}
```

`fetch` is used exclusively inside `src/providers/` — public headers never mention
it, so consumers have no transitive include exposure. `nlohmann/json` is surfaced
deliberately in `tools.hpp` since the schema and arguments are JSON by nature.

---

## Quick Start

```cpp
#include <model/client.hpp>

int main() {
    auto client = model::openai("gpt-4o", "your-api-key");
    auto res = client.chat("What is the capital of France?");
    std::cout << res.text() << std::endl;
    return 0;
}
```

---

## API Reference

### Factory Functions

```cpp
namespace model {
    Client openai (std::string model, std::string api_key, Config cfg = {});
    Client claude (std::string model, std::string api_key, Config cfg = {});
    Client gemini (std::string model, std::string api_key, Config cfg = {});
}
```

Swap one line to change provider. Everything else stays the same.

```cpp
auto client = model::openai("gpt-4o",            api_key);
// auto client = model::claude("claude-sonnet-4-6", api_key);
// auto client = model::gemini("gemini-2.0-flash",  api_key);

auto res = client.chat("Explain quantum entanglement.");
std::cout << res.text() << std::endl;
```

---

### Config

Shared configuration across all providers.

```cpp
model::Config cfg;
cfg.temperature = 0.7f;
cfg.max_tokens  = 1024;
cfg.timeout_ms  = 5000;

auto client = model::openai("gpt-4o", api_key, cfg);
```

| Field         | Type    | Default | Description                     |
|---------------|---------|---------|---------------------------------|
| `temperature` | `float` | `1.0`   | Sampling temperature            |
| `max_tokens`  | `int`   | `1024`  | Maximum completion tokens       |
| `timeout_ms`  | `int`   | `30000` | Request timeout in milliseconds |

---

### Response

Every provider returns the same `model::Response` type.

```cpp
auto res = client.chat("Hello!");

res.text();           // response content as std::string
res.tokens();         // model::Tokens { prompt, completion, total }
res.finish_reason();  // "stop", "length", "tool_call", "error", etc.
res.model();          // model name echoed back from the provider
res.ok();             // false if the request failed
res.error();          // std::string error message if !ok()

// Tool calling
res.has_tool_call();  // true if the model wants to invoke a tool
res.tool_call();      // model::ToolCall — asserts exactly one
res.tool_calls();     // std::vector<model::ToolCall> — when model returns several
```

---

### Client

`model::Client` is the one-shot entry point. Use it directly for single-turn
requests, or pass it into `Chat` or `Session` for multi-turn work.

```cpp
auto client = model::claude("claude-sonnet-4-6", api_key);

// Single-turn
auto res = client.chat("What is the speed of light?");
std::cout << res.text();

// Single-turn stream
client.stream("Write a haiku about the ocean.", [](std::string_view chunk) {
    std::cout << chunk << std::flush;
});

// Replay a History in one shot
auto res2 = client.chat(history);
```

---

### Chat

`model::Chat` is the multi-turn workhorse. It holds a `Client` reference and
owns a `History` internally, appending every turn automatically.

```cpp
model::Chat chat(client);
chat.system("You are a terse assistant. One sentence max.");

auto r1 = chat.send("What is 2 + 2?");
std::cout << r1.text();                   // "4."

auto r2 = chat.send("Multiply that by 10.");
std::cout << r2.text();                   // "40."

// Access the accumulated history at any point
const model::History& h = chat.history();

// Construct a Chat from an existing History to resume a prior exchange
model::Chat resumed(client, history);
resumed.send("Actually, can you revisit your last point?");
```

---

### History

`model::History` is plain data — a message log with no client attached.
It serialises to and from JSONL, one JSON object per line, making it trivial
to store, stream, inspect, and replay.

```cpp
// Build manually
model::History h;
h.system("You are a helpful assistant.");
h.user("What is the capital of France?");
h.assistant("Paris.");      // manually append a known prior turn
h.user("And Germany?");

// Replay on any client
auto res = client.chat(h);
std::cout << res.text();    // "Berlin."

// Save
h.save("chat.jsonl");       // writes one JSON object per line

// Load
auto h2 = model::History::load("chat.jsonl");

// Inspect
for (const auto& msg : h2.messages()) {
    std::cout << msg.role << ": " << msg.content << "\n";
}
```

#### JSONL format

Each line is a self-contained JSON object:

```jsonl
{"role":"system",    "content":"You are a helpful assistant."}
{"role":"user",      "content":"What is the capital of France?"}
{"role":"assistant", "content":"Paris."}
{"role":"user",      "content":"And Germany?"}
{"role":"assistant", "content":"Berlin."}
```

Any line range is itself valid JSONL, files are trivially appendable, and the
format is easy to inspect with standard tools (`jq`, `grep`, etc.).

---

### Session

`model::Session` is a RAII wrapper around `Chat`. It adds lifecycle management —
auto-saving the `History` to a JSONL file on destruction, making persistence
zero-effort for the common case.

```cpp
struct SessionOptions {
    std::string system;     // system prompt
    std::string log_file;   // path to write History as JSONL on close
};
```

```cpp
{
    model::Session session(client, {
        .system   = "You are a code reviewer.",
        .log_file = "review_session.jsonl",
    });

    session.send("Review this: int x = 1/0;");
    session.send("How would you fix it?");

}   // destructor fires — full History flushed to review_session.jsonl


// Resume that session later
auto history = model::History::load("review_session.jsonl");
model::Chat resumed(client, history);
resumed.send("Can you also check for null pointer dereferences?");
```

---

### Streaming

Streaming is available on both `Client` (single-turn) and `Chat` (multi-turn).
The completed assistant turn is automatically appended to `Chat`'s internal
`History` once the stream finishes.

```cpp
// Single-turn stream on Client
client.stream("Write a haiku about the ocean.", [](std::string_view chunk) {
    std::cout << chunk << std::flush;
});

// Multi-turn stream on Chat
model::Chat chat(client);
chat.system("You are a creative writing assistant.");

chat.send_stream("Write a short story about a robot.", [](std::string_view chunk) {
    std::cout << chunk << std::flush;
});

// Assistant turn was appended — conversation continues naturally
auto r2 = chat.send("Give the robot a name and a backstory.");
std::cout << r2.text();
```

---

### Tool Calling

Tools are defined with a name, a description, and a JSON Schema object for
parameters. The schema is plain `nlohmann::json` — no wrapper types, no
code generation. Write it once, it works on all three providers.

#### Defining tools

```cpp
#include <model/tools.hpp>
#include <nlohmann/json.hpp>

model::Tool weather_tool {
    .name        = "get_weather",
    .description = "Get the current weather for a location.",
    .parameters  = nlohmann::json {
        { "type", "object" },
        { "properties", {
            { "location", { {"type","string"}, {"description","City and country, e.g. Paris, FR"} } },
            { "units",    { {"type","string"}, {"enum", {"celsius","fahrenheit"}}                 } },
        }},
        { "required", { "location" } },
    }
};
```

#### Single tool call

```cpp
model::Chat chat(client);
chat.tools({ weather_tool });

auto res = chat.send("What's the weather like in Paris right now?");

if (res.has_tool_call()) {
    auto call = res.tool_call();          // model::ToolCall { id, name, arguments }

    std::string location = call.arguments["location"];
    std::string result   = my_weather_api(location);

    // Feed the result back — Chat splices it into History automatically
    auto final = chat.submit_tool_result(call.id, result);
    std::cout << final.text();            // "It's currently 18°C and cloudy in Paris."
}
```

#### Parallel tool calls

Some providers return multiple tool calls in a single turn. Use `tool_calls()`
and submit each result before sending the next user message.

```cpp
auto res = chat.send("What's the weather in Paris and Tokyo?");

for (const auto& call : res.tool_calls()) {
    std::string result = my_weather_api(call.arguments["location"]);
    chat.submit_tool_result(call.id, result);
}

auto final = chat.flush_tool_results();
std::cout << final.text();
```

#### Tool types

```cpp
// model::Tool — what you register
struct Tool {
    std::string    name;
    std::string    description;
    nlohmann::json parameters;   // JSON Schema object
};

// model::ToolCall — what the model returns
struct ToolCall {
    std::string    id;           // opaque call id, echoed back in submit_tool_result
    std::string    name;         // which tool the model wants to invoke
    nlohmann::json arguments;    // parsed arguments, ready to index directly
};
```

Serialising each provider's wire format (OpenAI `tools`, Anthropic `tools`,
Gemini `functionDeclarations`) is handled internally. Callers never see it.

---

### Error Handling

`model` does not throw. Every call returns a `model::Response` — check `ok()`
before using the result.

```cpp
auto res = client.chat("Hello!");

if (!res.ok()) {
    std::cerr << "Error: " << res.error() << "\n";
    return 1;
}

std::cout << res.text() << "\n";
```

Provider-level errors (rate limits, auth failures, malformed requests) are
surfaced through `res.error()` and reflected in `res.finish_reason()`.

Network and TLS failures from the underlying `fetch` layer are caught internally
and re-surfaced the same way — callers never need to handle exceptions.

---

## Core Types

| Type              | Description                                                        |
|-------------------|--------------------------------------------------------------------|
| `model::Client`   | One-shot client — constructed per provider via factory function    |
| `model::Chat`     | Multi-turn exchange — owns a `History`, holds a `Client` ref       |
| `model::History`  | Plain message log — serialises to/from JSONL, no client attached   |
| `model::Session`  | RAII `Chat` — auto-saves `History` to a JSONL file on destruction  |
| `model::Config`   | Shared configuration (temperature, max_tokens, timeout)            |
| `model::Response` | Unified response returned by all providers and all call sites      |
| `model::Tokens`   | Token usage breakdown `{ prompt, completion, total }`              |
| `model::Tool`     | Tool definition — name, description, JSON Schema parameters        |
| `model::ToolCall` | Tool invocation returned by the model — id, name, arguments        |

---

## Type Relationships

```
History          raw message log — just data, no client
   │
   └──▶ Chat     active turn-taking — owns a History, holds a Client ref
            │
            └──▶ Session    RAII wrapper — adds log_file auto-save on destruction
```

---

## Design Goals

- **Provider-agnostic** — swap providers with a single line change
- **No-throw API** — errors surface through `Response`, never exceptions
- **History is just JSONL** — trivially storable, inspectable, and resumable
- **Tools are just JSON** — write a JSON Schema once, works on all providers
- **Modern C++17/20** — no macros, strong types, RAII throughout
- **Minimal dependencies** — only requires `fetch` and `nlohmann/json`
- **Clean seam** — `fetch` is fully encapsulated in `src/`; public headers never expose it

---

## Requirements

- C++17 or later
- [fetch](https://github.com/carbon-os/fetch) (via vcpkg)
- nlohmann/json (via vcpkg)

---

## License

MIT