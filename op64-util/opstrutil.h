#pragma once

#include <vector>
#include <string>
#include <sstream>

#include "oppreproc.h"

// From https://stackoverflow.com/questions/236129/split-a-string-in-c
inline std::vector<std::string>& split(const std::string& s, char delim, std::vector<std::string>& elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


inline std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

namespace op
{
    OP_API size_t strlcpy(char *dst, const char *src, size_t dsize);
}
