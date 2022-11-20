#include "Commands.h"
#include "utils/string-utils.h"
#include "utils/validators.h"

#include <algorithm>
#include <fstream>
#include <cmath>

void pathCheck(const std::string &path) {
    if (!validateFilePath(path))
        throw InvalidOptionException(INVALID_DIR_PATH_ERROR);
}

bool isSpecialLabel(int label) {
    return label == FAT_UNUSED || label == FAT_FILE_END || label == FAT_BAD_CLUSTER;
}

void seekStreamToDataCluster(std::shared_ptr<FileSystem> &fs, int cluster) {
    int32_t address = fs->clusterToDataAddress(cluster);
    fs->seek(address);
}

int getDirectoryNextFreeEntryAddress(std::shared_ptr<FileSystem> &fs, int cluster) {
    auto address = fs->clusterToDataAddress(cluster);
    fs->seek(address);

    DirectoryEntry temp{};
    for (int entriesCount = 0; entriesCount < MAX_ENTRIES; entriesCount++) {
        temp.read(fs->mStream);
        if (!isAllocatedDirectoryEntry(temp.mItemName)) {
            if (entriesCount < DEFAULT_DIR_SIZE)
                throw std::runtime_error(DE_MISSING_REFERENCES_ERROR);
            return address + entriesCount * DirectoryEntry::SIZE;
        }
    }
    throw std::runtime_error(DE_LIMIT_REACHED_ERROR);
}

void writeNewDirectoryEntry(std::shared_ptr<FileSystem> &fs, int directoryCluster, DirectoryEntry &newDE) {
    int32_t freeParentEntryAddr = getDirectoryNextFreeEntryAddress(fs, directoryCluster);
    fs->seek(freeParentEntryAddr);
    newDE.write(fs->mStream);
}

void writeToFatByCluster(std::shared_ptr<FileSystem> &fs, int cluster, int label) {
    int address = fs->clusterToFatAddress(cluster);
    FAT::write(fs->mStream, address, label);
}

int readFromFatByCluster(std::shared_ptr<FileSystem> &fs, int cluster) {
    int address = fs->clusterToFatAddress(cluster);
    return FAT::read(fs->mStream, address);
}


/**
 * @param parentDE modifies item name to ".."
 * @param newDE modifies item name to "."
 */
void writeDirectoryEntryReferences(std::shared_ptr<FileSystem> &fs, DirectoryEntry &parentDE, DirectoryEntry &newDE,
                                   int newFreeCluster) {
    // Create new directory "." at new cluster
    seekStreamToDataCluster(fs, newFreeCluster);
    newDE.mItemName = ".";
    newDE.write(fs->mStream);

    // Create new directory ".." at new cluster
    parentDE.mItemName = "..";
    parentDE.write(fs->mStream);
}

std::vector<std::string> getDirectoryContents(std::shared_ptr<FileSystem> &fs, int directoryCluster) {
    std::vector<std::string> fileNames{};
    DirectoryEntry de{};
    seekStreamToDataCluster(fs, directoryCluster);
    for (int i = 0; i < MAX_ENTRIES; i++) {
        de.read(fs->mStream);
        if (!isAllocatedDirectoryEntry(de.mItemName)) {
            if (i < DEFAULT_DIR_SIZE)
                throw std::runtime_error(DE_MISSING_REFERENCES_ERROR);
        }
        fileNames.push_back(de.mItemName);
    }
    return fileNames;
}


bool directoryEntryExists(std::shared_ptr<FileSystem> &fs, int cluster, const std::string &itemName) {
    DirectoryEntry temp{};
    return fs->findDirectoryEntry(cluster, itemName, temp);
}

void labelFatChain(std::shared_ptr<FileSystem> &fs, std::vector<int> &clusters) {
    for (int i = 0; i < clusters.size() - 1; i++) {
        writeToFatByCluster(fs, clusters.at(i), clusters.at(i + 1));
    }
    writeToFatByCluster(fs, clusters.back(), FAT_FILE_END);
}

/**
 * Returns FAT cluster chain, where last cluster points to FAT_FILE_END label.
 * E.g.:
 *  FAT chain: 1 -> 3 -> 5 -> 2 -> FAT_FILE_END
 *  Cluster chain: {1, 3, 5, 2}
 */
