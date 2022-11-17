#include "Commands.h"
#include "utils/string-utils.h"
#include "utils/validators.h"

#include <algorithm>

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
        throw std::runtime_error("internal error, received empty path accumulator");

    auto newDirectoryName = this->mAccumulator.back();

    DirectoryEntry parentDE = this->mFS->mWorkingDirectory;

    if (this->mAccumulator.size() > 1) {
        int curCluster = parentDE.mStartCluster;
        // Iterates over all directory entries (itemNames) in accumulator except the last entry, which is
        // directory to create. If any of the entry fails validation, a PATH NOT FOUND exception is thrown.
        for (auto it{this->mAccumulator.begin()}; it != std::prev(this->mAccumulator.end()); it++) {
            if (this->mFS->findDirectoryEntry(curCluster, *it, parentDE)) {
                curCluster = parentDE.mStartCluster;
            } else {
                throw InvalidOptionException("PATH NOT FOUND");
            }
        }
    }

    if (this->mFS->findDirectoryEntry(parentDE.mStartCluster, newDirectoryName, parentDE))
        throw InvalidOptionException("EXIST");

    std::fstream stream(this->mFS->mFileName, std::ios::in | std::ios::out | std::ios::binary);

    if (!stream.is_open()) {
        throw std::runtime_error(FS_OPEN_ERROR);
    }

    int32_t newFreeCluster = FAT::getFreeCluster(stream, this->mFS->mBootSector);

    // Update parent directory with the new directory entry
    DirectoryEntry newDE{newDirectoryName, false, 0, newFreeCluster};
    int32_t freeParentEntryAddr = this->mFS->getDirectoryNextFreeEntryAddress(parentDE.mStartCluster);
    stream.seekp(freeParentEntryAddr);
    newDE.write(stream);

    // Create new directory "." at new cluster
    int32_t newDEAddress = this->mFS->clusterToAddress(newFreeCluster);
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
        throw InvalidOptionException("invalid directory path");

    this->mAccumulator = split(this->mOpt1, "/");
    auto newDirectoryName = this->mAccumulator.back();

    if (!validateFileName(newDirectoryName))
        throw InvalidOptionException("invalid directory name");

    // todo check if path exists
    return true;
}

bool RmdirCommand::run() {
    return true;
}

bool RmdirCommand::validate_arguments() {
    return this->mOptCount == 1;
}

bool LsCommand::run() {
    return true;
}

bool LsCommand::validate_arguments() {
    return this->mOptCount == 0 || this->mOptCount == 1;
}

bool CatCommand::run() {
    return true;
}

bool CatCommand::validate_arguments() {
    return this->mOptCount == 1;
}

bool CdCommand::run() {
    return true;
}

bool CdCommand::validate_arguments() {
    return this->mOptCount == 1;
}

bool PwdCommand::run() {
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
    return this->mOptCount == 2;
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
    //    eraseAllSubString(this->mOpt1, allowedFormats[0]);

    std::transform(this->mOpt1.begin(), this->mOpt1.end(), this->mOpt1.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    std::vector<std::string> allowedFormats{"MB"};

    auto pos = this->mOpt1.find(allowedFormats[0]);

    if (pos == std::string::npos)
        throw InvalidOptionException("CANNOT CREATE FILE (wrong unit)");

    this->mOpt1.erase(pos, allowedFormats[0].length());
    if (!is_number(this->mOpt1))
        throw InvalidOptionException("CANNOT CREATE FILE (not a number)");
    return true;
}

bool DefragCommand::run() {
    return true;
}

bool DefragCommand::validate_arguments() {
    return this->mOptCount == 2;
}
