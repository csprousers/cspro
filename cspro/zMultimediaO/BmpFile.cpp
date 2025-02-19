#include "stdafx.h"
#include "BmpFile.h"
#include "Image.h"


namespace
{
    constexpr int BytesPerPixel24Bit            = 3;
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


Multimedia::BmpFile::BmpFile(const cs::span<const std::byte> content, const int width, const int height)
    :   m_width(width),
        m_height(height)
{
    ParseContent(content, BytesPerPixel24Bit, { 0, 1, 2 });
}


#ifdef WIN_DESKTOP

Multimedia::BmpFile::BmpFile(HBITMAP hBitmap)
{
    ASSERT(hBitmap != nullptr);

    // get the pixel data for the bitmap, from https://stackoverflow.com/questions/22050413/c-get-raw-pixel-data-from-hbitmap

    HDC hDC = CreateCompatibleDC(nullptr);
    HBITMAP hOldBitmap = static_cast<HBITMAP>(SelectObject(hDC, hBitmap));

    BITMAP bmp;
    GetObject(hBitmap, sizeof(bmp), &bmp);

    BITMAPINFO bmp_info = { 0 };
    bmp_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmp_info.bmiHeader.biWidth = bmp.bmWidth;
    bmp_info.bmiHeader.biHeight = bmp.bmHeight;
    bmp_info.bmiHeader.biPlanes = 1;
    bmp_info.bmiHeader.biBitCount = bmp.bmBitsPixel;
    bmp_info.bmiHeader.biCompression = BI_RGB;
    bmp_info.bmiHeader.biSizeImage = ( ( bmp.bmWidth * bmp.bmBitsPixel + 31 ) / 32 ) * 4 * bmp.bmHeight;

    auto pixels = std::make_unique_for_overwrite<std::byte[]>(bmp_info.bmiHeader.biSizeImage);

    GetDIBits(hDC, hBitmap, 0, bmp.bmHeight, pixels.get(), &bmp_info, DIB_RGB_COLORS);
    SelectObject(hDC, hOldBitmap);

    DeleteDC(hDC);

    m_width = bmp_info.bmiHeader.biWidth;
    m_height = std::abs(bmp_info.bmiHeader.biHeight);

    const int content_bytes_per_pixel = bmp_info.bmiHeader.biBitCount / 8;
    ASSERT(content_bytes_per_pixel == 4);

    ParseContent(cs::span<const std::byte>(pixels.get(), bmp_info.bmiHeader.biSizeImage), content_bytes_per_pixel, { 2, 1, 0 });
}

#endif // WIN_DESKTOP


void Multimedia::BmpFile::ParseContent(const cs::span<const std::byte> content, const int content_bytes_per_pixel, const std::tuple<int, int, int> rgb_index)
{
    ASSERT(content_bytes_per_pixel >= BytesPerPixel24Bit);
    ASSERT(content.size() == static_cast<size_t>(m_width * m_height * content_bytes_per_pixel));

    m_bytesPerRow = m_width * BytesPerPixel24Bit;
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
    bitmap_core_header->bits_per_pixel = BytesPerPixel24Bit * 8;

    // copy the pixels (which in BMP format are stored as BGR, not RGB)
    const std::byte* content_itr = content.data();

    for( int h = m_height; h > 0; --h )
    {
        for( int w = m_width; w > 0; --w )
        {
            *(bmp_file_itr++) = content_itr[std::get<2>(rgb_index)];
            *(bmp_file_itr++) = content_itr[std::get<1>(rgb_index)];
            *(bmp_file_itr++) = content_itr[std::get<0>(rgb_index)];

            content_itr += content_bytes_per_pixel;
        }

        // pad each row
        for( int padding = padding_bytes_per_row; padding > 0; --padding )
            *(bmp_file_itr++) = std::byte(0);
    }
}


const std::byte* Multimedia::BmpFile::GetPixelData() const
{
    const BitmapFileHeader* const bitmap_file_header = reinterpret_cast<const BitmapFileHeader*>(m_bmpData.get());

    return m_bmpData.get() + bitmap_file_header->pixel_array_offset;
}


void Multimedia::BmpFile::Save(const std::string& file_path)
{
    FileIO::Write(file_path, cs::span<const std::byte>(m_bmpData.get(), m_bmpDataLength));
}


std::unique_ptr<Multimedia::Image> Multimedia::BmpFile::CreateImage()
{
    return Image::FromBuffer(cs::span<const std::byte>(m_bmpData.get(), m_bmpDataLength));
}
