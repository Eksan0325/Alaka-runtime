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

bool isRoudiAlive() {
    const bool sock = alaka::filesystem::exists("/tmp/roudi");
    const bool fpid = alaka::helper::find_pid_by_name("alaka_roudi");
    const bool fshm = alaka::filesystem::exists("/dev/shm/iceoryx_mgmt");
    return sock && fpid && fshm;
}

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
        ::setenv("ALAKA_COMPONENT_NAME", info.entry.name.c_str(), 1);
        ::setenv("ALAKA_PERIOD_US", std::to_string(info.entry.system_config.period_us).c_str(), 1);
        ::setenv("ALAKA_PRIORITY", std::to_string(info.entry.system_config.priority).c_str(), 1);

        void * handle = ::dlopen(libstr.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            std::fprintf(stderr, "[Alaka] Component %s load failed: %s\n", info.entry.name.c_str(), ::dlerror());
            _exit(21);
        }
        //Call component's init and run function to block
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
    std::printf("[Alaka] alaka-runtime %s: %zu components count\n", config_.version.c_str(), config_.components.size());
    
    if (!startRoudi()) return -1;

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
    
    writePidsFile();
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
    int64_t last_pids_write = 0;
    while (!st.load(std::memory_order_relaxed)) {
        std::string cmd = alaka::filesystem::read_file(g_command_file_path);
        const auto t = alaka::rt::clock_now_ns();
        if (t - last_pids_write > 200LL * 1000000LL) {
            writePidsFile();
            last_pids_write = t;
        }
        alaka::rt::sleep_ns(50LL * 1000000LL);
    }
}

bool ComponentManager::startRoudi() {
    if (isRoudiAlive()) { 
        roudi_pid_ = static_cast<pid_t>(alaka::helper::find_pid_by_name("alaka_roudi"));
        return true; 
    }

    ::unlink("/tmp/roudi");
    ::unlink("/tmp/roudi.lock");
    ::unlink("/tmp/iox-unique-roudi.lock");
    
    std::string rd = alaka::filesystem::exe_dir() + "/alaka_roudi";
    if (!alaka::filesystem::exists(rd)) return false;
    pid_t rpid = fork();
    if (rpid < 0) return false;
    if (rpid == 0) {
        setsid();
        ::execl(rd.c_str(), "alaka_roudi", "-m", "off", static_cast<char*>(nullptr));
        _exit(127);
    }
    roudi_pid_ = rpid;
    for (auto i = 0; i < 200; ++i) {
        bool exroudi = alaka::filesystem::exists("/tmp/roudi");
        bool exrmgmt = alaka::filesystem::exists("/dev/shm/iceoryx_mgmt");
        if (exroudi && exrmgmt) return true;
        int st = 0;
        pid_t r = waitpid(rpid, &st, WNOHANG);
        if (r = rpid) {
            roudi_pid_ = -1;
            return false;
        }
        alaka::rt::sleep_ns(50 * 1000 * 1000);
    }
    return false;
}

void ComponentManager::stopRoudi() {
    pid_t pid = roudi_pid_ > 0 ? roudi_pid_ : alaka::helper::find_pid_by_name("alaka_roudi");
    if (pid <= 0) return;
    ::kill(pid, SIGTERM);
    for (auto i = 0; i < 30; ++i) { if (::kill(pid, 0) != 0) {return;} alaka::rt::sleep_ns(50 * 1000 * 1000); }
    ::kill(pid, SIGKILL);
    int st = 0;
    waitpid(pid, &st, 0); //Blocking wait
}

void ComponentManager::writePidsFile() const {
    std::ostringstream ss;
    for (const auto& p : pinfoes_) {
        const char* st = "RUNNING";
        if (p.state == ProcessState::Failed) st = "FAILED";
        else if (p.pid < 0) st = "STOPPED";
        ss << p.entry.name << ' ' << p.pid << ' ' << st << ' ' << p.restarts << '\n';
    }
    ss << "alaka_roudi " << (roudi_pid_ > 0 ? roudi_pid_ : static_cast<pid_t>(-1))
       << " RUNNING 0\n";
    alaka::filesystem::write_file(g_pids_file_path, ss.str());
    ::chmod(g_pids_file_path, 0666);
}

}