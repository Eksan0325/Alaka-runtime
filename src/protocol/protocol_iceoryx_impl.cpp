#include "protocol/protocol_base.hpp"
#include "system.hpp"

#include <atomic>
#include <memory>
#include <cstddef>
#include <string>
#include <cstring>
#include <mutex>

#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "iceoryx_posh/popo/client_options.hpp"
#include "iceoryx_posh/popo/publisher_options.hpp"
#include "iceoryx_posh/popo/rpc_header.hpp"
#include "iceoryx_posh/popo/server_options.hpp"
#include "iceoryx_posh/popo/subscriber_options.hpp"
#include "iceoryx_posh/popo/untyped_client.hpp"
#include "iceoryx_posh/popo/untyped_publisher.hpp"
#include "iceoryx_posh/popo/untyped_server.hpp"
#include "iceoryx_posh/popo/untyped_subscriber.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"

namespace alaka {
namespace protocol {
namespace iceoryx {

namespace {

struct SplitName {
    iox::capro::IdString_t service_;
    iox::capro::IdString_t instance_;
};

SplitName splitIoxName(const std::string& name) {
    auto ps = name.find('/');
    if (ps == std::string::npos || ps == 0 || ps + 1 >= name.size()) {
        return {iox::capro::IdString_t(iox::cxx::TruncateToCapacity, name.c_str()), iox::capro::IdString_t(iox::cxx::TruncateToCapacity, "default")};
    }
    auto s = name.substr(0, ps);
    auto i = name.substr(ps + 1);
    return {iox::capro::IdString_t(iox::cxx::TruncateToCapacity, s.c_str()), iox::capro::IdString_t(iox::cxx::TruncateToCapacity, i.c_str())};
}

iox::capro::ServiceDescription makeServiceDesc(const std::string& name, const char * kind) {
    auto split = splitIoxName(name);
    return iox::capro::ServiceDescription(split.service_, split.instance_, iox::capro::IdString_t(iox::cxx::TruncateToCapacity, kind));
}

uint64_t dataSeq(const void * data) {
    const auto * header = iox::mepoo::ChunkHeader::fromUserPayload(data);
    return header ? header->sequenceNumber() : 0;
}
uint64_t dataSize(const void * data) {
    const auto * header = iox::mepoo::ChunkHeader::fromUserPayload(data);
    return header ? header->userPayloadSize() : 0;
}

}

class IceoryxPublisherImpl final : public PublisherPortBase {
public:
    IceoryxPublisherImpl(const TopicDescription& desc, const iox::popo::PublisherOptions& opt)
        : desc_(desc), publisher_(makeServiceDesc(desc.name_, "data"), opt) {}

    void * loan(size_t len) override;
    bool commit(void * data) override;
    bool publish(const void * data, size_t len) override;
    
    bool hasSubscribers() const override;
    const TopicDescription& description() const override;
    
private:
    TopicDescription desc_;
    iox::popo::UntypedPublisher publisher_;
};

void * IceoryxPublisherImpl::loan(size_t len)  {
    if (len > UINT32_MAX) return nullptr;
    auto result = publisher_.loan(static_cast<uint32_t>(len));
    if (result.has_error()) return nullptr;
    return * result;
}

// Will be write data to chunk, Zero-Copy API
bool IceoryxPublisherImpl::commit(void * data) {
    if (!data) return false;
    publisher_.publish(data);
    return true;
}

// Will be copy once user data to shared memory chunk
bool IceoryxPublisherImpl::publish(const void * data, size_t len) {
    void * p = this->loan(len);
    if (!p) return false;
    std::memcpy(p, data, len); //Copy once
    publisher_.publish(p);
    return true;
}

bool IceoryxPublisherImpl::hasSubscribers() const {
    return publisher_.hasSubscribers();
}

const TopicDescription& IceoryxPublisherImpl::description() const {
    return desc_;
}

class IceoryxSubscriberImpl final : public SubscriberPortBase {
public:
    IceoryxSubscriberImpl(const TopicDescription& desc, const iox::popo::SubscriberOptions& opt)
        : desc_(desc), subscriber_(makeServiceDesc(desc.name_, "data"), opt) 
    {
        recv_sq = 0;
    }

    int get(void * buffer, size_t capacity, uint64_t * ns, uint32_t * seq, size_t * outlen = nullptr) override;
    int get_next(void * buffer, size_t capacity, uint64_t * ns, uint32_t * seq) override;
    int raw_next(void * buffer, size_t capacity, uint64_t * ns, uint32_t * seq) override;
    void release(const void * data) override;
    
