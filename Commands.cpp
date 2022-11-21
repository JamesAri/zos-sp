#include "Commands.h"
#include "utils/string-utils.h"
#include "utils/validators.h"

#include <algorithm>
#include <fstream>
#include <cmath>

enum class EFileOption {
    FILE,
    DIRECTORY,
    UNSPECIFIED,
};

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
    // Erase previous cluster data
    seekStreamToDataCluster(fs, newFreeCluster);
    auto clusterSize = fs->mBootSector.mClusterSize;
    char emptyCluster[CLUSTER_SIZE] = {'\00'};
    fs->mStream.write(emptyCluster, clusterSize);

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


bool directoryEntryExists(std::shared_ptr<FileSystem> &fs, int cluster, const std::string &itemName, bool isFile) {
    DirectoryEntry temp{};
    return fs->findDirectoryEntry(cluster, itemName, temp, isFile);
}

void makeFatChain(std::shared_ptr<FileSystem> &fs, std::vector<int> &clusters) {
    for (int i = 0; i < clusters.size() - 1; i++) {
        writeToFatByCluster(fs, clusters.at(i), clusters.at(i + 1));
    }
    writeToFatByCluster(fs, clusters.back(), FAT_FILE_END);
}

void labelFatClusterChain(std::shared_ptr<FileSystem> &fs, std::vector<int> &clusters, const int32_t label) {
    for (auto &it : clusters) {
        writeToFatByCluster(fs, it, label);
    }
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
    clusters.reserve(clusterCount);
    int curCluster = fromCluster;
    for (int i = 0; i < clusterCount - 1; i++) {
        clusters.push_back(curCluster);
        curCluster = readFromFatByCluster(fs, curCluster);
        if (isSpecialLabel(curCluster) || curCluster >= fs->mBootSector.mClusterCount) break;
    }
    clusters.push_back(curCluster);
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

    std::vector<char> buffer(fileSize);
    for (int i = 0; i < clusters.size() - 1; i++) {
        seekStreamToDataCluster(fs, clusters.at(i));
        fs->mStream.read((char *) (&buffer[i * clusterSize]), clusterSize);
    }
    seekStreamToDataCluster(fs, clusters.back());
    fs->mStream.read((char *) (&buffer[fileSize - trailingBytes]), trailingBytes);
    return buffer;
}

DirectoryEntry getPathLastDirectoryEntry(std::shared_ptr<FileSystem> &fs,
                                         int startCluster,
                                         std::vector<std::string> &fileNames,
                                         EFileOption lastEntryOpt = EFileOption::UNSPECIFIED) {
    DirectoryEntry parentDE{};

    if (fileNames.empty())
        throw std::runtime_error("received empty path");

    int curCluster = startCluster;
    for (int i = 0; i < fileNames.size() - 1; i++) {
        if (fs->findDirectoryEntry(curCluster, fileNames.at(i), parentDE, false)) {
            curCluster = parentDE.mStartCluster;
        } else {
            throw InvalidOptionException(PATH_NOT_FOUND_ERROR);
        }
    }
    if (lastEntryOpt == EFileOption::DIRECTORY) {
        if (!fs->findDirectoryEntry(curCluster, fileNames.back(), parentDE, false))
            throw InvalidOptionException(PATH_NOT_FOUND_ERROR);
    } else if (lastEntryOpt == EFileOption::FILE) {
        if (!fs->findDirectoryEntry(curCluster, fileNames.back(), parentDE, true))
            throw InvalidOptionException(FILE_NOT_FOUND_ERROR);
    } else if (!fs->findDirectoryEntry(curCluster, fileNames.back(), parentDE)) {
        throw InvalidOptionException(PATH_NOT_FOUND_ERROR);
    }
    return parentDE;
}


bool CpCommand::run() {
    auto newFileName = mAccumulator.back();
    mAccumulator.pop_back();

    DirectoryEntry parentDE;
    if (mAccumulator.empty())
        parentDE = mFS->mWorkingDirectory;
    else
        parentDE = getPathLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator,
                                             EFileOption::DIRECTORY);

    if (directoryEntryExists(mFS, parentDE.mStartCluster, newFileName, true))
        throw InvalidOptionException(EXIST_ERROR);

    // From clusters
    auto fromClusters = getFatClusterChain(mFS, mFromDE.mStartCluster, mFromDE.mSize);

    // Get free clusters
    auto freeClusters = FAT::getFreeClusters(mFS, static_cast<int>(fromClusters.size()));

    // Mark clusters in FAT tables
    makeFatChain(mFS, freeClusters);

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
    DirectoryEntry fromDE = getPathLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, fileNames,
                                                      EFileOption::FILE);

    mFromDE = fromDE;
    mAccumulator = split(mOpt2, "/");
    return true;
}

bool MvCommand::run() {
    auto fromDEFile = getPathLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator1,
                                                EFileOption::FILE);
    mAccumulator1.pop_back();
    DirectoryEntry fromDEDirectory{};
    if (mAccumulator1.empty())
        fromDEDirectory = mFS->mWorkingDirectory;
    else
        fromDEDirectory = getPathLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator1,
                                                    EFileOption::DIRECTORY);

    auto newItemName = mAccumulator2.back();
    mAccumulator2.pop_back();
    DirectoryEntry toDEDirectory{};
    if (mAccumulator2.empty())
        toDEDirectory = mFS->mWorkingDirectory;
    else
        toDEDirectory = getPathLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator2,
                                                  EFileOption::DIRECTORY);

    mFS->removeDirectoryEntry(fromDEDirectory.mStartCluster, fromDEFile.mItemName, true);
    fromDEFile.mItemName = newItemName;
    writeNewDirectoryEntry(mFS, toDEDirectory.mStartCluster, fromDEFile);
    return true;
}

