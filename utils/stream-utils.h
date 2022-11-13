#ifndef ZOS_SP_STREAM_UTILS_H
#define ZOS_SP_STREAM_UTILS_H

#include <fstream>

template<typename T>
void writeToStream(std::ofstream &f, T &data, int streamSize = sizeof(T)) {
    f.write(reinterpret_cast<char *>(&data), streamSize);
}

template<typename T>
void readFromStream(std::ifstream &f, T &data, int streamSize = sizeof(T)) {
    f.read(reinterpret_cast<char *>(&data), streamSize);
}

void writeToStream(std::ofstream &f, std::string &stream, int streamSize) {
    f.write(stream.c_str(), streamSize);
}

void readFromStream(std::ifstream &f, std::string &stream, int streamSize) {
    char temp[streamSize];
    f.read(temp, streamSize);
    stream = temp;
}

#endif //ZOS_SP_STREAM_UTILS_H
