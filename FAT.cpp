#include "FAT.h"

#include "definitions.h"
#include "utils/stream-utils.h"

void FAT::write(std::fstream &f, int32_t label) {
    writeToStream(f, label);
}

void FAT::write(std::fstream &f, int32_t pos, int32_t label) {
    f.seekp(pos);
    FAT::write(f, label);
}

int FAT::read(std::fstream &f) {
    int32_t clusterTag;
    readFromStream(f, clusterTag);
    return clusterTag;
}

int FAT::read(std::fstream &f, int32_t pos) {
    f.seekg(pos);
    return FAT::read(f);
}

void FAT::wipe(std::ostream &f, int32_t startAddress, int32_t clusterCount) {
    f.seekp(startAddress);

    auto labelUnused = reinterpret_cast<const char *>(&FAT_UNUSED);
    for (int i = 0; i < clusterCount; i++) {
        f.write(labelUnused, sizeof(FAT_UNUSED));
    }
}

