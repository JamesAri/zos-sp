cmake_minimum_required(VERSION 3.23)
project(zos_sp)

set(CMAKE_CXX_STANDARD 14)

add_executable(zos_sp main.cpp ICommand.h fat.cpp fat.h utils/input-parser.h Commands2.cpp Commands2.h)
