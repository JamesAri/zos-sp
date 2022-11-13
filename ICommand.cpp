#include "ICommand.h"

ICommand::ICommand(const std::vector<std::string> &options) {
    mOptCount = static_cast<int>(options.size());
    if (!mOptCount) return;
    mOpt1 = options[0];
    if (mOptCount == 2) {
        mOpt2 = options[1];
    }
}

void ICommand::process() {
    if (!this->validate_arguments()) {
        throw InvalidOptionException("invalid option(s)");
    }
    this->run();
}