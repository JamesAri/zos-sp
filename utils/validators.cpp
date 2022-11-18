#include "validators.h"
#include "../definitions.h"

constexpr auto rFileName = R"(^[^\/]+$)";
constexpr auto rFilePath = R"(^[^\/]+(\/[^\/]+)*\/?$)";

bool validateFileName(std::string &fileName) {
    size_t fnSize = fileName.length();
    if(fileName.empty() || fnSize >= ITEM_NAME_LENGTH) return false;
    return std::regex_match(fileName, std::regex(rFileName));
}

bool validateFilePath(std::string &fileName) {
    return std::regex_match(fileName, std::regex(rFilePath));
}

bool isAllocatedDirectoryEntry(std::string &itemName) {
    return itemName.at(0) != '\00';
}