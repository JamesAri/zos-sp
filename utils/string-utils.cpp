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
    size_t pos;
    while ((pos = str.find(toErase)) != std::string::npos) {
        str.erase(pos, toErase.length());
    }
}

//void eraseAllSubStrings(std::string &str, const std::vector<std::string> &strList) {
//    for (const auto &it: strList) {
//        eraseAllSubString(str, it);
//    }
//}

void eraseAllSubStrings(std::string &mainStr, const std::vector<std::string> &strList) {
    std::for_each(strList.begin(), strList.end(),
                  [&mainStr](auto &&PH1) { return eraseAllSubString(mainStr, std::forward<decltype(PH1)>(PH1)); });
}

bool is_number(const std::string &s) {
    return !s.empty() && std::find_if(s.begin(), s.end(),
                                      [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}