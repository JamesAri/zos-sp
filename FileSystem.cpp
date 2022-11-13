#include "FileSystem.h"
#include <iostream>
#include <vector>

template<typename T>
void write_util(std::ofstream &f, T& stream, int streamSize = sizeof(T)) {
    f.write(reinterpret_cast<char*>(stream), streamSize);
}

template<typename T>
void read(std::ifstream &f, T& stream, int streamSize = sizeof(T)) {
    f.read(reinterpret_cast<char*>(stream), streamSize);
}


void DirectoryItem::write(std::ofstream &f) {
//    write_util(f, this->mItemName, ITEM_NAME_LENGTH);
//    write_util(f, this->mIsFile);
    f.write(this->mItemName, ITEM_NAME_LENGTH);
    f.write(reinterpret_cast<char*>(&this->mIsFile), sizeof(this->mIsFile));
    f.write(reinterpret_cast<char*>(&this->mSize), sizeof(this->mSize));
    f.write(reinterpret_cast<char*>(&this->mStartCluster), sizeof(this->mStartCluster));
}

void DirectoryItem::read(std::ifstream &f) {
    f.read(this->mItemName, ITEM_NAME_LENGTH);
    f.read(reinterpret_cast<char*>(&this->mIsFile), sizeof(this->mIsFile));
    f.read(reinterpret_cast<char*>(&this->mSize), sizeof(this->mSize));
    f.read(reinterpret_cast<char*>(&this->mStartCluster), sizeof(this->mStartCluster));
}

void FAT::read(std::ifstream &f) {

}

void FAT::write(std::ofstream &f) {

}

BootSector::BootSector() : mSignature(SIGNATURE) {

}

void BootSector::write(std::ofstream &f) {
    f.write(this->mSignature, SIGNATURE_LENGTH);
}

void BootSector::read(std::ifstream &f) {
    f.read(this->mSignature, SIGNATURE_LENGTH);
}

FileSystem::FileSystem(std::string &fileName) : mFileName(fileName) {
    std::ifstream input(fileName.c_str(), std::ios::binary);
    if (input.good()) {
        this->read(input);
    } else {
        std::ofstream output(fileName.c_str(), std::ios::binary);
        this->write(output);
    }
}

void FileSystem::write(std::ofstream &f) {
    this->mBootSector.write(f);
    this->mFat1.write(f);
    this->mFat2.write(f);
    this->mRootDir.write(f);
}

void FileSystem::read(std::ifstream &f) {
    this->mBootSector.read(f);
    this->mFat1.read(f);
    this->mFat2.read(f);
    this->mRootDir.read(f);

//    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(f), {});
//    auto fsObjSize = sizeof(*this);
}


