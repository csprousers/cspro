#pragma once

#include <zMultimediaO/zMultimediaO.h>
#include <zToolsO/span.h>
#include <zUtilO/DataTypes.h>


namespace Multimedia
{
    CREATE_CSPRO_EXCEPTION(ImageException)


    struct ImageDetails
    {
        int width;
        int height;
        int channels;
        std::optional<ImageType> image_type;
    };


    class ZMULTIMEDIAO_API Image
    {
    protected:
        Image(ImageDetails details);

    public:
        virtual ~Image() { }

        const ImageDetails& GetDetails() const { return m_details; }

        virtual const std::byte* GetData() const = 0;

        static std::unique_ptr<Image> FromImage(const Image& image);

        static std::unique_ptr<Image> FromFile(const std::wstring& filename);

        static std::unique_ptr<Image> FromBuffer(cs::span<const std::byte> content);

        // returns std::nullopt if the details cannot be read
        static std::optional<ImageDetails> GetDetailsFromBuffer(cs::span<const std::byte> content);

        void ToFile(const std::wstring& filename, std::optional<int> jpeg_quality = std::nullopt) const;

        std::unique_ptr<std::vector<std::byte>> ToBuffer(ImageType image_type, std::optional<int> jpeg_quality = std::nullopt) const;

        std::unique_ptr<Image> GetResizedImage(int new_width, int new_height) const;
        std::unique_ptr<Image> GetResizedImage(double scale_percent) const;

    protected:
        const ImageDetails m_details;
    };
}
