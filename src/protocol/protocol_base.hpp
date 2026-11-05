/**
 * @brief Transport protocol Base class def in here.
 * @author Eksan Ablikim
 * @note Component class, RPC Manager class and Scheduler use this API
 */
#ifndef ALAKA_RUNTIME_SRC_PROTOCOL_PROTOCOL_BASE_HPP
#define ALAKA_RUNTIME_SRC_PROTOCOL_PROTOCOL_BASE_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace alaka {
namespace protocol {

struct RequestUnit {
    const void * data_ = nullptr;
    uint32_t len_ = 0;
    int64_t  seq_ = 0;
    void * context_ = nullptr;
};

struct TopicDescription {
    std::string name_;
    uint32_t size_ = 0;
    uint64_t hash_ = 0;
    uint32_t queue_capacity_ = 0;
};

typedef TopicDescription EventDescription ;

struct ServiceDescription {
    std::string name_;
    uint32_t req_size_ = 0;
    uint32_t rep_size_ = 0;
    uint64_t req_hash_ = 0;
    uint64_t rep_hash_ = 0;
    uint32_t queue_capacity_ = 0;
};

class PublisherPortBase {
public:
    virtual ~PublisherPortBase() = default;
    virtual void * loan(size_t len) = 0;
    virtual bool commit(void * data) = 0;
    virtual bool publish(const void * data, size_t len) = 0;

    virtual bool hasSubscribers() const = 0;
    virtual const TopicDescription& description() const = 0;
};

class SubscriberPortBase {
public:
    virtual ~SubscriberPortBase() = default;
    virtual int get(void * buffer, size_t capacity, uint64_t * ns, uint32_t * seq, size_t * outlen = nullptr) = 0;
    virtual int get_next(void * buffer, size_t capacity, uint64_t * ns, uint32_t * seq) = 0;

    virtual int raw_next(void * buffer, size_t capacity, uint64_t * ns, uint32_t * seq) = 0;

    virtual void release(const void * data) = 0;

    virtual bool hasData() const = 0;
    virtual const TopicDescription& description() const = 0;
};

class ServicePortBase {
public:
    virtual ~ServicePortBase() = default;
    virtual bool accept(RequestUnit& out) = 0;
    virtual bool response(const RequestUnit& req, const void * data, size_t len) = 0;
    virtual void drop(const RequestUnit& req) = 0;
    virtual bool hasRequest() const = 0;
    virtual const ServiceDescription& description() const = 0;
};

class ClientPortBase {
public:
    virtual ~ClientPortBase() = default;
    virtual int send(const void * data, size_t len) = 0;
    virtual int poll(int req_id, void * buffer, size_t capacity, size_t * outlen, int * gid) = 0;
    virtual bool hasResponse() const = 0;
    virtual bool connected() const = 0;
    virtual const ServiceDescription& description() const = 0;
};

enum class ConnectionChannel { Iceoryx, None };

class TransportProtoBase {
public:
    virtual ~TransportProtoBase() = default;

    virtual bool init(const std::string& protocol) = 0;
    virtual void deinit() = 0;
    
    virtual std::unique_ptr<PublisherPortBase> create_pub(const TopicDescription& desc) = 0;
    virtual std::unique_ptr<SubscriberPortBase> create_sub(const TopicDescription& desc) = 0;
    virtual std::unique_ptr<ServicePortBase> create_srv(const ServiceDescription& desc) = 0;
    virtual std::unique_ptr<ClientPortBase> create_cli(const ServiceDescription& desc) = 0;
};
namespace iceoryx {
TransportProtoBase& iceoryxProtocolPort();
}
}

}

#endif