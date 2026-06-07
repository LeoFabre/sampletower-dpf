#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <cstdlib>
#include "json.hpp"

namespace st {

struct SlotConfig {
    std::string path;
    bool loop = false;
};

// Default config location: $HOME/SampleTower/samples_config.json
inline std::string defaultConfigPath() {
    const char* home = std::getenv("HOME");
    std::string base = home ? home : ".";
    return base + "/SampleTower/samples_config.json";
}

// Parse the config. Never throws; returns empty on any error. Entries without
// a "path" are skipped; "loop" defaults to false.
inline std::vector<SlotConfig> loadSampleConfig(const std::string& path) {
    std::vector<SlotConfig> out;
    std::ifstream in(path, std::ios::binary);
    if (!in.good()) return out;
    nlohmann::json j = nlohmann::json::parse(in, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.contains("samples") || !j["samples"].is_array()) return out;
    for (const auto& e : j["samples"]) {
        if (!e.is_object() || !e.contains("path") || !e["path"].is_string()) continue;
        SlotConfig sc;
        sc.path = e["path"].get<std::string>();
        if (e.contains("loop") && e["loop"].is_boolean()) sc.loop = e["loop"].get<bool>();
        out.push_back(std::move(sc));
    }
    return out;
}

} // namespace st
