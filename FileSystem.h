#ifndef ZOS_SP_FILESYSTEM_H
#define ZOS_SP_FILESYSTEM_H

#include "definitions.h"
#include <string>
#include <fstream>

class DirectoryEntry {
private:
    std::string mItemName;
    bool mIsFile;
    int mSize;                   //soubor: velikost, adresar: kolik ma entries
    int mStartCluster;           //počáteční cluster položky
public:
    DirectoryEntry() {};

    DirectoryEntry(const std::string &&mItemName, bool mIsFile, int mSize, int mStartCluster);

    DirectoryEntry(const std::string &mItemName, bool mIsFile, int mSize, int mStartCluster);

    static const int SIZE = ALLOWED_ITEM_NAME_LENGTH + sizeof(mIsFile) + sizeof(mSize) + sizeof(mStartCluster);

    void write(std::ofstream &f);

    void read(std::ifstream &f);

    friend std::ostream &operator<<(std::ostream &os, DirectoryEntry const &fs);
};


class FAT {

public:
    static const int SIZE = 0;

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
    int mDiskSize;             //celkova velikost VFS
    int mFatCount;             //pocet polozek kazde FAT tabulce
    int mFat1StartAddress;     //adresa pocatku FAT1 tabulky
    int mFat2StartAddress;     //adresa pocatku FAT2 tabulky
    int mDataStartAddress;     //adresa pocatku datovych bloku (hl. adresar)
    int mPaddingSize;
    std::string mPadding;

    static const int SIZE = SIGNATURE_LENGTH + sizeof(mClusterSize) + sizeof(mClusterCount) +
                            sizeof(mDiskSize) + sizeof(mFatCount) + sizeof(mFat1StartAddress) +
                            sizeof(mFat2StartAddress) + sizeof(mDataStartAddress);

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
};

constexpr int MAX_ENTRIES = CLUSTER_SIZE / DirectoryEntry::SIZE;


#endif //ZOS_SP_FILESYSTEM_H
