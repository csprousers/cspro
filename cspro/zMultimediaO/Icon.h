#pragma once

#include <zMultimediaO/zMultimediaO.h>

namespace Multimedia { class Icon; class Image; }


// --------------------------------------------------------------------------
// Multimedia::Icon
//
// This class supports loading icon (.ico) files on Windows and saving icon
// files (in PNG format) on all platforms.
// --------------------------------------------------------------------------

class ZMULTIMEDIAO_API Multimedia::Icon
{
public:
    // Icons cannot be greater than 256 pixels (width or height).
    static constexpr int MaxSize = 256;

    // Returns true if the image's extension is .ico, compared in a case-insensitive manner.
    static bool IsExtensionIcon(std::string_view file_path_sv) { return Path::ExtensionMatches(file_path_sv, "ico"); }

    // Loads the icon from a file (on Windows), returning its bytes as a PNG, throwing exceptions on error.
    static std::unique_ptr<std::vector<std::byte>> LoadIconAsPng(const std::string& file_path);

    // Saves a PNG buffer as an icon file, throwing exceptions on error.
    // The width or height of the image cannot be greater than 256.
    // It is assumed that the directories for the file have already been created.
    static void SavePngAsIcon(const std::string& file_path, const std::vector<std::byte>& png_data, int width, int height);
};
