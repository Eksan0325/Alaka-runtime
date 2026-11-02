#include "component_config.hpp"

#include <cstdio>
#include <cctype>
#include <set>

#include <sys/stat.h>

#include <yaml-cpp/yaml.h>

using namespace alaka;

bool loadConfig(const std::string& path, AlakaRuntimeConfig& out) {
    YAML::Node r;
    try { r = YAML::LoadFile(path); } catch (...) { return false; }
    if (!r.IsMap()) return false;
    const YAML::Node sys_config = r["system"];
    if (!sys_config.IsMap()) return false;
    try {
        if (sys_config["version"]) out.version = sys_config["version"].as<std::string>();
    } catch (...) { return false; }
    const YAML::Node comp_config = r["components"];
    if (!comp_config.IsSequence()) return false;
    std::set<std::string> seen;
    for (const auto& comp : comp_config) {
        ComponentConfigEntry entry;
        try { entry.name = comp["name"].as<std::string>(); } catch (...) { return false; }
        if (!seen.insert(entry.name).second) return false;  // duplicate name
        try {
            if (comp["version"]) entry.version = comp["version"].as<std::string>();
            if (comp["type"]) entry.type = comp["type"].as<std::string>();
            if (comp["period_us"]) entry.system_config.period_us = comp["period_us"].as<int>();
            if (comp["priority"]) entry.system_config.priority = comp["priority"].as<int>();
            if (comp["cpu_affinity"]) {
                const YAML::Node cpu_affinity = comp["cpu_affinity"];
                if (!cpu_affinity.IsSequence()) return false;
                for (const auto& v : cpu_affinity) {
                    if (entry.system_config.cpu_count >= 64) return false;
                    int cpu = v.as<int>();
                    if (cpu < 0) return false;
                    entry.system_config.cpu_affinity[entry.system_config.cpu_count++] = cpu;
                }
            }
            if (comp["channel"]) entry.alaka_config.channel = comp["channel"].as<std::string>();
            if (comp["watched_topic"]) entry.alaka_config.watched_topic = comp["watched_topic"].as<std::string>();
        } catch (...) { return false; }
        out.components.push_back(entry);
    }
    if (out.components.empty()) return false;
    return true;
}

std::string findComponentLib(const std::string& r, const std::string& name, const std::string& version) {
    struct stat st;
    auto checked = [&](const std::string& path) { return stat(path.c_str(), &st) == 0;};
    std::string candidate;
    version.empty() ? candidate = r + "/components/lib" + name + ".so" : candidate = r + "/components/lib" + name + "-" + version + ".so";
    if (checked(candidate)) { return candidate; }
    return "";
}