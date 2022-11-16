#include "FileSystem.h"
#include "utils/stream-utils.h"

#include <iostream>


DirectoryEntry::DirectoryEntry(const std::string &&mItemName, bool mIsFile, int mSize, int mStartCluster) :
        mIsFile(mIsFile), mSize(mSize), mStartCluster(mStartCluster) {
    if (mItemName.length() >= ITEM_NAME_LENGTH) {
        throw std::runtime_error(std::string("Error: file name too long. Character limit is ") +
                                 std::to_string(ITEM_NAME_LENGTH - 1) + std::string("\n"));
    }
    this->mItemName = mItemName + std::string(ITEM_NAME_LENGTH - mItemName.length(), '\00');
}

DirectoryEntry::DirectoryEntry(const std::string &mItemName, bool mIsFile, int mSize, int mStartCluster) :
        mIsFile(mIsFile), mSize(mSize), mStartCluster(mStartCluster) {
    if (mItemName.length() >= ITEM_NAME_LENGTH) {
        throw std::runtime_error(std::string("Error: file name too long. Character limit is ") +
                                 std::to_string(ITEM_NAME_LENGTH - 1) + std::string("\n"));
    }
    this->mItemName = mItemName + std::string(ITEM_NAME_LENGTH - mItemName.length(), '\00');
}

void DirectoryEntry::write(std::ostream &f) {
    writeToStream(f, this->mItemName, ITEM_NAME_LENGTH);
    writeToStream(f, this->mIsFile);
    writeToStream(f, this->mSize);
    writeToStream(f, this->mStartCluster);
}


void DirectoryEntry::write(std::ostream &f, int32_t pos) {
    f.seekp(pos);
    this->write(f);
}

void DirectoryEntry::read(std::istream &f) {
    readFromStream(f, this->mItemName, ITEM_NAME_LENGTH);
    readFromStream(f, this->mIsFile);
    readFromStream(f, this->mSize);
    readFromStream(f, this->mStartCluster);
}

void DirectoryEntry::read(std::istream &f, int32_t pos) {
    f.seekg(pos);
    this->read(f);
}

std::ostream &operator<<(std::ostream &os, DirectoryEntry const &di) {
    return os << "  ItemName: " << di.mItemName.c_str() << "\n"
              << "  IsFile: " << di.mIsFile << "\n"
              << "  Size: " << di.mSize << "\n"
              << "  StartCluster: " << di.mStartCluster << "\n";
}

void FAT::write(std::ostream &f, int32_t label) {
    writeToStream(f, label);
}

void FAT::write(std::ostream &f, int32_t pos, int32_t label) {
    f.seekp(pos);
    FAT::write(f, label);
}

int FAT::read(std::istream &f) {
    int32_t clusterTag;
    readFromStream(f, clusterTag);
    return clusterTag;
}

int FAT::read(std::istream &f, int32_t pos) {
    f.seekg(pos);
    return FAT::read(f);
}

void FAT::wipe(std::ostream &f, int32_t startAddress, int32_t size) {
    f.seekp(startAddress);

    auto wipeUnit = reinterpret_cast<const char *>(&FAT_UNUSED);
    for (int i = 0; i < size; i++) {
        f.write(wipeUnit, sizeof(FAT_UNUSED));
    }
}

int FAT::getFreeCluster(std::istream &f, const BootSector &bs) {
    f.seekg(bs.mFat1StartAddress);
    int32_t label;
    for (int32_t i = 0; i < bs.mClusterCount; i++) {
        readFromStream(f, label);
        if (label == FAT_UNUSED) return i;
    }
    return -1;
}


BootSector::BootSector(int size) : mDiskSize(size * FORMAT_UNIT) {
    this->mSignature = SIGNATURE;
    this->mClusterSize = CLUSTER_SIZE;
    this->mFatCount = FAT_COUNT;

    size_t freeSpaceInBytes = this->mDiskSize - BootSector::SIZE;
    this->mClusterCount = static_cast<int>(freeSpaceInBytes / (sizeof(int32_t) + this->mClusterSize));
    this->mFatSize = static_cast<int>(this->mClusterCount * sizeof(int32_t));
    this->mFat1StartAddress = BootSector::SIZE;

    size_t dataSize = this->mClusterCount * this->mClusterSize;
    size_t fatTablesSize = this->mFatSize * this->mFatCount;
    this->mPaddingSize = static_cast<int>(freeSpaceInBytes - (dataSize + fatTablesSize));

    auto fatEndAddress = this->mFat1StartAddress + this->mFatSize;
    this->mDataStartAddress = static_cast<int>(mPaddingSize + fatEndAddress);
}

void BootSector::write(std::ofstream &f) {
    writeToStream(f, this->mSignature, SIGNATURE_LENGTH);
    writeToStream(f, this->mClusterSize);
    writeToStream(f, this->mClusterCount);
    writeToStream(f, this->mDiskSize);
    writeToStream(f, this->mFatCount);
    writeToStream(f, this->mFat1StartAddress);
    writeToStream(f, this->mDataStartAddress);
    writeToStream(f, this->mPaddingSize);
}

void BootSector::read(std::ifstream &f) {
    readFromStream(f, this->mSignature, SIGNATURE_LENGTH);
    readFromStream(f, this->mClusterSize);
    readFromStream(f, this->mClusterCount);
    readFromStream(f, this->mDiskSize);
    readFromStream(f, this->mFatCount);
    readFromStream(f, this->mFat1StartAddress);
    readFromStream(f, this->mDataStartAddress);
    readFromStream(f, this->mPaddingSize);

    this->mFatSize = static_cast<int>(this->mClusterCount * sizeof(int32_t));
}

