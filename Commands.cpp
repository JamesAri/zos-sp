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

void seekStreamToDataCluster(std::shared_ptr<FileSystem> &fs, std::fstream &stream, int cluster) {
    int32_t address = fs->clusterToDataAddress(cluster);
    stream.seekp(address);
}

int getDirectoryNextFreeEntryAddress(std::shared_ptr<FileSystem> &fs, int cluster) {
    std::ifstream stream(fs->mFileName, std::ios::binary);
    auto address = fs->clusterToDataAddress(cluster);
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
    throw std::runtime_error(DE_LIMIT_REACHED_ERROR);
}

void writeNewDirectoryEntry(std::shared_ptr<FileSystem> &fs, int directoryCluster, DirectoryEntry &newDE) {
    int32_t freeParentEntryAddr = getDirectoryNextFreeEntryAddress(fs, directoryCluster);

    std::ofstream stream(fs->mFileName, std::ios::binary | std::ios::app);
    stream.seekp(freeParentEntryAddr);
    newDE.write(stream);
}

void writeToFatByCluster(std::shared_ptr<FileSystem> &fs, int cluster, int label) {
    int address = fs->clusterToFatAddress(cluster);
    std::ofstream stream(fs->mFileName, std::ios::binary | std::ios::app);
    FAT::write(stream, address, label);
}

int readFromFatByCluster(std::shared_ptr<FileSystem> &fs, int cluster) {
    int address = fs->clusterToFatAddress(cluster);
    std::ifstream stream(fs->mFileName, std::ios::binary);
    return FAT::read(stream, address);
}

bool isSpecialLabel(int label) {
    return label == FAT_UNUSED || label == FAT_FILE_END || label == FAT_BAD_CLUSTER;
}

