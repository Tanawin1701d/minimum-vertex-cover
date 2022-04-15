cmake_minimum_required(VERSION 3.21)
project(ass8_proj)
FIND_PACKAGE(OpenMP REQUIRED)
#set(CMAKE_CXX_STANDARD 14 ${OpenMP_C_FLAGS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fopenmp -march=native")

add_executable(ass8_proj
        #s64.cpp
        #backUp/s64_h.cpp
        #s64_hs.cpp
        s64_hsrm.cpp
        #s48_32_nh.cpp
        #s48_32_nhns.cpp
        #iter64_test.cpp
        #s64_sf.cpp
        #opm_test.cpp
        #controller.h
        #controller.cpp
        )
