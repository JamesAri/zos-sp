#ifndef ZOS_SP_BOOTSECTOR_H
#define ZOS_SP_BOOTSECTOR_H

#include "definitions.h"

class BootSector {
private:
    int mFatSize;
public:
    // actual memory structure
    std::string mSignature;
    int mClusterSize;
    int mClusterCount;
    int mDiskSize;
    int mFatCount;
    int mFat1StartAddress;
    int mDataStartAddress;     //adresa pocatku datovych bloku (hl. adresar)
    int mPaddingSize;

    static const int SIZE = SIGNATURE_LENGTH + sizeof(mClusterSize) + sizeof(mClusterCount) +
                            sizeof(mDiskSize) + sizeof(mFatCount) + sizeof(mFat1StartAddress) +
                            sizeof(mDataStartAddress) + sizeof(mPaddingSize);

    BootSector(){}

    explicit BootSector(int diskSize);

    void write(std::fstream &f);

    void read(std::fstream &f);

    friend std::ostream &operator<<(std::ostream &os, BootSector const &fs);

    int getFatSize() const { return this->mFatSize; };
};


#endif //ZOS_SP_BOOTSECTOR_H
