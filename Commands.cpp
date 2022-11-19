#include "Commands.h"
#include "utils/string-utils.h"
#include "utils/validators.h"

#include <algorithm>
#include <fstream>

bool CpCommand::run() {
    return true;
}

bool CpCommand::validate_arguments() {
    return this->mOptCount == 2;
}

bool MvCommand::run() {
    return true;
}

bool MvCommand::validate_arguments() {
    return this->mOptCount == 2;
}

bool RmCommand::run() {
    return true;
}

bool RmCommand::validate_arguments() {
    return this->mOptCount == 2;
}

bool MkdirCommand::run() {
    if (this->mAccumulator.empty())
        throw std::runtime_error(EMPTY_ACCUMULATOR_ERROR);

    auto newDirectoryName = this->mAccumulator.back();

    DirectoryEntry parentDE = this->mFS->mWorkingDirectory;

    if (this->mAccumulator.size() > 1) {
        int curCluster = parentDE.mStartCluster;
        // Iterates over all directory entries (file names) in accumulator except the last entry, which is
        // directory to create. If any of the entry fails validation, a PATH NOT FOUND exception is thrown.
        for (auto it{this->mAccumulator.begin()}; it != std::prev(this->mAccumulator.end()); it++) {
            if (this->mFS->findDirectoryEntry(curCluster, *it, parentDE)) {
                curCluster = parentDE.mStartCluster;
            } else {
                throw InvalidOptionException(PATH_NOT_FOUND_ERROR);
            }
        }
    }

    if (this->mFS->findDirectoryEntry(parentDE.mStartCluster, newDirectoryName, parentDE))
        throw InvalidOptionException(EXIST_ERROR);

    std::fstream stream(this->mFS->mFileName, std::ios::in | std::ios::out | std::ios::binary);

    if (!stream.is_open())
        throw std::runtime_error(FS_OPEN_ERROR);

    int32_t newFreeCluster = FAT::getFreeCluster(stream, this->mFS->mBootSector);

    // Update parent directory with the new directory entry
    DirectoryEntry newDE{newDirectoryName, false, 0, newFreeCluster};
    int32_t freeParentEntryAddr = this->mFS->getDirectoryNextFreeEntryAddress(parentDE.mStartCluster);
    stream.seekp(freeParentEntryAddr);
    newDE.write(stream);

    // Create new directory "." at new cluster
    int32_t newDEAddress = this->mFS->clusterToDataAddress(newFreeCluster);
    stream.seekp(newDEAddress);
    newDE.mItemName = ".";
    newDE.write(stream);

    // Create new directory ".." at new cluster
    parentDE.mItemName = "..";
    parentDE.write(stream);

    // All went ok, label new cluster as allocated
    int32_t fatNewClusterAddress = this->mFS->clusterToFatAddress(newFreeCluster);
    FAT::write(stream, fatNewClusterAddress, FAT_FILE_END);
    return true;
}

bool MkdirCommand::validate_arguments() {
    if (this->mOptCount != 1) return false;

    if (!validateFilePath(this->mOpt1))
        throw InvalidOptionException(INVALID_DIR_PATH_ERROR);

    this->mAccumulator = split(this->mOpt1, "/");
    auto newDirectoryName = this->mAccumulator.back();

    return true;
}

bool RmdirCommand::run() {
    DirectoryEntry de{};
    if (!this->mFS->findDirectoryEntry(this->mFS->mWorkingDirectory.mStartCluster, this->mOpt1, de))
        throw InvalidOptionException(FILE_NOT_FOUND_ERROR);

    if (this->mFS->getDirectoryEntryCount(de.mStartCluster) > DEFAULT_DIR_SIZE)
        throw InvalidOptionException(NOT_EMPTY_ERROR);
    // move implementation here? todo
    return this->mFS->removeDirectoryEntry(this->mFS->mWorkingDirectory.mStartCluster, this->mOpt1);
}

bool RmdirCommand::validate_arguments() {
    if (this->mOptCount != 1) return false;
    return validateFileName(this->mOpt1);
}

bool LsCommand::run() {
    // Resolve path

    DirectoryEntry parentDE = this->mFS->mWorkingDirectory;

    if (!this->mAccumulator.empty()) {
        int curCluster = parentDE.mStartCluster;
        // Iterates over all directory entries (file names) in accumulator.
        // If any of the entry fails validation, a PATH NOT FOUND exception is thrown.
        for (auto &it: this->mAccumulator) {
            if (this->mFS->findDirectoryEntry(curCluster, it, parentDE)) {
                curCluster = parentDE.mStartCluster;
            } else {
                throw InvalidOptionException(PATH_NOT_FOUND_ERROR);
            }
        }
    }

    // Get filenames
    std::ifstream stream(this->mFS->mFileName, std::ios::binary);

    if (!stream.is_open())
        throw std::runtime_error(FS_OPEN_ERROR);

    auto address = this->mFS->clusterToDataAddress(parentDE.mStartCluster);
    stream.seekg(address);

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
    return true;
}