    bool hasData() const override;
    const TopicDescription& description() const override;

private:
    TopicDescription desc_;
    iox::popo::UntypedSubscriber subscriber_;
    uint32_t recv_sq;
};

int IceoryxSubscriberImpl::get(void * buffer, size_t capacity, uint64_t * ns, uint32_t * seq, size_t * outlen) {
    const void * lastptr = nullptr;
    uint32_t lastsize = 0;
    while(subscriber_.hasData()) {
        auto result = subscriber_.take();
        if (result.has_error()) break;
        const void* p = *result;
        if (lastptr) subscriber_.release(lastptr);
        lastptr = p;
        lastsize = dataSize(p);
    }
    if (lastptr == nullptr) return -1;
    if (lastsize > capacity) {
        subscriber_.release(lastptr);
        return -1;
    }
    //Copy shared memory chunk data to user data pointer
    std::memcpy(buffer, lastptr, lastsize);
    if (ns) *ns = alaka::rt::clock_now_ns();
    if (seq) *seq = ++recv_sq;
    if (outlen) *outlen = lastsize;
    subscriber_.release(lastptr);
    return 0;
}

int IceoryxSubscriberImpl::get_next(void * buffer, size_t capacity, uint64_t * ns, uint32_t * seq) {
    if (!subscriber_.hasData()) return -1;
    auto result = subscriber_.take();
    if (result.has_error()) return -1;
    const void * p = * result;
    uint32_t size = dataSize(p);
    if (size > capacity) {
        subscriber_.release(p);
        return -1;
    }
    //Copy shared memory chunk data to user data pointer
    std::memcpy(buffer, p, size);
    if (ns) *ns = alaka::rt::clock_now_ns();
    if (seq) *seq = ++recv_sq;
    subscriber_.release(p);
    return 0;
}

int IceoryxSubscriberImpl::raw_next(void * buffer, size_t capacity, uint64_t * ns, uint32_t * seq) {
    if (!buffer) return -1;
    auto result = subscriber_.take();
    if (result.has_error()) return -1;
    const void * p = *result;
    const size_t len = static_cast<size_t>(dataSize(p));
    if (len > capacity) {
        release(p);   
        return -1;
    }
    //Hand the shared memory pointer back, no copy. Ownership stays with the caller until release().
    *static_cast<const void **>(buffer) = p;
    
    if (ns) *ns = alaka::rt::clock_now_ns();
    if (seq) *seq = ++recv_sq;
    return static_cast<int>(len);
}

void IceoryxSubscriberImpl::release(const void * data) {
    if (data) subscriber_.release(data);
}

bool IceoryxSubscriberImpl::hasData() const {
    return subscriber_.hasData();
}

const TopicDescription& IceoryxSubscriberImpl::description() const { 
    return desc_;
}

class IceoryxServiceImpl final : public ServicePortBase {
public:
    IceoryxServiceImpl(const ServiceDescription& desc, const iox::popo::ServerOptions& opt)
        : desc_(desc), server_(makeServiceDesc(desc.name_, "rpc"), opt) {}
    
    bool accept(RequestUnit& out) override;
    bool response(const RequestUnit& req, const void * data, size_t len) override;
    void drop(const RequestUnit& req) override;
    bool hasRequest() const override;
    const ServiceDescription& description() const override;
private:
    ServiceDescription desc_;
    iox::popo::UntypedServer server_;
};

bool IceoryxServiceImpl::accept(RequestUnit& out) {
    if (!server_.hasRequests()) return false;
    auto result = server_.take();
    if (result.has_error()) return false;
    
    const void * p = *result;
    out.data_ = p;
    out.len_  = dataSize(p);
    const auto* header = iox::popo::RequestHeader::fromPayload(p);
    out.seq_ = header ? static_cast<int64_t>(header->getSequenceId()) : 0;
    out.context_ = const_cast<iox::popo::RequestHeader*>(header);

    return true;
}

bool IceoryxServiceImpl::response(const RequestUnit& req, const void * data, size_t len) {
    const auto* header = static_cast<const iox::popo::RequestHeader*>(req.context_);
    if (!header) return false;
    auto result = server_.loan(header, static_cast<uint32_t>(len), static_cast<uint32_t>(alignof(std::max_align_t)));
    
    if (result.has_error()) {
        server_.releaseRequest(req.data_);
        return false;
    }
    void * response = *result;
    if (data && len) std::memcpy(response, data, len); //Copy response data once
    auto send_result = server_.send(response);
    server_.releaseRequest(req.data_);
    return !send_result.has_error();
}

void IceoryxServiceImpl::drop(const RequestUnit& req) {
    server_.releaseRequest(req.data_);
}

bool IceoryxServiceImpl::hasRequest() const { return server_.hasRequests(); }

const ServiceDescription& IceoryxServiceImpl::description() const { return desc_; }

class IceoryxClientImpl final : public ClientPortBase {
public:
    IceoryxClientImpl(const ServiceDescription& desc, const iox::popo::ClientOptions& opt)
        : desc_(desc), client_(makeServiceDesc(desc.name_, "rpc"), opt) {}

