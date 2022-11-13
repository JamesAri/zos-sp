#include "string-utils.h"
#include <functional>
#include <vector>

void eraseSubString(std::string &str, const std::string &toErase) {
    size_t pos = str.find(toErase);
    if (pos != std::string::npos) {
        str.erase(pos, toErase.length());
    }
}

void eraseAllSubString(std::string &str, const std::string &toErase) {
    size_t pos = std::string::npos;
    while ((pos = str.find(toErase)) != std::string::npos) {
        str.erase(pos, toErase.length());
    }
}

void eraseAllSubStrings(std::string &str, const std::vector<std::string> &strList) {
    for (const auto &it: strList) {
        eraseAllSubString(str, it);
    }
}