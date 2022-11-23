cmake_minimum_required(VERSION 3.23)
project(zos_sp)

set(CMAKE_CXX_STANDARD 14)

add_executable(zos_sp main.cpp Commands.h Commands.cpp ICommand.h ICommand.cpp utils/input-parser.h FileSystem.cpp
        FileSystem.h utils/stream-utils.h utils/stream-utils.cpp utils/string-utils.cpp utils/string-utils.h utils/validators.cpp utils/validators.h
        definitions.h FAT.cpp FAT.h BootSector.cpp BootSector.h DirectoryEntry.cpp DirectoryEntry.h utils/stream-utils.cpp)
