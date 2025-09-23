#include "stdafx.h"
#include "Image.h"
#include "Icon.h"
#include <zToolsO/Tools.h>
#include <zUtilO/MimeType.h>
#include <external/libwebp/src/webp/decode.h>
#include <external/libwebp/src/webp/encode.h>
#include <external/zlib/zlib.h>


#define STBI_WINDOWS_UTF8

#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STB_IMAGE_IMPLEMENTATION
#include <external/stb/stb_image.h>

unsigned char* zlib_compress(unsigned char* data, int data_len, int* out_len, int quality);
#define STBIW_ZLIB_COMPRESS zlib_compress
#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable: 4996)
#include <external/stb/stb_image_write.h>
#pragma warning(pop)

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <external/stb/stb_image_resize2.h>


unsigned char* zlib_compress(unsigned char* const data, const int data_len, int* const out_len, const int quality)
{
    // instead of using stb's default compression algorithm (when saving PNG files), use one from zlib
    uLong buffer_len = compressBound(data_len);
    unsigned char* const buffer = static_cast<unsigned char*>(malloc(buffer_len));

    if( buffer == nullptr || compress2(buffer, &buffer_len, data, data_len, quality) != 0 )
    {
        free(buffer);
        return nullptr;
    }

    *out_len = buffer_len;
    return buffer;
}



// --------------------------------------------------------------------------
// StbImage
// --------------------------------------------------------------------------

class StbImage : public Multimedia::Image
{
public:
    StbImage(stbi_uc* const image_data, Multimedia::ImageDetails details)
        :   Image(std::move(details)),
            m_imageData(image_data)
    {
        ASSERT(m_imageData != nullptr);
    }

    ~StbImage()
    {
        stbi_image_free(m_imageData);
    }

    const std::byte* GetData() const override
    {
        return reinterpret_cast<const std::byte*>(m_imageData);
    }

    static std::optional<ImageType> ToImageType(const LoadedImageType loaded_image_type)
    {
        if( loaded_image_type == LoadedImageType::Gif )
            return std::nullopt;

        static_assert(static_cast<ImageType>(LoadedImageType::Jpeg) == ImageType::Jpeg &&
                      static_cast<ImageType>(LoadedImageType::Png) == ImageType::Png &&
                      static_cast<ImageType>(LoadedImageType::Bitmap) == ImageType::Bitmap);

        return static_cast<ImageType>(loaded_image_type);
    }

private:
    stbi_uc* m_imageData;
};



// --------------------------------------------------------------------------
// BinaryBlockImage
// --------------------------------------------------------------------------

class BinaryBlockImage : public Multimedia::Image
{
public:
    BinaryBlockImage(Multimedia::ImageDetails details)
        :   Image(std::move(details)),
            m_imageData(m_details.width * m_details.height * m_details.channels)
    {
    }

    BinaryBlockImage(const Image& image)
        :   BinaryBlockImage(image.GetDetails())
    {
        memcpy(m_imageData.data(), image.GetData(), m_imageData.size());
    }

    const std::byte* GetData() const override
    {
        return m_imageData.data();
    }

    std::byte* GetDataBuffer()
    {
        return m_imageData.data();
    }

private:
    BinaryBlock m_imageData;
};



// --------------------------------------------------------------------------
// WebPImage
// --------------------------------------------------------------------------

class WebPImage : public Multimedia::Image
{
public:
    WebPImage(uint8_t* const image_data, Multimedia::ImageDetails details)
        :   Image(std::move(details)),
            m_imageData(image_data)
    {
        ASSERT(m_imageData != nullptr);
    }

    ~WebPImage()
    {
        WebPFree(m_imageData);
    }

    const std::byte* GetData() const override
    {
        return reinterpret_cast<const std::byte*>(m_imageData);
    }

private:
    uint8_t* m_imageData;
};



// --------------------------------------------------------------------------
// Multimedia::Image
// --------------------------------------------------------------------------

Multimedia::Image::Image(ImageDetails details)
    :   m_details(std::move(details))
{
}


std::unique_ptr<Multimedia::Image> Multimedia::Image::FromImage(const Multimedia::Image& image)
{
    return std::make_unique<BinaryBlockImage>(image);
}


