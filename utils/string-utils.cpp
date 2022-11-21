#include "string-utils.h"
#include <functional>
#include <vector>

/**
 * Erases first substring.
 */
void eraseSubString(std::string &str, const std::string &toErase) {
    size_t pos = str.find(toErase);
    if (pos != std::string::npos) {
        str.erase(pos, toErase.length());
    }
}

/**
 * Erases ALL occurrences of substring.
 */
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

/**
 * Erases ALL occurrences of ALL substrings.
 */
void eraseAllSubStrings(std::string &mainStr, const std::vector<std::string> &strList) {
    std::for_each(strList.begin(), strList.end(),
                  [&mainStr](auto &&PH1) { return eraseAllSubString(mainStr, std::forward<decltype(PH1)>(PH1)); });
}

bool is_number(const std::string &s) {
    return !s.empty() && std::find_if(s.begin(), s.end(),
                                      [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

/**
 * Splits string by delimiter and doesn't return empty/whitespace tokens.
 */
std::vector<std::string> split(const std::string &s, const std::string &delimiter) {
    size_t posStart = 0, posEnd, delimLen = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((posEnd = s.find(delimiter, posStart)) != std::string::npos) {
        token = s.substr(posStart, posEnd - posStart);
        posStart = posEnd + delimLen;
        if(!token.empty()) res.push_back(token);
    }
    token = s.substr(posStart);
    if(!token.empty()) res.push_back(token);
    return res;
}