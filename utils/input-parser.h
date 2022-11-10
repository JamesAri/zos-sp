#ifndef ZOS_SP_INPUT_PARSER_H
#define ZOS_SP_INPUT_PARSER_H

#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <iostream>

template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
        if (!item.empty())
            *result++ = item;
    }
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

#endif //ZOS_SP_INPUT_PARSER_H
