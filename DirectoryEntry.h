#ifndef ZOS_SP_DIRECTORYENTRY_H
#define ZOS_SP_DIRECTORYENTRY_H

#include "definitions.h"

class DirectoryEntry {
public:
    std::string mItemName;
    bool mIsFile;
    int mSize;
    int mStartCluster;

    DirectoryEntry(){}

    DirectoryEntry(const std::string &&mItemName, bool mIsFile, int mSize, int mStartCluster);

    DirectoryEntry(const std::string &mItemName, bool mIsFile, int mSize, int mStartCluster);

    static const int SIZE = ITEM_NAME_LENGTH + sizeof(mIsFile) + sizeof(mSize) + sizeof(mStartCluster);

    void write(std::fstream &f);

    void read(std::fstream &f);

    friend std::ostream &operator<<(std::ostream &os, DirectoryEntry const &fs);
};


#endif //ZOS_SP_DIRECTORYENTRY_H
