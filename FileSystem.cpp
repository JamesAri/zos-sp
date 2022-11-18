#include "FileSystem.h"
#include "utils/stream-utils.h"
#include "utils/validators.h"

#include <iostream>

DirectoryEntry::DirectoryEntry(const std::string &&mItemName, bool mIsFile, int mSize, int mStartCluster) :
        mIsFile(mIsFile), mSize(mSize), mStartCluster(mStartCluster) {
    if (mItemName.length() >= ITEM_NAME_LENGTH)
        throw std::runtime_error(DE_ITEM_NAME_LENGTH_ERROR);
    this->mItemName = mItemName + std::string(ITEM_NAME_LENGTH - mItemName.length(), '\00');
}

DirectoryEntry::DirectoryEntry(const std::string &mItemName, bool mIsFile, int mSize, int mStartCluster) :
        mIsFile(mIsFile), mSize(mSize), mStartCluster(mStartCluster) {
    if (mItemName.length() >= ITEM_NAME_LENGTH) {
        throw std::runtime_error(DE_ITEM_NAME_LENGTH_ERROR);
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

        DirectoryEntry rootDir{std::string("."), false, 0, 0};
        DirectoryEntry rootDir2{std::string(".."), false, 0, 0}; // do i need it? todo
        rootDir.write(f);
        rootDir2.write(f);
        this->mWorkingDirectory = rootDir;

        // Wipe each data cluster
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

    std::ofstream stream{this->mFileName, std::ios::binary | std::ios::out};

    if (!stream.is_open()) {
        throw std::runtime_error(FS_OPEN_ERROR);
    }

    // Clean boot-sector and root directory
    this->write(stream, true);

    // Wipe FAT tables
    FAT::wipe(stream, this->mBootSector.mFat1StartAddress, this->mBootSector.mClusterCount);

    // Label root directory cluster in FAT
    FAT::write(stream, this->mBootSector.mFat1StartAddress, FAT_FILE_END);
}

int FileSystem::clusterToDataAddress(int cluster) const {
    return this->mBootSector.mDataStartAddress + cluster * this->mBootSector.mClusterSize;
}

int FileSystem::clusterToFatAddress(int cluster) const {
    return this->mBootSector.mFat1StartAddress + cluster * static_cast<int32_t>(sizeof(int32_t));
}


int FileSystem::getDirectoryNextFreeEntryAddress(int cluster) const {
    std::ifstream stream(this->mFileName, std::ios::binary);
    auto address = this->clusterToDataAddress(cluster);
    stream.seekg(address);

    DirectoryEntry temp{};
    for (int entriesCount = 0; entriesCount < MAX_ENTRIES; entriesCount++) {
        temp.read(stream);
        if (!isAllocatedDirectoryEntry(temp.mItemName)) {
            if (entriesCount < DEFAULT_DIR_SIZE)
                throw std::runtime_error(DE_MISSING_REFERENCES_ERROR);
            return address + entriesCount * DirectoryEntry::SIZE;
        }
    }
    stream.close();
    throw std::runtime_error(DE_LIMIT_REACHED_ERROR);
}

/**
 * @param de Mutates DirectoryEntry only if entry is found, in which case it
 * copies the found data into passed object.
 */
bool FileSystem::findDirectoryEntry(int cluster, const std::string &itemName, DirectoryEntry &de) {
    std::ifstream stream(this->mFileName, std::ios::binary);

    if (!stream.is_open())
        throw std::runtime_error(FS_OPEN_ERROR);

    auto address = this->clusterToDataAddress(cluster);
    stream.seekg(address);

    DirectoryEntry tempDE{};
    auto itemNameCharArr = itemName.c_str();
    for (int i = 0; i < MAX_ENTRIES; i++) {
        tempDE.read(stream);
        if (tempDE.mItemName.empty()) {
            if (i < DEFAULT_DIR_SIZE)
                throw std::runtime_error(DE_MISSING_REFERENCES_ERROR);
            return false;
        }
        if (!strcmp(tempDE.mItemName.c_str(), itemNameCharArr)) { // ignore \00 (NULL) paddings
            de = tempDE;
            return true;
        }
    }
    return false;
}

bool FileSystem::findDirectoryEntry(int parentCluster, int childCluster, DirectoryEntry &de) {
    std::ifstream stream(this->mFileName, std::ios::binary);

    if (!stream.is_open())
        throw std::runtime_error(FS_OPEN_ERROR);


    auto address = this->clusterToDataAddress(parentCluster);

    stream.seekg(address);

    DirectoryEntry tempDE{};
    for (int i = 0; i < MAX_ENTRIES; i++) {
        tempDE.read(stream);
        if (!isAllocatedDirectoryEntry(tempDE.mItemName)) {
            if (i < DEFAULT_DIR_SIZE)
                throw std::runtime_error(DE_MISSING_REFERENCES_ERROR);
            return false;
        }
        if (tempDE.mStartCluster == childCluster) {
            de = tempDE;
            return true;
        }
    }
    return false;
}

bool FileSystem::getDirectory(int cluster, DirectoryEntry &de) {
    std::ifstream stream(this->mFileName, std::ios::binary);

    if (!stream.is_open())
        throw std::runtime_error(FS_OPEN_ERROR);

    DirectoryEntry toFindDE{}, parentDE{};

    auto address = this->clusterToDataAddress(cluster);
    stream.seekg(address);

    toFindDE.read(stream);
    if (!isAllocatedDirectoryEntry(toFindDE.mItemName)) return false;
    parentDE.read(stream);
    if (!isAllocatedDirectoryEntry(parentDE.mItemName)) return false;
    stream.close();

    if (this->findDirectoryEntry(parentDE.mStartCluster, toFindDE.mStartCluster, toFindDE)) {
        de = toFindDE;
        return true;
    }
    return false;
}

/**
 * Force delete, i.e. doesn't check if directory is empty.
 */
bool FileSystem::removeDirectoryEntry(int parentCluster, const std::string &itemName) {
    auto itemNameCharArr = itemName.c_str();

    if (!strcmp(itemNameCharArr, ".") || !strcmp(itemNameCharArr, "..")) return false;

    std::fstream stream(this->mFileName, std::ios::out | std::ios::in | std::ios::binary);

    if (!stream.is_open())
        throw std::runtime_error(FS_OPEN_ERROR);

    auto startAddress = this->clusterToDataAddress(parentCluster);
    int32_t removeAddress;
    stream.seekg(startAddress);

    DirectoryEntry tempDE{}, lastDE{};
    char empty[DirectoryEntry::SIZE] = {'\00'};
    bool erased = false;
    for (int i = 0; i < MAX_ENTRIES; i++) {
        tempDE.read(stream);
        if (!isAllocatedDirectoryEntry(tempDE.mItemName)) {
            if (i < DEFAULT_DIR_SIZE)
                throw std::runtime_error(DE_MISSING_REFERENCES_ERROR);
            if (!erased) return false; // we are at the end, and we didn't find entry to remove
            stream.seekp(removeAddress);
            lastDE.write(stream); // write last entry instead of erased entry
            auto lastAddress = startAddress + (i - 1) * DirectoryEntry::SIZE;
            stream.seekp(lastAddress);
            stream.write(empty, DirectoryEntry::SIZE); // erase last entry
            return true;
        }
        lastDE = tempDE;
        if (!erased && !strcmp(tempDE.mItemName.c_str(), itemNameCharArr)) {
            // label FAT cluster as unused
            FAT::write(stream, this->clusterToFatAddress(tempDE.mStartCluster), FAT_UNUSED);
            // remove entry from data cluster
            removeAddress = startAddress + i * DirectoryEntry::SIZE;
            stream.seekp(removeAddress);
            stream.write(empty, DirectoryEntry::SIZE); // erase entry to be removed
            erased = true;
            stream.flush();
        }
    }
    return false;
}

int FileSystem::getDirectoryEntryCount(int cluster) {
    std::ifstream stream(this->mFileName, std::ios::binary);

    if (!stream.is_open())
        throw std::runtime_error(FS_OPEN_ERROR);

    auto address = this->clusterToDataAddress(cluster);

    stream.seekg(address);

    int i;
    DirectoryEntry tempDE{};
    for (i = 0; i < MAX_ENTRIES; i++) {
        tempDE.read(stream);
        if (!isAllocatedDirectoryEntry(tempDE.mItemName)) {
            if (i < DEFAULT_DIR_SIZE)
                throw std::runtime_error(DE_MISSING_REFERENCES_ERROR);
            break;
        }
    }
    return i;
}


