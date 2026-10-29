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
    init_signal_handler();
    //todo run loop;
    return 0;
}