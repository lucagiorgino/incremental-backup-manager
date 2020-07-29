cmake_minimum_required(VERSION 3.16)
project(Remote_backup)

set(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_FLAGS -pthread)

message(STATUS "start running cmake...")

find_package(OpenSSL)

if(OpenSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIRS})
endif()

find_package(Boost 1.71.0 COMPONENTS system filesystem REQUIRED)

if(Boost_FOUND)
    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
    message(STATUS "Boost_VERSION: ${Boost_VERSION}")
    include_directories(${Boost_INCLUDE_DIRS})
endif()

add_executable(BoostTest main.cpp)

if(Boost_FOUND)
    target_link_libraries(BoostTest ${Boost_LIBRARIES})
endif()

add_executable(Asynchronous_IO main.cpp client/FileWatcher.cpp client/FileWatcher.h client/Buffer.h client/Client.cpp client/Client.h)