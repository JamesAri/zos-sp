#include "FileSystem.h"
#include "FAT.h"
#include "utils/stream-utils.h"
#include "utils/validators.h"

#include <iostream>
#include <sstream>
#include <cmath>

bool fileExists(const std::string &fileName) {
    std::ifstream stream(fileName);
    return stream.good();
}

bool isSpecialLabel(int label) {
    return label == FAT_UNUSED || label == FAT_FILE_END || label == FAT_BAD_CLUSTER;
}

// ================================================================================
// ================================================================================

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
    if (!mStream.is_open()) mStream.close();
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
    if (!mStream.is_open()) mStream.close();
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
        return;
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

bool FileSystem::editDirectoryEntry(int parentCluster, int childCluster, DirectoryEntry &de) {
    auto startAddress = clusterToDataAddress(parentCluster);
    seek(startAddress);

    DirectoryEntry tempDE{};
    for (int i = 0; i < MAX_ENTRIES; i++) {
        tempDE.read(mStream);
        if (!isAllocatedDirectoryEntry(tempDE.mItemName)) {
            if (i < DEFAULT_DIR_SIZE)
                throw std::runtime_error(DE_MISSING_REFERENCES_ERROR);
            return false;
        }
        if (tempDE.mStartCluster == childCluster) {
            auto lastAddress = startAddress + i * DirectoryEntry::SIZE;
            seek(lastAddress);
            de.write(mStream);
            return true;
        }
    }
    return false;
}

std::vector<int> FileSystem::getFreeClusters(int count, bool ordered) {
    if (count > mBootSector.mClusterCount)
        throw std::runtime_error("not enough space, format file system");

    seek(mBootSector.mFat1StartAddress);

    int32_t label;
    std::vector<int> clusters{};
    clusters.reserve(count);
    for (int32_t i = 0; i < mBootSector.mClusterCount && clusters.size() < count; i++) {
        readFromStream(mStream, label);
        if (label == FAT_UNUSED) {
            if (ordered && !clusters.empty()) {
                if (clusters.back() + 1 != i) {
                    clusters.clear();
                    continue;
                }
            }
            clusters.push_back(i);
        }
    }

    if (clusters.size() != count)
        throw std::runtime_error("not enough space, format file system or free some space");

    return clusters;
}

void FileSystem::seekStreamToDataCluster(int cluster) {
    int32_t address = clusterToDataAddress(cluster);
    seek(address);
}

int FileSystem::getDirectoryNextFreeEntryAddress(int cluster) {
    auto address = clusterToDataAddress(cluster);
    seek(address);

    DirectoryEntry temp{};
    for (int entriesCount = 0; entriesCount < MAX_ENTRIES; entriesCount++) {
        temp.read(mStream);
        if (!isAllocatedDirectoryEntry(temp.mItemName)) {
            if (entriesCount < DEFAULT_DIR_SIZE)
                throw std::runtime_error(DE_MISSING_REFERENCES_ERROR);
            return address + entriesCount * DirectoryEntry::SIZE;
        }
    }
    throw std::runtime_error(DE_LIMIT_REACHED_ERROR);
}

void FileSystem::writeNewDirectoryEntry(int directoryCluster, DirectoryEntry &newDE) {
    int32_t freeParentEntryAddr = getDirectoryNextFreeEntryAddress(directoryCluster);
    seek(freeParentEntryAddr);
    newDE.write(mStream);
}

void FileSystem::writeToFatByCluster(int cluster, int label) {
    int address = clusterToFatAddress(cluster);
    FAT::write(mStream, address, label);
}

int FileSystem::readFromFatByCluster(int cluster) {
    int address = clusterToFatAddress(cluster);
    return FAT::read(mStream, address);
}


/**
 * @param parentDE modifies item name to ".."
 * @param newDE modifies item name to "."
 */
void FileSystem::writeDirectoryEntryReferences(DirectoryEntry &parentDE, DirectoryEntry &newDE, int newFreeCluster) {
    // Erase previous cluster data
    seekStreamToDataCluster(newFreeCluster);
    auto clusterSize = mBootSector.mClusterSize;
    char emptyCluster[CLUSTER_SIZE] = {'\00'};
    mStream.write(emptyCluster, clusterSize);

    // Create new directory "." at new cluster
    seekStreamToDataCluster(newFreeCluster);
    newDE.mItemName = ".";
    newDE.write(mStream);

    // Create new directory ".." at new cluster
    parentDE.mItemName = "..";
    parentDE.write(mStream);


}

std::vector<std::string> FileSystem::getDirectoryContents(int directoryCluster) {
    std::vector<std::string> fileNames{};
    DirectoryEntry de{};
    seekStreamToDataCluster(directoryCluster);
    for (int i = 0; i < MAX_ENTRIES; i++) {
        de.read(mStream);
        if (!isAllocatedDirectoryEntry(de.mItemName)) {
            if (i < DEFAULT_DIR_SIZE)
                throw std::runtime_error(DE_MISSING_REFERENCES_ERROR);
        }
        fileNames.push_back(de.mItemName);
    }
    return fileNames;
}


