#ifndef ZOS_SP_DEFINITIONS_H
#define ZOS_SP_DEFINITIONS_H

#include <cstdint>
#include <string>

constexpr int32_t FAT_UNUSED = INT32_MAX - 1; // ffff fffe
constexpr int32_t FAT_FILE_END = INT32_MAX - 2; // ffff fffd
constexpr int32_t FAT_BAD_CLUSTER = INT32_MAX - 3; // ffff fffc

constexpr auto SIGNATURE = "A20B0234P\00";
constexpr auto SIGNATURE_LENGTH = 10; // with EOF

constexpr auto DEFAULT_FORMAT_SIZE = 100; // MB
constexpr auto FORMAT_UNIT = 1'000'000; // MB -> B

constexpr auto FAT_COUNT = 2;
constexpr auto CLUSTER_SIZE = 512 * 8;

constexpr auto ALLOWED_ITEM_NAME_LENGTH = 11; // without EOF
constexpr auto ROOT_DIR_NAME = "/";

const std::string FS_OPEN_ERROR{"internal error, couldn't open file system file\""};

#endif //ZOS_SP_DEFINITIONS_H
