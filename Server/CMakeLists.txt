cmake_minimum_required(VERSION 3.17)
project(Server)

set(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_FLAGS "-pthread -lcrypto")

set(dir ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${dir}/bin")
message(STATUS "start running cmake...")

link_libraries(crypto)
link_libraries(sqlite3)

find_package(OpenSSL)

if(OpenSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIRS})
endif()

find_package(Boost 1.65.0 COMPONENTS system filesystem date_time REQUIRED)

if(Boost_FOUND)
    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
    message(STATUS "Boost_VERSION: ${Boost_VERSION}")

    include_directories(${Boost_INCLUDE_DIRS})
endif()

add_executable(Server main.cpp Server.h Hash.h Hash.cpp ClientHandler.cpp ClientHandler.h Database.h Database.cpp Debug.h)

if(Boost_FOUND)
    target_link_libraries(Server ${Boost_LIBRARIES})
endif()

