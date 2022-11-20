#ifndef ZOS_SP_FILESYSTEM_H
#define ZOS_SP_FILESYSTEM_H

#include "definitions.h"
#include <fstream>

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

    void write(std::fstream &f, int32_t pos);

    void read(std::fstream &f);

    void read(std::fstream &f, int32_t pos);

    friend std::ostream &operator<<(std::ostream &os, DirectoryEntry const &fs);
};

constexpr int MAX_ENTRIES = CLUSTER_SIZE / DirectoryEntry::SIZE;

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
    const std::string mFileName;
public:
    std::fstream mStream;

    BootSector mBootSector;
    DirectoryEntry mWorkingDirectory;

    explicit FileSystem(std::string &fileName);

    void readVFS();

    friend std::ostream &operator<<(std::ostream &os, FileSystem const &fs);

    void formatFS(int diskSize = DEFAULT_FORMAT_SIZE);

    bool getDirectory(int cluster, DirectoryEntry &de);

    bool findDirectoryEntry(int cluster, const std::string &itemName, DirectoryEntry &de);

    bool findDirectoryEntry(int parentCluster, int childCluster, DirectoryEntry &de);

    bool removeDirectoryEntry(int parentCluster, const std::string &itemName);

    int getDirectoryEntryCount(int cluster);

    int getNeededClustersCount(int fileSize) const;

    int clusterToDataAddress(int cluster) const;

    int clusterToFatAddress(int cluster) const;

    void seek(int pos);

    void flush();
};


class FAT {
private:
    FAT(){}

public:
    static void write(std::fstream &f, int32_t pos, int32_t label);

    static void write(std::fstream &f, int32_t label);

    static int read(std::fstream &f, int32_t pos);

    static int read(std::fstream &f);

    static void wipe(std::ostream &f, int32_t startAddress, int32_t clusterCount);

    static std::vector<int> getFreeClusters(std::shared_ptr<FileSystem> &fs, int count = 1);
};


#endif //ZOS_SP_FILESYSTEM_H
