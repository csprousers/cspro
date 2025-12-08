#pragma once

#include <gdiplus.h>


namespace IconToPngConverter
{
    struct IconAlphaBitmap
    {
        std::unique_ptr<Gdiplus::Bitmap> bitmap;
        std::unique_ptr<int32_t[]> color_bits;
    };

    IconAlphaBitmap ConvertIconToAlphaBitmap(const ICONINFO& icon_info);

    std::unique_ptr<std::vector<std::byte>> CreatePngFromBitmap(Gdiplus::Bitmap& bitmap);

    // this function will also delete the icon resources
    std::unique_ptr<std::vector<std::byte>> GetPngFromIcon(HICON hIcon);
}



inline IconToPngConverter::IconAlphaBitmap IconToPngConverter::ConvertIconToAlphaBitmap(const ICONINFO& icon_info)
{
    // modified from https://stackoverflow.com/questions/1818990/save-hicon-as-a-png

    // Get the screen DC
    HDC dc = GetDC(NULL);

    // Get icon size info
    BITMAP bm = {0};
    GetObject( icon_info.hbmColor, sizeof( BITMAP ), &bm );

    // Set up BITMAPINFO
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bm.bmWidth;
    bmi.bmiHeader.biHeight = -bm.bmHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    // Extract the color bitmap
    int nBits = bm.bmWidth * bm.bmHeight;
    auto colorBits = std::make_unique<int32_t[]>(nBits);
    GetDIBits(dc, icon_info.hbmColor, 0, bm.bmHeight, colorBits.get(), &bmi, DIB_RGB_COLORS);

    // Check whether the color bitmap has an alpha channel.
    bool hasAlpha = false;
    for (int i = 0; i < nBits; i++) {
        if ((colorBits[i] & 0xff000000) != 0) {
            hasAlpha = true;
            break;
        }
    }

    // If no alpha values available, apply the mask bitmap
    if (!hasAlpha) {
        // Extract the mask bitmap
        auto maskBits = std::make_unique<int32_t[]>(nBits);
        GetDIBits(dc, icon_info.hbmMask, 0, bm.bmHeight, maskBits.get(), &bmi, DIB_RGB_COLORS);
        // Copy the mask alphas into the color bits
        for (int i = 0; i < nBits; i++) {
            if (maskBits[i] == 0) {
                colorBits[i] |= 0xff000000;
            }
        }
    }

    // Release DC and GDI bitmaps
    ReleaseDC(NULL, dc);

    // Create GDI+ Bitmap
    return IconAlphaBitmap
    {
        std::make_unique<Gdiplus::Bitmap>(bm.bmWidth, bm.bmHeight, bm.bmWidth*4, PixelFormat32bppARGB, (BYTE*)colorBits.get()),
        std::move(colorBits)
    };
}


std::unique_ptr<std::vector<std::byte>> IconToPngConverter::CreatePngFromBitmap(Gdiplus::Bitmap& bitmap)
{
    // save as a PNG file to memory
    IStream* memory_stream;

    if( !SUCCEEDED(CreateStreamOnHGlobal(nullptr, TRUE, &memory_stream)) )
        return nullptr;

    CLSID pngClsid;
    CLSIDFromString(_T("{557cf406-1a04-11d3-9a73-0000f81ef32e})"), &pngClsid);
    bitmap.Save(memory_stream, &pngClsid);

    ULARGE_INTEGER stream_size;
    IStream_Size(memory_stream, &stream_size);

    // the file size is the LowPart of the stream size
    auto png_data = std::make_unique<std::vector<std::byte>>(stream_size.LowPart);

    // reset the stream and write it to the vector
    IStream_Reset(memory_stream);
    IStream_Read(memory_stream, png_data->data(), png_data->size());

    memory_stream->Release();

    return png_data;
}


std::unique_ptr<std::vector<std::byte>> IconToPngConverter::GetPngFromIcon(HICON hIcon)
{
    std::unique_ptr<std::vector<std::byte>> png_data;

    if( hIcon != nullptr )
    {
        ICONINFO icon_info;

        if( GetIconInfo(hIcon, &icon_info) )
        {
            ASSERT(icon_info.hbmMask != nullptr && icon_info.hbmColor != nullptr);

            // startup GDI+ to do the conversions
            Gdiplus::GdiplusStartupInput gdiplus_startup_input;
            ULONG_PTR gdiplus_token;
            Gdiplus::GdiplusStartup(&gdiplus_token, &gdiplus_startup_input, nullptr);

            IconAlphaBitmap icon_alpha_bitmap = ConvertIconToAlphaBitmap(icon_info);

            if( icon_alpha_bitmap.bitmap != nullptr )
                png_data = CreatePngFromBitmap(*icon_alpha_bitmap.bitmap);

            // shutdown GDI+
            icon_alpha_bitmap.bitmap.reset();
            Gdiplus::GdiplusShutdown(gdiplus_token);

            DeleteObject(icon_info.hbmMask);
            DeleteObject(icon_info.hbmColor);
        }

        DestroyIcon(hIcon);
    }

    return png_data;
}
