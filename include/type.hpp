/**
 * @brief Alaka Runtime typedef
 * @author Eksan Ablikim
 */
#ifndef ALAKA_RUNTIME_INCLUDE_TYPE_HPP
#define ALAKA_RUNTIME_INCLUDE_TYPE_HPP

#include <cstdint>
#include <cstddef>
#include <functional>

namespace alaka {

constexpr uint64_t ALAKA_FNV_OFFSET = 1469598103934665603ULL;
constexpr uint64_t ALAKA_FNV_PRIME  = 1099511628211ULL;

constexpr uint64_t fnv1a(uint64_t h, size_t val) noexcept {
    h ^= static_cast<uint64_t>(val);
    h *= ALAKA_FNV_PRIME;
    return h;
}

template<typename T>
struct MetaTypeLayout {
    static constexpr const char* name() { return nullptr;}
    static constexpr uint64_t    hash() { return 0;}
};

#define ALAKA_BEGIN_LAYOUT(T) \
    namespace alaka { \
    template<> struct MetaTypeLayout<T> { \
        static constexpr const char* name() { return #T; } \
        static constexpr uint64_t    hash() { \
            using TT = T;\
            uint64_t _h = ALAKA_FNV_OFFSET; 

#define ALAKA_FIELD(F) \
            _h = alaka::fnv1a(_h, offsetof(TT, F)); \
            _h = alaka::fnv1a(_h, sizeof(decltype(TT::F)));

#define ALAKA_END_LAYOUT() \
            return _h; \
        } \
    }; \
    }

#define ALAKA_LAYOUT_1(T, f1)                 ALAKA_FIELD(f1)
#define ALAKA_LAYOUT_2(T, f1, f2)             ALAKA_FIELD(f1) ALAKA_FIELD(f2)
#define ALAKA_LAYOUT_3(T, f1, f2, f3)         ALAKA_FIELD(f1) ALAKA_FIELD(f2) ALAKA_FIELD(f3)
#define ALAKA_LAYOUT_4(T, f1, f2, f3, f4)     ALAKA_FIELD(f1) ALAKA_FIELD(f2) ALAKA_FIELD(f3) ALAKA_FIELD(f4)
#define ALAKA_LAYOUT_5(T, f1, f2, f3, f4, f5) \
    ALAKA_FIELD(f1) ALAKA_FIELD(f2) ALAKA_FIELD(f3) ALAKA_FIELD(f4) ALAKA_FIELD(f5)
#define ALAKA_LAYOUT_6(T, f1, f2, f3, f4, f5, f6) \
    ALAKA_FIELD(f1) ALAKA_FIELD(f2) ALAKA_FIELD(f3) ALAKA_FIELD(f4) ALAKA_FIELD(f5) ALAKA_FIELD(f6)
#define ALAKA_LAYOUT_7(T, f1, f2, f3, f4, f5, f6, f7) \
    ALAKA_FIELD(f1) ALAKA_FIELD(f2) ALAKA_FIELD(f3) ALAKA_FIELD(f4) ALAKA_FIELD(f5) ALAKA_FIELD(f6) ALAKA_FIELD(f7)
#define ALAKA_LAYOUT_8(T, f1, f2, f3, f4, f5, f6, f7, f8) \
    ALAKA_FIELD(f1) ALAKA_FIELD(f2) ALAKA_FIELD(f3) ALAKA_FIELD(f4) ALAKA_FIELD(f5) ALAKA_FIELD(f6) \
    ALAKA_FIELD(f7) ALAKA_FIELD(f8)

#define	ALAKA_LAYOUT_PICK(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME

#define ALAKA_LAYOUT(T, ...)                                                  \
    ALAKA_BEGIN_LAYOUT(T)                                                     \
    ALAKA_LAYOUT_PICK(__VA_ARGS__,                                            \
        ALAKA_LAYOUT_8, ALAKA_LAYOUT_7, ALAKA_LAYOUT_6, ALAKA_LAYOUT_5,             \
        ALAKA_LAYOUT_4, ALAKA_LAYOUT_3, ALAKA_LAYOUT_2, ALAKA_LAYOUT_1)             \
    (T, __VA_ARGS__)                                                       \
    ALAKA_END_LAYOUT()

#define ALAKA_LAYOUT_TOO_MANY_FIELDS \
    static_assert(false, "ALAKA_LAYOUT Only supports up to 8 fields; please split into sub-structs or use ALAKA_BEGIN_LAYOUT/ALAKA_FIELD/ALAKA_END_LAYOUT")


enum class ComponentState : uint8_t { UNCONFIGURED, CONFIGURED, ACTIVE, DEACTIVATED, ERROR };
enum class ComponentLifeState : uint8_t { CREATED, CONFIGURED, ACTIVE, DEACTIVATED, ERROR, DESTROYED };
enum class ComponentTriggerMode : uint8_t { PERIODIC, EVENT, DATA };

struct MessageMeta {
    uint64_t timestamp_ns = 0;
    uint32_t seq = 0;
};

enum class RpcServiceCallResult : int {
    OK        = 0,
    Timeout   = 1,
    PortError = 2,
    NotFound  = 3,
    Pending   = 4,
    Busy      = 5,
    Rejected  = 6
};

using RpcServiceHandler = std::function<int(const void* req, size_t reqlen, void* rep, size_t* replen)>;
using RpcServiceResponsiveFunction = std::function<void(int32_t status, const void* data, size_t len)>;
using RpcEnqueueFunction = std::function<void(uint32_t seq, uint32_t cid, const void* req, size_t reqlen, RpcServiceResponsiveFunction reply)>;
}

#endif