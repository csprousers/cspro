#include "stdafx.h"
#include "Image.h"
#include "Icon.h"
#include <zToolsO/Tools.h>
#include <zUtilO/MimeType.h>
#include <external/zlib/zlib.h>


namespace
{
    constexpr int DefaultJpegQuality = 95;
}


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


Multimedia::Image::Image(ImageDetails details)
    :   m_details(std::move(details))
{
}


namespace
{
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

    private:
        stbi_uc* m_imageData;
    };


    class VectorImage : public Multimedia::Image
    {
    public:
        VectorImage(Multimedia::ImageDetails details)
            :   Image(std::move(details)),
                m_image(m_details.width * m_details.height * m_details.channels)
        {
        }

        VectorImage(const Image& image)
            :   Image(image.GetDetails()),
                m_image(image.GetData(), image.GetData() + ( m_details.width * m_details.height * m_details.channels ))
        {
        }

        const std::byte* GetData() const override
        {
            return m_image.data();
        }

        std::byte* GetDataBuffer()
        {
            return m_image.data();
        }

    private:
        std::vector<std::byte> m_image;
    };


    std::optional<ImageType> ToImageType(const LoadedImageType loaded_image_type)
    {
        if( loaded_image_type == LoadedImageType::Gif )
            return std::nullopt;

        static_assert(static_cast<ImageType>(LoadedImageType::Jpeg) == ImageType::Jpeg &&
                      static_cast<ImageType>(LoadedImageType::Png) == ImageType::Png &&
                      static_cast<ImageType>(LoadedImageType::Bitmap) == ImageType::Bitmap);

        return static_cast<ImageType>(loaded_image_type);
    }
}


std::unique_ptr<Multimedia::Image> Multimedia::Image::FromImage(const Multimedia::Image& image)
{
    return std::make_unique<VectorImage>(image);
}


std::unique_ptr<Multimedia::Image> Multimedia::Image::FromFile(const cs::string_sz file_path)
{
    ImageDetails details;

    stbi_uc* const image_data = stbi_load(file_path.c_str(),
                                          &details.width, &details.height, &details.channels, 0);

    if( image_data == nullptr )
        throw ImageException("Could not read the image file: %s", file_path.c_str());

    return std::make_unique<StbImage>(image_data, std::move(details));
}


std::unique_ptr<Multimedia::Image> Multimedia::Image::FromBuffer(const cs::span<const std::byte> content)
{
    ImageDetails details;
    LoadedImageType loaded_image_type;

    stbi_uc* const image_data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(content.data()), int32_cast(content.size()),
                                                      &details.width, &details.height, &details.channels, 0, &loaded_image_type);

    if( image_data == nullptr )
        throw ImageException("Could not parse the image");

    details.image_type = ToImageType(loaded_image_type);

    return std::make_unique<StbImage>(image_data, std::move(details));
}


std::optional<Multimedia::ImageDetails> Multimedia::Image::GetDetailsFromBuffer(const cs::span<const std::byte> content)
{
    ImageDetails details;
    LoadedImageType loaded_image_type;

    if( stbi_info_from_memory(reinterpret_cast<const stbi_uc*>(content.data()), int32_cast(content.size()),
                              &details.width, &details.height, &details.channels, &loaded_image_type) == 1 )
    {
        details.image_type = ToImageType(loaded_image_type);

        return details;
    }

    return std::nullopt;
}


void Multimedia::Image::ToFile(const std::string& file_path, const std::optional<int> jpeg_quality/* = std::nullopt*/) const
{
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
        result = stbi_write_jpg(file_path.c_str(), m_details.width, m_details.height, m_details.channels,
                                GetData(), jpeg_quality.value_or(DefaultJpegQuality));
    }

    else if( image_type == ImageType::Png )
    {
        result = stbi_write_png(file_path.c_str(), m_details.width, m_details.height, m_details.channels,
                                GetData(), m_details.width * m_details.channels);
    }

    else if( image_type == ImageType::Bitmap )
    {
        result = stbi_write_bmp(file_path.c_str(), m_details.width, m_details.height, m_details.channels,
                                GetData());
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


namespace
{
    void ToBufferCallback(void* const context, void* const data, const int size)
    {
        std::vector<std::byte>* image_buffer = static_cast<std::vector<std::byte>*>(context);

        const size_t current_size = image_buffer->size();
        image_buffer->resize(current_size + size);

        memcpy(image_buffer->data() + current_size, data, size);
    }
}


std::unique_ptr<std::vector<std::byte>> Multimedia::Image::ToBuffer(const ImageType image_type, const std::optional<int> jpeg_quality/* = std::nullopt*/) const
{
    auto image_buffer = std::make_unique<std::vector<std::byte>>();

    // to avoid a lot of buffer reallocations, reserve 64k in space
    image_buffer->reserve(64 * 1024);

    int result = 0;

    if( image_type == ImageType::Jpeg )
    {
        result = stbi_write_jpg_to_func(ToBufferCallback, image_buffer.get(), m_details.width, m_details.height, m_details.channels,
                                        GetData(), jpeg_quality.value_or(DefaultJpegQuality));
    }

    else if( image_type == ImageType::Png )
    {
        result = stbi_write_png_to_func(ToBufferCallback, image_buffer.get(), m_details.width, m_details.height, m_details.channels,
                                        GetData(), m_details.width * m_details.channels);
    }

    else if( image_type == ImageType::Bitmap )
    {
        result = stbi_write_bmp_to_func(ToBufferCallback, image_buffer.get(), m_details.width, m_details.height, m_details.channels,
                                        GetData());
    }

    else
    {
        ASSERT(false);
    }

    if( result == 0 )
        image_buffer.reset();

    return image_buffer;
}


std::unique_ptr<Multimedia::Image> Multimedia::Image::GetResizedImage(const int new_width, const int new_height) const
{
    auto resized_image = std::make_unique<VectorImage>(ImageDetails { new_width, new_height, m_details.channels, m_details.image_type });

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
