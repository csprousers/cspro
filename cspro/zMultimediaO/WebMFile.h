#pragma once

#include <zMultimediaO/zMultimediaO.h>

namespace mkvparser { class MkvReader; }


// --------------------------------------------------------------------------
// WebMFile
//
// This class wraps libwebm functionality for reading WebM files.
// --------------------------------------------------------------------------

class ZMULTIMEDIAO_API WebMFile
{
public:
    // Returns true if the file exists and is a valid WebM file.
    // A CSProException is thrown if the file does not exist or cannot be opened.
    static bool IsValidFile(cs::string_sz file_path);

private:
    static std::unique_ptr<mkvparser::MkvReader> CreateReader(const char* file_path);
};
