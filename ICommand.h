#ifndef ZOS_SP_ICOMMAND_H
#define ZOS_SP_ICOMMAND_H

#include <iostream>
#include <utility>

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual int process() = 0;

    void serPar1(std::string s) {
        this->par1 = std::move(s);
    }

    void serPar2(std::string s) {
       this->par2 = std::move(s);
    }

protected:
    std::string par1;
    std::string par2;
};


#endif //ZOS_SP_ICOMMAND_H
