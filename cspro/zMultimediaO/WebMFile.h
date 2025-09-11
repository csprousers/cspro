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
    // If passing in a non-null FILE* pointer, the caller retains ownership of the FILE*.
    // A CSProException is thrown if the file does not exist or cannot be opened.
    static bool IsValidFile(std::variant<cs::string_sz, FILE*> file_path_or_file);

    // Calculates the duration of the WebM file, returning it in seconds.
    // If passing in a non-null FILE* pointer, the caller retains ownership of the FILE*.
    // The flag indicates if the segment duration should be used, when set.
    // Otherwise, each cluster's blocks are examined to determine the length, with the duration of the
    // last block approximated, so this length may be an undercount by a hundredth of a second or so.
    // A CSProException is thrown on error.
    static double GetDuration(std::variant<cs::string_sz, FILE*> file_path_or_file, bool use_segment_duration_if_set);

private:
    static std::unique_ptr<mkvparser::MkvReader> CreateReader(std::variant<cs::string_sz, FILE*> file_path_or_file);
};
