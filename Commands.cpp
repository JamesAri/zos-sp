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
    if (this->mAccumulator.empty()) {
        throw std::runtime_error("internal error, received empty path accumulator");
    }

    auto newDirectoryName = this->mAccumulator.back();

    DirectoryEntry de = this->mFS->mWorkingDirectory;

    if (this->mAccumulator.size() > 1) {
        // Iterates over all directory entries (itemNames) in accumulator except the last entry, which is
        // directory to create. If any of the entry fails validation, a PATH NOT FOUND exception is thrown.
        int curCluster = de.mStartCluster;
        for (auto it{this->mAccumulator.begin()}; it != std::prev(this->mAccumulator.end()); it++) {
            if (this->mFS->findDirectoryEntry(curCluster, *it, de)) {
                curCluster = de.mStartCluster;
            } else {
                throw InvalidOptionException("PATH NOT FOUND");
            }
        }
    }

    if (this->mFS->findDirectoryEntry(de.mStartCluster, newDirectoryName, de)) {
        throw InvalidOptionException("EXIST");
    }

    std::fstream stream(this->mFS->mFileName, std::ios::in | std::ios::out | std::ios::binary);

    if (!stream.is_open())
        throw std::runtime_error(FS_OPEN_ERROR);

    int32_t newFreeCluster = FAT::getFreeCluster(stream, this->mFS->mBootSector);
    DirectoryEntry newDirectoryEntry{newDirectoryName, false, DEFAULT_DIR_SIZE, newFreeCluster};

    int32_t freeDirEntryAddrOffset = this->mFS->getDirectoryNextFreeEntryAddress(de.mStartCluster, de.mSize);
    stream.seekp(freeDirEntryAddrOffset);
    newDirectoryEntry.write(stream);

    int32_t localDirectoryAddress = this->mFS->clusterToAddress(de.mStartCluster);
    de.mSize++;
    // update state in loaded memory if necessary
    if (this->mAccumulator.size() == 1) this->mFS->mWorkingDirectory.mSize++;
    stream.seekp(localDirectoryAddress);
    de.write(stream);


    int32_t newDirectoryAddress = this->mFS->clusterToAddress(newFreeCluster);
    stream.seekp(newDirectoryAddress);
    newDirectoryEntry.mItemName = ".";
    newDirectoryEntry.write(stream);
    de.mItemName = "..";
    de.write(stream);

    FAT::write(stream, this->mFS->clusterToFatAddress(newFreeCluster), FAT_FILE_END); // all went ok, label cluster
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

//FormatCommand& FormatCommand::registerFS(const std::shared_ptr<FileSystem> &pFS) {
//    this->mFS = pFS;
//    return *this;
//}

bool DefragCommand::run() {
    return true;
}

bool DefragCommand::validate_arguments() {
    return this->mOptCount == 2;
}
