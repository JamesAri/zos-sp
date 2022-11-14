#include "FileSystem.h"
#include "utils/stream-utils.h"

#include <iostream>


DirectoryEntry::DirectoryEntry(const std::string &&mItemName, bool mIsFile, int mSize, int mStartCluster) :
        mIsFile(mIsFile), mSize(mSize), mStartCluster(mStartCluster) {
    if (mItemName.length() >= ALLOWED_ITEM_NAME_LENGTH) {
        throw std::runtime_error(std::string("Error: file name too long. Character limit is ") +
                                 std::to_string(ALLOWED_ITEM_NAME_LENGTH - 1) + std::string("\n"));
    }
    this->mItemName = mItemName + std::string(ALLOWED_ITEM_NAME_LENGTH - mItemName.length(), '\00');
}

DirectoryEntry::DirectoryEntry(const std::string &mItemName, bool mIsFile, int mSize, int mStartCluster) :
        mIsFile(mIsFile), mSize(mSize), mStartCluster(mStartCluster) {
    if (mItemName.length() >= ALLOWED_ITEM_NAME_LENGTH) {
        throw std::runtime_error(std::string("Error: file name too long. Character limit is ") +
                                 std::to_string(ALLOWED_ITEM_NAME_LENGTH - 1) + std::string("\n"));
    }
    this->mItemName = mItemName + std::string(ALLOWED_ITEM_NAME_LENGTH - mItemName.length(), '\00');
}

void DirectoryEntry::write(std::ofstream &f) {
    writeToStream(f, this->mItemName, ALLOWED_ITEM_NAME_LENGTH);
    writeToStream(f, this->mIsFile);
    writeToStream(f, this->mSize);
    writeToStream(f, this->mStartCluster);
    // padding
//    std::string zeroFill(CLUSTER_SIZE - DirectoryEntry::SIZE, '\00');
//    writeToStream(f, zeroFill, static_cast<int>(zeroFill.length()));
}

void DirectoryEntry::read(std::ifstream &f) {
    readFromStream(f, this->mItemName, ALLOWED_ITEM_NAME_LENGTH);
    readFromStream(f, this->mIsFile);
    readFromStream(f, this->mSize);
    readFromStream(f, this->mStartCluster);
}

std::ostream &operator<<(std::ostream &os, DirectoryEntry const &di) {
    return os << "  ItemName: " << di.mItemName.c_str() << "\n"
              << "  IsFile: " << di.mIsFile << "\n"
              << "  Size: " << di.mSize << "\n"
              << "  StartCluster: " << di.mStartCluster << "\n";
}

int FAT::write(std::ofstream &f, int32_t pos) {
    return -1;
}

int FAT::read(std::ifstream &f, int32_t pos) {
    return -1;
}

void FAT::wipe(std::ofstream &f, int32_t startAddress, int32_t size) {
    f.seekp(startAddress);

    auto wipeUnit = reinterpret_cast<const char *>(&FAT_UNUSED);
    for (int i = 0; i < size; i++) {
        f.write(wipeUnit, sizeof(FAT_UNUSED));
    }
}

BootSector::BootSector(int size) : mDiskSize(size * FORMAT_UNIT) {
    this->mSignature = SIGNATURE;
    this->mClusterSize = CLUSTER_SIZE;
    this->mFatCount = FAT_COUNT;

    size_t freeSpaceInBytes = this->mDiskSize - BootSector::SIZE;
    this->mClusterCount = static_cast<int>(freeSpaceInBytes / (sizeof(int32_t) + this->mClusterSize));
    this->mFatSize = static_cast<int>(this->mClusterCount * sizeof(int32_t));
    this->mFat1StartAddress = BootSector::SIZE;
    this->mFat2StartAddress = BootSector::SIZE + this->mFatSize;

    size_t dataSize = this->mClusterCount * this->mClusterSize;
    size_t fatTablesSize = this->mFatSize * this->mFatCount;
    this->mPaddingSize = static_cast<int>(freeSpaceInBytes - (dataSize + fatTablesSize));
//    this->mPadding = std::string(mPaddingSize, '\00');

    auto fatEndAddress = this->mFat2StartAddress + this->mFatSize;
    this->mDataStartAddress = static_cast<int>(mPaddingSize + fatEndAddress);
}

