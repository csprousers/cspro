#pragma once

#include <zMultimediaO/zMultimediaO.h>
#include <zToolsO/span.h>
#include <zUtilO/DataTypes.h>


namespace Multimedia
{
    class Image;
    struct ImageDetails;

    constexpr int DefaultLossyQuality = 95;

    CREATE_CSPRO_EXCEPTION(ImageException);
}


struct Multimedia::ImageDetails
{
    int width;
    int height;
    int channels;
    std::optional<ImageType> image_type;
};


class ZMULTIMEDIAO_API Multimedia::Image
{
protected:
    Image(ImageDetails details);

public:
    virtual ~Image() { }

    const ImageDetails& GetDetails() const { return m_details; }

    virtual const std::byte* GetData() const = 0;

    static std::unique_ptr<Image> FromImage(const Image& image);

    // Loads the image from a file, throwing exceptions on error.
    // Supported image types are: JPEG, PNG, BMP, and WebP.
    static std::unique_ptr<Image> FromFile(const std::string& file_path);

    // Loads the image from a buffer, throwing exceptions on error.
    // Supported image types are: JPEG, PNG, BMP, and WebP.
    static std::unique_ptr<Image> FromBuffer(cs::span<const std::byte> content);

    // Returns std::nullopt if the details cannot be read.
    static std::optional<ImageDetails> GetDetailsFromBuffer(cs::span<const std::byte> content);

    // Saves the image to a file, throwing exceptions on error.
    // Supported image types are: JPEG, PNG, BMP, ICO, and WebP.
    // For WebP, use std::numeric_limits<int>::max() for lossless.
    void ToFile(const std::string& file_path, int lossy_quality = DefaultLossyQuality) const;

    // Saves the image to the buffer, returning null on error.
    // Supported image types are: JPEG, PNG, BMP, and WebP.
    // For WebP, use std::numeric_limits<int>::max() for lossless.
    std::unique_ptr<std::vector<std::byte>> ToBuffer(ImageType image_type, int lossy_quality = DefaultLossyQuality) const;

    std::unique_ptr<Image> GetResizedImage(int new_width, int new_height) const;
    std::unique_ptr<Image> GetResizedImage(double scale_percent) const;

protected:
    const ImageDetails m_details;

private:
    static bool GetWebPDetailsFromBuffer(cs::span<const std::byte> content, ImageDetails& details);
    static std::unique_ptr<Image> GetWebPImageFromBuffer(cs::span<const std::byte> content);

    template<typename CF>
    void ToWebP(int lossy_quality, const CF& callback_function) const;

    static void ToBufferFromStbImageCallback(void* context, void* data, int size);

    // The method returns an integer so as to match the signature of the stbi_write_... functions.
    int ToIconFile(const std::string& file_path) const;

    // If the image has an alpha channel, it is blended against a white background.
    cs::non_null_shared_or_raw_ptr<const Image> GetRgbImage() const;
};
