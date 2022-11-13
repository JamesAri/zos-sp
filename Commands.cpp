#include "Commands.h"
#include "utils/string-utils.h"
#include <algorithm>

int CpCommand::run() {
    return 0;
}

bool CpCommand::validate_arguments() {
    return this->mOptCount == 2;
}

int MvCommand::run() {
    return 0;
}

bool MvCommand::validate_arguments() {
    return this->mOptCount == 2;
}

int RmCommand::run() {
    return 0;
}

bool RmCommand::validate_arguments() {
    return this->mOptCount == 2;
}

int MkdirCommand::run() {
    return 0;
}

bool MkdirCommand::validate_arguments() {
    return this->mOptCount == 1;
}

int RmdirCommand::run() {
    return 0;
}

bool RmdirCommand::validate_arguments() {
    return this->mOptCount == 1;
}

int LsCommand::run() {
    return 0;
}

bool LsCommand::validate_arguments() {
    return this->mOptCount == 0 || this->mOptCount == 1;
}

int CatCommand::run() {
    return 0;
}

bool CatCommand::validate_arguments() {
    return this->mOptCount == 1;
}

int CdCommand::run() {
    return 0;
}

bool CdCommand::validate_arguments() {
    return this->mOptCount == 1;
}

int PwdCommand::run() {
    return 0;
}

bool PwdCommand::validate_arguments() {
    return this->mOptCount == 0;
}

int InfoCommand::run() {
    return 0;
}

bool InfoCommand::validate_arguments() {
    return this->mOptCount == 1;
}

int IncpCommand::run() {
    return 0;
}

bool IncpCommand::validate_arguments() {
    return this->mOptCount == 2;
}

int OutcpCommand::run() {
    return 0;
}

bool OutcpCommand::validate_arguments() {
    return this->mOptCount == 2;
}

int LoadCommand::run() {
    return 0;
}

bool LoadCommand::validate_arguments() {
    return this->mOptCount == 1;
}

int FormatCommand::run() {
    try {
        this->mFS->formatFS(std::stoi(this->mOpt1));
        std::cout << "OK" << std::endl;
    } catch (...) {
        std::cerr << "Internal error, terminating" << std::endl;
        exit(1);
    }
    return 0;
}

bool FormatCommand::validate_arguments() {
    if (this->mOptCount != 1) return false;
    //    eraseAllSubString(this->mOpt1, allowedFormats[0]);

    std::transform(this->mOpt1.begin(), this->mOpt1.end(), this->mOpt1.begin(),
                   [](unsigned char c){ return std::toupper(c); });

    std::vector<std::string> allowedFormats{"MB"};

    auto pos = this->mOpt1.find(allowedFormats[0]);

    if (pos == std::string::npos)
        throw InvalidOptionException("CANNOT CREATE FILE (wrong unit)");

    this->mOpt1.erase(pos, allowedFormats[0].length());
    if (!is_number(this->mOpt1))
        throw InvalidOptionException("CANNOT CREATE FILE (not a number)");
    return true;
}

FormatCommand& FormatCommand::registerFS(const std::shared_ptr<FileSystem> &pFS) {
    this->mFS = pFS;
    return *this;
}

int DefragCommand::run() {
    return 0;
}

bool DefragCommand::validate_arguments() {
    return this->mOptCount == 2;
}
