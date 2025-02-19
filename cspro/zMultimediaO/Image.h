#pragma once

#include <zMultimediaO/zMultimediaO.h>
#include <zToolsO/span.h>
#include <zUtilO/DataTypes.h>


namespace Multimedia
{
    class Image;
    struct ImageDetails;

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
    // Supported image types are: JPEG, PNG, and BMP.
    static std::unique_ptr<Image> FromFile(cs::string_sz file_path);

    // Loads the image from a buffer, throwing exceptions on error.
    // Supported image types are: JPEG, PNG, and BMP.
    static std::unique_ptr<Image> FromBuffer(cs::span<const std::byte> content);

    // Returns std::nullopt if the details cannot be read.
    static std::optional<ImageDetails> GetDetailsFromBuffer(cs::span<const std::byte> content);

    // Saves the image to a file, throwing exceptions on error.
    // Supported image types are: JPEG, PNG, BMP, and ICO.
    void ToFile(const std::string& file_path, std::optional<int> jpeg_quality = std::nullopt) const;

    std::unique_ptr<std::vector<std::byte>> ToBuffer(ImageType image_type, std::optional<int> jpeg_quality = std::nullopt) const;

    std::unique_ptr<Image> GetResizedImage(int new_width, int new_height) const;
    std::unique_ptr<Image> GetResizedImage(double scale_percent) const;

protected:
    const ImageDetails m_details;

private:
    // The method returns an integer so as to match the signature of the stbi_write_... functions.
    int ToIconFile(const std::string& file_path) const;
};
