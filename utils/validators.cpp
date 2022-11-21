#include "validators.h"
#include "../definitions.h"

constexpr auto rFilePath = R"(^\/?[^\/]+(\/[^\/]+)*\/?$)";

/**
 * Checks if item name is allocated directory entry, i.e. valid entry item name.
 */
bool isAllocatedDirectoryEntry(const std::string &itemName) {
    return itemName.at(0) != '\00';
}

bool validateFilePath(const std::string &path) {
    return std::regex_match(path, std::regex(rFilePath));
}
