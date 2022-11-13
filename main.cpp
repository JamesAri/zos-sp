#include "FileSystem.h"
#include "utils/input-parser.h"
#include "Commands.h"

#include <iostream>
#include <utility>
#include <vector>

constexpr auto PROMPT_HEAD = "$ ";

bool handleUserInput(std::vector<std::string> arguments) {
    if(arguments.empty()) return true;

    auto command = arguments[0];
    const auto options = std::vector<std::string>(arguments.begin() + 1, arguments.end());

    switch (getCommandCode(command)) {
        case ECommands::eCpCommand:
            CpCommand(options).process();
            break;
        case ECommands::eMvCommand:
            MvCommand(options).process();
            break;
        case ECommands::eRmCommand:
            RmCommand(options).process();
            break;
        case ECommands::eMkdirCommand:
            MkdirCommand(options).process();
            break;
        case ECommands::eRmdirCommand:
            RmdirCommand(options).process();
            break;
        case ECommands::eLsCommand:
            LsCommand(options).process();
            break;
        case ECommands::eCatCommand:
            CatCommand(options).process();
            break;
        case ECommands::eCdCommand:
            CdCommand(options).process();
            break;
        case ECommands::ePwdCommand:
            PwdCommand(options).process();
            break;
        case ECommands::eInfoCommand:
            InfoCommand(options).process();
            break;
        case ECommands::eIncpCommand:
            IncpCommand(options).process();
            break;
        case ECommands::eOutcpCommand:
            OutcpCommand(options).process();
            break;
        case ECommands::eLoadCommand:
            LoadCommand(options).process();
            break;
        case ECommands::eFormatCommand:
            FormatCommand(options).process();
            break;
        case ECommands::eDefragCommand:
            DefragCommand(options).process();
            break;
        case ECommands::eExitCommand:
            return false;
        case ECommands::eUnknownCommand:
            std::cout << "fs: Unknown command: " << command << std::endl;
            break;
    }
    return true;
}

void startConsole() {
    bool run = true;
    std::string sInput;
    std::vector<std::string> args;
    do {
        std::cout << PROMPT_HEAD;
        std::getline(std::cin, sInput);
        args = split(sInput, ' ');
        try {
            run = handleUserInput(args);
        } catch (InvalidOptionException &ex) {
            std::cout << "fs: " << ex.what() << std::endl;
        }
    } while(run);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Invalid argument.\n"
                     "Usage: <executable> fs_file_name" << std::endl;
    }

    std::string fsFileName{argv[1]};
    FileSystem fs{fsFileName};
    std::cout << fs << std::endl;
    startConsole();
    return 0;
}
