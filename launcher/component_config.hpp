/**
 * @brief Parse alaka_system.yaml and check the component priority and cpu affinity, then start RouDi process.
 * @author Eksan Ablikim
 */
#ifndef ALAKA_RUNTIME_LAUNCHER_COMPONENT_CONFIG_HPP
#define ALAKA_RUNTIME_LAUNCHER_COMPONENT_CONFIG_HPP

#include <string>
#include <vector>

namespace alaka {

struct ComponentSystemConfig {
    int period_us = 0;
    int priority = 0;
    int cpu_affinity[64] = {};
    int cpu_count = 0;
};

struct ComponentAlakaConfig {
    std::string channel;
    std::string watched_topic;
};

struct ComponentConfigEntry {
    std::string name;
    std::string version;
    std::string type;
    ComponentSystemConfig system_config;
    ComponentAlakaConfig alaka_config;
};

struct AlakaRuntimeConfig {
    std::string version;
    std::vector<ComponentConfigEntry> components;
};

}

#endif