std::unique_ptr<Multimedia::Image> Multimedia::Image::FromFile(const std::string& file_path)
{
    ImageDetails details;

    // WebP
    if( MimeType::GetSupportedImageTypeFromFileExtension(Path::GetExtension(file_path)) == ImageType::WebP )
    {
        try
        {
            const std::unique_ptr<std::vector<std::byte>> webp_data = FileIO::Read(file_path);
            std::unique_ptr<Image> image = GetWebPImageFromBuffer(*webp_data);

            if( image != nullptr )
                return image;
        }
        catch(...) { }
    }

    // stb_image-supported images
    else
    {
        stbi_uc* const image_data = stbi_load(file_path.c_str(),
                                              &details.width, &details.height, &details.channels, 0);

        if( image_data != nullptr )
            return std::make_unique<StbImage>(image_data, std::move(details));
    }

    throw ImageException("Could not read the image file: %s", file_path.c_str());
}


std::unique_ptr<Multimedia::Image> Multimedia::Image::FromBuffer(const cs::span<const std::byte> content)
{
    // stb_image-supported images
    {
        ImageDetails details;
        LoadedImageType loaded_image_type;

        stbi_uc* const image_data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(content.data()), int32_cast(content.size()),
                                                          &details.width, &details.height, &details.channels, 0, &loaded_image_type);

        if( image_data != nullptr )
        {
            details.image_type = StbImage::ToImageType(loaded_image_type);

            return std::make_unique<StbImage>(image_data, std::move(details));
        }
    }

    // WebP
    {
        std::unique_ptr<Image> image = GetWebPImageFromBuffer(content);

        if( image != nullptr )
            return image;
    }

    throw ImageException("Could not parse the image");
}


std::optional<Multimedia::ImageDetails> Multimedia::Image::GetDetailsFromBuffer(const cs::span<const std::byte> content)
{
    ImageDetails details;
    LoadedImageType loaded_image_type;

    // stb_image-supported images
    if( stbi_info_from_memory(reinterpret_cast<const stbi_uc*>(content.data()), int32_cast(content.size()),
                              &details.width, &details.height, &details.channels, &loaded_image_type) == 1 )
    {
        details.image_type = StbImage::ToImageType(loaded_image_type);
        return details;
    }

    // WebP
    if( GetWebPDetailsFromBuffer(content, details) )
    {
        ASSERT(details.image_type == ImageType::WebP);
        return details;
    }

    return std::nullopt;
}


bool Multimedia::Image::GetWebPDetailsFromBuffer(const cs::span<const std::byte> content, ImageDetails& details)
{
    WebPBitstreamFeatures webp_features;

    if( WebPGetFeatures(reinterpret_cast<const uint8_t*>(content.data()), content.size(), &webp_features) != VP8_STATUS_OK )
        return false;

    details.width = webp_features.width;
    details.height = webp_features.height;
    details.channels = webp_features.has_alpha ? 4 : 3;
    details.image_type = ImageType::WebP;

    return true;
}


std::unique_ptr<Multimedia::Image> Multimedia::Image::GetWebPImageFromBuffer(const cs::span<const std::byte> content)
{
    ImageDetails details;

    if( GetWebPDetailsFromBuffer(content, details) )
    {
        ASSERT(details.image_type == ImageType::WebP);

        uint8_t* const image_data = ( details.channels == 3 )
            ? WebPDecodeRGB(reinterpret_cast<const uint8_t*>(content.data()), content.size(), nullptr, nullptr)
            : WebPDecodeRGBA(reinterpret_cast<const uint8_t*>(content.data()), content.size(), nullptr, nullptr);

        if( image_data != nullptr )
            return std::make_unique<WebPImage>(image_data, std::move(details));
    }

    return nullptr;
}


void Multimedia::Image::ToFile(const std::string& file_path, const int lossy_quality/* = DefaultLossyQuality*/) const
{
    ASSERT(( lossy_quality >= 0 && lossy_quality <= 100 ) ||
           ( lossy_quality == std::numeric_limits<int>::max() ));

    try
    {
        FileIO::CreateDirectoriesForFile(file_path);
    }

    catch( const FileIO::Exception& exception )
    {
        throw ImageException(exception.what());
    }

    const std::optional<ImageType> image_type = MimeType::GetSupportedImageTypeFromFileExtension(PortableFunctions::PathGetFileExtension(file_path));
    int result;

    if( image_type == ImageType::Jpeg )
    {
        const cs::non_null_shared_or_raw_ptr<const Image> rgb_image = GetRgbImage();
        const int jpeg_quality = std::min(lossy_quality, 100);

        result = stbi_write_jpg(file_path.c_str(), rgb_image->m_details.width, rgb_image->m_details.height,
                                rgb_image->m_details.channels, rgb_image->GetData(), jpeg_quality);
    }

    else if( image_type == ImageType::Png )
    {
        const int stride = m_details.width * m_details.channels;

        result = stbi_write_png(file_path.c_str(), m_details.width, m_details.height,
                                m_details.channels, GetData(), stride);
    }

    else if( image_type == ImageType::Bitmap )
    {
        result = stbi_write_bmp(file_path.c_str(), m_details.width, m_details.height,
                                m_details.channels, GetData());
    }

    else if( image_type == ImageType::WebP )
    {
        result = 0;

        try
        {
            ToWebP(lossy_quality,
                [&](const std::byte* const webp_data, const size_t webp_data_length)
                {
                    FileIO::Write(file_path, webp_data, webp_data_length);
                    result = 1;
                });
        }
        catch(...) { ASSERT(result == 0); }
    }

    else if( Icon::IsExtensionIcon(file_path) )
    {
        result = ToIconFile(file_path);
    }

    else
    {
        throw ImageException("Unknown image file extension: " + PortableFunctions::PathGetFileExtension(file_path));
    }

    if( result == 0 )
        throw ImageException("Could not write to the image file: " + file_path);
}


