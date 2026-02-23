#include "stdafx.h"
#include "ImageManager.h"
#include "Image.h"
#include <zToolsO/WinMemoryStream.h>


namespace
{
    struct ImageData
    {
        std::string file_path;
        int64_t file_size;
        int64_t file_time;
        std::shared_ptr<CImage> image;
    };

    std::vector<ImageData> cached_image_data;
}


std::shared_ptr<const CImage> ImageManager::GetImage(const std::string& file_path)
{
    const int64_t file_time = PortableFunctions::FileModifiedTime(file_path);
    int64_t current_cache_file_size_bytes = 0;

    for( const ImageData& image_data : cached_image_data )
    {
        // return a cached image when possible
        if( file_time == image_data.file_time && SO::EqualsNoCase(file_path, image_data.file_path) )
            return image_data.image;

        current_cache_file_size_bytes += image_data.file_size;
    }

    // load the image
    ImageData new_image_data
    {
        file_path,
        PortableFunctions::FileSize(file_path),
        file_time,
        std::make_unique<CImage>()
    };

    if( new_image_data.file_size == -1 )
        return nullptr;

    // WebP
    if( MimeType::GetSupportedImageTypeFromFileExtension(Path::GetExtension(file_path)) == ImageType::WebP )
    {
        if( !ConvertWebPToBitmap(*new_image_data.image, file_path) )
            return nullptr;
    }

    // BMP, GIF, JPEG, PNG, and TIFF
    else
    {
        if( new_image_data.image->Load(TC::ToWide(file_path).c_str()) != S_OK )
            return nullptr;
    }

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


bool ImageManager::ConvertWebPToBitmap(CImage& image, const std::string& file_path)
{
    try
    {
        const std::unique_ptr<Multimedia::Image> webp_image = Multimedia::Image::FromFile(file_path);
        ASSERT(webp_image != nullptr);

        const std::unique_ptr<std::vector<std::byte>> bitmap_image = webp_image->ToBuffer(ImageType::Bitmap);

        if( bitmap_image != nullptr )
        {
            const std::unique_ptr<WinMemoryStream> memory_stream = WinMemoryStream::Create(bitmap_image->data(), bitmap_image->size());

            if( memory_stream != nullptr && 
                image.Load(memory_stream->GetStream()) == S_OK )
            {
                return true;
            }
        }
    }
    catch(...) { }

    return false;
}


std::unique_ptr<CImage> ImageManager::GetResizedImageWorker(const CImage& source_image,
                                                            const int new_width, const int new_height, const HBRUSH hBackgroundBrush)
{
    auto scaled_image = std::make_unique<CImage>();

    // use full-24 bit color which can represent any image depth we load
    if( !scaled_image->Create(new_width, new_height, 24) )
        return std::make_unique<CImage>(source_image);

    const HDC scaledDC = scaled_image->GetDC();

    // COLORONCOLOR is a good balance of quality vs. speed
    SetStretchBltMode(scaledDC, COLORONCOLOR);

    // fill with the background color so that the transparency in the image looks correct
    if( hBackgroundBrush != nullptr )
        FillRect(scaledDC, CRect(0, 0, new_width, new_height), hBackgroundBrush);

    source_image.Draw(scaledDC, 0, 0, new_width, new_height);

    scaled_image->ReleaseDC();

    return scaled_image;
}


std::unique_ptr<CImage> ImageManager::GetResizedImage(const CImage& source_image,
                                                      const double scale_percent, const HBRUSH hBackgroundBrush/* = nullptr*/)
{
    if( hBackgroundBrush == nullptr && scale_percent == 1 )
        return std::make_unique<CImage>(source_image);

    return GetResizedImageWorker(source_image,
                                 static_cast<int>(source_image.GetWidth() * scale_percent),
                                 static_cast<int>(source_image.GetHeight() * scale_percent),
                                 hBackgroundBrush);
}


std::unique_ptr<CImage> ImageManager::GetResizedImage(const CImage& source_image,
                                                      const int new_width, const int new_height, const HBRUSH hBackgroundBrush/* = nullptr*/)
{
    if( hBackgroundBrush == nullptr && source_image.GetWidth() == new_width && source_image.GetHeight() == new_height )
        return std::make_unique<CImage>(source_image);

    return GetResizedImageWorker(source_image, new_width, new_height, hBackgroundBrush);
}


std::unique_ptr<CImage> ImageManager::GetResizedImageToSize(const CImage& source_image,
                                                            const int max_size, const HBRUSH hBackgroundBrush/*= nullptr*/)
{
    ASSERT(source_image != nullptr && max_size > 0);

    const double scale_x = max_size / static_cast<double>(source_image.GetWidth());
    const double scale_y = max_size / static_cast<double>(source_image.GetHeight());

    return GetResizedImage(source_image, std::min(scale_x, scale_y), hBackgroundBrush);
}
