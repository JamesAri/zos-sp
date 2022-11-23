#include "stream-utils.h"

void writeToStream(std::fstream &stream, std::string &string, int streamSize) {
    auto str = string + std::string(streamSize - string.length(), '\00');
    stream.write(str.c_str(), streamSize);
}

void readFromStream(std::fstream &stream, std::string &string, int streamSize) {
    char temp[streamSize];
    stream.read(temp, streamSize);
    string = std::string(temp, streamSize);
}