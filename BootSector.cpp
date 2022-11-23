#include "BootSector.h"
#include "utils/stream-utils.h"

BootSector::BootSector(int diskSize) : mDiskSize(diskSize * FORMAT_UNIT) {
    mSignature = SIGNATURE;
    mClusterSize = CLUSTER_SIZE;
    mFatCount = FAT_COUNT;

    size_t freeSpaceInBytes = mDiskSize - BootSector::SIZE;
    mClusterCount = static_cast<int>(freeSpaceInBytes / (sizeof(int32_t) + mClusterSize));
    mFatSize = static_cast<int>(mClusterCount * sizeof(int32_t));
    mFat1StartAddress = BootSector::SIZE;

    size_t dataSize = mClusterCount * mClusterSize;
    size_t fatTablesSize = mFatSize * mFatCount;
    mPaddingSize = static_cast<int>(freeSpaceInBytes - (dataSize + fatTablesSize));

    auto fatEndAddress = mFat1StartAddress + mFatSize;
    mDataStartAddress = static_cast<int>(mPaddingSize + fatEndAddress);
}

void BootSector::write(std::fstream &f) {
    writeToStream(f, mSignature, SIGNATURE_LENGTH);
    writeToStream(f, mClusterSize);
    writeToStream(f, mClusterCount);
    writeToStream(f, mDiskSize);
    writeToStream(f, mFatCount);
    writeToStream(f, mFat1StartAddress);
    writeToStream(f, mDataStartAddress);
    writeToStream(f, mPaddingSize);
}

void BootSector::read(std::fstream &f) {
    readFromStream(f, mSignature, SIGNATURE_LENGTH);
    readFromStream(f, mClusterSize);
    readFromStream(f, mClusterCount);
    readFromStream(f, mDiskSize);
    readFromStream(f, mFatCount);
    readFromStream(f, mFat1StartAddress);
    readFromStream(f, mDataStartAddress);
    readFromStream(f, mPaddingSize);

    mFatSize = static_cast<int>(mClusterCount * sizeof(int32_t));
}

std::ostream &operator<<(std::ostream &os, BootSector const &bs) {
    return os << "  Signature: " << bs.mSignature.c_str() << "\n"
              << "  ClusterSize: " << bs.mClusterSize << "B\n"
              << "  ClusterCount: " << bs.mClusterCount << "\n"
              << "  DiskSize: " << bs.mDiskSize / FORMAT_UNIT << "MB\n"
              << "  FatCount: " << bs.mFatCount << "\n"
              << "  Fat1StartAddress: " << bs.mFat1StartAddress << "-" << bs.mFat1StartAddress + bs.mFatSize << "\n"
              << "  Padding size: " << bs.mPaddingSize << "B\n"
              << "  PaddingAddress: " << bs.mFat1StartAddress + bs.mFatSize << "-" << bs.mDataStartAddress << "\n"
              << "  DataStartAddress: " << bs.mDataStartAddress << "\n";
}