    int send(const void * data, size_t len) override;
    int poll(int req_id, void * buffer, size_t capacity, size_t * outlen, int * gid) override;
    
    bool hasResponse() const override;
    bool connected() const override;
    const ServiceDescription& description() const override;
private:
    ServiceDescription desc_;
    iox::popo::UntypedClient client_;
    int64_t id_ = 0;
};

int IceoryxClientImpl::send(const void * data, size_t len) {
    if (client_.getConnectionState() != iox::ConnectionState::CONNECTED) return -1;
    auto result = client_.loan(static_cast<uint32_t>(len), alignof(std::max_align_t));
    if (result.has_error()) return -1;
    void * p = *result;
    if (data && len) std::memcpy(p, data, len); //Copy response data once
    int64_t id = ++id_;
    auto * header = iox::popo::RequestHeader::fromPayload(p);
    if (header) header->setSequenceId(id);
    auto send_result = client_.send(p);
    if (send_result.has_error()) {
        client_.releaseRequest(p);
        return -1;
    }
    return static_cast<int>(id);
}

int IceoryxClientImpl::poll(int req_id, void * buffer, size_t capacity, size_t * outlen, int * gid) {
    if (!client_.hasResponses()) return -1;
    auto result = client_.take();
    if (result.has_error()) return -1;
    const void * p = * result;
    const auto * header = iox::popo::ResponseHeader::fromPayload(p);
    int id = header ? static_cast<int>(header->getSequenceId()) : 0;
    uint32_t size = dataSize(p);
    // If req_id != id, drop the response of send, it's not same request
    if (req_id > 0 && id != req_id) {
        client_.releaseResponse(p);
        return 0;
    }
    if (size > capacity) {
        client_.releaseResponse(p);
        return -1;
    }
    std::memcpy(buffer, p, size);
    if (outlen) *outlen = size;
    if (gid) *gid = id;
    client_.releaseResponse(p);
    return 0;
}

bool IceoryxClientImpl::hasResponse() const { return client_.hasResponses(); }

bool IceoryxClientImpl::connected() const { return client_.getConnectionState() == iox::ConnectionState::CONNECTED; }

const ServiceDescription& IceoryxClientImpl::description() const { return desc_;}

class IceoryxProtoImpl final : public TransportProtoBase {
public:
    bool init(const std::string& protocol) override;
    void deinit() override;
    
    std::unique_ptr<PublisherPortBase> create_pub(const TopicDescription& desc) override;
    std::unique_ptr<SubscriberPortBase> create_sub(const TopicDescription& desc) override;
    std::unique_ptr<ServicePortBase> create_srv(const ServiceDescription& desc) override;
    std::unique_ptr<ClientPortBase> create_cli(const ServiceDescription& desc) override;

    bool isReady() const;
private:
    std::mutex mtx_;
    std::atomic_bool ready_ {false};
};

bool IceoryxProtoImpl::init(const std::string& protocol) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (ready_.load(std::memory_order_relaxed)) return true;
    iox::runtime::PoshRuntime::initRuntime(iox::RuntimeName_t(iox::cxx::TruncateToCapacity, protocol.c_str()));
    ready_.store(true);
    return true;
}

void IceoryxProtoImpl::deinit() { ready_.store(false); }

bool IceoryxProtoImpl::isReady() const { return ready_;}

std::unique_ptr<PublisherPortBase> IceoryxProtoImpl::create_pub(const TopicDescription& d)  {
    iox::popo::PublisherOptions opt;
    opt.historyCapacity = 1;
    return std::make_unique<IceoryxPublisherImpl>(d, opt);
}

std::unique_ptr<SubscriberPortBase> IceoryxProtoImpl::create_sub(const TopicDescription& d)  {
    iox::popo::SubscriberOptions opt;
    opt.queueCapacity = static_cast<uint64_t>(d.queue_capacity_ == 0 ? 8 : d.queue_capacity_);
    return std::make_unique<IceoryxSubscriberImpl>(d, opt);
}

std::unique_ptr<ServicePortBase> IceoryxProtoImpl::create_srv(const ServiceDescription& d) {
    iox::popo::ServerOptions opt;
    opt.requestQueueCapacity = d.queue_capacity_ == 0 ? 16 : d.queue_capacity_;
    return std::make_unique<IceoryxServiceImpl>(d, opt);
}

std::unique_ptr<ClientPortBase> IceoryxProtoImpl::create_cli(const ServiceDescription& d)  {
    iox::popo::ClientOptions opt;
    opt.responseQueueCapacity = d.queue_capacity_ == 0 ? 16 : d.queue_capacity_;
    opt.connectOnCreate = true;
    return std::make_unique<IceoryxClientImpl>(d, opt);
}

TransportProtoBase& iceoryxProtocolPort() {
    static IceoryxProtoImpl ins;
    return ins;
}

} //namespace alaka::protocol::iceoryx
} //namespace alaka::protocol

}