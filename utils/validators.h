#ifndef ZOS_SP_VALIDATORS_H
#define ZOS_SP_VALIDATORS_H

#include <regex>

bool validateFileName(const std::string &fileName);

bool validateFilePath(const std::string &fileName);

bool isAllocatedDirectoryEntry(const std::string &itemName);

#endif //ZOS_SP_VALIDATORS_H
