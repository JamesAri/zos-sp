#ifndef ZOS_SP_STRING_UTILS_H
#define ZOS_SP_STRING_UTILS_H

#include <string>
#include <vector>

void eraseSubString(std::string &str, const std::string &toErase);

void eraseAllSubString(std::string &str, const std::string &toErase);

//void eraseAllSubStrings(std::string &str, const std::vector<std::string> &strList);

void eraseAllSubStrings(std::string &mainStr, const std::vector<std::string> &strList);

bool is_number(const std::string &s);

std::vector<std::string> split(const std::string &s, const std::string &delimiter);

#endif //ZOS_SP_STRING_UTILS_H
