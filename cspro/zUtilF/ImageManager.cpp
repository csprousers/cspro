#include "StdAfx.h"
#include "ImageManager.h"


namespace
{
    struct ImageData
    {
        CString filename;
        int64_t file_size;
        time_t file_time;
        std::shared_ptr<CImage> image;
    };

    std::vector<ImageData> cached_image_data;
}


std::shared_ptr<CImage> ImageManager::GetImage(const CString& filename)
{
    time_t file_time = PortableFunctions::FileModifiedTime(filename);
    int64_t current_cache_file_size_bytes = 0;

    for( const auto& image_data : cached_image_data )
    {
        // return a cached image when possible
        if( filename.CompareNoCase(image_data.filename) == 0 && file_time == image_data.file_time )
            return image_data.image;

        current_cache_file_size_bytes += image_data.file_size;
    }

    // load the image
    ImageData new_image_data
    {
        filename,
        PortableFunctions::FileSize(filename),
        file_time,
        std::make_shared<CImage>()
    };

    if( new_image_data.file_size == -1 || new_image_data.image->Load(filename) != S_OK )
        return nullptr;

    // make sure not too many images are cached
    current_cache_file_size_bytes += new_image_data.file_size;

    while( !cached_image_data.empty() && current_cache_file_size_bytes > MaxCacheFileSizeBytes )
    {
        current_cache_file_size_bytes -= cached_image_data.front().file_size;
        cached_image_data.erase(cached_image_data.begin());
    }

    // cache the image data and return the image
    return cached_image_data.emplace_back(std::move(new_image_data)).image;
}


std::unique_ptr<CImage> ImageManager::GetResizedImageWorker(std::shared_ptr<CImage> source_image,
                                                            int new_width, int new_height, HBRUSH hBackgroundBrush)
{
    auto scaled_image = std::make_unique<CImage>();

    // use full-24 bit color which can represent any image depth we load
    if( !scaled_image->Create(new_width, new_height, 24) )
        return std::make_unique<CImage>(*source_image);

    HDC scaledDC = scaled_image->GetDC();

    // COLORONCOLOR is a good balance of quality vs. speed
    SetStretchBltMode(scaledDC, COLORONCOLOR); 

    // fill with the background color so that the transparency in the image looks correct
    if( hBackgroundBrush != nullptr )
        FillRect(scaledDC, CRect(0, 0, new_width, new_height), hBackgroundBrush);

    source_image->Draw(scaledDC, 0, 0, new_width, new_height);

    scaled_image->ReleaseDC();

    return scaled_image;
}


std::unique_ptr<CImage> ImageManager::GetResizedImage(std::shared_ptr<CImage> source_image,
                                                      double scale_percent, HBRUSH hBackgroundBrush/*= nullptr*/)
{
    ASSERT(source_image != nullptr);

    if( hBackgroundBrush == nullptr && scale_percent == 1 )
        return std::make_unique<CImage>(*source_image);

    return GetResizedImageWorker(source_image, (int)( source_image->GetWidth() * scale_percent ),
                                               (int)( source_image->GetHeight() * scale_percent ), hBackgroundBrush);
}


std::unique_ptr<CImage> ImageManager::GetResizedImage(std::shared_ptr<CImage> source_image,
                                                      int new_width, int new_height, HBRUSH hBackgroundBrush/*= nullptr*/)
{
    ASSERT(source_image != nullptr);

    if( hBackgroundBrush == nullptr && source_image->GetWidth() == new_width && source_image->GetHeight() == new_height )
        return std::make_unique<CImage>(*source_image);

    return GetResizedImageWorker(source_image, new_width, new_height, hBackgroundBrush);
}


std::unique_ptr<CImage> ImageManager::GetResizedImageToSize(std::shared_ptr<CImage> source_image,
                                                            int max_size, HBRUSH hBackgroundBrush/*= nullptr*/)
{
    ASSERT(source_image != nullptr && max_size > 0);

    double scale_x = max_size / (double)source_image->GetWidth();
    double scale_y = max_size / (double)source_image->GetHeight();

    return GetResizedImage(source_image, std::min(scale_x, scale_y), hBackgroundBrush);
}
