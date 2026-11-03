cmake_minimum_required(VERSION 3.16)
project(AlakaRuntimeSDK VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

# 统一产物布局: bin/ 放可执行文件, lib/ 放库
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# ---------------------------------------------------------------------------
# 源码头文件目录
#   include/   : SDK 公共头 (system.hpp / type.hpp / component.hpp)
#   launcher/  : launcher 私有头 (component_config.hpp / component_manager.hpp)
# ---------------------------------------------------------------------------
set(ALAKA_SOURCE_DIR   "${CMAKE_CURRENT_SOURCE_DIR}")
set(ALAKA_INCLUDE_DIR  "${ALAKA_SOURCE_DIR}/include")
set(ALAKA_LAUNCHER_DIR "${ALAKA_SOURCE_DIR}/launcher")

# ---------------------------------------------------------------------------
# 3rdparty: vendored iceoryx (static) + libacl from sysroot
# ---------------------------------------------------------------------------
set(ICEORYX_ROOT "${ALAKA_SOURCE_DIR}/3rdparty/iceoryx-install" CACHE PATH "iceoryx install prefix")

add_library(iceoryx INTERFACE)
target_include_directories(iceoryx INTERFACE "${ICEORYX_ROOT}/include/iceoryx/v2.0.8")
file(GLOB ICEORYX_LIBS "${ICEORYX_ROOT}/lib/*.a")

set(EBR_SYSROOT "${ALAKA_SOURCE_DIR}/3rdparty/sysroot/usr")
find_library(ACL_LIB acl PATHS "${EBR_SYSROOT}/lib/x86_64-linux-gnu" NO_DEFAULT_PATH)
if(NOT ACL_LIB)
    find_library(ACL_LIB acl)
endif()
target_link_directories(iceoryx INTERFACE "${EBR_SYSROOT}/lib/x86_64-linux-gnu")

target_link_libraries(iceoryx INTERFACE
    -Wl,--start-group ${ICEORYX_LIBS} -Wl,--end-group
    ${ACL_LIB} pthread dl rt)

# yaml-cpp: 0.8 提供 yaml-cpp::yaml-cpp, 旧版本只有 yaml-cpp
find_package(yaml-cpp REQUIRED)
if(TARGET yaml-cpp::yaml-cpp)
    set(ALAKA_YAML_CPP yaml-cpp::yaml-cpp)
else()
    set(ALAKA_YAML_CPP yaml-cpp)
endif()

# 统一编译告警选项
add_library(alaka_build_options INTERFACE)
target_compile_options(alaka_build_options INTERFACE
    -Wall -Wextra -Wno-unused-parameter)

# ---------------------------------------------------------------------------
# alaka_runtime : 组件运行时 SDK 库 (业务组件链接此库并实现 runtime_init_and_run_cpp)
#   src/component.cpp 内部使用 "include/component.hpp" 形式, 故 PRIVATE 加源根目录;
#   对外暴露 include/ 供业务组件 #include <component.hpp>。
#   暂用 STATIC: SHARED 需要 Component 全部符号定义完成 (见后续修复)。
# ---------------------------------------------------------------------------
add_library(alaka_runtime STATIC
    src/component.cpp
)
target_include_directories(alaka_runtime
    PUBLIC  "${ALAKA_INCLUDE_DIR}"
    PRIVATE "${ALAKA_SOURCE_DIR}"
)
target_link_libraries(alaka_runtime
    PUBLIC  iceoryx
    PRIVATE alaka_build_options
)

# ---------------------------------------------------------------------------
# alaka_launcher : 系统启动器, 解析配置 + 拉起 RouDi 与各组件进程
# ---------------------------------------------------------------------------
add_executable(alaka_launcher
    launcher/launcher.cpp
    launcher/component_manager.cpp
    launcher/component_config.cpp
)
target_include_directories(alaka_launcher PRIVATE
    "${ALAKA_INCLUDE_DIR}"
    "${ALAKA_LAUNCHER_DIR}"
)
target_link_libraries(alaka_launcher PRIVATE
    ${ALAKA_YAML_CPP}
    alaka_build_options
)

# ---------------------------------------------------------------------------
# alaka_cli : 命令行入口 (run/stop/version)
# ---------------------------------------------------------------------------
add_executable(alaka_cli
    cli/cli.cpp
)
target_include_directories(alaka_cli PRIVATE
    "${ALAKA_INCLUDE_DIR}"
)
target_link_libraries(alaka_cli PRIVATE
    alaka_build_options
)

# ---------------------------------------------------------------------------
# alaka_roudi : iceoryx RouDi daemon (共享内存段 + mempool 配置)
# ---------------------------------------------------------------------------
add_executable(alaka_roudi
    roudi/roudi_launch.cpp
)
target_link_libraries(alaka_roudi PRIVATE
    iceoryx
    alaka_build_options
)