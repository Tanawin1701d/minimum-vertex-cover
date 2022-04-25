cmake_minimum_required(VERSION 3.21)
project(ass8_proj)
FIND_PACKAGE(OpenMP REQUIRED)
#set(CMAKE_CXX_STANDARD 14 ${OpenMP_C_FLAGS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fopenmp ")

add_executable(ass8_proj
        #s64.cpp
        #backUp/s64_h.cpp
        #s64_hs.cpp
        #s64_hsrm.cpp
        #s64_hsHrm.cpp
        #s64_hsHrmjb4.cpp
        #s128_hsHrmjb4.cpp
        #s128_test.cpp
        #s64_hsHrmpq.cpp
        #s64_hsHrmj2.cpp
        #s64_hsHrmJB.cpp
        #s64_tree.cpp
        #s48_32_nh.cpp
        #s48_32_nhns.cpp
        #iter64_test.cpp
        #s64_sf.cpp
        #opm_test.cpp
        #controller.h
        #controller.cpp
        #s128_test.cpp
        s128_final.cpp

        )

