#include "stdafx.h"
#include "BmpFile.h"
#include "Image.h"


namespace
{
    constexpr int BytesPerPixel                 = 3;
    constexpr int PixelPaddingPerRowRowMultiple = 4;


    // header formats taken from https://en.wikipedia.org/wiki/BMP_file_format
#ifdef _MSC_VER
    __pragma(pack(push, 1))
#endif
    struct BitmapFileHeader
    {
        uint8_t header_id1;
        uint8_t header_id2;
        uint32_t file_size;
        uint16_t unused1;
        uint16_t unused2;
        uint32_t pixel_array_offset;
    }
#ifdef __GNUC__
    __attribute__((__packed__))
#endif
    ;

#ifdef _MSC_VER
    __pragma(pack(push, 1))
#endif
    struct BitmapCoreHeader
    {
        uint32_t header_size;
        uint16_t width;
        uint16_t height;
        uint16_t color_planes;
        uint16_t bits_per_pixel;
    }
#ifdef __GNUC__
    __attribute__((__packed__))
#endif
    ;

    static_assert(sizeof(BitmapFileHeader) == 14 && sizeof(BitmapCoreHeader) == 12);
}


Multimedia::BmpFile::BmpFile(const std::byte* content, size_t content_size, int width, int height)
    :   m_width(width),
        m_height(height)
{
    ASSERT(content_size == static_cast<size_t>(m_width * m_height * BytesPerPixel));

    m_bytesPerRow = m_width * BytesPerPixel;
    int padding_bytes_per_row = m_bytesPerRow % PixelPaddingPerRowRowMultiple;

    if( padding_bytes_per_row != 0 )
    {
        padding_bytes_per_row = PixelPaddingPerRowRowMultiple - padding_bytes_per_row;
        m_bytesPerRow += padding_bytes_per_row;
    }

    m_bmpDataLength = sizeof(BitmapFileHeader) + sizeof(BitmapCoreHeader) + ( m_bytesPerRow * m_height );

    m_bmpData = std::make_unique_for_overwrite<std::byte[]>(m_bmpDataLength);

    std::byte* bmp_file_itr = m_bmpData.get();

    // fill the file header
    BitmapFileHeader* bitmap_file_header = reinterpret_cast<BitmapFileHeader*>(bmp_file_itr);
    bmp_file_itr += sizeof(BitmapFileHeader);

    bitmap_file_header->header_id1 = 'B';
    bitmap_file_header->header_id2 = 'M';
    bitmap_file_header->file_size = m_bmpDataLength;
    bitmap_file_header->unused1 = 0;
    bitmap_file_header->unused2 = 0;
    bitmap_file_header->pixel_array_offset = sizeof(BitmapFileHeader) + sizeof(BitmapCoreHeader);

    // fill the bitmap information header
    BitmapCoreHeader* bitmap_core_header = reinterpret_cast<BitmapCoreHeader*>(bmp_file_itr);
    bmp_file_itr += sizeof(BitmapCoreHeader);

    bitmap_core_header->header_size = sizeof(BitmapCoreHeader);
    bitmap_core_header->width = static_cast<uint16_t>(m_width);
    bitmap_core_header->height = static_cast<uint16_t>(m_height);
    bitmap_core_header->color_planes = 1;
    bitmap_core_header->bits_per_pixel = BytesPerPixel * 8;

    // copy the pixels (which in BMP format are stored as BGR, not RGB)
    for( int h = m_height; h > 0; --h )
    {
        for( int w = m_width; w > 0; --w )
        {
            *(bmp_file_itr++) = static_cast<std::byte>(content[2]);
            *(bmp_file_itr++) = static_cast<std::byte>(content[1]);
            *(bmp_file_itr++) = static_cast<std::byte>(content[0]);

            content += 3;
        }

        // pad each row
        for( int padding = padding_bytes_per_row; padding > 0; --padding )
            *(bmp_file_itr++) = std::byte(0);
    }
}


const std::byte* Multimedia::BmpFile::GetPixelData() const
{
    const BitmapFileHeader* bitmap_file_header = reinterpret_cast<const BitmapFileHeader*>(m_bmpData.get());

    return m_bmpData.get() + bitmap_file_header->pixel_array_offset;
}


std::unique_ptr<Multimedia::Image> Multimedia::BmpFile::CreateImage()
{
    return Image::FromBuffer(cs::span<const std::byte>(m_bmpData.get(), m_bmpDataLength));
}
