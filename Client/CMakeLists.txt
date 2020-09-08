cmake_minimum_required(VERSION 3.17)
project(Client)

set(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_FLAGS "-pthread -lcrypto")

set(dir ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${dir}/bin")
message(STATUS "start running cmake...")

link_libraries(crypto)

find_package(OpenSSL)

if(OpenSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIRS})
endif()

find_package(Boost 1.65.0 COMPONENTS system filesystem REQUIRED)

if(Boost_FOUND)
    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
    message(STATUS "Boost_VERSION: ${Boost_VERSION}")

    include_directories(${Boost_INCLUDE_DIRS})
endif()

add_executable(Client main.cpp Client.cpp Client.h Buffer.h FileWatcher.cpp FileWatcher.h Hash.cpp Hash.h ResponseBuffer.h Action.h ResponseBuffer.cpp)

if(Boost_FOUND)
    target_link_libraries(Client ${Boost_LIBRARIES})
endif()

