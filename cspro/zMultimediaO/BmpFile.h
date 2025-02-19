#pragma once

#include <zMultimediaO/zMultimediaO.h>
#include <zToolsO/span.h>

namespace Multimedia { class BmpFile; class Image; }


// a class that can be used to create a 24-bit uncompressed BMP image

class ZMULTIMEDIAO_API Multimedia::BmpFile
{
public:
    // the buffer must contain RGB bytes for each pixel, with the pixel at bytes RGB(content[0], content[1], content[2])
    // corresponding to the image's bottom-left pixel
    BmpFile(cs::span<const std::byte> content, int width, int height);

#ifdef WIN_DESKTOP
    // the bitmap handle must be valid
    BmpFile(HBITMAP hBitmap);
#endif

    int GetWidth() const  { return m_width; }
    int GetHeight() const { return m_height; }
    int GetStride() const { return m_bytesPerRow; }

    // returns a pointer to the raw 24-bit pixel data
    const std::byte* GetPixelData() const;

    // saves the bitmap, throwing an exception on error
    void Save(const std::string& file_path);

    // returns the bitmap as an image
    std::unique_ptr<Image> CreateImage();

private:
    // called by the constructors ... the m_width and m_height values must be set
    void ParseContent(cs::span<const std::byte> content, int content_bytes_per_pixel, std::tuple<int, int, int> rgb_index);

private:
    std::unique_ptr<std::byte[]> m_bmpData;
    uint32_t m_bmpDataLength;

    int m_width;
    int m_height;
    int m_bytesPerRow;
};
