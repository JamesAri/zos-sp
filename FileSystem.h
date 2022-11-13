#ifndef ZOS_SP_FILESYSTEM_H
#define ZOS_SP_FILESYSTEM_H

#include <string>
#include <fstream>

constexpr int32_t FAT_UNUSED = INT32_MAX - 1; // ffff fffe
constexpr int32_t FAT_FILE_END = INT32_MAX - 2; // ffff fffd
constexpr int32_t FAT_BAD_CLUSTER = INT32_MAX - 3; // ffff fffc

constexpr auto SIGNATURE = "A20B0234P\00";
constexpr auto SIGNATURE_LENGTH = 10;
constexpr auto DEFAULT_FORMAT_SIZE = 600; // MB
constexpr auto FORMAT_UNIT = 1'000'000; // MB -> B
constexpr auto FAT_COUNT = 2;
constexpr auto CLUSTER_SIZE = 512 * 8;

constexpr auto ROOT_DIR_NAME = "/";
constexpr auto ITEM_NAME_LENGTH = 11;


class DirectoryItem {
private:
    std::string mItemName;
    bool mIsFile;
    int mSize;                   //velikost souboru, u adresáře 0 (bude zabirat jeden blok)
    int mStartCluster;           //počáteční cluster položky
public:
    DirectoryItem(){};

    DirectoryItem(std::string &&mItemName, bool mIsFile, int mSize, int mStartCluster);

    static const int SIZE = ITEM_NAME_LENGTH + sizeof(mIsFile) + sizeof(mSize) + sizeof(mStartCluster);

    void write(std::ofstream &f);

    void read(std::ifstream &f);

    friend std::ostream &operator<<(std::ostream &os, DirectoryItem const &fs);
};

class FAT {

public:
    static const int SIZE = 69;

    FAT();

    void write(std::ofstream &f);

    void read(std::ifstream &f);

    friend std::ostream &operator<<(std::ostream &os, FAT const &fs);
};

class BootSector {
private:
    std::string mSignature;
    int mClusterSize;
    int mClusterCount;
    int mDiskSize;             //celkova velikost VFS
    int mFatCount;             //pocet polozek kazde FAT tabulce
    int mFat1StartAddress;     //adresa pocatku FAT1 tabulky
    int mFat2StartAddress;     //adresa pocatku FAT2 tabulky
    int mPaddingSize;
    std::string mPadding;
    int mDataStartAddress;     //adresa pocatku datovych bloku (hl. adresar)

public:
    static const int SIZE = SIGNATURE_LENGTH + sizeof(mClusterSize) + sizeof(mClusterCount) +
                            sizeof(mDiskSize) + sizeof(mFatCount) + sizeof(mFat1StartAddress) +
                            sizeof(mFat2StartAddress) + sizeof(mDataStartAddress);

    BootSector() {};

    explicit BootSector(int size);

    void write(std::ofstream &f);

    void read(std::ifstream &f);

    friend std::ostream &operator<<(std::ostream &os, BootSector const &fs);
};


/**
 * FAT FS
 */
class FileSystem {
private:
    std::string mFileName;
    // actual memory structure
    BootSector mBootSector;
    FAT mFat1;
    FAT mFat2;
    DirectoryItem mRootDir;

public:
    explicit FileSystem(std::string &fileName);

    void write(std::ofstream &f);

    void read(std::ifstream &f);

    friend std::ostream &operator<<(std::ostream &os, FileSystem const &fs);

    void formatFS(int size = DEFAULT_FORMAT_SIZE);
};


#endif //ZOS_SP_FILESYSTEM_H
