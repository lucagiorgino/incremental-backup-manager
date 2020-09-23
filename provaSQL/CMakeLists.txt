cmake_minimum_required(VERSION 3.17)
project(provaSQL)

#Used to link sqlite3
link_libraries(sqlite3)

set(CMAKE_CXX_STANDARD 17)

add_executable(provaSQL main.cpp)