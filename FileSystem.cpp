#include "FileSystem.h"
#include "utils/stream-utils.h"
#include "utils/validators.h"

#include <iostream>
#include <sstream>
#include <cmath>




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


void DirectoryEntry::write(std::fstream &f, int32_t pos) {
    f.seekp(pos);
    write(f);
}

void DirectoryEntry::read(std::fstream &f) {
    readFromStream(f, mItemName, ITEM_NAME_LENGTH);
    readFromStream(f, mIsFile);
    readFromStream(f, mSize);
    readFromStream(f, mStartCluster);
}

void DirectoryEntry::read(std::fstream &f, int32_t pos) {
    f.seekg(pos);
    read(f);
}

std::ostream &operator<<(std::ostream &os, DirectoryEntry const &di) {
    return os << "  ItemName: " << di.mItemName.c_str() << "\n"
              << "  IsFile: " << di.mIsFile << "\n"
              << "  Size: " << di.mSize << "\n"
              << "  StartCluster: " << di.mStartCluster << "\n";
}

void FAT::write(std::fstream &f, int32_t label) {
    writeToStream(f, label);
}

void FAT::write(std::fstream &f, int32_t pos, int32_t label) {
    f.seekp(pos);
    FAT::write(f, label);
}

int FAT::read(std::fstream &f) {
    int32_t clusterTag;
    readFromStream(f, clusterTag);
    return clusterTag;
}

int FAT::read(std::fstream &f, int32_t pos) {
    f.seekg(pos);
    return FAT::read(f);
}

void FAT::wipe(std::ostream &f, int32_t startAddress, int32_t clusterCount) {
    f.seekp(startAddress);

    auto labelUnused = reinterpret_cast<const char *>(&FAT_UNUSED);
    for (int i = 0; i < clusterCount; i++) {
        f.write(labelUnused, sizeof(FAT_UNUSED));
    }
}

std::vector<int> FAT::getFreeClusters(std::shared_ptr<FileSystem> &fs, int count) {
    if (count > fs->mBootSector.mClusterCount)
        throw std::runtime_error("not enough space, format file system");

    fs->seek(fs->mBootSector.mFat1StartAddress);

    int32_t label;
    std::vector<int> clusters{};
    clusters.reserve(count);
    for (int32_t i = 0; i < fs->mBootSector.mClusterCount && clusters.size() < count; i++) {
        readFromStream(fs->mStream, label);
        if (label == FAT_UNUSED) {
            clusters.push_back(i);
        }
    }

    if (clusters.size() != count)
        throw std::runtime_error("not enough space, format file system or free some space");

    return clusters;
}


BootSector::BootSector(int diskSize) : mDiskSize(diskSize * FORMAT_UNIT) {
    mSignature = SIGNATURE;
    mClusterSize = CLUSTER_SIZE;
    mFatCount = FAT_COUNT;

    size_t freeSpaceInBytes = mDiskSize - BootSector::SIZE;
    mClusterCount = static_cast<int>(freeSpaceInBytes / (sizeof(int32_t) + mClusterSize));
    mFatSize = static_cast<int>(mClusterCount * sizeof(int32_t));
    mFat1StartAddress = BootSector::SIZE;

    size_t dataSize = mClusterCount * mClusterSize;
    size_t fatTablesSize = mFatSize * mFatCount;
    mPaddingSize = static_cast<int>(freeSpaceInBytes - (dataSize + fatTablesSize));

    auto fatEndAddress = mFat1StartAddress + mFatSize;
    mDataStartAddress = static_cast<int>(mPaddingSize + fatEndAddress);
}

void BootSector::write(std::fstream &f) {
    writeToStream(f, mSignature, SIGNATURE_LENGTH);
    writeToStream(f, mClusterSize);
    writeToStream(f, mClusterCount);
    writeToStream(f, mDiskSize);
    writeToStream(f, mFatCount);
    writeToStream(f, mFat1StartAddress);
    writeToStream(f, mDataStartAddress);
    writeToStream(f, mPaddingSize);
}

