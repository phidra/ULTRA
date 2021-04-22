#pragma once

#include <filesystem>
#include <cstdio>

namespace my {

struct AutoDeleteTempFile {
    // file path is computed at construction, but no file is created on disk :
    inline AutoDeleteTempFile() : file{tmpnam(nullptr)} {}

    // attemps to remove file on destruction :
    inline ~AutoDeleteTempFile() { std::filesystem::remove(file); }

    std::filesystem::path file;
};

}  // namespace my
