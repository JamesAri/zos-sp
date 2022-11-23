#include "Commands.h"
#include "FAT.h"
#include "utils/string-utils.h"
#include "utils/validators.h"

#include <algorithm>
#include <fstream>
#include <cmath>


enum class ECommands {
    eCpCommand,
    eMvCommand,
    eRmCommand,
    eMkdirCommand,
    eRmdirCommand,
    eLsCommand,
    eCatCommand,
    eCdCommand,
    ePwdCommand,
    eInfoCommand,
    eIncpCommand,
    eOutcpCommand,
    eLoadCommand,
    eFormatCommand,
    eDefragCommand,
    // Classless commands
    eExitCommand,
    eUnknownCommand,
};

constexpr ECommands getCommandCode(std::string const &string) {
    if (string == "cp") return ECommands::eCpCommand;
    if (string == "mv") return ECommands::eMvCommand;
    if (string == "rm") return ECommands::eRmCommand;
    if (string == "mkdir") return ECommands::eMkdirCommand;
    if (string == "rmdir") return ECommands::eRmdirCommand;
    if (string == "ls") return ECommands::eLsCommand;
    if (string == "cat") return ECommands::eCatCommand;
    if (string == "cd") return ECommands::eCdCommand;
    if (string == "pwd") return ECommands::ePwdCommand;
    if (string == "info") return ECommands::eInfoCommand;
    if (string == "incp") return ECommands::eIncpCommand;
    if (string == "outcp") return ECommands::eOutcpCommand;
    if (string == "load") return ECommands::eLoadCommand;
    if (string == "format") return ECommands::eFormatCommand;
    if (string == "defrag") return ECommands::eDefragCommand;
    if (string == "exit") return ECommands::eExitCommand;
    return ECommands::eUnknownCommand;
}

bool handleUserInput(std::vector<std::string> arguments, const std::shared_ptr<FileSystem> &pFS) {
    if (arguments.empty()) return true;

    auto command = arguments[0];
    const auto options = std::vector<std::string>(arguments.begin() + 1, arguments.end());

    switch (getCommandCode(command)) {
        case ECommands::eCpCommand:
            CpCommand(options).registerFS(pFS).process();
            break;
        case ECommands::eMvCommand:
            MvCommand(options).registerFS(pFS).process();
            break;
        case ECommands::eRmCommand:
            RmCommand(options).registerFS(pFS).process();
            break;
        case ECommands::eMkdirCommand:
            MkdirCommand(options).registerFS(pFS).process();
            break;
        case ECommands::eRmdirCommand:
            RmdirCommand(options).registerFS(pFS).process();
            break;
        case ECommands::eLsCommand:
            LsCommand(options).registerFS(pFS).process();
            break;
        case ECommands::eCatCommand:
            CatCommand(options).registerFS(pFS).process();
            break;
        case ECommands::eCdCommand:
            CdCommand(options).registerFS(pFS).process();
            pFS->updateWorkingDirectoryPath();
            break;
        case ECommands::ePwdCommand:
            PwdCommand(options).registerFS(pFS).process();
            break;
        case ECommands::eInfoCommand:
            InfoCommand(options).registerFS(pFS).process();
            break;
        case ECommands::eIncpCommand:
            IncpCommand(options).registerFS(pFS).process();
            break;
        case ECommands::eOutcpCommand:
            OutcpCommand(options).registerFS(pFS).process();
            break;
        case ECommands::eLoadCommand:
            LoadCommand(options).registerFS(pFS).process();
            break;
        case ECommands::eFormatCommand:
            FormatCommand(options).registerFS(pFS).process();
            break;
        case ECommands::eDefragCommand:
            DefragCommand(options).registerFS(pFS).process();
            break;
        case ECommands::eExitCommand:
            return false;
        case ECommands::eUnknownCommand:
            std::cout << "fs: Unknown command: " << command << std::endl;
            break;
    }
    return true;
}

void pathCheck(const std::string &path) {
    if (!validateFilePath(path))
        throw InvalidOptionException(INVALID_DIR_PATH_ERROR);
}

//===============================================================================
//                                COMMANDS                                     //
//===============================================================================

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
    auto freeClusters = mFS->getFreeClusters(static_cast<int>(fromClusters.size()));

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

