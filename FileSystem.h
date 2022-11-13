#ifndef ZOS_SP_FILESYSTEM_H
#define ZOS_SP_FILESYSTEM_H

#include <string>
#include <fstream>

constexpr int32_t FAT_UNUSED = INT32_MAX - 1; // ffff fffe
constexpr int32_t FAT_FILE_END = INT32_MAX - 2; // ffff fffd
constexpr int32_t FAT_BAD_CLUSTER = INT32_MAX - 3; // ffff fffc

constexpr auto SIGNATURE = "A20B0234P";
constexpr auto SIGNATURE_LENGTH = 9;
constexpr auto ITEM_NAME_LENGTH = 11;
constexpr auto CLUSTER_SIZE = 512 * 8;


class DirectoryItem {
private:
    char mItemName[ITEM_NAME_LENGTH];
    bool mIsFile;
    int mSize;                   //velikost souboru, u adresáře 0 (bude zabirat jeden blok)
    int mStartCluster;           //počáteční cluster položky
public:
    static const int SIZE = ITEM_NAME_LENGTH + sizeof(mIsFile) + sizeof(mSize) + sizeof(mStartCluster);

    void write(std::ofstream &f);

    void read(std::ifstream &f);
};

class FAT {

public:
    static const int SIZE = 0;

    void write(std::ofstream &f);

    void read(std::ifstream &f);
};

class BootSector {
private:
    char mSignature[SIGNATURE_LENGTH];
    int mClusterSize;
    int mClusterCount;
    int mDiskSize;             //celkova velikost VFS
    int mFatCount;             //pocet polozek kazde FAT tabulce
    int mFat1StartAddress;     //adresa pocatku FAT1 tabulky
    int mFat2StartAddress;     //adresa pocatku FAT2 tabulky
    int mDataStartAddress;     //adresa pocatku datovych bloku (hl. adresar)

public:
    static const auto SIZE = strlen(SIGNATURE) + sizeof(mClusterSize) + sizeof(mClusterCount) +
                            sizeof(mDiskSize) + sizeof(mFatCount) + sizeof(mFat1StartAddress) +
                            sizeof(mFat2StartAddress) + sizeof(mDataStartAddress);

    BootSector();

    void write(std::ofstream &f);

    void read(std::ifstream &f);
};


/**
 * FAT FS
 */
class FileSystem {
private:
    std::string mFileName;
    BootSector mBootSector;
    FAT mFat1;
    FAT mFat2;
    DirectoryItem mRootDir;

public:
    explicit FileSystem(std::string &fileName);

    void write(std::ofstream &f);

    void read(std::ifstream &f);
};


#endif //ZOS_SP_FILESYSTEM_H