std::vector<int>
getFatClusterChain(std::shared_ptr<FileSystem> &fs, int fromCluster, int fileSize) {
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

//todo complete refactor
bool CpCommand::run() {
//    auto newFileName = mAccumulator.back();
//
//    DirectoryEntry parentDE = mFS->mWorkingDirectory;
//
//    if (mAccumulator.size() > 1) {
//        int curCluster = parentDE.mStartCluster;
//        for (auto it{mAccumulator.begin()}; it != std::prev(mAccumulator.end()); it++) {
//            if (mFS->findDirectoryEntry(curCluster, *it, parentDE)) {
//                curCluster = parentDE.mStartCluster;
//            } else {
//                throw InvalidOptionException(PATH_NOT_FOUND_ERROR);
//            }
//        }
//    }
//
//    if (mFS->findDirectoryEntry(parentDE.mStartCluster, newFileName, parentDE)) // exists -> delete
//        mFS->removeDirectoryEntry(parentDE.mStartCluster, newFileName);
//
////    std::fstream stream(mFS->mFileName, std::ios::binary | std::ios::in | std::ios::out);
//
//    int clusterSize = mFS->mBootSector.mClusterSize;
//    int fileSize = mFromDE.mSize;
//    int neededClusters = mFS->getNeededClustersCount(fileSize);
//
//    // Get free clusters
//    auto clusters = FAT::getFreeClusters(mFS, neededClusters);
//
//    // Mark clusters in FAT tables
//    for (int i = 0; i < clusters.size() - 1; i++) {
//        writeToFatByCluster(mFS, clusters.at(i), clusters.at(i + 1));
//    }
//    writeToFatByCluster(mFS, clusters.back(), FAT_FILE_END);
//
//    // Move data
//    int curCluster = mFromDE.mStartCluster;
//    char clusterBfr[clusterSize];
//
//    for (int i = 0; i < clusters.size() - 1; i++) {
//        if (curCluster == FAT_UNUSED
//            || curCluster == FAT_FILE_END
//            || curCluster == FAT_BAD_CLUSTER
//            || curCluster >= mFS->mBootSector.mClusterCount) {
//            // todo mark bad section function
//            throw std::runtime_error(CORRUPTED_FS_ERROR);
//        }
//        seekStreamToDataCluster(mFS, stream, curCluster);
//        stream.read(clusterBfr, clusterSize);
//
//        seekStreamToDataCluster(mFS, stream, clusters.at(i));
//        stream.write(clusterBfr, clusterSize);
//
//        curCluster = readFromFatByCluster(mFS, stream, curCluster);
//    }
//    if (readFromFatByCluster(mFS, stream, curCluster) != FAT_FILE_END) {
//        // todo mark bad section function
//        throw std::runtime_error(CORRUPTED_FS_ERROR);
//    }
//    seekStreamToDataCluster(mFS, stream, curCluster);
//    stream.read(clusterBfr, clusterSize);
//
//    seekStreamToDataCluster(mFS, stream, clusters.back());
//    stream.write(clusterBfr, clusterSize);
//
//    // Write directory entry
//    DirectoryEntry newFileDE{newFileName, true, fileSize, clusters.at(0)};
//    writeNewDirectoryEntry(mFS, parentDE.mStartCluster, newFileDE);
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

    DirectoryEntry parentDE = mFS->mWorkingDirectory;

    if (mAccumulator.size() > 1) {
        int curCluster = parentDE.mStartCluster;
        for (auto it{mAccumulator.begin()}; it != std::prev(mAccumulator.end()); it++) {
            if (mFS->findDirectoryEntry(curCluster, *it, parentDE)) curCluster = parentDE.mStartCluster;
            else throw InvalidOptionException(PATH_NOT_FOUND_ERROR);
        }
    }

    if (mFS->findDirectoryEntry(parentDE.mStartCluster, newDirectoryName, parentDE))
        throw InvalidOptionException(EXIST_ERROR);


    int32_t newFreeCluster = FAT::getFreeClusters(mFS).back();

    // Update parent directory with the new directory entry
    DirectoryEntry newDE{newDirectoryName, false, 0, newFreeCluster};
    writeNewDirectoryEntry(mFS, parentDE.mStartCluster, newDE);

    // Create new directory "." at new cluster
    std::fstream stream(mFS->mFileName, std::ios::binary | std::ios::out | std::ios::ate); // todo bug
    seekStreamToDataCluster(mFS, stream, newFreeCluster);
    newDE.mItemName = ".";
    newDE.write(stream);

    // Create new directory ".." at new cluster
    parentDE.mItemName = "..";
    parentDE.write(stream);
    stream.close(); // todo

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
    DirectoryEntry parentDE = mFS->mWorkingDirectory;

    if (!mAccumulator.empty()) {
        int curCluster = parentDE.mStartCluster;
        // Iterates over all directory entries (file names) in accumulator.
        // If any of the entry fails validation, a PATH NOT FOUND exception is thrown.
        for (auto &it: mAccumulator) {
            if (mFS->findDirectoryEntry(curCluster, it, parentDE)) {
                curCluster = parentDE.mStartCluster;
            } else {
                throw InvalidOptionException(PATH_NOT_FOUND_ERROR);
            }
        }
    }

    // Get filenames
    std::fstream stream(mFS->mFileName, std::ios::binary | std::ios::in | std::ios::app); // todo
    seekStreamToDataCluster(mFS, stream, parentDE.mStartCluster);

    std::vector<std::string> fileNames{};
    DirectoryEntry de{};

    for (int i = 0; i < MAX_ENTRIES; i++) {
        de.read(stream);
        if (!isAllocatedDirectoryEntry(de.mItemName)) {
            if (i < DEFAULT_DIR_SIZE)
                throw std::runtime_error(DE_MISSING_REFERENCES_ERROR);
        }
        fileNames.push_back(de.mItemName);
    }
    for (auto &fn: fileNames) {
        std::cout << fn.c_str() << " ";
    }
    std::cout << std::endl;
    stream.close(); //todo
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
    DirectoryEntry parentDE = mFS->mWorkingDirectory;

    int curCluster = parentDE.mStartCluster;
    // Iterates over all directory entries (file names) in accumulator.
    // If any of the entry fails validation, a PATH NOT FOUND exception is thrown.
    for (auto &it: mAccumulator) {
        if (mFS->findDirectoryEntry(curCluster, it, parentDE)) {
            curCluster = parentDE.mStartCluster;
        } else {
            throw InvalidOptionException(PATH_NOT_FOUND_ERROR);
        }
    }
    if (!strcmp(parentDE.mItemName.c_str(), "..") || !strcmp(parentDE.mItemName.c_str(), ".")) {
        if (!mFS->getDirectory(parentDE.mStartCluster, parentDE))
            throw std::runtime_error(CORRUPTED_FS_ERROR);
    }
    mFS->mWorkingDirectory = parentDE;
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
    DirectoryEntry de = mFS->mWorkingDirectory;

    int curCluster = de.mStartCluster;
    for (auto &it: mAccumulator) {
        if (mFS->findDirectoryEntry(curCluster, it, de)) {
            curCluster = de.mStartCluster;
        } else {
            throw InvalidOptionException(PATH_NOT_FOUND_ERROR);
        }
    }
    if (!de.mIsFile) {
        std::cout << de << std::endl;
        return true;
    }

    auto clusters = getFatClusterChain(mFS, de.mStartCluster, de.mSize);

    for (auto it{clusters.begin()}; it != std::prev(clusters.end()); it++) {
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

    int clusterSize = mFS->mBootSector.mClusterSize;
    int fileSize = static_cast<int>(mBuffer.size());
    int trailingBytes = fileSize % clusterSize;
    int neededClusters = mFS->getNeededClustersCount(fileSize);

    // Get free clusters
    auto clusters = FAT::getFreeClusters(mFS, neededClusters);

    // Mark clusters in FAT tables
    for (int i = 0; i < clusters.size() - 1; i++) {
        writeToFatByCluster(mFS, clusters.at(i), clusters.at(i + 1));
    }
    writeToFatByCluster(mFS, clusters.back(), FAT_FILE_END);

    // Move data
    std::fstream stream(mFS->mFileName, std::ios::binary | std::ios::out);
    for (int i = 0; i < clusters.size() - 1; i++) {
        seekStreamToDataCluster(mFS, stream, clusters.at(i));
        stream.write((char *) (&mBuffer[i * clusterSize]), clusterSize);
    }
    seekStreamToDataCluster(mFS, stream, clusters.back());
    stream.write((char *) (&mBuffer[fileSize - trailingBytes]), trailingBytes); // todo check on bigger files
    stream.close(); // todo

    DirectoryEntry newFileDE{mOpt1, true, fileSize, clusters.at(0)};
    writeNewDirectoryEntry(mFS, mDestDE.mStartCluster, newFileDE);
    return true;
}

bool IncpCommand::validate_arguments() {
    if (mOptCount != 2) return false;

    // Input file check
    pathCheck(mOpt1);

    std::ifstream stream(mOpt1, std::ios::binary);

    if (!stream.good())
        throw InvalidOptionException(FILE_NOT_FOUND_ERROR);

    std::streamsize size = stream.tellg();
    stream.seekg(0, std::ios::beg);

    mBuffer = std::vector<char>(size);
    if (!stream.read(mBuffer.data(), size))
        throw std::runtime_error(FILE_READ_ERROR);

    stream.close();

    pathCheck(mOpt2);

    auto fileNames = split(mOpt2, "/");

    DirectoryEntry de{};
    int curCluster = mFS->mWorkingDirectory.mStartCluster;
    for (auto &it: fileNames) {
        if (mFS->findDirectoryEntry(curCluster, it, de)) {
            curCluster = de.mStartCluster;
        } else {
            throw InvalidOptionException(PATH_NOT_FOUND_ERROR);
        }
    }

    mDestDE = de;
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
