#pragma once


class Helpers
{
public:
    // Compares file paths case-insensitively and so that files at directory roots are listed before subdirectory files.
    static bool CompareFilePathsByDirectory(const std::string& file_path1, const std::string& file_path2);
};
