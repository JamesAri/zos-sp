#ifndef ZOS_SP_STRING_UTILS_H
#define ZOS_SP_STRING_UTILS_H

#include <string>

void eraseSubString(std::string &str, const std::string &toErase);

void eraseAllSubString(std::string &str, const std::string &toErase);

//void eraseAllSubStrings(std::string &str, const std::vector<std::string> &strList);

void eraseAllSubStrings(std::string &mainStr, const std::vector<std::string> &strList);

bool is_number(const std::string& s);

#endif //ZOS_SP_STRING_UTILS_H