std::vector<int> getFatClusterChain(std::shared_ptr<FileSystem> &fs, int fromCluster, int fileSize) {
    int clusterCount = fs->getNeededClustersCount(fileSize);

    if (clusterCount > MAX_ENTRIES)
        throw std::runtime_error("internal error, incorrect cluster count");

    std::vector<int> clusters{};
    int curCluster = fromCluster;
    for (int i = 0; i < clusterCount; i++) {
        clusters.push_back(curCluster);
        curCluster = readFromFatByCluster(fs, curCluster);
        if (isSpecialLabel(curCluster) || curCluster >= fs->mBootSector.mClusterCount) break;
    }
    int lastLabel = readFromFatByCluster(fs, curCluster);
    if (clusters.size() != clusterCount || lastLabel != FAT_FILE_END)
        throw std::runtime_error("filesystem corrupted"); // todo mark as bad clusters

    return clusters;
}

void writeFile(std::shared_ptr<FileSystem> &fs, std::vector<int> &clusters, std::vector<char> &buffer) {
    int filesSize = static_cast<int>(buffer.size());

    if (!filesSize) return;

    int clusterSize = fs->mBootSector.mClusterSize;
    int trailingBytes = filesSize % clusterSize;

    for (int i = 0; i < clusters.size() - 1; i++) {
        seekStreamToDataCluster(fs, clusters.at(i));
        fs->mStream.write((char *) (&buffer[i * clusterSize]), clusterSize);
    }
    seekStreamToDataCluster(fs, clusters.back());
    fs->mStream.write((char *) (&buffer[filesSize - trailingBytes]), trailingBytes);
    fs->flush();
}

std::vector<char> readFile(std::shared_ptr<FileSystem> &fs, std::vector<int> &clusters, int fileSize) {
    int clusterSize = fs->mBootSector.mClusterSize;
    int trailingBytes = fileSize % clusterSize;

    std::vector<char> buffer{};
    buffer.reserve(fileSize);
    for (int i = 0; i < clusters.size() - 1; i++) {
        seekStreamToDataCluster(fs, clusters.at(i));
        fs->mStream.read((char *) (&buffer[i * clusterSize]), clusterSize);
    }
    seekStreamToDataCluster(fs, clusters.back());
    fs->mStream.read((char *) (&buffer[fileSize - trailingBytes]), trailingBytes);
    return buffer;
}

DirectoryEntry
getLastDirectoryEntry(std::shared_ptr<FileSystem> &fs, int startCluster, std::vector<std::string> &fileNames) {
    DirectoryEntry parentDE{};

    if (fileNames.empty())
        throw std::runtime_error("received empty path");

    int curCluster = startCluster;
    for (auto it{fileNames.begin()}; it != std::prev(fileNames.end()); it++) {
        if (fs->findDirectoryEntry(curCluster, *it, parentDE)) {
            curCluster = parentDE.mStartCluster;
        } else {
            throw InvalidOptionException(PATH_NOT_FOUND_ERROR);
        }
    }
    return parentDE;
}


bool CpCommand::run() {
    auto newFileName = mAccumulator.back();

    DirectoryEntry parentDE = mFS->mWorkingDirectory;

    if (mAccumulator.size() > 1) {
        int curCluster = parentDE.mStartCluster;
        for (auto it{mAccumulator.begin()}; it != std::prev(mAccumulator.end()); it++) {
            if (mFS->findDirectoryEntry(curCluster, *it, parentDE)) {
                curCluster = parentDE.mStartCluster;
            } else {
                throw InvalidOptionException(PATH_NOT_FOUND_ERROR);
            }
        }
    }

    if (directoryEntryExists(mFS, parentDE.mStartCluster, newFileName))
        mFS->removeDirectoryEntry(parentDE.mStartCluster, newFileName);

    // From clusters
    auto fromClusters = getFatClusterChain(mFS, mFromDE.mStartCluster, mFromDE.mSize);

    // Get free clusters
    auto freeClusters = FAT::getFreeClusters(mFS, static_cast<int>(fromClusters.size()));

    // Mark clusters in FAT tables
    labelFatChain(mFS, freeClusters);

    // Move data
    auto fileData = readFile(mFS, fromClusters, mFromDE.mSize);
    writeFile(mFS, freeClusters, fileData);

    // Write directory entry
    DirectoryEntry newFileDE{newFileName, true, mFromDE.mSize, freeClusters.at(0)};
    writeNewDirectoryEntry(mFS, parentDE.mStartCluster, newFileDE);
    return true;
}

