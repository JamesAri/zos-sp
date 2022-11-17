#ifndef ZOS_SP_STREAM_UTILS_H
#define ZOS_SP_STREAM_UTILS_H

#include <fstream>
#include <algorithm>

//todo -> move to cpp?

//void isOpenFatalCheck(const std::fstream &stream) {
//    if (!stream.is_open()) {
//        throw std::runtime_error(FS_OPEN_ERROR);
//    }
//}

template<typename T>
void writeToStream(std::ostream &f, T &data, int streamSize = sizeof(T)) {
    f.write(reinterpret_cast<char *>(&data), streamSize);
}

template<typename T>
void readFromStream(std::istream &f, T &data, int streamSize = sizeof(T)) {
    f.read(reinterpret_cast<char *>(&data), streamSize);
}

void writeToStream(std::ostream &f, std::string &stream, int streamSize) {
    auto str = stream + std::string(streamSize - stream.length(), '\00');
//    const char *charArr = &stream[0]; // we need \00 included
    f.write(str.c_str(), streamSize);
}

void readFromStream(std::istream &f, std::string &stream, int streamSize) {
    char temp[streamSize];
    f.read(temp, streamSize);
    stream = std::string(temp, streamSize);
}

#endif //ZOS_SP_STREAM_UTILS_H
