#ifndef ZOS_SP_ICOMMAND_H
#define ZOS_SP_ICOMMAND_H

#include <utility>
#include <vector>
#include <iostream>
#include <utility>
#include <stdexcept>

class InvalidOptionException : public std::exception {
public:
    const char *what() const _NOEXCEPT override {
        return "invalid option(s)";
    }
};

class ICommand {
public:
    virtual ~ICommand() = default;

    virtual int run() = 0;


    explicit ICommand(const std::vector<std::string> &options);


private:
    virtual bool validate_arguments() = 0;

protected:
    std::string mOpt1;
    std::string mOpt2;
};


#endif //ZOS_SP_ICOMMAND_H
