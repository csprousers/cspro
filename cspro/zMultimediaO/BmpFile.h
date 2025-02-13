#pragma once

#include <zMultimediaO/zMultimediaO.h>

namespace Multimedia { class Image; }


namespace Multimedia
{
    // a class that can be used to create a 24-bit uncompressed BMP image

    class ZMULTIMEDIAO_API BmpFile
    {
    public:
        // the buffer must contain RGB bytes for each pixel, with the pixel at bytes RGB(content[0], content[1], content[2])
        // corresponding to the image's bottom-left pixel
        BmpFile(const std::byte* content, size_t content_size, int width, int height);

        int GetWidth() const  { return m_width; }
        int GetHeight() const { return m_height; }
        int GetStride() const { return m_bytesPerRow; }

        const std::byte* GetPixelData() const;

        std::unique_ptr<Multimedia::Image> CreateImage();

    private:
        std::unique_ptr<std::byte[]> m_bmpData;
        size_t m_bmpDataLength;

        int m_width;
        int m_height;
        int m_bytesPerRow;
    };
}
