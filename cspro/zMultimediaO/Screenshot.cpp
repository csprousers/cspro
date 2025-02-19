#include "stdafx.h"
#include "Screenshot.h"


Multimedia::BmpFile GetClipboardAsScreenshot()
{
    if( !IsClipboardFormatAvailable(CF_BITMAP) )
        throw CSProException("There is no image on the clipboard.");

    HBITMAP hBitmap = OpenClipboard(nullptr) ? static_cast<HBITMAP>(GetClipboardData(CF_BITMAP)) :
                                               nullptr;
    if( hBitmap == nullptr )
        throw CSProException("Could not open the clipboard.");

    Multimedia::BmpFile bmp_file(hBitmap);

    CloseClipboard();

    return bmp_file;
}
