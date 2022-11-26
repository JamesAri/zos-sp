#ifndef ZOS_SP_STREAM_UTILS_H
#define ZOS_SP_STREAM_UTILS_H

#include <fstream>
#include <algorithm>

template<typename T>
void writeToStream(std::fstream &f, T &data, int streamSize = sizeof(T)) {
    f.write(reinterpret_cast<char *>(&data), streamSize);
}

template<typename T>
void readFromStream(std::fstream &stream, T &data, int streamSize = sizeof(T)) {
    stream.read(reinterpret_cast<char *>(&data), streamSize);
}

void writeToStream(std::fstream &stream, std::string &string, int streamSize);

void readFromStream(std::fstream &stream, std::string &string, int streamSize);


#endif //ZOS_SP_STREAM_UTILS_H
