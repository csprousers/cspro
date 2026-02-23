#pragma once

#include <zMultimediaO/zMultimediaO.h>
#include <atlimage.h>


// --------------------------------------------------------------------------
// ImageManager
//
// A class for managing CImage objects on Windows.
// 
// The class will cache some number of images in memory
// to avoid repeatedly reading the files from the disk.
// --------------------------------------------------------------------------

class ZMULTIMEDIAO_API ImageManager
{
    static constexpr int64_t MaxCacheFileSizeBytes = 50 * 1024 * 1024;

public:
    // Loads an image, returning null on error.
    // CImage::Load is used for these formats: BMP, GIF, JPEG, PNG, and TIFF.
    // WebP files are converted to BMP files and then loaded using CImage::Load.
    static std::shared_ptr<const CImage> GetImage(const std::string& file_path);

    // Resizes the image as necessary:
    // - If a background brush is provided, the image will always be resized
    //   as the transparency will be applied using the background color.
    // - If the resizing fails, a copy of the source image is returned
    //   (so the return value is always non-null).
    // - Resized images are not cached.
    static std::unique_ptr<CImage> GetResizedImage(const CImage& source_image,
                                                   double scale_percent, HBRUSH hBackgroundBrush = nullptr);

    static std::unique_ptr<CImage> GetResizedImage(const CImage& source_image,
                                                   int new_width, int new_height, HBRUSH hBackgroundBrush = nullptr);

    // Resizes the image with the width or height equaling the size and the other
    // dimension being equal or less than the size.
    static std::unique_ptr<CImage> GetResizedImageToSize(const CImage& source_image,
                                                         int max_size, HBRUSH hBackgroundBrush = nullptr);

private:
    static bool ConvertWebPToBitmap(CImage& image, const std::string& file_path);

    static std::unique_ptr<CImage> GetResizedImageWorker(const CImage& source_image,
                                                         int new_width, int new_height, HBRUSH hBackgroundBrush);
};
