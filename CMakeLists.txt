cmake_minimum_required(VERSION 3.16.3)
project(RavennaRecorder)

find_package(PkgConfig REQUIRED)

pkg_check_modules(SNDFILE REQUIRED sndfile)

include_directories(lib)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCE_FILES
        src/main.cpp
        src/*.cpp
        src/*.h
        src/*.hpp
)

add_executable(RavennaRecorder ${SOURCE_FILES})

target_precompile_headers(RavennaRecorder PRIVATE src/pch.h)

target_link_libraries(RavennaRecorder ${SNDFILE_LIBRARIES})

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")
