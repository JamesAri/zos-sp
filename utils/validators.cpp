#include "validators.h"
#include "../definitions.h"

constexpr auto rFileName = R"(^[^\/]+$)";
constexpr auto rFilePath = R"(^[^\/]+(\/[^\/]+)*\/?$)";

/**
 * Checks if item name is allocated directory entry, i.e. valid entry item name.
 */
bool isAllocatedDirectoryEntry(std::string &itemName) {
    return itemName.at(0) != '\00';
}

bool validateFilePath(std::string &fileName) {
    return std::regex_match(fileName, std::regex(rFilePath));
}

bool validateFileName(std::string &fileName) {
    size_t fnSize = fileName.length();
    if(fileName.empty() || fnSize >= ITEM_NAME_LENGTH) return false;
    return std::regex_match(fileName, std::regex(rFileName)) && isAllocatedDirectoryEntry(fileName);
}