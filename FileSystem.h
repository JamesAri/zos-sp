#ifndef ZOS_SP_FILESYSTEM_H
#define ZOS_SP_FILESYSTEM_H

#include "definitions.h"
#include <string>
#include <fstream>

// MEMORY:
//  BOOT SECTOR
//  FAT1
//  FAT2
//  padding (0 <= padding < CLUSTER_SIZE), fill: (\00)
//  DATA

class DirectoryEntry {
public:
    std::string mItemName;
    bool mIsFile;
    int mSize;                   //soubor: velikost, adresar: kolik ma entries
    int mStartCluster;

    DirectoryEntry() {};

    DirectoryEntry(const std::string &&mItemName, bool mIsFile, int mSize, int mStartCluster);

    DirectoryEntry(const std::string &mItemName, bool mIsFile, int mSize, int mStartCluster);

    static const int SIZE = ITEM_NAME_LENGTH + sizeof(mIsFile) + sizeof(mSize) + sizeof(mStartCluster);

    void write(std::ofstream &f);

    void read(std::ifstream &f);

    friend std::ostream &operator<<(std::ostream &os, DirectoryEntry const &fs);
};


class FAT {
private:
    FAT(){/* static class */}

public:
    static int write(std::ofstream &f, int32_t pos);

    static int read(std::ifstream &f, int32_t pos);

    static void wipe(std::ofstream &f, int32_t startAddress, int32_t size);
};


class BootSector {
private:
    int mFatSize;
public:
    std::string mSignature;
    int mClusterSize;
    int mClusterCount;
    int mDiskSize;
    int mFatCount;
    int mFat1StartAddress;
    int mFat2StartAddress;
    int mDataStartAddress;     //adresa pocatku datovych bloku (hl. adresar)
    int mPaddingSize;

    static const int SIZE = SIGNATURE_LENGTH + sizeof(mClusterSize) + sizeof(mClusterCount) +
                            sizeof(mDiskSize) + sizeof(mFatCount) + sizeof(mFat1StartAddress) +
                            sizeof(mFat2StartAddress) + sizeof(mDataStartAddress) + sizeof(mPaddingSize);

    BootSector() {};

    explicit BootSector(int size);

    void write(std::ofstream &f);

    void read(std::ifstream &f);

    friend std::ostream &operator<<(std::ostream &os, BootSector const &fs);

    int getFatSize() const { return this->mFatSize; };
};


/**
 * FAT FS
 */
class FileSystem {
public:
    const std::string mFileName;

    BootSector mBootSector;
    DirectoryEntry mRootDir;

    explicit FileSystem(std::string &fileName);

    void write(std::ofstream &f, bool wipeData = false);

    void read(std::ifstream &f);

    friend std::ostream &operator<<(std::ostream &os, FileSystem const &fs);

    void formatFS(int size = DEFAULT_FORMAT_SIZE);

    inline int clusterToAddress(int cluster);

    inline int getFreeDirectoryEntryAddress(int cluster, int entriesCount);

    bool findDirectoryEntry(int cluster, const std::string &itemName, DirectoryEntry &de);
};

constexpr int MAX_ENTRIES = CLUSTER_SIZE / DirectoryEntry::SIZE;


#endif //ZOS_SP_FILESYSTEM_H
