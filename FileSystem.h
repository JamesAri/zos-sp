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
    std::string mWorkingDirectoryPath{"/"};
public:
    std::fstream mStream;

    BootSector mBootSector;
    DirectoryEntry mWorkingDirectory;

    explicit FileSystem(std::string &fileName);

    void readVFS();

    friend std::ostream &operator<<(std::ostream &os, FileSystem const &fs);

    void formatFS(int diskSize = DEFAULT_FORMAT_SIZE);

    bool getDirectory(int cluster, DirectoryEntry &de);

    void updateWorkingDirectoryPath();

    std::string getWorkingDirectoryPath();

    bool findDirectoryEntry(int cluster, const std::string &itemName, DirectoryEntry &de);

    bool findDirectoryEntry(int cluster, const std::string &itemName, DirectoryEntry &de, bool isFile);

    bool findDirectoryEntry(int parentCluster, int childCluster, DirectoryEntry &de);

    bool editDirectoryEntry(int parentCluster, int childCluster, DirectoryEntry &de);

    bool removeDirectoryEntry(int parentCluster, const std::string &itemName, bool isFile);

    int getDirectoryEntryCount(int cluster);

    int getNeededClustersCount(int fileSize) const;

    int clusterToDataAddress(int cluster) const;

    int clusterToFatAddress(int cluster) const;

    std::vector<int> getFreeClusters(int count = 1, bool ordered = false);

    void seek(int pos);

    void flush();

    void seekStreamToDataCluster(int cluster);

    int getDirectoryNextFreeEntryAddress(int cluster);

    void writeNewDirectoryEntry(int directoryCluster, DirectoryEntry &newDE);

    void writeToFatByCluster(int cluster, int label);

    int readFromFatByCluster(int cluster);

    void writeDirectoryEntryReferences(DirectoryEntry &parentDE, DirectoryEntry &newDE, int newFreeCluster);

    std::vector<std::string> getDirectoryContents(int directoryCluster);

    bool directoryEntryExists(int cluster, const std::string &itemName, bool isFile);

    void makeFatChain(std::vector<int> &clusters);

    void labelFatClusterChain(std::vector<int> &clusters, int32_t label);

    std::vector<int> getFatClusterChain(int fromCluster, int fileSize);

    void writeFile(std::vector<int> &clusters, std::vector<char> &buffer);

    std::vector<char> readFile(std::vector<int> &clusters, int fileSize);

    DirectoryEntry
    getPathLastDirectoryEntry(int startCluster, std::vector<std::string> &fileNames, EFileOption lastEntryOpt = EFileOption::UNSPECIFIED);
};

#endif //ZOS_SP_FILESYSTEM_H
