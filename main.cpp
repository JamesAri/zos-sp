#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include "Commands2.h"
//#include "utils/input-parser.h"


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

    std::string command = arguments[0];

    switch (getCommandCode(command)) {
        case ECommands::eCpCommand:
            CpCommand().process();
            break;
        case ECommands::eMvCommand:
            MvCommand().process();
            break;
        case ECommands::eRmCommand:
            RmCommand().process();
            break;
        case ECommands::eMkdirCommand:
            MkdirCommand().process();
            break;
        case ECommands::eRmdirCommand:
            RmdirCommand().process();
            break;
        case ECommands::eLsCommand:
            LsCommand().process();
            break;
        case ECommands::eCatCommand:
            CatCommand().process();
            break;
        case ECommands::eCdCommand:
            CdCommand().process();
            break;
        case ECommands::ePwdCommand:
            PwdCommand().process();
            break;
        case ECommands::eInfoCommand:
            InfoCommand().process();
            break;
        case ECommands::eIncpCommand:
            IncpCommand().process();
            break;
        case ECommands::eOutcpCommand:
            OutcpCommand().process();
            break;
        case ECommands::eLoadCommand:
            LoadCommand().process();
            break;
        case ECommands::eFormatCommand:
            FormatCommand().process();
            break;
        case ECommands::eDefragCommand:
            DefragCommand().process();
            break;
        case ECommands::eExitCommand:
            return false;
        case ECommands::eUnknownCommand:
            std::cout << "Unknown command: " << command << std::endl;
            break;
    }
    return true;
}

void startConsole() {
    std::string sInput;
    std::vector<std::string> x;
    do {
        std::cout << "$ ";
        std::cin >> sInput;
//        x = split(sInput, ' ');
    } while(handleUserInput(x));
}

int main() {
    startConsole();
    return 0;
}
