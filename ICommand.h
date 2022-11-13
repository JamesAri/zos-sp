#ifndef ZOS_SP_ICOMMAND_H
#define ZOS_SP_ICOMMAND_H

#include <utility>
#include <vector>
#include <iostream>
#include <utility>
#include <stdexcept>

class InvalidOptionException : public std::exception {
private:
    std::string mErrMsg;
public:
    InvalidOptionException() : mErrMsg("invalid option(s)") {}
    explicit InvalidOptionException(std::string &&errMsg) : mErrMsg(errMsg) {}
    const char *what() const _NOEXCEPT override {
        return this->mErrMsg.c_str();
    }
};

class ICommand {
public:
    virtual ~ICommand() = default;

    void process();

    explicit ICommand(const std::vector<std::string> &options);


private:
    virtual bool validate_arguments() = 0;
    virtual int run() = 0;

protected:
    int mOptCount;
    std::string mOpt1;
    std::string mOpt2;
};


#endif //ZOS_SP_ICOMMAND_H