bool CpCommand::validate_arguments() {
    if (mOptCount != 2) return false;

    pathCheck(mOpt1);
    pathCheck(mOpt2);

    auto fileNames = split(mOpt1, "/");
    DirectoryEntry fromDE{};
    int curCluster = mFS->mWorkingDirectory.mStartCluster;
    for (auto &it: fileNames) {
        if (mFS->findDirectoryEntry(curCluster, it, fromDE)) {
            curCluster = fromDE.mStartCluster;
        } else {
            throw InvalidOptionException(PATH_NOT_FOUND_ERROR + " (first argument)");
        }
    }

    if (!fromDE.mIsFile)
        throw InvalidOptionException("cannot copy directory");

    mFromDE = fromDE;
    mAccumulator = split(mOpt2, "/");
    return true;
}

bool MvCommand::run() {
    return true;
}

bool MvCommand::validate_arguments() {
    return mOptCount == 2;
}

bool RmCommand::run() {
    return true;
}

bool RmCommand::validate_arguments() {
    return mOptCount == 2;
}

bool MkdirCommand::run() {
    auto newDirectoryName = mAccumulator.back();
    mAccumulator.pop_back();

    DirectoryEntry parentDE{};
    if (mAccumulator.size() == 1) // we have only new name of directory in accumulator
        parentDE = mFS->mWorkingDirectory;
    else
        parentDE = getLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator);

    if (directoryEntryExists(mFS, parentDE.mStartCluster, newDirectoryName))
        throw InvalidOptionException(EXIST_ERROR);

    int32_t newFreeCluster = FAT::getFreeClusters(mFS).back();

    // Update parent directory with the new directory entry
    DirectoryEntry newDE{newDirectoryName, false, 0, newFreeCluster};
    writeNewDirectoryEntry(mFS, parentDE.mStartCluster, newDE);

    // Write references ".' and ".."
    writeDirectoryEntryReferences(mFS, parentDE, newDE, newFreeCluster);

    // All went ok, label new cluster as allocated
    writeToFatByCluster(mFS, newFreeCluster, FAT_FILE_END);
    return true;
}

bool MkdirCommand::validate_arguments() {
    if (mOptCount != 1) return false;
    pathCheck(mOpt1);
    mAccumulator = split(mOpt1, "/");
    return true;
}

bool RmdirCommand::run() {
    DirectoryEntry de{};

    if (!mFS->findDirectoryEntry(mFS->mWorkingDirectory.mStartCluster, mOpt1, de))
        throw InvalidOptionException(FILE_NOT_FOUND_ERROR);

    if (mFS->getDirectoryEntryCount(de.mStartCluster) > DEFAULT_DIR_SIZE)
        throw InvalidOptionException(NOT_EMPTY_ERROR);

    return mFS->removeDirectoryEntry(mFS->mWorkingDirectory.mStartCluster, mOpt1);
}

bool RmdirCommand::validate_arguments() {
    if (mOptCount != 1) return false;
    return validateFileName(mOpt1);
}

bool LsCommand::run() {
    // Resolve path
    DirectoryEntry de{};
    if (mAccumulator.empty())
        de = mFS->mWorkingDirectory;
    else
        de = getLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator);

    if (de.mIsFile)
        throw InvalidOptionException(LS_FILE_ERROR);

    // Get filenames
    auto fileNames = getDirectoryContents(mFS, de.mStartCluster);

    for (auto &fn: fileNames) {
        std::cout << fn.c_str() << " ";
    }
    std::cout << std::endl;
    return true;
}

bool LsCommand::validate_arguments() {
    if (mOptCount == 0) return true;
    if (mOptCount != 1) return false;
    pathCheck(mOpt1);
    mAccumulator = split(mOpt1, "/");
    return true;
}

bool CatCommand::run() {
    return true;
}

bool CatCommand::validate_arguments() {
    return mOptCount == 1;
}

bool CdCommand::run() {
    DirectoryEntry de = getLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator);

    if (de.mIsFile)
        throw InvalidOptionException(CD_FILE_ERROR);

    // If it is reference, get correct name from parent directory
    if (!strcmp(de.mItemName.c_str(), "..") || !strcmp(de.mItemName.c_str(), ".")) {
        if (!mFS->getDirectory(de.mStartCluster, de))
            throw std::runtime_error(CORRUPTED_FS_ERROR);
    }

    mFS->mWorkingDirectory = de;
    return true;
}

bool CdCommand::validate_arguments() {
    if (mOptCount != 1) return false;
    pathCheck(mOpt1);
    mAccumulator = split(mOpt1, "/");
    return true;
}

