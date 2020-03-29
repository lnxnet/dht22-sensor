# dht22-sensor CMake main file

cmake_minimum_required(VERSION 3.6)
project(dht22-sensor LANGUAGES C VERSION 1.0.1)

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()

# Compiler flags for specific build types
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -W -Wall -O0 -g")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")

include_directories("${PROJECT_BINARY_DIR}")
set(SOURCES src/sensor.h src/sensor.c)

set(SYSTEMD_UNIT_DIR "lib/systemd/system/")

# Executable
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin/)
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} -lwiringPi -lwiringPiDev -lcurl)

# Install targets
install(TARGETS ${PROJECT_NAME} DESTINATION sbin)
install(FILES "systemd/${PROJECT_NAME}.service" DESTINATION ${SYSTEMD_UNIT_DIR})

# Debian packaging
set(CPACK_DEBIAN_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "wiringpi (>= 2.44), libcurl4 (>= 7.64.0)")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Thomas Favre-Bulle <thomasfavrebulle@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "misc")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/someplace")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/DEBIAN/postinst;${CMAKE_CURRENT_SOURCE_DIR}/DEBIAN/prerm")

# Package
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.LESSER")
set(CPACK_PACKAGE_VERSION_MAJOR "${dht22-sensor_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${dht22-sensor_VERSION_MINOR}")
include(CPack)
