cmake_minimum_required(VERSION 3.19)
project(KickTree_v0_4)

set(CMAKE_CXX_STANDARD 14)

add_executable(KickTree_v0_4 main.cpp ElementaryClasses.h KickTree/KickTree.cpp KickTree/KickTree.h OVS/MapExtensions.cpp OVS/MapExtensions.h KickTree/Tools.cpp KickTree/Tools.h)