bool PwdCommand::run() {
    if (mFS->mWorkingDirectory.mStartCluster == 0) {
        std::cout << "/" << std::endl;
        return true;
    }

    std::vector<std::string> fileNames{};

    DirectoryEntry de = mFS->mWorkingDirectory;

    int childCluster = de.mStartCluster, parentCluster;
    int safetyCounter = 0;
    while (true) {
        safetyCounter++;
        if (safetyCounter > MAX_ENTRIES)
            throw std::runtime_error(CORRUPTED_FS_ERROR);

        if (childCluster == 0) break;

        if (!mFS->findDirectoryEntry(childCluster, "..", de))
            throw std::runtime_error(CORRUPTED_FS_ERROR);

        parentCluster = de.mStartCluster;

        if (!mFS->findDirectoryEntry(parentCluster, childCluster, de))
            throw std::runtime_error(CORRUPTED_FS_ERROR);

        childCluster = parentCluster;

        fileNames.push_back(de.mItemName);
    }
    for (auto it = fileNames.rbegin(); it != fileNames.rend(); ++it) {
        std::cout << "/" << it->c_str();
    }
    std::cout << std::endl;
    return true;
}

bool PwdCommand::validate_arguments() {
    return mOptCount == 0;
}

bool InfoCommand::run() {
    DirectoryEntry de = getLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator);

    if (!de.mIsFile) {
        std::cout << de << std::endl;
        return true;
    }

    auto clusters = getFatClusterChain(mFS, de.mStartCluster, de.mSize);

    for (auto it{clusters.begin()}; it != std::prev(clusters.end()); it++) { // todo correct?
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    return true;
}

bool InfoCommand::validate_arguments() {
    if (mOptCount != 1) return false;
    pathCheck(mOpt1);
    mAccumulator = split(mOpt1, "/");
    return true;
}

bool IncpCommand::run() {
    int fileSize = static_cast<int>(mBuffer.size());
    int neededClusters = (fileSize) ? mFS->getNeededClustersCount(fileSize) : 1;

    // Get free clusters
    auto clusters = FAT::getFreeClusters(mFS, neededClusters);

    // Mark clusters in FAT tables
    labelFatChain(mFS, clusters);

    // Move data
    writeFile(mFS, clusters, mBuffer);

    auto newFileName = mAccumulator.back();
    mAccumulator.pop_back();

    DirectoryEntry parentDE{};
    if (mAccumulator.size() == 1) // we have only new name of file in accumulator
        parentDE = mFS->mWorkingDirectory;
    else
        parentDE = getLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator);

    DirectoryEntry newFileDE{newFileName, true, fileSize, clusters.at(0)};
    writeNewDirectoryEntry(mFS, parentDE.mStartCluster, newFileDE);
    return true;
}

bool IncpCommand::validate_arguments() {
    if (mOptCount != 2) return false;

    std::ifstream stream(mOpt1, std::ios::binary | std::ios::ate);

    if (!stream.good())
        throw InvalidOptionException(FILE_NOT_FOUND_ERROR);

    std::streamsize size = stream.tellg();
    stream.seekg(0, std::ios::beg);

    mBuffer = std::vector<char>(size);
    if (!stream.read(mBuffer.data(), size))
        throw std::runtime_error(FILE_READ_ERROR);

    stream.close();

    pathCheck(mOpt2);
    mAccumulator = split(mOpt2, "/");
    return true;
}

bool OutcpCommand::run() {
    return true;
}

bool OutcpCommand::validate_arguments() {
    return mOptCount == 2;
}

bool LoadCommand::run() {
    return true;
}

bool LoadCommand::validate_arguments() {
    return mOptCount == 1;
}

bool FormatCommand::run() {
    try {
        mFS->formatFS(std::stoi(mOpt1));
    } catch (...) {
        std::cerr << "internal error, couldn't format file system" << std::endl;
        exit(1);
    }
    return true;
}

bool FormatCommand::validate_arguments() {
    if (mOptCount != 1) return false;

    std::transform(mOpt1.begin(), mOpt1.end(), mOpt1.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    auto pos = mOpt1.find(ALLOWED_FORMATS[0]);

    if (pos == std::string::npos)
        throw InvalidOptionException(CANNOT_CREATE_FILE_ERROR + " (wrong unit)");

    mOpt1.erase(pos, ALLOWED_FORMATS[0].length());

    if (!is_number(mOpt1))
        throw InvalidOptionException(CANNOT_CREATE_FILE_ERROR + " (not a number)");

    return true;
}

bool DefragCommand::run() {
    return true;
}

bool DefragCommand::validate_arguments() {
    return mOptCount == 2;
}
