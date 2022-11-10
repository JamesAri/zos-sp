#ifndef ZOS_SP_FAT_H
#define ZOS_SP_FAT_H

#include <cstdint>

constexpr int32_t FAT_UNUSED = INT32_MAX - 1; // ffff fffe
constexpr int32_t FAT_FILE_END = INT32_MAX - 2; // ffff fffd
constexpr int32_t FAT_BAD_CLUSTER = INT32_MAX - 3; // ffff fffc

#endif //ZOS_SP_FAT_H
