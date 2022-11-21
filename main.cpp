#include "utils/input-parser.h"
#include "Commands.h"

#include <iostream>
#include <utility>
#include <vector>

constexpr auto PROMPT_HEAD = " $ ";

void startConsole(const std::shared_ptr<FileSystem> &pFS) {
    bool run = true;
    std::string sInput;
    std::vector<std::string> args;
    do {
        std::cout << pFS->getWorkingDirectoryPath() + PROMPT_HEAD;
        std::getline(std::cin, sInput);
        args = split(sInput, ' ');
        try {
            run = handleUserInput(args, pFS);
        } catch (InvalidOptionException &ex) {
            std::cout << "fs: " << ex.what() << std::endl;
        }
    } while (run);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Invalid argument.\n"
                     "Usage: <executable> fs_file_name" << std::endl;
    }

    std::string fsFileName{argv[1]};

    auto pFS = std::make_shared<FileSystem>(fsFileName);

    std::cout << *pFS << std::endl;

    startConsole(pFS);
    return 0;
}
