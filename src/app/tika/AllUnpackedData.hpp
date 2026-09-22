#pragma once

#include <string>
#include <vector>

namespace tika
{

struct FileData
{
    std::string type;
    std::string fileName;
    std::string data;
};
struct ImageData
{
    unsigned width = 0;
    unsigned height = 0;
    std::string type;
    std::string fileName;
    std::string data;
};

struct AllUnpackedData
{
    std::vector<FileData> textParsedFiles;
    std::vector<ImageData> imageFiles;
};
} // namespace tika