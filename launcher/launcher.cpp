/**
 * @brief Process manager of alaka runtime, start all components of alaka runtime.
 * @author Eksan Ablikim
 */

#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <string>

#include <unistd.h>

#include "component_manager.hpp"
#include "component_config.hpp"

std::atomic_bool global_stop = false;
static void on_signal(int sig) { global_stop.store(true); }

static void init_signal_handler() {
    struct sigaction s_action;
    memset(&s_action, 0, sizeof(s_action));
    s_action.sa_handler = on_signal;
    sigemptyset(&s_action.sa_mask);
    sigaction(SIGINT, &s_action, nullptr);
    sigaction(SIGTERM, &s_action, nullptr);
    signal(SIGPIPE, SIG_IGN);
}
int main(int argc, char** argv) {

    alaka::AlakaRuntimeConfig cfg;
    if (!alaka::loadConfig("alaka_system.yaml", cfg)) {
        std::fprintf(stderr, "[Alaka] Failed to load alaka_system.yaml\n");
        return -1;
    }

    std::string root;
    if (const char * r = std::getenv("ALAKA_ROOT")) root = r;
    if (root.empty()) {
        char buf[4096] = {};
        ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (n > 0) {
            std::string p(buf, static_cast<size_t>(n));
            auto s = p.rfind('/');
            if (s != std::string::npos) {
                p.resize(s);
                s = p.rfind('/');
                if (s != std::string::npos) root = p.substr(0, s);
            }
        }
    }
    if (root.empty()) root = ".";
    std::printf("[Alaka] alaka-runtime %s: %zu components count\n", cfg.version.c_str(), cfg.components.size());
    std::printf("[Alaka] root: %s\n", root.c_str());

    init_signal_handler();

    alaka::ComponentManager manager(cfg, root);
    int rc = manager.start();
    if (rc != 0) {
        std::fprintf(stderr, "[Alaka] Failed to start components, rc=%d\n", rc);
        return rc;
    }
    manager.loop(global_stop);
    manager.stop();
    return 0;
}