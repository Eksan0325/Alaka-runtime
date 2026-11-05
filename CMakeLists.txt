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

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set(ALAKA_SOURCE_DIR   "${CMAKE_CURRENT_SOURCE_DIR}")
set(ALAKA_INCLUDE_DIR  "${ALAKA_SOURCE_DIR}/include")
set(ALAKA_LAUNCHER_DIR "${ALAKA_SOURCE_DIR}/launcher")
set(ALAKA_PROTOCOL_DIR "${ALAKA_SOURCE_DIR}/src/protocol")

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


find_package(yaml-cpp REQUIRED)
if(TARGET yaml-cpp::yaml-cpp)
    set(ALAKA_YAML_CPP yaml-cpp::yaml-cpp)
else()
    set(ALAKA_YAML_CPP yaml-cpp)
endif()


add_library(alaka_build_options INTERFACE)
target_compile_options(alaka_build_options INTERFACE
    -Wall -Wextra -Wno-unused-parameter)


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

add_library(alaka_protocol STATIC
    src/protocol/protocol_iceoryx_impl.cpp
)

target_include_directories(alaka_protocol
    PUBLIC  "${ALAKA_PROTOCOL_DIR}" "${ALAKA_INCLUDE_DIR}"
    PRIVATE "${ALAKA_SOURCE_DIR}/src"
)
target_link_libraries(alaka_protocol
    PUBLIC  iceoryx
    PRIVATE alaka_build_options
)

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
# alaka_roudi : iceoryx RouDi daemon 
# ---------------------------------------------------------------------------
add_executable(alaka_roudi
    roudi/roudi_launch.cpp
)
target_link_libraries(alaka_roudi PRIVATE
    iceoryx
    alaka_build_options
)