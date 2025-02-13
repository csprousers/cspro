#pragma once

#include <zUtilF/zUtilF.h>
#include <atlimage.h>


// a class for managing CImage objects on Windows;
// the class will cache some number of images in memory
// to avoid repeatedly reading the files from the disk

class CLASS_DECL_ZUTILF ImageManager
{
    static constexpr int64_t MaxCacheFileSizeBytes = 50 * 1024 * 1024;

public:
    // loads an image, returning null on error
    static std::shared_ptr<CImage> GetImage(const CString& filename);


    // resizes the image as necessary
    // - if a background brush is provided, the image will always be resized as the transparency will be applied using the background color
    // - if the resizing fails, a copy of the source image is returned (so the return value is always non-null)
    // - resized images are not cached
    static std::unique_ptr<CImage> GetResizedImage(std::shared_ptr<CImage> source_image,
                                                   double scale_percent, HBRUSH hBackgroundBrush = nullptr);

    static std::unique_ptr<CImage> GetResizedImage(std::shared_ptr<CImage> source_image,
                                                   int new_width, int new_height, HBRUSH hBackgroundBrush = nullptr);

    // resizes the image with the width or height equaling the size and the other dimension being
    // equal or less than the size
    static std::unique_ptr<CImage> GetResizedImageToSize(std::shared_ptr<CImage> source_image,
                                                         int max_size, HBRUSH hBackgroundBrush = nullptr);

private:
    static std::unique_ptr<CImage> GetResizedImageWorker(std::shared_ptr<CImage> source_image,
                                                        int new_width, int new_height, HBRUSH hBackgroundBrush);
};
