#include "stdafx.h"
#include "Icon.h"
#include "Image.h"
#include <zToolsO/File.h>

#ifdef WIN_DESKTOP
#include <zUtilF/IconToPngConverter.h>
#include <atlimage.h>
#endif


std::unique_ptr<std::vector<std::byte>> Multimedia::Icon::LoadIconAsPng(const std::string& file_path)
{
#ifdef WIN_DESKTOP
    HICON hIcon = static_cast<HICON>(LoadImage(nullptr, TC::ToWide(file_path).c_str(),
                                               IMAGE_ICON, 0, 0,
                                               LR_LOADFROMFILE));

    if( hIcon != nullptr )
        return IconToPngConverter::GetPngFromIcon(hIcon);
#endif

    throw ImageException("Could not read the icon file: %s", file_path.c_str());
}


void Multimedia::Icon::SavePngAsIcon(const std::string& file_path, const std::vector<std::byte>& png_data, const int width, const int height)
{
    // implementation based on the description here: https://en.wikipedia.org/wiki/ICO_(file_format)
    static_assert(sizeof(char) == 1);
    static_assert(sizeof(short) == 2);
    static_assert(sizeof(int) == 4);

    auto get_icon_dimension = [](const int dimension) -> char
    {
        return ( dimension > 0 && dimension < MaxSize ) ? static_cast<char>(dimension) :
               ( dimension == MaxSize )                 ? 0 :
                                                          throw ImageException("Invalid icon dimension: %d", dimension);
    };

    const char icon_width = get_icon_dimension(width);
    const char icon_height = get_icon_dimension(height);
    const int icon_bytes = static_cast<int>(png_data.size());

    FileIO::File file;
    file.SetDeleteFileOnError(true)
        .OpenForWritingCreate(file_path);

    // write ICONDIR
    file.WriteBinary<short>(0)          // Reserved. Must always be 0.
        .WriteBinary<short>(1)          // Specifies image type: 1 for icon (.ICO) image, 2 for cursor (.CUR) image. Other values are invalid.
        .WriteBinary<short>(1);         // Specifies number of images in the file.

    // write ICONDIRENTRY
    file.WriteBinary<char>(icon_width)  // Specifies image width in pixels. Can be any number between 0 and 255. Value 0 means image width is 256 pixels.
        .WriteBinary<char>(icon_height) // Specifies image height in pixels. Can be any number between 0 and 255. Value 0 means image height is 256 pixels.
        .WriteBinary<char>(0)           // Specifies number of colors in the color palette. Should be 0 if the image does not use a color palette.
        .WriteBinary<char>(0)           // Reserved. Should be 0.
        .WriteBinary<short>(1)          // In ICO format: Specifies color planes. Should be 0 or 1
        .WriteBinary<short>(24)         // In ICO format: Specifies bits per pixel.
        .WriteBinary<int>(icon_bytes)   // Specifies the size of the image's data in bytes.
        .WriteBinary<int>(22);          // Specifies the offset of BMP or PNG data from the beginning of the ICO/CUR file.

    // write the PNG data
    file.Write(png_data.data(), png_data.size());

    file.Close();
}
