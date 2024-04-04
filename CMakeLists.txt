cmake_minimum_required(VERSION 3.27)
project(lab2)

set(CMAKE_CXX_STANDARD 17)

add_executable(lab2
        TaskQueue.cpp
        ThreadPool.cpp
        main.cpp
)
