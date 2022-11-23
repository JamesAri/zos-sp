#ifndef ZOS_SP_FAT_H
#define ZOS_SP_FAT_H

#include <fstream>

class FAT {
private:
    FAT(){}

public:
    static void write(std::fstream &f, int32_t pos, int32_t label);

    static void write(std::fstream &f, int32_t label);

    static int read(std::fstream &f, int32_t pos);

    static int read(std::fstream &f);

    static void wipe(std::ostream &f, int32_t startAddress, int32_t clusterCount);
};


#endif //ZOS_SP_FAT_H