bool CpCommand::validateArguments() {
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

bool MvCommand::validateArguments() {
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

bool RmCommand::validateArguments() {
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

    int32_t newFreeCluster = mFS->getFreeClusters().back();

    // Update parent directory with the new directory entry
    DirectoryEntry newDE{newDirectoryName, false, 0, newFreeCluster};
    writeNewDirectoryEntry(mFS, parentDE.mStartCluster, newDE);

    // Write references ".' and ".."
    writeDirectoryEntryReferences(mFS, parentDE, newDE, newFreeCluster);

    // All went ok, label new cluster as allocated
    writeToFatByCluster(mFS, newFreeCluster, FAT_FILE_END);
    return true;
}

bool MkdirCommand::validateArguments() {
    if (mOptCount != 1) return false;
    pathCheck(mOpt1);
    mAccumulator = split(mOpt1, "/");
    return true;
}

bool RmdirCommand::run() {
    auto toRemoveDE = getPathLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator,
                                                EFileOption::DIRECTORY);
    mAccumulator.pop_back();

    DirectoryEntry parentDE{};
    if (mAccumulator.empty())
        parentDE = mFS->mWorkingDirectory;
    else
        parentDE = getPathLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator,
                                             EFileOption::DIRECTORY);

    if (mFS->getDirectoryEntryCount(toRemoveDE.mStartCluster) > DEFAULT_DIR_SIZE)
        throw InvalidOptionException(NOT_EMPTY_ERROR);

    bool removed = mFS->removeDirectoryEntry(parentDE.mStartCluster, toRemoveDE.mItemName, false);

    if (!removed) throw InvalidOptionException(DELETE_DIR_REFERENCE_ERROR);

    writeToFatByCluster(mFS, toRemoveDE.mStartCluster, FAT_UNUSED);

    return true;
}

bool RmdirCommand::validateArguments() {
    if (mOptCount != 1) return false;
    pathCheck(mOpt1);
    mAccumulator = split(mOpt1, "/");
    return true;
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

bool LsCommand::validateArguments() {
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

bool CatCommand::validateArguments() {
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

bool CdCommand::validateArguments() {
    if (mOptCount != 1) return false;
    pathCheck(mOpt1);
    mAccumulator = split(mOpt1, "/");
    return true;
}

bool PwdCommand::run() {
    std::cout << mFS->getWorkingDirectoryPath() << std::endl;
    return true;
}

bool PwdCommand::validateArguments() {
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

bool InfoCommand::validateArguments() {
    if (mOptCount != 1) return false;
    pathCheck(mOpt1);
    mAccumulator = split(mOpt1, "/");
    return true;
}

bool IncpCommand::run() {
    int fileSize = static_cast<int>(mBuffer.size());
    int neededClusters = (fileSize) ? mFS->getNeededClustersCount(fileSize) : 1;

    // Get free clusters
    auto clusters = mFS->getFreeClusters(neededClusters);

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

bool IncpCommand::validateArguments() {
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

bool OutcpCommand::validateArguments() {
    if (mOptCount != 2) return false;
    pathCheck(mOpt1);
    mAccumulator = split(mOpt1, "/");
    return true;
}

bool LoadCommand::run() {
    std::ifstream stream(mOpt1, std::ios::binary);

    if (!stream.good())
        throw InvalidOptionException(FILE_NOT_FOUND_ERROR);

    std::vector<std::string> args;
    for (std::string line; getline(stream, line);) {
        std::cout << line << std::endl;
        args = split(line, " ");
        try {
            handleUserInput(args, mFS);
        } catch (InvalidOptionException &ex) { // ¯\_(ツ)_/¯
            std::cout << ex.what() << std::endl;
        }
    }
    return true;
}

bool LoadCommand::validateArguments() {
    if (mOptCount != 1) return false;
    return true;
}

bool FormatCommand::run() {
    try {
        mFS->formatFS(std::stoi(mOpt1));
    } catch (...) {
        std::cerr << "internal error, couldn't format file system" << std::endl;
        exit(1);
    }
    std::cout << *mFS << std::endl;
    return true;
}

bool FormatCommand::validateArguments() {
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
    auto fileDE = getPathLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator, EFileOption::FILE);
    mAccumulator.pop_back();
    DirectoryEntry parentDE{};
    if (mAccumulator.empty())
        parentDE = mFS->mWorkingDirectory;
    else
        parentDE = getPathLastDirectoryEntry(mFS, mFS->mWorkingDirectory.mStartCluster, mAccumulator, EFileOption::DIRECTORY);

    auto clusters = getFatClusterChain(mFS, fileDE.mStartCluster, fileDE.mSize);
    if (clusters.size() == 1) return true;
    bool isOrdered = true;
    for (int i = 1; i < clusters.size(); i++) {
        if (clusters.at(i - 1) + 1 != clusters.at(i)) {
            isOrdered = false;
            break;
        }
    }
    if (isOrdered) return true;

    // Get data
    auto fileData = readFile(mFS, clusters, fileDE.mSize);
    // Label previous clusters as free
    labelFatClusterChain(mFS, clusters, FAT_UNUSED);
    // Get new continuous clusters
    clusters = mFS->getFreeClusters(static_cast<int>(clusters.size()), true);
    // Write data
    writeFile(mFS, clusters, fileData);
    // Mark continuous clusters in FAT tables
    makeFatChain(mFS, clusters);
    // Edit directory entry
    int oldCluster = fileDE.mStartCluster;
    fileDE.mStartCluster = clusters.at(0);
    return mFS->editDirectoryEntry(parentDE.mStartCluster, oldCluster, fileDE);
}

bool DefragCommand::validateArguments() {
    if (mOptCount != 1) return false;
    pathCheck(mOpt1);
    mAccumulator = split(mOpt1, "/");
    return true;
}
