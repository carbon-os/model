#include "model/history.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

namespace model {

void History::push(std::string role, std::string content,
                   std::string tool_call_id)
{
    messages_.push_back({
        std::move(role),
        std::move(content),
        std::move(tool_call_id),
    });
}

void History::system   (std::string content) { push("system",    std::move(content)); }
void History::user     (std::string content) { push("user",      std::move(content)); }
void History::assistant(std::string content) { push("assistant", std::move(content)); }
void History::tool_result(std::string id, std::string content) {
    push("tool", std::move(content), std::move(id));
}

void History::save(const std::string& path) const {
    std::ofstream f(path);
    if (!f) throw std::runtime_error("History::save: cannot open " + path);
    for (const auto& m : messages_) {
        json obj = { {"role", m.role}, {"content", m.content} };
        if (!m.tool_call_id.empty()) obj["tool_call_id"] = m.tool_call_id;
        f << obj.dump() << "\n";
    }
}

History History::load(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("History::load: cannot open " + path);
    History h;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        auto obj = json::parse(line);
        h.push(
            obj["role"]   .get<std::string>(),
            obj["content"].get<std::string>(),
            obj.value("tool_call_id", "")
        );
    }
    return h;
}

} // namespace model