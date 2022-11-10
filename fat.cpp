#include "fat.h"

class DirectoryItem {
private:
    /* Maximální délka názvu souboru bude 8+3=11 znaků (jméno.přípona) + \0 (ukončovací znak v
    C/C++), tedy 12 bytů.
    Každý název bude zabírat právě 12 bytů (do délky 12 bytů doplníte \0 - při kratších názvech). */
    char itemName[12];          //8+3 + /0 (name, extension, terminating char)
    bool isFile;                //identifikace zda je soubor (TRUE), nebo adresář (FALSE)
    int size;                   //velikost souboru, u adresáře 0 (bude zabirat jeden blok)
    int startCluster;           //počáteční cluster položky
};

class FAT {
    // todo
};

class BootSector {
private:
    char signature[9];        //login autora FS (kdo to naformatoval.. MSDOS5.0)
    int clusterSize;          //velikost clusteru (allocation unit size, 512 * 8 napr. (to maj widle))
    int clusterCount;         //pocet clusteru per sector (mam secotr?)
    int diskSize;             //celkova velikost VFS
    int fatCount;             //pocet polozek kazde FAT tabulce
    int fat1StartAddress;     //adresa pocatku FAT1 tabulky
    int fat2StartAddress;     //adresa pocatku FAT2 tabulky
    int dataStartAddress;     //adresa pocatku datovych bloku (hl. adresar)
};


class VFS {
private:
    BootSector mBootSector;
    FAT mFat1;
    FAT mFat2;
    DirectoryItem mRootDir;
};