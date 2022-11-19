#ifndef ZOS_SP_FILESYSTEM_H
#define ZOS_SP_FILESYSTEM_H

#include "definitions.h"

class DirectoryEntry {
public:
    std::string mItemName;
    bool mIsFile;
    int mSize;
    int mStartCluster;

    DirectoryEntry() {};

    DirectoryEntry(const std::string &&mItemName, bool mIsFile, int mSize, int mStartCluster);

    DirectoryEntry(const std::string &mItemName, bool mIsFile, int mSize, int mStartCluster);

    static const int SIZE = ITEM_NAME_LENGTH + sizeof(mIsFile) + sizeof(mSize) + sizeof(mStartCluster);

    void write(std::ostream &f);

    void write(std::ostream &f, int32_t pos);

    void read(std::istream &f);

    void read(std::istream &f, int32_t pos);

    friend std::ostream &operator<<(std::ostream &os, DirectoryEntry const &fs);
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
    int mDataStartAddress;     //adresa pocatku datovych bloku (hl. adresar)
    int mPaddingSize;

    static const int SIZE = SIGNATURE_LENGTH + sizeof(mClusterSize) + sizeof(mClusterCount) +
                            sizeof(mDiskSize) + sizeof(mFatCount) + sizeof(mFat1StartAddress) +
                            sizeof(mDataStartAddress) + sizeof(mPaddingSize);

    BootSector() {};

    explicit BootSector(int size);

    void write(std::ofstream &f);

    void read(std::ifstream &f);

    friend std::ostream &operator<<(std::ostream &os, BootSector const &fs);

    int getFatSize() const { return this->mFatSize; };
};


class FAT {
private:
    FAT() {/* static class */}

public:
    static void write(std::ostream &f, int32_t pos, int32_t label);

    static void write(std::ostream &f, int32_t label);

    static int read(std::istream &f, int32_t pos);

    static int read(std::istream &f);

    static void wipe(std::ostream &f, int32_t startAddress, int32_t size);

    static std::vector<int> getFreeClusters(std::istream &f, const BootSector &bs, int count = 1);
};

/**
 * FS MEMORY STRUCTURE:
 *
 * BOOT SECTOR
 * FAT1
 * FAT2
 * padding (0 <= padding < CLUSTER_SIZE), fill value: \00
 * DATA
 */
class FileSystem {
public:
    const std::string mFileName;

    BootSector mBootSector;
    DirectoryEntry mWorkingDirectory;

    explicit FileSystem(std::string &fileName);

    void write(std::ofstream &f, bool wipeData = false);

    void read(std::ifstream &f);

    friend std::ostream &operator<<(std::ostream &os, FileSystem const &fs);

    void formatFS(int size = DEFAULT_FORMAT_SIZE);

    int clusterToDataAddress(int cluster) const;

    int clusterToFatAddress(int cluster) const;

    bool getDirectory(int cluster, DirectoryEntry &de);

    int getDirectoryNextFreeEntryAddress(int cluster) const;

    bool findDirectoryEntry(int cluster, const std::string &itemName, DirectoryEntry &de);

    bool findDirectoryEntry(int parentCluster, int childCluster, DirectoryEntry &de);

    bool removeDirectoryEntry(int parentCluster, const std::string &itemName);

    int getDirectoryEntryCount(int cluster);

    int getNeededClustersCount(int fileSize);
};

constexpr int MAX_ENTRIES = CLUSTER_SIZE / DirectoryEntry::SIZE;


#endif //ZOS_SP_FILESYSTEM_H