bool MvCommand::validate_arguments() {
    if (mOptCount != 2) return false;
    pathCheck(mOpt1);
    pathCheck(mOpt2);
    mAccumulator1 = split(mOpt1, "/");
    mAccumulator2 = split(mOpt2, "/");
    return true;
}

bool RmCommand::run() {
    auto fileDE = getPathLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator,
                                                EFileOption::FILE);
    mAccumulator.pop_back();
    DirectoryEntry directoryDE{};
    if (mAccumulator.empty())
        directoryDE = mFS->mWorkingDirectory;
    else
        directoryDE = getPathLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator,
                                                    EFileOption::DIRECTORY);

    auto clusters = getFatClusterChain(mFS, fileDE.mStartCluster, fileDE.mSize);
    labelFatClusterChain(mFS, clusters, FAT_UNUSED);
    mFS->removeDirectoryEntry(directoryDE.mStartCluster, fileDE.mItemName, true);
    return true;
}

bool RmCommand::validate_arguments() {
    if (mOptCount != 1) return false;
    pathCheck(mOpt1);
    mAccumulator = split(mOpt1, "/");
    return true;
}



bool MkdirCommand::run() {
    auto newDirectoryName = mAccumulator.back();
    mAccumulator.pop_back();

    DirectoryEntry parentDE{};
    if (mAccumulator.empty()) // we have only new name of directory in accumulator
        parentDE = mFS->mWorkingDirectory;
    else
        parentDE = getPathLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator,
                                             EFileOption::DIRECTORY);

    if (directoryEntryExists(mFS, parentDE.mStartCluster, newDirectoryName, false))
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

    if (!mFS->findDirectoryEntry(mFS->mWorkingDirectory.mStartCluster, mOpt1, de, false))
        throw InvalidOptionException(FILE_NOT_FOUND_ERROR);

    if (mFS->getDirectoryEntryCount(de.mStartCluster) > DEFAULT_DIR_SIZE)
        throw InvalidOptionException(NOT_EMPTY_ERROR);

    mFS->removeDirectoryEntry(mFS->mWorkingDirectory.mStartCluster, de.mItemName, false);
    // label FAT cluster as unused
    writeToFatByCluster(mFS, de.mStartCluster, FAT_UNUSED);
    return true;
}

bool RmdirCommand::validate_arguments() {
    if (mOptCount != 1) return false;
    // todo path
    return validateFileName(mOpt1);
}

bool LsCommand::run() {
    // Resolve path
    DirectoryEntry de{};
    if (mAccumulator.empty())
        de = mFS->mWorkingDirectory;
    else
        de = getPathLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator, EFileOption::DIRECTORY);

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
    DirectoryEntry de = getPathLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator,
                                                  EFileOption::FILE);
    auto clusters = getFatClusterChain(mFS, de.mStartCluster, de.mSize);
    auto fileData = readFile(mFS, clusters, de.mSize);
    for (auto &it: fileData) {
        std::cout << it;
    }
    std::cout << std::endl;
    return true;
}

bool CatCommand::validate_arguments() {
    if (mOptCount != 1) return false;
    pathCheck(mOpt1);
    mAccumulator = split(mOpt1, "/");
    return true;
}

bool CdCommand::run() {
    DirectoryEntry de = getPathLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator,
                                                  EFileOption::DIRECTORY);

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
    std::cout << mFS->getWorkingDirectoryPath() << std::endl;
    return true;
}

bool PwdCommand::validate_arguments() {
    return mOptCount == 0;
}

bool InfoCommand::run() {
    DirectoryEntry de = getPathLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator);

    if (!de.mIsFile) {
        std::cout << de << std::endl;
        return true;
    }

    auto clusters = getFatClusterChain(mFS, de.mStartCluster, de.mSize);

    for (auto &it: clusters) {
        std::cout << it << " ";
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
    makeFatChain(mFS, clusters);

    // Move data
    writeFile(mFS, clusters, mBuffer);

    auto newFileName = mAccumulator.back();
    mAccumulator.pop_back();

    DirectoryEntry parentDE{};
    if (mAccumulator.empty()) // we have had new name of file in accumulator
        parentDE = mFS->mWorkingDirectory;
    else
        parentDE = getPathLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator,
                                             EFileOption::DIRECTORY);

    if (directoryEntryExists(mFS, parentDE.mStartCluster, newFileName, true))
        throw InvalidOptionException(EXIST_ERROR);

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
    DirectoryEntry de = getPathLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator,
                                                  EFileOption::FILE);
    auto clusters = getFatClusterChain(mFS, de.mStartCluster, de.mSize);
    auto fileData = readFile(mFS, clusters, de.mSize);

    std::ofstream stream(mOpt2, std::ios::binary);

    if (!stream.good())
        throw InvalidOptionException(FILE_NOT_FOUND_ERROR);

    stream.write((char *) &fileData[0], static_cast<int>(fileData.size()));

    return true;
}

bool OutcpCommand::validate_arguments() {
    if (mOptCount != 2) return false;
    pathCheck(mOpt1);
    mAccumulator = split(mOpt1, "/");
    return true;
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
