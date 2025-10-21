cmake_minimum_required(VERSION 3.10)
project(MultiApps)

option(BUILD_AI_APPS "Build AI Apps" ON)

if(BUILD_AI_APPS)
    message(STATUS "Building AI Apps ...")
    include(${CMAKE_SOURCE_DIR}/AIAppsCMakeLists.txt)
endif()