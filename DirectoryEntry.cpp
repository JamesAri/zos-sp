#include "DirectoryEntry.h"
#include "utils/stream-utils.h"

DirectoryEntry::DirectoryEntry(const std::string &&itemName, bool mIsFile, int mSize, int mStartCluster) :
        mIsFile(mIsFile), mSize(mSize), mStartCluster(mStartCluster) {
    if (itemName.length() >= ITEM_NAME_LENGTH)
        throw std::runtime_error(DE_ITEM_NAME_LENGTH_ERROR);
    mItemName = itemName + std::string(ITEM_NAME_LENGTH - itemName.length(), '\00');
}

DirectoryEntry::DirectoryEntry(const std::string &itemName, bool mIsFile, int mSize, int mStartCluster) :
        mIsFile(mIsFile), mSize(mSize), mStartCluster(mStartCluster) {
    if (itemName.length() >= ITEM_NAME_LENGTH) {
        throw std::runtime_error(DE_ITEM_NAME_LENGTH_ERROR);
    }
    mItemName = itemName + std::string(ITEM_NAME_LENGTH - itemName.length(), '\00');
}

void DirectoryEntry::write(std::fstream &f) {
    writeToStream(f, mItemName, ITEM_NAME_LENGTH);
    writeToStream(f, mIsFile);
    writeToStream(f, mSize);
    writeToStream(f, mStartCluster);
}

void DirectoryEntry::read(std::fstream &f) {
    readFromStream(f, mItemName, ITEM_NAME_LENGTH);
    readFromStream(f, mIsFile);
    readFromStream(f, mSize);
    readFromStream(f, mStartCluster);
}

std::ostream &operator<<(std::ostream &os, DirectoryEntry const &di) {
    return os << "  ItemName: " << di.mItemName.c_str() << "\n"
              << "  IsFile: " << di.mIsFile << "\n"
              << "  Size: " << di.mSize << "\n"
              << "  StartCluster: " << di.mStartCluster << "\n";
}