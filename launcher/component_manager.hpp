/**
 * @brief Manager of Component lifecycle,
 * fork process, including loading, starting, stopping and monitoring components.
 * @author Eksan Ablikim
 */
#ifndef ALAKA_RUNTIME_LAUNCHER_COMPONENT_MANAGER_HPP
#define ALAKA_RUNTIME_LAUNCHER_COMPONENT_MANAGER_HPP

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include <component_config.hpp>

namespace alaka {

enum class ProcessState { Stopped, Starting, Running, Failed };

struct ProcessInfo {
    ComponentConfigEntry entry;
    pid_t      pid = -1;
    ProcessState  state = ProcessState::Stopped;
    int        restarts = 0;
    int64_t    last_start_ns = 0;
    int64_t    last_hb_ns = 0;
    int64_t    next_start_ns = 0;
    std::string lib;
};

class ComponentManager {
public:
    explicit ComponentManager(const AlakaRuntimeConfig& cfg, const std::string& root);
    ~ComponentManager();

    int start();
    void stop();

    void loop(std::atomic<bool>& stop);

private:
    const AlakaRuntimeConfig& config_;
    std::vector<ProcessInfo> pinfoes_;
    std::string root_;
    pid_t roudi_pid_ = -1;
    bool stopping_;
};

}

#endif