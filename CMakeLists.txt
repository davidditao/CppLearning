cmake_minimum_required(VERSION 3.0)

project(CppLearning)

set(CMAKE_CXX_STANDARD 17)

add_executable(threadpool DDthreadpool.cpp)

target_link_libraries(threadpool pthread)
