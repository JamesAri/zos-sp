#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include "Commands.h"
#include "utils/input-parser.h"
#include "valarray"

constexpr auto PROMPT_HEAD = "$ ";

void workWithBinFiles() {
    // ofstream has auto. std::ios::out
    std::ifstream input("soubor.txt", std::ios::binary);
    input.seekg(0, input.end);
    auto fileSize = input.tellg();
    std::cout << fileSize << std::endl;
    const char *binbuf = "Binarni \x00zapis.\x00";
//    input.write(binbuf, 14);
}


bool handleUserInput(std::vector<std::string> arguments) {
    if(arguments.empty()) return true;

    auto command = arguments[0];
    const auto options = std::vector<std::string>(arguments.begin() + 1, arguments.end());

    switch (getCommandCode(command)) {
        case ECommands::eCpCommand:
            CpCommand(options).run();
            break;
        case ECommands::eMvCommand:
            MvCommand(options).run();
            break;
        case ECommands::eRmCommand:
            RmCommand(options).run();
            break;
        case ECommands::eMkdirCommand:
            MkdirCommand(options).run();
            break;
        case ECommands::eRmdirCommand:
            RmdirCommand(options).run();
            break;
        case ECommands::eLsCommand:
            LsCommand(options).run();
            break;
        case ECommands::eCatCommand:
            CatCommand(options).run();
            break;
        case ECommands::eCdCommand:
            CdCommand(options).run();
            break;
        case ECommands::ePwdCommand:
            PwdCommand(options).run();
            break;
        case ECommands::eInfoCommand:
            InfoCommand(options).run();
            break;
        case ECommands::eIncpCommand:
            IncpCommand(options).run();
            break;
        case ECommands::eOutcpCommand:
            OutcpCommand(options).run();
            break;
        case ECommands::eLoadCommand:
            LoadCommand(options).run();
            break;
        case ECommands::eFormatCommand:
            FormatCommand(options).run();
            break;
        case ECommands::eDefragCommand:
            DefragCommand(options).run();
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

int main() {
    startConsole();
    return 0;
}
