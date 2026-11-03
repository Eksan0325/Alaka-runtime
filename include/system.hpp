/**
 * @brief  HAL adapter
 * @author Eksan Ablikim
 */

#ifndef ALAKA_RUNTIME_INCLUDE_SYSTEM_HPP
#define ALAKA_RUNTIME_INCLUDE_SYSTEM_HPP

#include <cerrno>
#include <cstdint>
#include <ctime>
#include <cstring>
#include <string>

#include <fstream>
#include <ostream>
#include <sstream>

#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#include <sys/stat.h>

namespace alaka {

namespace rt {

inline int64_t clock_now_ns() noexcept {
    struct timespec tspec {};
    clock_gettime(CLOCK_MONOTONIC, &tspec);
    return static_cast<int64_t>(tspec.tv_sec) * 1000000000LL + tspec.tv_nsec;
}

inline int64_t clock_wall_ns() noexcept {
    struct timespec tspec {};
    clock_gettime(CLOCK_REALTIME, &tspec);
    return static_cast<int64_t>(tspec.tv_sec) * 1000000000LL + tspec.tv_nsec;
}

inline void sleep_until_ns(int64_t ddl_ns) noexcept {
    struct timespec tspec {};
    tspec.tv_sec = static_cast<time_t>(ddl_ns/1000000000LL);
    tspec.tv_nsec = static_cast<long>(ddl_ns%1000000000LL);
    while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tspec, nullptr) == EINTR){}
}

inline void sleep_ns(int64_t ns) noexcept {
    if (ns <= 0) return;
    sleep_until_ns(clock_now_ns() + ns);
}

inline int set_sched_fifo(int priority) noexcept {
    struct sched_param p; p.sched_priority = priority; return pthread_setschedparam(pthread_self(), SCHED_FIFO, &p);
}
inline int set_sched_rr(int priority) noexcept { 
    struct sched_param p; p.sched_priority = priority; return pthread_setschedparam(pthread_self(), SCHED_RR, &p);
}
inline int set_sched_other() noexcept {
    struct sched_param p; p.sched_priority = 0; return pthread_setschedparam(pthread_self(), SCHED_OTHER, &p);
}

inline int bind_cpu(const int* cpus, int count) noexcept {
    cpu_set_t cpu_setter;
    CPU_ZERO(&cpu_setter);
    for (int i = 0; i < count; ++i) { if (cpus[i] >= 0) CPU_SET(cpus[i], &cpu_setter); }
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_setter);
}

inline void cpu_relax() noexcept {
#if defined(__x86_64__)
    __builtin_ia32_pause();
#elif defined(__aarch64__)
    asm volatile("yield" ::: "memory");
#else
    std::atomic_signal_fence(std::memory_order_seq_cst);
#endif
}

}

namespace filesystem {

inline bool is_dir(const std::string& p) {
    struct stat st{};
    return !p.empty() && stat(p.c_str(), &st) == 0;
}

inline bool exists(const std::string& p) {
    struct stat st{};
    return !p.empty() && stat(p.c_str(), &st) == 0;
}

inline bool mkdir_p(const std::string& p) {
    if (p.empty()) return false;
    if (is_dir(p)) return true;
    std::string current;
    for (auto i = 0; i < p.size(); ++i) {
        current.push_back(p[i]);
        if (p[i] == '/' || i + 1 == p.size()) {
            if (current != "/" && !current.empty()) ::mkdir(current.c_str(), 0775);
        }
    }
    return is_dir(p);
}

inline std::string read_file(const std::string& p) {
    std::ifstream f(p);
    if (!f) return {};
    std::ostringstream ss; ss << f.rdbuf(); return ss.str();
}
inline bool write_file(const std::string& p, const std::string& content) {
    std::ofstream f(p, std::ios::trunc);
    if (!f) return false;
    f << content; return static_cast<bool>(f);
}

inline std::string exe_dir() {
    char buf[4096] = {};
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) return ".";
    std::string s(buf, static_cast<size_t>(n));
    auto slash = s.rfind('/');
    return slash == std::string::npos ? std::string(".") : s.substr(0, slash);
}

inline std::string arch_str() {
    if (const char* t = std::getenv("ALAKA_TARGET")) return t;
#if defined(__aarch64__)
    return "aarch64";
#else
    return "x86_64";
#endif
}

}

}


#endif