#ifndef ZOS_SP_DEFINITIONS_H
#define ZOS_SP_DEFINITIONS_H

#include <cstdint>
#include <string>

const int32_t FAT_UNUSED = INT32_MAX - 1; // ffff fffe
const int32_t FAT_FILE_END = INT32_MAX - 2; // ffff fffd
const int32_t FAT_BAD_CLUSTER = INT32_MAX - 3; // ffff fffc

constexpr auto SIGNATURE = "A20B0234P\00";
constexpr auto SIGNATURE_LENGTH = 10; // with EOF

constexpr auto DEFAULT_FORMAT_SIZE = 1; // MB
constexpr auto FORMAT_UNIT = 1'000'000; // MB -> B

constexpr auto FAT_COUNT = 1;
constexpr auto CLUSTER_SIZE = 512 * 8;

constexpr auto ITEM_NAME_LENGTH = 12; // with EOF
constexpr auto DEFAULT_DIR_SIZE = 2; // '.' and '..' references

// fatal errors
const std::string FS_OPEN_ERROR{"internal error, couldn't open file system file"};
const std::string DE_MISSING_REFERENCES_ERROR{"internal error, directory missing references"};
const std::string DE_LIMIT_REACHED_ERROR{"internal error, directory file count reached"};
const std::string DE_ITEM_NAME_LENGTH_ERROR{"internal error, received invalid (too long) entry name"};
const std::string DE_CORRUPTED_FS_ERROR{"internal error, filesystem is corrupted"};


#endif //ZOS_SP_DEFINITIONS_H
