#ifndef ZOS_SP_STREAM_UTILS_H
#define ZOS_SP_STREAM_UTILS_H

#include <fstream>
#include <algorithm>

//todo -> move to cpp?

template<typename T>
void writeToStream(std::fstream &f, T &data, int streamSize = sizeof(T)) {
    f.write(reinterpret_cast<char *>(&data), streamSize);
}

template<typename T>
void readFromStream(std::fstream &stream, T &data, int streamSize = sizeof(T)) {
    stream.read(reinterpret_cast<char *>(&data), streamSize);
}

void writeToStream(std::fstream &stream, std::string &string, int streamSize) {
    auto str = string + std::string(streamSize - string.length(), '\00');
    stream.write(str.c_str(), streamSize);
}

void readFromStream(std::fstream &stream, std::string &string, int streamSize) {
    char temp[streamSize];
    stream.read(temp, streamSize);
    string = std::string(temp, streamSize);
}

#endif //ZOS_SP_STREAM_UTILS_H