void BootSector::read(std::fstream &f) {
    readFromStream(f, mSignature, SIGNATURE_LENGTH);
    readFromStream(f, mClusterSize);
    readFromStream(f, mClusterCount);
    readFromStream(f, mDiskSize);
    readFromStream(f, mFatCount);
    readFromStream(f, mFat1StartAddress);
    readFromStream(f, mDataStartAddress);
    readFromStream(f, mPaddingSize);

    mFatSize = static_cast<int>(mClusterCount * sizeof(int32_t));
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

bool fileExists(const std::string &fileName) {
    std::ifstream stream(fileName);
    return stream.good();
}

FileSystem::FileSystem(std::string &fileName) : mFileName(fileName) {
    bool exists = fileExists(fileName);

    if (exists) {
        try {
            readVFS();
        } catch (...) {
            std::cerr << "An error occurred during the loading process.\nInput file might be corrupted." << std::endl;
        }
    } else {
        formatFS();
        flush();
    }
}

void FileSystem::readVFS() {
    if(!mStream.is_open()) mStream.close();
    auto mode = std::ios_base::in | std::ios_base::out | std::ios_base::binary | std::ios_base::ate;
    mStream = std::fstream(mFileName, mode);
    mStream.seekg(0, std::ios::beg);

    mBootSector.read(mStream);
    seek(mBootSector.mDataStartAddress);
    mWorkingDirectory.read(mStream);
}

std::ostream &operator<<(std::ostream &os, FileSystem const &fs) {
    return os << "==========    FILE SYSTEM SPECS    ========== \n\n"
              << "BOOT-SECTOR (" << BootSector::SIZE << "B)\n" << fs.mBootSector << "\n"
              << "FAT count: " << fs.mBootSector.mFatCount << "\n"
              << "FAT size: " << fs.mBootSector.getFatSize() << "\n\n"
              << "PWD (root dir):\n" << fs.mWorkingDirectory << "\n"
              << "========== END OF FILE SYSTEM SPECS ========== \n";
}

void FileSystem::formatFS(int diskSize) {
    if(!mStream.is_open()) mStream.close();
    auto mode = std::ios_base::in | std::ios_base::out | std::ios_base::binary | std::ios_base::trunc;
    mStream = std::fstream(mFileName, mode);

    // Write boot-sector
    mBootSector = BootSector{diskSize};
    mBootSector.write(mStream);

    // Wipe each data cluster
    seek(mBootSector.mDataStartAddress);
    char wipedCluster[CLUSTER_SIZE] = {'\00'};
    for (int i = 0; i < mBootSector.mClusterCount; i++) {
        writeToStream(mStream, wipedCluster, CLUSTER_SIZE);
    }

    // Make root directory
    DirectoryEntry rootDir{std::string("."), false, 0, 0};
    DirectoryEntry rootDir2{std::string(".."), false, 0, 0}; // do i need it? todo
    mWorkingDirectory = rootDir;
    seek(mBootSector.mDataStartAddress);
    rootDir.write(mStream);
    rootDir2.write(mStream);

    // Wipe FAT tables
    FAT::wipe(mStream, mBootSector.mFat1StartAddress, mBootSector.mClusterCount);

    // Label root directory cluster in FAT
    FAT::write(mStream, mBootSector.mFat1StartAddress, FAT_FILE_END);
}

bool FileSystem::findDirectoryEntry(int cluster, const std::string &itemName, DirectoryEntry &de) {
    seek(clusterToDataAddress(cluster));

    DirectoryEntry tempDE{};
    auto itemNameCharArr = itemName.c_str();
    for (int i = 0; i < MAX_ENTRIES; i++) {
        tempDE.read(mStream);
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

/**
 * @param de Mutates DirectoryEntry only if entry is found, in which case it
 * copies the found data into passed object.
 */
bool FileSystem::findDirectoryEntry(int cluster, const std::string &itemName, DirectoryEntry &de, bool isFile) {
    seek(clusterToDataAddress(cluster));

    DirectoryEntry tempDE{};
    auto itemNameCharArr = itemName.c_str();
    for (int i = 0; i < MAX_ENTRIES; i++) {
        tempDE.read(mStream);
        if (tempDE.mItemName.empty()) {
            if (i < DEFAULT_DIR_SIZE)
                throw std::runtime_error(DE_MISSING_REFERENCES_ERROR);
            return false;
        }
        if (!strcmp(tempDE.mItemName.c_str(), itemNameCharArr) &&
            tempDE.mIsFile == isFile) { // ignore \00 (NULL) paddings
            de = tempDE;
            return true;
        }
    }
    return false;
}

bool FileSystem::findDirectoryEntry(int parentCluster, int childCluster, DirectoryEntry &de) {
    seek(clusterToDataAddress(parentCluster));

    DirectoryEntry tempDE{};
    for (int i = 0; i < MAX_ENTRIES; i++) {
        tempDE.read(mStream);
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

/**
 * Resolves "." DirectoryEntry reference from passed cluster id and returns the
 * same DirectoryEntry, but with name from parent directory (directory name).
 * @param de DirectoryEntry with correct name.
 * @return True on success, false otherwise.
 */
bool FileSystem::getDirectory(int cluster, DirectoryEntry &de) {
    DirectoryEntry toFindDE{}, parentDE{};

    seek(clusterToDataAddress(cluster));

    toFindDE.read(mStream);
    if (!isAllocatedDirectoryEntry(toFindDE.mItemName)) return false;

    parentDE.read(mStream);
    if (!isAllocatedDirectoryEntry(parentDE.mItemName)) return false;

    if (findDirectoryEntry(parentDE.mStartCluster, toFindDE.mStartCluster, toFindDE)) {
        de = toFindDE;
        return true;
    }
    return false;
}

/**
 * Force delete, i.e. doesn't check if directory is empty.
 */
bool FileSystem::removeDirectoryEntry(int parentCluster, const std::string &itemName, bool isFile) {
    auto itemNameCharArr = itemName.c_str();

    if (!isFile && (!strcmp(itemNameCharArr, ".") || !strcmp(itemNameCharArr, ".."))) return false;

    auto startAddress = clusterToDataAddress(parentCluster);
    seek(startAddress);
    int32_t removeAddress;

    DirectoryEntry tempDE{}, lastDE{};
    char emptyBfr[DirectoryEntry::SIZE] = {'\00'};
    bool erased = false;
    for (int i = 0; i < MAX_ENTRIES; i++) {
        tempDE.read(mStream);
        if (!isAllocatedDirectoryEntry(tempDE.mItemName)) { // we are at the end
            if (i < DEFAULT_DIR_SIZE)
                throw std::runtime_error(DE_MISSING_REFERENCES_ERROR);
            if (!erased) return false; // we are at the end, and we didn't find entry to remove
            seek(removeAddress);
            lastDE.write(mStream); // write last entry instead of erased entry
            auto lastAddress = startAddress + (i - 1) * DirectoryEntry::SIZE;
            seek(lastAddress);
            mStream.write(emptyBfr, DirectoryEntry::SIZE); // erase last entry
            return true;
        }
        lastDE = tempDE;
        if (!erased && !strcmp(tempDE.mItemName.c_str(), itemNameCharArr) && tempDE.mIsFile == isFile) {
            // remove entry from data cluster
            removeAddress = startAddress + i * DirectoryEntry::SIZE;
            seek(removeAddress);
            mStream.write(emptyBfr, DirectoryEntry::SIZE); // erase entry to be removed
            erased = true;
            flush();
        }
    }
    return false;
}

int FileSystem::getDirectoryEntryCount(int cluster) {
    seek(clusterToDataAddress(cluster));

    int i;
    DirectoryEntry tempDE{};
    for (i = 0; i < MAX_ENTRIES; i++) {
        tempDE.read(mStream);
        if (!isAllocatedDirectoryEntry(tempDE.mItemName)) {
            if (i < DEFAULT_DIR_SIZE)
                throw std::runtime_error(DE_MISSING_REFERENCES_ERROR);
            break;
        }
    }
    return i;
}

int FileSystem::getNeededClustersCount(int fileSize) const {
    return std::ceil(fileSize / static_cast<double>(mBootSector.mClusterSize));
}

int FileSystem::clusterToDataAddress(int cluster) const {
    return mBootSector.mDataStartAddress + cluster * mBootSector.mClusterSize;
}

int FileSystem::clusterToFatAddress(int cluster) const {
    return mBootSector.mFat1StartAddress + cluster * static_cast<int32_t>(sizeof(int32_t));
}

void FileSystem::seek(int pos) {
    mStream.seekp(pos);
}

void FileSystem::flush() {
    mStream.flush();
}

void FileSystem::updateWorkingDirectoryPath() {
    if (mWorkingDirectory.mStartCluster == 0) {
        mWorkingDirectoryPath = "/";
    }
    std::queue<std::string> fileNames{};

    DirectoryEntry de = mWorkingDirectory;

    int childCluster = de.mStartCluster, parentCluster;
    int safetyCounter = 0;
    while (true) {
        safetyCounter++;
        if (safetyCounter > MAX_ENTRIES)
            throw std::runtime_error(CORRUPTED_FS_ERROR);

        if (childCluster == 0) break;

        if (!findDirectoryEntry(childCluster, "..", de, false))
            throw std::runtime_error(CORRUPTED_FS_ERROR);

        parentCluster = de.mStartCluster;

        if (!findDirectoryEntry(parentCluster, childCluster, de))
            throw std::runtime_error(CORRUPTED_FS_ERROR);

        childCluster = parentCluster;

        fileNames.push(de.mItemName);
    }
    std::stringstream stream{};

    while (!fileNames.empty()) {
        stream << "/" << fileNames.front().c_str();
        fileNames.pop();
    }
    mWorkingDirectoryPath = stream.str();
}

std::string FileSystem::getWorkingDirectoryPath() {
    return mWorkingDirectoryPath;
}





