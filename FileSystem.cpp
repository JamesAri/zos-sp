#include "FileSystem.h"
#include "utils/stream-utils.h"

#include <iostream>

DirectoryItem::DirectoryItem(std::string &&mItemName, bool mIsFile, int mSize, int mStartCluster) :
        mIsFile(mIsFile), mSize(mSize), mStartCluster(mStartCluster) {
    if (mItemName.length() >= ITEM_NAME_LENGTH) {
        throw std::runtime_error(std::string("Error: file name too long. Character limit is ") +
                                 std::to_string(ITEM_NAME_LENGTH - 1) + std::string("\n"));
    }
    this->mItemName = mItemName + std::string(ITEM_NAME_LENGTH - mItemName.length(), '\00');
}

void DirectoryItem::write(std::ofstream &f) {
    writeToStream(f, this->mItemName, ITEM_NAME_LENGTH);
    writeToStream(f, this->mIsFile);
    writeToStream(f, this->mSize);
    writeToStream(f, this->mStartCluster);
    // padding
    std::string zeroFill(CLUSTER_SIZE - DirectoryItem::SIZE, '\00');
    writeToStream(f, zeroFill, static_cast<int>(zeroFill.length()));
}

void DirectoryItem::read(std::ifstream &f) {
    readFromStream(f, this->mItemName, ITEM_NAME_LENGTH);
    readFromStream(f, this->mIsFile);
    readFromStream(f, this->mSize);
    readFromStream(f, this->mStartCluster);
}

std::ostream &operator<<(std::ostream &os, DirectoryItem const &di) {
    return os << "ItemName: " << di.mItemName.c_str() << "\n"
              << "IsFile: " << di.mIsFile << "\n"
              << "Size: " << di.mSize << "\n"
              << "StartCluster: " << di.mStartCluster << "\n";
}

FAT::FAT() {

}

void FAT::write(std::ofstream &f) {

}

void FAT::read(std::ifstream &f) {

}

std::ostream &operator<<(std::ostream &os, FAT const &fs) {
    return os << "FAT" << "\n";
}

BootSector::BootSector(int size) : mDiskSize(size * FORMAT_UNIT) {
    this->mSignature = SIGNATURE;
    this->mClusterSize = CLUSTER_SIZE;
    this->mFatCount = FAT_COUNT;
    this->mFat1StartAddress = BootSector::SIZE;
    this->mFat2StartAddress = BootSector::SIZE + FAT::SIZE;
    auto metadataEndAddress = BootSector::SIZE + FAT_COUNT * FAT::SIZE + sizeof(this->mPaddingSize);
    auto freeSpaceInBytes = this->mDiskSize - metadataEndAddress;
    this->mPaddingSize = static_cast<int>(freeSpaceInBytes % this->mClusterSize);
    this->mPadding = std::string(mPaddingSize, '\00');
    this->mClusterCount = static_cast<int>(freeSpaceInBytes / this->mClusterSize);
    this->mDataStartAddress = static_cast<int>(mPaddingSize + metadataEndAddress);
}

void BootSector::write(std::ofstream &f) {
    writeToStream(f, this->mSignature, SIGNATURE_LENGTH);
    writeToStream(f, this->mClusterSize);
    writeToStream(f, this->mClusterCount);
    writeToStream(f, this->mDiskSize);
    writeToStream(f, this->mFatCount);
    writeToStream(f, this->mFat1StartAddress);
    writeToStream(f, this->mFat2StartAddress);
    writeToStream(f, this->mPaddingSize);
    writeToStream(f, this->mPadding, this->mPaddingSize);
    writeToStream(f, this->mDataStartAddress);
}

void BootSector::read(std::ifstream &f) {
    readFromStream(f, this->mSignature, SIGNATURE_LENGTH);
    readFromStream(f, this->mClusterSize);
    readFromStream(f, this->mClusterCount);
    readFromStream(f, this->mDiskSize);
    readFromStream(f, this->mFatCount);
    readFromStream(f, this->mFat1StartAddress);
    readFromStream(f, this->mFat2StartAddress);
    readFromStream(f, this->mPaddingSize);
    f.ignore(this->mPaddingSize);
    readFromStream(f, this->mDataStartAddress);
}

std::ostream &operator<<(std::ostream &os, BootSector const &bs) {
    return os << "Signature: " << bs.mSignature.c_str() << "\n"
              << "ClusterSize: " << bs.mClusterSize << "\n"
              << "ClusterCount: " << bs.mClusterCount << "\n"
              << "DiskSize: " << bs.mDiskSize / FORMAT_UNIT << "MB" << "\n"
              << "FatCount: " << bs.mFatCount << "\n"
              << "Fat1StartAddress: " << bs.mFat1StartAddress << "\n"
              << "Fat2StartAddress: " << bs.mFat2StartAddress << "\n"
              << "DataStartAddress: " << bs.mDataStartAddress << "\n";
}


FileSystem::FileSystem(std::string &fileName) : mFileName(fileName) {
    std::ifstream f_in(fileName, std::ios::binary | std::ios::in);
    if (f_in.is_open())
        try {
            this->read(f_in);
        } catch (...) {
            std::cerr << "Error occurred during the loading process. Input file might be corrupted." << std::endl;
        }
    else
        this->formatFS();
}

void FileSystem::write(std::ofstream &f) {
    this->mBootSector.write(f);
    this->mFat1.write(f);
    this->mFat2.write(f);
    this->mRootDir.write(f);

    char zeroFill[CLUSTER_SIZE] = {'\00'};
    for (int i = 0; i < this->mBootSector.getClusterCount() - 1; i++) { // -1 for root entry.
        writeToStream(f, zeroFill, CLUSTER_SIZE);
    }
}

void FileSystem::read(std::ifstream &f) {
    this->mBootSector.read(f);
    this->mFat1.read(f);
    this->mFat2.read(f);
    this->mRootDir.read(f);
}

std::ostream &operator<<(std::ostream &os, FileSystem const &fs) {
    return os << "\n ==========    FILE SYSTEM SPECS    ========== \n\n"
              << "BOOT-SECTOR\n" << fs.mBootSector << "\n"
              << "FAT1:\n" << fs.mFat1 << "\n"
              << "FAT2:\n" << fs.mFat2 << "\n"
              << "ROOT-DIR:\n" << fs.mRootDir << "\n"
              << "\n ========== END OF FILE SYSTEM SPECS ========== \n";
}

void FileSystem::formatFS(int size) {
    this->mBootSector = BootSector{size};
    this->mFat1 = FAT{};
    this->mFat2 = FAT{};
    this->mRootDir = DirectoryItem{std::string(ROOT_DIR_NAME), false, 0, 0};

    std::ofstream f_out{this->mFileName, std::ios::binary | std::ios::out};
    this->write(f_out);
}

//    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(f), {});
//    auto fsObjSize = sizeof(*this);