bool FileSystem::directoryEntryExists(int cluster, const std::string &itemName, bool isFile) {
    DirectoryEntry temp{};
    return findDirectoryEntry(cluster, itemName, temp, isFile);
}

void FileSystem::makeFatChain(std::vector<int> &clusters) {
    for (int i = 0; i < clusters.size() - 1; i++) {
        writeToFatByCluster(clusters.at(i), clusters.at(i + 1));
    }
    writeToFatByCluster(clusters.back(), FAT_FILE_END);
}

void FileSystem::labelFatClusterChain(std::vector<int> &clusters, int32_t label) {
    for (auto &it: clusters) {
        writeToFatByCluster(it, label);
    }
}

/**
 * Returns FAT cluster chain, where last cluster points to FAT_FILE_END label.
 * E.g.:
 *  FAT chain: 1 -> 3 -> 5 -> 2 -> FAT_FILE_END
 *  Cluster chain: {1, 3, 5, 2}
 */
std::vector<int> FileSystem::getFatClusterChain(int fromCluster, int fileSize) {
    int clusterCount = getNeededClustersCount(fileSize);

    if (clusterCount > mBootSector.mClusterCount)
        throw std::runtime_error("internal error, incorrect cluster count");

    std::vector<int> clusters{};
    clusters.reserve(clusterCount);
    int curCluster = fromCluster;
    for (int i = 0; i < clusterCount - 1; i++) {
        clusters.push_back(curCluster);
        curCluster = readFromFatByCluster(curCluster);
        if (isSpecialLabel(curCluster) || curCluster >= mBootSector.mClusterCount) break;
    }
    clusters.push_back(curCluster);
    int lastLabel = readFromFatByCluster(curCluster);
    if (clusters.size() != clusterCount || lastLabel != FAT_FILE_END)
        throw std::runtime_error("filesystem corrupted"); // todo mark as bad clusters

    return clusters;
}

void FileSystem::writeFile(std::vector<int> &clusters, std::vector<char> &buffer) {
    int filesSize = static_cast<int>(buffer.size());

    if (!filesSize) return;

    int clusterSize = mBootSector.mClusterSize;
    int trailingBytes = filesSize % clusterSize;
    trailingBytes = trailingBytes ? trailingBytes : clusterSize;

    for (int i = 0; i < clusters.size() - 1; i++) {
        seekStreamToDataCluster(clusters.at(i));
        mStream.write((char *) (&buffer[i * clusterSize]), clusterSize);
    }
    seekStreamToDataCluster(clusters.back());
    mStream.write((char *) (&buffer[filesSize - trailingBytes]), trailingBytes);
    flush();
}

std::vector<char> FileSystem::readFile(std::vector<int> &clusters, int fileSize) {
    int clusterSize = mBootSector.mClusterSize;
    int trailingBytes = fileSize % clusterSize;
    trailingBytes = trailingBytes ? trailingBytes : clusterSize;

    std::vector<char> buffer(fileSize);
    for (int i = 0; i < clusters.size() - 1; i++) {
        seekStreamToDataCluster(clusters.at(i));
        mStream.read((char *) (&buffer[i * clusterSize]), clusterSize);
    }
    seekStreamToDataCluster(clusters.back());
    mStream.read((char *) (&buffer[fileSize - trailingBytes]), trailingBytes);
    return buffer;
}

DirectoryEntry
FileSystem::getLastRelativeDirectoryEntry(std::vector<std::string> &fileNames, EFileOption lastEntryOpt) {
    if(fileNames.empty()) return mWorkingDirectory;

    if (fileNames.empty())
        throw std::runtime_error("received empty path");

    DirectoryEntry parentDE{};
    int curCluster = mWorkingDirectory.mStartCluster;
    for (int i = 0; i < fileNames.size() - 1; i++) {
        if (findDirectoryEntry(curCluster, fileNames.at(i), parentDE, false)) {
            curCluster = parentDE.mStartCluster;
        } else {
            throw InvalidOptionException(PATH_NOT_FOUND_ERROR);
        }
    }
    if (lastEntryOpt == EFileOption::DIRECTORY) {
        if (!findDirectoryEntry(curCluster, fileNames.back(), parentDE, false))
            throw InvalidOptionException(PATH_NOT_FOUND_ERROR);
    } else if (lastEntryOpt == EFileOption::FILE) {
        if (!findDirectoryEntry(curCluster, fileNames.back(), parentDE, true))
            throw InvalidOptionException(FILE_NOT_FOUND_ERROR);
    } else if (!findDirectoryEntry(curCluster, fileNames.back(), parentDE)) {
        throw InvalidOptionException(PATH_NOT_FOUND_ERROR);
    }
    return parentDE;
}





