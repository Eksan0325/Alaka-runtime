#include "include/component.hpp"

#include <string>

using namespace alaka;
struct Component::Impl {
    std::string name_;
    std::string watched_topics_;
    std::string version_;
    int period_us_ = 0;
};

Component::Component(const char* name, ComponentTriggerMode mode, int period_us, const char* watch_topics)
    : impl_(std::make_unique<Impl>()) {
    impl_->name_ = (name && *name) ? name : "component";
    impl_->period_us_ = period_us;
    impl_->watched_topics_ = watch_topics ? watch_topics : "";
}