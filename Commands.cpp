#include "Commands.h"

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
    return 0;
}

bool FormatCommand::validate_arguments() {
    return this->mOptCount == 1;
}

int DefragCommand::run() {
    return 0;
}

bool DefragCommand::validate_arguments() {
    return this->mOptCount == 2;
}
