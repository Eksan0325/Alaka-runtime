#include "component_manager.hpp"

#include <sstream>
#include <fstream>

#include <dlfcn.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include <sys/wait.h>

#include "system.hpp"

constexpr const char * g_pids_file_path = "/dev/shm/alaka_runtime_pids";
constexpr const char * g_command_file_path = "/dev/shm/alaka_runtime_manager.cmd";


namespace alaka {

namespace {

bool startComponentsProcess(const std::string& rp, ProcessInfo& info) {
    const std::string libstr = findComponentLib(rp, info.entry.name, info.entry.version);
    if (libstr.empty()) {
        std::fprintf(stderr, "[Alaka] Component %s not found\n", info.entry.name.c_str());
        info.state = ProcessState::Failed;
        return false;
    }
    pid_t pid = ::fork();
    if (pid < 0) { info.state = ProcessState::Failed; return false; }
    if (pid == 0) {
        ::setsid();
        ::prctl(PR_SET_NAME, info.entry.name.substr(0, 15).c_str(), 0, 0, 0);
        ::setenv("ALAKA_ROOT", rp.c_str(), 1);
        void * handle = ::dlopen(libstr.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            std::fprintf(stderr, "[Alaka] Component %s load failed: %s\n", info.entry.name.c_str(), ::dlerror());
            _exit(21);
        }
        //Call component's init and run function
        auto fnc = reinterpret_cast<int(*)(void*)>(::dlsym(handle, "runtime_init_and_run_cpp"));
        if (!fnc) {
            std::fprintf(stderr, "[Alaka] Component %s missing symbol runtime_init_and_run_cpp\n", info.entry.name.c_str());
            _exit(22);
        }
        int rc = fnc(nullptr);
        ::dlclose(handle);
        _exit(rc);
    }
    info.pid = pid;
    info.state = ProcessState::Starting;
    const auto& rd = std::string() + "/dev/shm/alaka_runtime_ready_" + info.entry.name;
    for (int i = 0; i < 100; ++i) {
        std::ifstream ifs(rd);
        std::stringstream ss; ss << ifs.rdbuf();
        const std::string content = ss.str();
        if (!content.empty() && std::atoi(content.c_str()) == static_cast<int>(pid)) {
            std::fprintf(stderr, "[Alaka] Component %s started with pid %d\n", info.entry.name.c_str(), static_cast<int>(pid));
            info.state = ProcessState::Running;
            return true;
        }
        int status = 0;
        pid_t w = ::waitpid(pid, &status, WNOHANG);
        if (w == pid) {
            std::fprintf(stderr, "[Alaka] Component %s exited with status %d\n", info.entry.name.c_str(), status);
            info.pid = -1;
            info.state = ProcessState::Failed;
            return false;
        }
        alaka::rt::sleep_ns(50 * 1000 * 1000);
    }
    std::fprintf(stderr, "[Alaka] Component %s ready handshake timeout (5s)\n", info.entry.name.c_str());
    info.state = ProcessState::Running;
    return true;
}

}

ComponentManager::ComponentManager(const AlakaRuntimeConfig& cfg, const std::string& root)
    : config_(cfg), root_(root), stopping_(false) 
{
    for (const auto& i : cfg.components) { ProcessInfo p; p.entry = i; pinfoes_.push_back(p); }
}
ComponentManager::~ComponentManager() { stop(); }

int ComponentManager::start() {
    std::printf("[Alaka] alaka-runtime %s: %zu components count\n", config_.version.c_str(), config_.components.count());
    for (const auto& comp : config_.components) {
        const auto& rd = std::string() + "/dev/shm/alaka_runtime_ready_" + comp.name;
        const auto& hb = std::string() + "/dev/shm/alaka_runtime_hb_" + comp.name;
        ::unlink(rd.c_str());
        ::unlink(hb.c_str());
    }
    std::ofstream ofs(g_command_file_path, std::ios::trunc);
    if (ofs) ofs << ""; //empty write
    for (auto& p : pinfoes_) {
        if (!startComponentsProcess(root_, p)) std::fprintf(stderr, "[Alaka] Component %s failed to start\n", p.entry.name.c_str());
    }
    std::printf("[Alaka] %zu components started (Ctrl+C to stop)\n", config_.components.size());
    return 0;
}

void ComponentManager::stop() {
    if (stopping_) return;
    stopping_ = true;
    for (auto& p : pinfoes_) { if (p.pid > 0) ::kill(p.pid, SIGTERM); }
    for (int i = 0; i < 100; ++i) {
        bool flag = false;
        for (auto& p : pinfoes_) if (p.pid > 0 && ::kill(p.pid, 0) == 0) flag = true;
        if (!flag) break;
        alaka::rt::sleep_ns(50 * 1000 * 1000);
    }
    for (auto& p : pinfoes_) {
        if (p.pid > 0 && ::kill(p.pid, 0) == 0) {
            std::fprintf(stderr, "[Alaka] Component %s did not exit in 5s, sending SIGKILL\n", p.entry.name.c_str());
            ::kill(p.pid, SIGKILL);
        }
        p.pid = -1;
        p.state = ProcessState::Stopped;
    }
    std::printf("[Alaka] All components stopped\n");
}

void ComponentManager::loop(std::atomic_bool& st) {
    while (!st.load(std::memory_order_relaxed)) alaka::rt::sleep_ns(50LL * 1000000LL);
}

}