std::unique_ptr<std::vector<std::byte>> Multimedia::Image::ToBuffer(const ImageType image_type, const int lossy_quality/* = DefaultLossyQuality*/) const
{
    ASSERT(( lossy_quality >= 0 && lossy_quality <= 100 ) ||
           ( lossy_quality == std::numeric_limits<int>::max() ));

    std::unique_ptr<std::vector<std::byte>> image_buffer;

    // WebP
    if( image_type == ImageType::WebP )
    {
        ToWebP(lossy_quality,
            [&](const std::byte* const webp_data, const size_t webp_data_length)
            {
                image_buffer = std::make_unique<std::vector<std::byte>>(webp_data, webp_data + webp_data_length);
            });

        return image_buffer;
    }

    // stb_image-supported images

    // to avoid a lot of buffer reallocations, reserve 64k in space
    image_buffer = std::make_unique<std::vector<std::byte>>();
    image_buffer->reserve(64 * 1024);

    int result = 0;

    if( image_type == ImageType::Jpeg )
    {
        const cs::non_null_shared_or_raw_ptr<const Image> rgb_image = GetRgbImage();
        const int jpeg_quality = std::min(lossy_quality, 100);

        result = stbi_write_jpg_to_func(ToBufferFromStbImageCallback, image_buffer.get(), rgb_image->m_details.width, rgb_image->m_details.height,
                                        rgb_image->m_details.channels, rgb_image->GetData(), jpeg_quality);
    }

    else if( image_type == ImageType::Png )
    {
        const int stride = m_details.width * m_details.channels;

        result = stbi_write_png_to_func(ToBufferFromStbImageCallback, image_buffer.get(), m_details.width, m_details.height,
                                        m_details.channels, GetData(), stride);
    }

    else if( image_type == ImageType::Bitmap )
    {
        result = stbi_write_bmp_to_func(ToBufferFromStbImageCallback, image_buffer.get(), m_details.width, m_details.height,
                                        m_details.channels, GetData());
    }

    else
    {
        ASSERT(false);
    }

    if( result == 0 )
        image_buffer.reset();

    return image_buffer;
}


template<typename CF>
void Multimedia::Image::ToWebP(const int lossy_quality, const CF& callback_function) const
{
    const int stride = m_details.width * m_details.channels;
    const bool has_alpha = ( m_details.channels == 4 );
    const bool lossless = ( lossy_quality == std::numeric_limits<int>::max() );
    const uint8_t* const image_data = reinterpret_cast<const uint8_t*>(GetData());

    ASSERT(lossless || ( lossy_quality >= 0 && lossy_quality <= 100 ));

    uint8_t* webp_data;
    const size_t webp_data_length =
        ( lossless && has_alpha ) ? WebPEncodeLosslessRGBA(image_data, m_details.width, m_details.height, stride, &webp_data) :
        ( lossless              ) ? WebPEncodeLosslessRGB(image_data, m_details.width, m_details.height, stride, &webp_data) :
        ( has_alpha )             ? WebPEncodeRGBA(image_data, m_details.width, m_details.height, stride, static_cast<float>(lossy_quality), &webp_data) :
                                    WebPEncodeRGB(image_data, m_details.width, m_details.height, stride, static_cast<float>(lossy_quality), &webp_data);

    if( webp_data_length != 0 )
    {
        callback_function(reinterpret_cast<const std::byte*>(webp_data), webp_data_length);
        WebPFree(webp_data);
    }
}