void BootSector::write(std::ofstream &f) {
    writeToStream(f, this->mSignature, SIGNATURE_LENGTH);
    writeToStream(f, this->mClusterSize);
    writeToStream(f, this->mClusterCount);
    writeToStream(f, this->mDiskSize);
    writeToStream(f, this->mFatCount);
    writeToStream(f, this->mFat1StartAddress);
    writeToStream(f, this->mFat2StartAddress);
    writeToStream(f, this->mDataStartAddress);
    writeToStream(f, this->mPaddingSize);
//    writeToStream(f, this->mPadding, this->mPaddingSize);
}

void BootSector::read(std::ifstream &f) {
    readFromStream(f, this->mSignature, SIGNATURE_LENGTH);
    readFromStream(f, this->mClusterSize);
    readFromStream(f, this->mClusterCount);
    readFromStream(f, this->mDiskSize);
    readFromStream(f, this->mFatCount);
    readFromStream(f, this->mFat1StartAddress);
    readFromStream(f, this->mFat2StartAddress);
    readFromStream(f, this->mDataStartAddress);
    readFromStream(f, this->mPaddingSize);
//    f.ignore(this->mPaddingSize);

    this->mFatSize = static_cast<int>(this->mClusterCount * sizeof(int32_t));
}

std::ostream &operator<<(std::ostream &os, BootSector const &bs) {
    return os << "  Signature: " << bs.mSignature.c_str() << "\n"
              << "  ClusterSize: " << bs.mClusterSize << "B\n"
              << "  ClusterCount: " << bs.mClusterCount << "\n"
              << "  DiskSize: " << bs.mDiskSize / FORMAT_UNIT << "MB\n"
              << "  FatCount: " << bs.mFatCount << "\n"
              << "  Padding: " << bs.mPaddingSize << "B\n"
              << "  Fat1StartAddress: " << bs.mFat1StartAddress << "\n"
              << "  Fat2StartAddress: " << bs.mFat2StartAddress << "\n"
              << "  DataStartAddress: " << bs.mDataStartAddress << "\n";
}


FileSystem::FileSystem(std::string &fileName) : mFileName(fileName) {
    std::ifstream f_in(fileName, std::ios::binary | std::ios::in);
    if (f_in.is_open())
        try {
            this->read(f_in);
        } catch (...) {
            std::cerr << "An error occurred during the loading process. Input file might be corrupted." << std::endl;
        }
    else
        this->formatFS();
}

void FileSystem::write(std::ofstream &f, bool wipeData) {
    this->mBootSector.write(f);
    f.seekp(this->mBootSector.mDataStartAddress); // jump to root dir (skip FAT tables)
    this->mRootDir.write(f);

    if (wipeData) {
        char wipedCluster[CLUSTER_SIZE] = {'\00'};
        for (int i = 0; i < this->mBootSector.mClusterCount - 1; i++) { // -1 for root entry.
            writeToStream(f, wipedCluster, CLUSTER_SIZE);
        }
    }
}

void FileSystem::read(std::ifstream &f) {
    this->mBootSector.read(f);
    f.seekg(this->mBootSector.mDataStartAddress);
    this->mRootDir.read(f);
}

std::ostream &operator<<(std::ostream &os, FileSystem const &fs) {
    return os << "==========    FILE SYSTEM SPECS    ========== \n\n"
              << "BOOT-SECTOR (" << BootSector::SIZE << "B)\n" << fs.mBootSector << "\n"
              << "FAT count: " << fs.mBootSector.mFatCount << "\n"
              << "FAT size: " << fs.mBootSector.getFatSize() << "\n\n"
              << "ROOT-DIR:\n" << fs.mRootDir << "\n"
              << "========== END OF FILE SYSTEM SPECS ========== \n";
}

void FileSystem::formatFS(int size) {
    this->mBootSector = BootSector{size};
    this->mRootDir = DirectoryEntry{std::string(ROOT_DIR_NAME), false, 0, 0};

    std::ofstream f_out{this->mFileName, std::ios::binary | std::ios::out};

    if (!f_out.is_open())
        throw std::runtime_error(FS_OPEN_ERROR);

    this->write(f_out, true);
    FAT::wipe(f_out, this->mBootSector.mFat1StartAddress, this->mBootSector.mClusterCount);
    FAT::wipe(f_out, this->mBootSector.mFat2StartAddress, this->mBootSector.mClusterCount);
}