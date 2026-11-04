#include "include/component.hpp"
#include "include/system.hpp"

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <string>
#include <unistd.h>

using namespace alaka;
struct Component::Impl {
    std::string name_;
    std::string watched_topics_;
    std::string version_;
    int period_us_ = 0;
    ComponentTriggerMode mode_ = ComponentTriggerMode::PERIODIC;
    ComponentState state_ = ComponentState::UNCONFIGURED;
};

namespace {
std::atomic_bool component_stop_requested{false};

void on_component_signal(int) {
    component_stop_requested.store(true, std::memory_order_relaxed);
}

void install_component_signal_handlers() {
    struct sigaction action {};
    action.sa_handler = on_component_signal;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGTERM, &action, nullptr);
}
}

Component::Component(const char* name, ComponentTriggerMode mode, int period_us, const char* watch_topics)
    : impl_(std::make_unique<Impl>()) {
    impl_->name_ = (name && *name) ? name : "component";
    impl_->period_us_ = period_us;
    impl_->watched_topics_ = watch_topics ? watch_topics : "";
    impl_->mode_ = mode;
}

Component::~Component() = default;

const char* Component::name() const {
    return impl_->name_.c_str();
}

ComponentState Component::state() const {
    return impl_->state_;
}

int Component::run() {
    component_stop_requested.store(false, std::memory_order_relaxed);
    install_component_signal_handlers();

    if (const char* env_name = std::getenv("ALAKA_COMPONENT_NAME")) {
        impl_->name_ = env_name;
    }
    if (impl_->period_us_ <= 0) {
        if (const char* env_period = std::getenv("ALAKA_PERIOD_US")) {
            impl_->period_us_ = std::atoi(env_period);
        }
    }

    int rc = on_configure();
    if (rc != 0) {
        impl_->state_ = ComponentState::ERROR;
        return rc;
    }
    impl_->state_ = ComponentState::CONFIGURED;

    rc = on_init();
    if (rc != 0) {
        impl_->state_ = ComponentState::ERROR;
        on_deinit();
        return rc;
    }
    impl_->state_ = ComponentState::ACTIVE;

    const std::string ready_file =
        std::string("/dev/shm/alaka_runtime_ready_") + impl_->name_;
    ::unlink(ready_file.c_str());
    alaka::filesystem::write_file(ready_file, std::to_string(static_cast<long long>(::getpid())));
    ::chmod(ready_file.c_str(), 0666);

    int lifecycle_rc = 0;
    if (impl_->mode_ == ComponentTriggerMode::PERIODIC) {
        int64_t period_ns = static_cast<int64_t>(impl_->period_us_) * 1000LL;
        if (period_ns <= 0) period_ns = 1000000000LL;

        int64_t next_ns = alaka::rt::clock_now_ns() + period_ns;
        while (!component_stop_requested.load(std::memory_order_relaxed)) {
            rc = on_update();
            if (rc != 0) {
                impl_->state_ = ComponentState::ERROR;
                lifecycle_rc = rc;
                break;
            }

            next_ns += period_ns;
            if (next_ns < alaka::rt::clock_now_ns()) {
                next_ns = alaka::rt::clock_now_ns() + period_ns;
            }

            while (!component_stop_requested.load(std::memory_order_relaxed) &&
                   alaka::rt::clock_now_ns() < next_ns) {
                alaka::rt::sleep_until_ns(next_ns);
            }
        }
    } else {
        while (!component_stop_requested.load(std::memory_order_relaxed)) {
            alaka::rt::sleep_ns(50LL * 1000000LL);
        }
    }

    impl_->state_ = ComponentState::DEACTIVATED;
    rc = on_deinit();
    ::unlink(ready_file.c_str());
    if (rc != 0) {
        impl_->state_ = ComponentState::ERROR;
        return rc;
    }
    return lifecycle_rc;
}