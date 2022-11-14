#include "validators.h"

constexpr auto rFileName = R"(^\.{3,}[\w\.]*$)";
constexpr auto rFilePath = R"(^\.{3,}[\w\./]*$)"; // something like " mkdir demo/.. " still proceeds...

bool validateFileName(std::string &fileName) {
    return std::regex_match(fileName, std::regex(rFileName));
}

bool validateFilePath(std::string &fileName) {
    return std::regex_match(fileName, std::regex(rFilePath));
}