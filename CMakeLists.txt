cmake_minimum_required(VERSION 3.23)
project(zos_sp)

set(CMAKE_CXX_STANDARD 14)

add_executable(zos_sp main.cpp Commands.h Commands.cpp fat.h fat.cpp ICommand.h utils/input-parser.h)
