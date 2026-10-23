# Install script for directory: /home/eksan-rokae-com/inexus/inex_sre/third_party/iceoryx/iceoryx_hoofs

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/home/eksan-rokae-com/inexus/inex_sre/third_party/iceoryx-install")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/eksan-rokae-com/inexus/inex_sre/third_party/iceoryx-build/hoofs/platform/cmake_install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "dev" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/iceoryx/v2.0.8" TYPE DIRECTORY FILES "/home/eksan-rokae-com/inexus/inex_sre/third_party/iceoryx/iceoryx_hoofs/platform/linux/include/")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "bin" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/eksan-rokae-com/inexus/inex_sre/third_party/iceoryx-build/hoofs/libiceoryx_hoofs.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "bin" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/eksan-rokae-com/inexus/inex_sre/third_party/iceoryx-build/hoofs/platform/libiceoryx_platform.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "dev" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/iceoryx/v2.0.8" TYPE DIRECTORY FILES "/home/eksan-rokae-com/inexus/inex_sre/third_party/iceoryx/iceoryx_hoofs/include/")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/iceoryx_hoofs" TYPE FILE FILES
    "/home/eksan-rokae-com/inexus/inex_sre/third_party/iceoryx-build/hoofs/iceoryx_hoofsConfigVersion.cmake"
    "/home/eksan-rokae-com/inexus/inex_sre/third_party/iceoryx-build/hoofs/iceoryx_hoofsConfig.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/iceoryx_hoofs/iceoryx_hoofsTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/iceoryx_hoofs/iceoryx_hoofsTargets.cmake"
         "/home/eksan-rokae-com/inexus/inex_sre/third_party/iceoryx-build/hoofs/CMakeFiles/Export/2df1df3350d90c929624816c3cc2d98f/iceoryx_hoofsTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/iceoryx_hoofs/iceoryx_hoofsTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/iceoryx_hoofs/iceoryx_hoofsTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/iceoryx_hoofs" TYPE FILE FILES "/home/eksan-rokae-com/inexus/inex_sre/third_party/iceoryx-build/hoofs/CMakeFiles/Export/2df1df3350d90c929624816c3cc2d98f/iceoryx_hoofsTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/iceoryx_hoofs" TYPE FILE FILES "/home/eksan-rokae-com/inexus/inex_sre/third_party/iceoryx-build/hoofs/CMakeFiles/Export/2df1df3350d90c929624816c3cc2d98f/iceoryx_hoofsTargets-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/iceoryx_hoofs" TYPE FILE FILES
    "/home/eksan-rokae-com/inexus/inex_sre/third_party/iceoryx/iceoryx_hoofs/cmake/IceoryxPackageHelper.cmake"
    "/home/eksan-rokae-com/inexus/inex_sre/third_party/iceoryx/iceoryx_hoofs/cmake/IceoryxPlatform.cmake"
    "/home/eksan-rokae-com/inexus/inex_sre/third_party/iceoryx/iceoryx_hoofs/cmake/IceoryxVersion.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "dev" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/iceoryx_hoofs" TYPE FILE FILES "/home/eksan-rokae-com/inexus/inex_sre/third_party/iceoryx/iceoryx_hoofs/LICENSE")
endif()

