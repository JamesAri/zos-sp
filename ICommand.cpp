#include "ICommand.h"

ICommand::ICommand(const std::vector<std::string> &options) {
    if (options.empty()) return;
    mOpt1 = options[0];
    if (options.size() == 2)
        mOpt2 = options[1];
}

void ICommand::process() {
    if (!this->validate_arguments()) {
        throw InvalidOptionException();
    }
    this->run();
}