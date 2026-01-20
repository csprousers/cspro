#pragma once

#include <zToolsO/TextEncoding.h>


enum class VersionType { TwoWithDot, FourWithDot, FourWithComma };

using VersionLineComponent = std::variant<std::string, VersionType>;

using VersionLine = std::vector<VersionLineComponent>;


struct VersionedFile
{
    std::string file_path;
    TextEncoding::Type text_encoding_type;
    BinaryBlock file_content;
    std::vector<VersionLine> version_lines;
    std::string longest_read_version;
};
