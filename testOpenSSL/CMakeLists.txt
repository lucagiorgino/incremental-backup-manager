cmake_minimum_required(VERSION 3.16)
project(testOpenSSL)

set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS -lcrypto)

link_libraries(crypto)

find_package(OpenSSL)

if(OpenSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIRS})
endif()
add_executable(testOpenSSL main.cpp)