bool LsCommand::validate_arguments() {
    if (this->mOptCount == 0) return true;
    if (this->mOptCount != 1) return false;

    if (!validateFilePath(this->mOpt1))
        throw InvalidOptionException(INVALID_DIR_PATH_ERROR);

    this->mAccumulator = split(this->mOpt1, "/");
    return true;
}

bool CatCommand::run() {
    return true;
}

bool CatCommand::validate_arguments() {
    return this->mOptCount == 1;
}

bool CdCommand::run() {
    if (this->mAccumulator.empty())
        throw std::runtime_error(EMPTY_ACCUMULATOR_ERROR);

    DirectoryEntry parentDE = this->mFS->mWorkingDirectory;

    int curCluster = parentDE.mStartCluster;
    // Iterates over all directory entries (file names) in accumulator.
    // If any of the entry fails validation, a PATH NOT FOUND exception is thrown.
    for (auto &it: this->mAccumulator) {
        if (this->mFS->findDirectoryEntry(curCluster, it, parentDE)) {
            curCluster = parentDE.mStartCluster;
        } else {
            throw InvalidOptionException(PATH_NOT_FOUND_ERROR);
        }
    }
    if (!strcmp(parentDE.mItemName.c_str(), "..") || !strcmp(parentDE.mItemName.c_str(), ".")) {
        if (!this->mFS->getDirectory(parentDE.mStartCluster, parentDE))
            throw std::runtime_error(CORRUPTED_FS_ERROR);
    }
    this->mFS->mWorkingDirectory = parentDE;
    return true;
}

bool CdCommand::validate_arguments() {
    if (this->mOptCount != 1) return false;

    if (!validateFilePath(this->mOpt1))
        throw InvalidOptionException(INVALID_DIR_PATH_ERROR);

    this->mAccumulator = split(this->mOpt1, "/");
    return true;
}

bool PwdCommand::run() {
    if (this->mFS->mWorkingDirectory.mStartCluster == 0) {
        std::cout << "/" << std::endl;
        return true;
    }

    std::vector<std::string> fileNames{};

    DirectoryEntry de = this->mFS->mWorkingDirectory;

    int childCluster = de.mStartCluster, parentCluster;
    int safetyCounter = 0;
    while (true) {
        safetyCounter++;
        if (safetyCounter > MAX_ENTRIES)
            throw std::runtime_error(CORRUPTED_FS_ERROR);

        if (childCluster == 0) break;

        if (!this->mFS->findDirectoryEntry(childCluster, "..", de))
            throw std::runtime_error(CORRUPTED_FS_ERROR);

        parentCluster = de.mStartCluster;

        if (!this->mFS->findDirectoryEntry(parentCluster, childCluster, de))
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
    return this->mOptCount == 0;
}

bool InfoCommand::run() {
    return true;
}

bool InfoCommand::validate_arguments() {
    return this->mOptCount == 1;
}

bool IncpCommand::run() {
    return true;
}

bool IncpCommand::validate_arguments() {
    if(this->mOptCount != 2) return false;
    std::ifstream stream(this->mOpt1);
    if(!stream.good())
        throw InvalidOptionException(FILE_NOT_FOUND_ERROR);
    if(!validateFilePath(this->mOpt2))
        throw InvalidOptionException(PATH_NOT_FOUND_ERROR);

    return true;
}

bool OutcpCommand::run() {
    return true;
}

bool OutcpCommand::validate_arguments() {
    return this->mOptCount == 2;
}

bool LoadCommand::run() {
    return true;
}

bool LoadCommand::validate_arguments() {
    return this->mOptCount == 1;
}

bool FormatCommand::run() {
    try {
        this->mFS->formatFS(std::stoi(this->mOpt1));
    } catch (...) {
        std::cerr << "internal error, terminating" << std::endl;
        exit(1);
    }
    return true;
}

bool FormatCommand::validate_arguments() {
    if (this->mOptCount != 1) return false;

    std::transform(this->mOpt1.begin(), this->mOpt1.end(), this->mOpt1.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    std::vector<std::string> allowedFormats{"MB"};

    auto pos = this->mOpt1.find(allowedFormats[0]);

    if (pos == std::string::npos)
        throw InvalidOptionException(CANNOT_CREATE_FILE_ERROR + " (wrong unit)");

    this->mOpt1.erase(pos, allowedFormats[0].length());

    if (!is_number(this->mOpt1))
        throw InvalidOptionException(CANNOT_CREATE_FILE_ERROR + " (not a number)");

    return true;
}

bool DefragCommand::run() {
    return true;
}

bool DefragCommand::validate_arguments() {
    return this->mOptCount == 2;
}