void Multimedia::Image::ToBufferFromStbImageCallback(void* const context, void* const data, const int size)
{
    std::vector<std::byte>* image_buffer = static_cast<std::vector<std::byte>*>(context);

    const size_t current_size = image_buffer->size();
    image_buffer->resize(current_size + size);

    memcpy(image_buffer->data() + current_size, data, size);
}


std::unique_ptr<Multimedia::Image> Multimedia::Image::GetResizedImage(const int new_width, const int new_height) const
{
    auto resized_image = std::make_unique<BinaryBlockImage>(ImageDetails { new_width, new_height, m_details.channels, m_details.image_type });

    unsigned char* const output_pixels = reinterpret_cast<unsigned char*>(resized_image->GetDataBuffer());

    if( stbir_resize_uint8_linear(reinterpret_cast<const unsigned char*>(GetData()), m_details.width, m_details.height, m_details.channels * m_details.width,
                                  output_pixels, new_width, new_height, m_details.channels * new_width,
                                  static_cast<stbir_pixel_layout>(m_details.channels)) != output_pixels )
    {
        throw ImageException("Could not resize the image file");
    }

    return resized_image;
}


std::unique_ptr<Multimedia::Image> Multimedia::Image::GetResizedImage(const double scale_percent) const
{
    return GetResizedImage(static_cast<int>(m_details.width * scale_percent),
                           static_cast<int>(m_details.height * scale_percent));
}


int Multimedia::Image::ToIconFile(const std::string& file_path) const
{
    cs::non_null_shared_or_raw_ptr<const Image> image_to_convert(this);

    // resize the image if needed
    const int max_dimension = std::max(m_details.width, m_details.height);

    if( max_dimension > Icon::MaxSize )
    {
        const double scale_percent = CreateProportion(Icon::MaxSize, max_dimension);
        image_to_convert = GetResizedImage(std::min(Icon::MaxSize, static_cast<int>(m_details.width * scale_percent)),
                                           std::min(Icon::MaxSize, static_cast<int>(m_details.height * scale_percent)));
    }

    ASSERT(image_to_convert->m_details.width <= Icon::MaxSize &&
           image_to_convert->m_details.height <= Icon::MaxSize);

    const std::unique_ptr<std::vector<std::byte>> png_data = image_to_convert->ToBuffer(ImageType::Png);

    if( png_data == nullptr )
        return 0;

    Icon::SavePngAsIcon(file_path, *png_data, image_to_convert->m_details.width, image_to_convert->m_details.height);

    return 1;
}


cs::non_null_shared_or_raw_ptr<const Multimedia::Image> Multimedia::Image::GetRgbImage() const
{
    constexpr int RgbChannels = 3;
    constexpr int RgbAChannels = 4;
    constexpr std::byte BackgroundColor = std::byte(255);

    if( m_details.channels != RgbAChannels )
        return this;

    const std::byte* from_buffer_itr = GetData();
    const std::byte* const from_buffer_end = from_buffer_itr + ( m_details.width * m_details.height * RgbAChannels );

    // create a new image, removing the alpha channel
    ImageDetails rgb_image_details = m_details;
    rgb_image_details.channels = RgbChannels;

    auto rgb_image = std::make_unique<BinaryBlockImage>(std::move(rgb_image_details));
    std::byte* to_buffer = rgb_image->GetDataBuffer();

    while( from_buffer_itr != from_buffer_end )
    {
        const int alpha = static_cast<int>(from_buffer_itr[3]);

        // if fully transparent, set to white
        if( alpha == 0 )
        {
            to_buffer[0] = BackgroundColor;
            to_buffer[1] = BackgroundColor;
            to_buffer[2] = BackgroundColor;
        }

        else
        {
            to_buffer[0] = from_buffer_itr[0];
            to_buffer[1] = from_buffer_itr[1];
            to_buffer[2] = from_buffer_itr[2];

            // if fully opaque, we do not need to do anything,
            // but otherwise we must apply an alpha blend
            if( alpha != 255 )
            {
                for( int i = 0; i < RgbChannels; ++i )
                {
                    const int combined_value = ( static_cast<unsigned char>(to_buffer[i]) * alpha ) +
                                               ( static_cast<unsigned char>(BackgroundColor) * ( 255 - alpha ) );
                    to_buffer[i] = static_cast<std::byte>(combined_value / 255);
                }
            }
        }

        from_buffer_itr += RgbAChannels;
        to_buffer += RgbChannels;
    }

    ASSERT(to_buffer == ( rgb_image->GetData() + ( rgb_image->m_details.width * rgb_image->m_details.height * RgbChannels ) ));

    return rgb_image;
}
