/**
 * @brief Alaka Runtime Components Basic Interface
 * @author Eksan Ablikim
 * @note All business components inherit from Component and override on_configure /
 * on_activate / on_update / on_deactivate / on_cleanup / on_error. Register
 * Pub/Sub/Event/RPC in on_configure, and perform periodic logic in on_update.
 * @note Use EBR_REGISTER_*_TRIGGER to export entry function symbol runtime_init_and_run_cpp。
 */
#ifndef ALAKA_RUNTIME_INCLUDE_COMPONENT_HPP
#define ALAKA_RUNTIME_INCLUDE_COMPONENT_HPP

#include <cstdint>
#include <cstddef>
#include <string>
#include <cstdio>

#include <functional>
#include <memory>

#include <type.hpp>

namespace alaka {

class Subscriber;

class Component {
public:
    Component(const char* name, ComponentTriggerMode mode, int period_us = 0, const char* watch_topics = nullptr);
    virtual ~Component();

    const char* name() const;
    ComponentState state() const;
    
    /**
     * @brief Publisher/Subscriber API
     */
    int pub_create(const char* topic, size_t data_size,
                   const char* type_name = nullptr, uint64_t layout_hash = 0) { return -1;}
    int sub_create(const char* topic, size_t data_size,
                   const char* type_name = nullptr, uint64_t layout_hash = 0) { return -1;}

    template<typename T>
    int pub_create(const char* topic) {
        return pub_create(topic, sizeof(T),
                          MetaTypeLayout<T>::name(), MetaTypeLayout<T>::hash());
    }
    template<typename T>
    int sub_create(const char* topic) {
        return sub_create(topic, sizeof(T), 
                          MetaTypeLayout<T>::name(), MetaTypeLayout<T>::hash());
    }

    int publish(int handle, const void* data, size_t len) { return -1;}
    int subscribe(int handle, void* buf, size_t buf_len) { return -1;}
    /**
     * @brief Event Drive API
     */
    void event_wait(Subscriber* sub) {}
    void event_wait(int subscribe_handle) {}

    int  event_create(const char* topic) { return -1;}
    int  event_register(const char* topic) { return -1;}
    int  event_emit(int handle) { return -1;}
    
    bool event_ready(int handle) const { return false;}
    int  event_info(int handle, MessageMeta& meta) const { return -1;}

    /**
     * @brief RPC Service API
     */
    int srv_create(const char * service, size_t req_size, size_t rep_size, 
                   RpcServiceHandler handler, const char * req_type = nullptr,
                   const char * rep_type = nullptr, uint64_t req_hash = 0, uint64_t rep_hash = 0) { return -1;}
    int cli_create(const char * service, size_t req_size, size_t rep_size,
                   const char * req_type = nullptr, const char * rep_type = nullptr,
                   uint64_t req_hash = 0, uint64_t rep_hash = 0) { return -1;}

    int call(int handle, const void* req, void* rep, int timeout_ms = 5000, uint8_t priority = 0) { return -1;} 
    int async_call(int handle, const void* req, uint8_t priority = 0) { return -1;}
    int async_poll(int handle, int req_id, void* rep) { return -1;}

    /**
     * @brief Block until the end of component's lifecycle, return code of process
     */
    int run(); //blocked loop
protected:
    virtual int on_configure() { return 0; }
    virtual int on_init()      { return 0; }
    virtual int on_update()    { return 0; }
    virtual int on_deinit()    { return 0; }
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}

#define ALAKA_REGISTER_PERIODIC_TRIGGER(C, N, V, P) \
    extern "C" int runtime_init_and_run_cpp(void * handle) { \
        (void)handle; \
        (void)P; \
        auto component = std::make_unique<C>();\
        return component->run();\
    }

#define ALAKA_REGISTER_EVENT_TRIGGER(C, N, V) ALAKA_REGISTER_PERIODIC_TRIGGER(C, N, V, 0)
#define ALAKA_REGISTER_DATA_TRIGGER(C, N, V) ALAKA_REGISTER_EVENT_TRIGGER(C, N, V)
#endif
