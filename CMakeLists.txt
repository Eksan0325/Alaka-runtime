cmake_minimum_required(VERSION 3.16)
project(AlakaRuntimeSDK VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(ICEORYX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/iceoryx-install" CACHE PATH "iceoryx install prefix")
find_package(yaml-cpp REQUIRED)

add_library(iceoryx INTERFACE)
target_include_directories(iceoryx INTERFACE "${ICEORYX_ROOT}/include/iceoryx/v2.0.8")
file(GLOB ICEORYX_LIBS "${ICEORYX_ROOT}/lib/*.a")

set(EBR_SYSROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/sysroot/usr")
find_library(ACL_LIB acl PATHS "${EBR_SYSROOT}/lib/x86_64-linux-gnu" NO_DEFAULT_PATH)
if(NOT ACL_LIB)
    find_library(ACL_LIB acl)
endif()
target_link_directories(iceoryx INTERFACE "${EBR_SYSROOT}/lib/x86_64-linux-gnu")

target_link_libraries(iceoryx INTERFACE
    -Wl,--start-group ${ICEORYX_LIBS} -Wl,--end-group
    ${ACL_LIB} pthread dl rt)

add_executable(alaka_roudi roudi/roudi_launch.cpp)
target_link_libraries(alaka_roudi PRIVATE iceoryx)
target_compile_options(alaka_roudi PRIVATE -Wall -Wextra -Wno-unused-parameter)