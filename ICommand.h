#ifndef ZOS_SP_ICOMMAND_H
#define ZOS_SP_ICOMMAND_H

#include "FileSystem.h"
#include <utility>
#include <vector>
#include <iostream>
#include <utility>
#include <stdexcept>

class ICommand {
private:
    virtual bool validateArguments() = 0;

    virtual bool run() = 0;

protected:
    std::shared_ptr<FileSystem> mFS;
    int mOptCount;
    std::string mOpt1;
    std::string mOpt2;

public:
    virtual ~ICommand() = default;

    void process();

    explicit ICommand(const std::vector<std::string> &options);

    ICommand &registerFS(const std::shared_ptr<FileSystem> &pFS);
};


#endif //ZOS_SP_ICOMMAND_H
