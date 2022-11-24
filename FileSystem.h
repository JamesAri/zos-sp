#ifndef ZOS_SP_FILESYSTEM_H
#define ZOS_SP_FILESYSTEM_H

#include "definitions.h"
#include "BootSector.h"
#include "DirectoryEntry.h"
#include <fstream>
#include <queue>

enum class EFileOption {
    FILE,
    DIRECTORY,
    UNSPECIFIED,
};

class InvalidOptionException : public std::exception {
private:
    std::string mErrMsg;
public:
    InvalidOptionException() : mErrMsg("invalid option(s)") {}

    explicit InvalidOptionException(std::string &&errMsg) : mErrMsg(errMsg) {}

    explicit InvalidOptionException(const std::string &errMsg) : mErrMsg(errMsg) {}

    const char *what() const _NOEXCEPT override {
        return this->mErrMsg.c_str();
    }
};

constexpr int MAX_ENTRIES = CLUSTER_SIZE / DirectoryEntry::SIZE;

/**
 * FS MEMORY STRUCTURE:
 *
 * BOOT SECTOR
 * FAT1
 * FAT2
 * padding (0 <= padding < CLUSTER_SIZE), fill value: \00
 * DATA
 */
class FileSystem {
    const std::string mFileName;
    std::fstream mStream;
    std::string mWorkingDirectoryPath{"/"};
public:
    BootSector mBootSector;
    DirectoryEntry mWorkingDirectory;

    explicit FileSystem(std::string &fileName);

    friend std::ostream &operator<<(std::ostream &os, FileSystem const &fs);

    void readVFS();

    void formatFS(int diskSize = DEFAULT_FORMAT_SIZE);

    void flush();

    void seek(int pos);

    void seekStreamToDataCluster(int cluster);

    int clusterToDataAddress(int cluster) const;

    int clusterToFatAddress(int cluster) const;

    void writeFile(std::vector<int> &clusters, std::vector<char> &buffer);

    std::vector<char> readFile(std::vector<int> &clusters, int fileSize);

    // DIRECTORY OPERATIONS

    void updateWorkingDirectoryPath();

    std::string getWorkingDirectoryPath();

    bool getDirectory(int cluster, DirectoryEntry &de);

    int getDirectoryNextFreeEntryAddress(int cluster);

    std::vector<std::string> getDirectoryContents(int directoryCluster);

    // DIRECTORY ENTRY OPERATIONS

    bool findDirectoryEntry(int cluster, const std::string &itemName, DirectoryEntry &de);

    bool findDirectoryEntry(int cluster, const std::string &itemName, DirectoryEntry &de, bool isFile);

    bool findDirectoryEntry(int parentCluster, int childCluster, DirectoryEntry &de);

    void writeNewDirectoryEntry(int directoryCluster, DirectoryEntry &newDE);

    void writeDirectoryEntryReferences(DirectoryEntry &parentDE, DirectoryEntry &newDE, int newFreeCluster);

    bool editDirectoryEntry(int parentCluster, int childCluster, DirectoryEntry &de);

    bool removeDirectoryEntry(int parentCluster, const std::string &itemName, bool isFile);

    bool directoryEntryExists(int cluster, const std::string &itemName, bool isFile);

    int getDirectoryEntryCount(int cluster);

    DirectoryEntry getLastRelativeDirectoryEntry(std::vector<std::string> &fileNames,
                                                 EFileOption lastEntryOpt = EFileOption::UNSPECIFIED);

    // FAT CLUSTER OPERATIONS

    std::vector<int> getFreeClusters(int count = 1, bool ordered = false);

    std::vector<int> getFatClusterChain(int fromCluster, int fileSize);

    void makeFatChain(std::vector<int> &clusters);

    void labelFatClusterChain(std::vector<int> &clusters, int32_t label);

    int getNeededClustersCount(int fileSize) const;

    void writeToFatByCluster(int cluster, int label);

    int readFromFatByCluster(int cluster);
};

#endif //ZOS_SP_FILESYSTEM_H