std::ostream &operator<<(std::ostream &os, BootSector const &bs) {
    return os << "  Signature: " << bs.mSignature.c_str() << "\n"
              << "  ClusterSize: " << bs.mClusterSize << "B\n"
              << "  ClusterCount: " << bs.mClusterCount << "\n"
              << "  DiskSize: " << bs.mDiskSize / FORMAT_UNIT << "MB\n"
              << "  FatCount: " << bs.mFatCount << "\n"
              << "  Fat1StartAddress: " << bs.mFat1StartAddress << "-" << bs.mFat1StartAddress + bs.mFatSize << "\n"
              << "  Padding size: " << bs.mPaddingSize << "B\n"
              << "  PaddingAddress: " << bs.mFat1StartAddress + bs.mFatSize << "-" << bs.mDataStartAddress << "\n"
              << "  DataStartAddress: " << bs.mDataStartAddress << "\n";
}


FileSystem::FileSystem(std::string &fileName) : mFileName(fileName) {
    std::ifstream f_in(fileName, std::ios::binary | std::ios::in);
    if (f_in.is_open())
        try {
            this->read(f_in);
        } catch (...) {
            std::cerr << "An error occurred during the loading process.\n"
                         "Input file might be corrupted." << std::endl;
        }
    else
        this->formatFS();
}

void FileSystem::write(std::ofstream &f, bool wipeData) {
    this->mBootSector.write(f);

    if (wipeData) {
        f.seekp(this->mBootSector.mDataStartAddress); // jump to root dir (skip FAT tables)

        DirectoryEntry rootDir{std::string("."), false, DEFAULT_DIR_SIZE, 0};
        DirectoryEntry rootDir2{std::string(".."), false, DEFAULT_DIR_SIZE, 0};
        rootDir.write(f);
        rootDir2.write(f);
        this->mWorkingDirectory = rootDir;

        // wipe each cluster
        char wipedCluster[CLUSTER_SIZE] = {'\00'};
        for (int i = 0; i < this->mBootSector.mClusterCount - 1; i++) { // -1 for root entry.
            writeToStream(f, wipedCluster, CLUSTER_SIZE);
        }
    }
}

void FileSystem::read(std::ifstream &f) {
    this->mBootSector.read(f);
    f.seekg(this->mBootSector.mDataStartAddress);
    this->mWorkingDirectory.read(f);
}

std::ostream &operator<<(std::ostream &os, FileSystem const &fs) {
    return os << "==========    FILE SYSTEM SPECS    ========== \n\n"
              << "BOOT-SECTOR (" << BootSector::SIZE << "B)\n" << fs.mBootSector << "\n"
              << "FAT count: " << fs.mBootSector.mFatCount << "\n"
              << "FAT size: " << fs.mBootSector.getFatSize() << "\n\n"
              << "PWD (root dir):\n" << fs.mWorkingDirectory << "\n"
              << "========== END OF FILE SYSTEM SPECS ========== \n";
}

void FileSystem::formatFS(int size) {
    this->mBootSector = BootSector{size};

    std::ofstream f_out{this->mFileName, std::ios::binary | std::ios::out};

    if (!f_out.is_open())
        throw std::runtime_error(FS_OPEN_ERROR);

    // clean boot-sector and root directory
    this->write(f_out, true);

    // wipe fat tables
    FAT::wipe(f_out, this->mBootSector.mFat1StartAddress, this->mBootSector.mClusterCount);

    // label root directory
    FAT::write(f_out, this->mBootSector.mFat1StartAddress, FAT_FILE_END);
}

int FileSystem::clusterToAddress(int cluster) const {
    return this->mBootSector.mDataStartAddress + cluster * this->mBootSector.mClusterSize;
}

int FileSystem::clusterToFatAddress(int cluster) const {
    return this->mBootSector.mFat1StartAddress + cluster * static_cast<int32_t>(sizeof(int32_t));
}


int FileSystem::getDirectoryNextFreeEntryAddress(int cluster, int entriesCount) const {
    if (entriesCount > MAX_ENTRIES)
        throw std::runtime_error("entries limit reached");
    if (entriesCount < 2)
        throw std::runtime_error("internal error, directory missing references");
    return clusterToAddress(cluster) + entriesCount * DirectoryEntry::SIZE;
}

/**
 * @param de Mutates DirectoryEntry only if entry is found, in which case it
 * copies the found data into passed object.
 */
bool FileSystem::findDirectoryEntry(int cluster, const std::string &itemName, DirectoryEntry &de) {
    auto address = this->clusterToAddress(cluster);

    DirectoryEntry clusterDirectory{};
    std::ifstream stream(this->mFileName, std::ios::binary);
    stream.seekg(address);
    clusterDirectory.read(stream);
    auto entriesCount = clusterDirectory.mSize;

    if (entriesCount > MAX_ENTRIES || entriesCount < 0)
        throw std::runtime_error("internal error, file system is corrupted");

    DirectoryEntry temp{};
    for (int i = 1; i < entriesCount; i++) { // i = 1, we've already read the '.' entry.
        temp.read(stream);
        if (temp.mItemName == itemName) {
            de = std::move(temp);
            return true;
        }
    }
    return false;
}
