#ifndef ZOS_SP_VALIDATORS_H
#define ZOS_SP_VALIDATORS_H

#include <regex>

bool validateFileName(std::string &fileName);

bool validateFilePath(std::string &fileName);

bool isAllocatedDirectoryEntry(std::string &itemName);

#endif //ZOS_SP_VALIDATORS_H
