#include "StdAfx.h"
#include "SystemIcon.h"
#include <zDesignerF/UWM.h>
#include <atlimage.h>
#include <comdef.h>
#include <commoncontrols.h>


_COM_SMARTPTR_TYPEDEF(IImageList, __uuidof(IImageList));


namespace
{
    struct IconAlphaBitmap
    {
        std::unique_ptr<Gdiplus::Bitmap> bitmap;
        std::unique_ptr<int32_t[]> color_bits;
    };


    IconAlphaBitmap ConvertIconToAlphaBitmap(const ICONINFO& icon_info)
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


    std::unique_ptr<std::vector<std::byte>> CreatePngFromBitmap(Gdiplus::Bitmap& bitmap)
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


    // this function will also delete the icon resources
    std::unique_ptr<std::vector<std::byte>> GetPngFromIcon(HICON hIcon)
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


    // returns the icon from the system image list; the calling function must delete the icon resources
    HICON GetIconFromSystemImageList(int icon_index, int icon_size)
    {
        // get the system image list to retrieve the appropriately-sized version of this icon
        IImageListPtr spiml;

        if( SHGetImageList(icon_size, IID_PPV_ARGS(&spiml)) == S_OK )
        {
            HICON hIcon;

            if( spiml->GetIcon(icon_index, ILD_TRANSPARENT, &hIcon) == S_OK )
                return hIcon;
        }

        return nullptr;
    }


    // returns the system icon; the calling function must delete the icon resources
    HICON GetSystemIcon(wstring_view extension, int icon_size)
    {
        // construct a fake filename with the given extension to query for the icon
        std::wstring fake_filename = PortableFunctions::PathAppendFileExtension<std::wstring>(_T("a"), extension);

        // get the system icon index (from https://devblogs.microsoft.com/oldnewthing/20140120-00/?p=2043)
        SHFILEINFO shell_file_info;

        if( SHGetFileInfo(fake_filename.c_str(), 0, &shell_file_info, sizeof(shell_file_info), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX) != 0 )
            return GetIconFromSystemImageList(shell_file_info.iIcon, icon_size);

        return nullptr;
    }


    // returns the stock icon; the calling function must delete the icon resources
    HICON GetStockIcon(SHSTOCKICONID siid, int icon_size)
    {
        SHSTOCKICONINFO stock_icon_info;
        stock_icon_info.cbSize = sizeof(SHSTOCKICONINFO);

        if( SHGetStockIconInfo(siid, SHGSI_SYSICONINDEX, &stock_icon_info) == S_OK )
            return GetIconFromSystemImageList(stock_icon_info.iSysImageIndex, icon_size);

        return nullptr;
    }
}


std::shared_ptr<const std::vector<std::byte>> SystemIcon::GetPngForCSProLogo()
{
    static std::shared_ptr<const std::vector<std::byte>> logo_png;

    if( logo_png == nullptr )
        logo_png = GetPngFromIcon((HICON)AfxGetMainWnd()->SendMessage(UWM::Designer::GetDesignerIcon));

    return logo_png;
}


std::shared_ptr<const std::vector<std::byte>> SystemIcon::GetPngForExtension(wstring_view extension)
{
    static std::map<std::wstring, std::shared_ptr<const std::vector<std::byte>>> extension_png_map;

    std::wstring upper_case_extension = SO::ToUpper(extension);
    ASSERT(upper_case_extension.empty() || upper_case_extension.front() != '.');
    
    // return the PNG for the icon if it has already been loaded
    const auto& icon_png_lookup = extension_png_map.find(upper_case_extension);

    if( icon_png_lookup != extension_png_map.cend() )
        return icon_png_lookup->second;

    // otherwise lookup the icon and create a PNG for it
    HICON hIcon = GetSystemIcon(extension, SHIL_JUMBO);
    std::unique_ptr<const std::vector<std::byte>> extension_png = GetPngFromIcon(hIcon);

    return extension_png_map.try_emplace(upper_case_extension, std::move(extension_png)).first->second;
}


std::shared_ptr<const std::vector<std::byte>> SystemIcon::GetPngForFolder()
{
    static std::shared_ptr<const std::vector<std::byte>> folder_png;

    if( folder_png == nullptr )
    {
        HICON hIcon = GetStockIcon(SHSTOCKICONID::SIID_FOLDER, SHIL_JUMBO);
        folder_png = GetPngFromIcon(hIcon);
    }

    return folder_png;
}


std::shared_ptr<const std::vector<std::byte>> SystemIcon::GetPngForPath(NullTerminatedString path)
{
    return PortableFunctions::FileIsDirectory(path) ? GetPngForFolder() :
                                                      GetPngForExtension(PortableFunctions::PathGetFileExtension(path));
}



// --------------------------------------------------------------------------
//
// SystemIcon::ImageList
//
// --------------------------------------------------------------------------

SystemIcon::ImageList::ImageList()
    :   m_iconSize(SHIL_SMALL)
{
}


SystemIcon::ImageList::~ImageList()
{
    for( const auto& [hash, hIcon] : m_fileIcons )
        DestroyIcon(hIcon);
}


BOOL SystemIcon::ImageList::Create(int cx, int cy, UINT nFlags)
{
    ASSERT(cx == cy && ( cx == 16 || cx == 32));

    m_iconSize = ( cx == 16 ) ? SHIL_SMALL :
                                SHIL_LARGE;

    return CImageList::Create(cx, cy, nFlags, 0, 2);
}


int SystemIcon::ImageList::GetIconIndexFromExtension(wstring_view extension)
{
    size_t extension_hash = wstring_view(SO::ToUpper(extension)).hash_code();

    const auto& file_icon_lookup = std::find_if(m_fileIcons.cbegin(), m_fileIcons.cend(),
                                                [&](const auto& hash_and_icon) { return ( extension_hash == std::get<0>(hash_and_icon) ); });

    if( file_icon_lookup != m_fileIcons.cend() )
        return static_cast<int>(std::distance(m_fileIcons.cbegin(), file_icon_lookup));

    HICON hIcon = GetSystemIcon(extension, m_iconSize);

    if( hIcon == nullptr )
        return -1;

    // add the icon to the image list
    m_fileIcons.emplace_back(extension_hash, hIcon);
    Add(hIcon);

    ASSERT(GetImageCount() == static_cast<int>(m_fileIcons.size()));

    return GetImageCount() - 1;
}


int SystemIcon::ImageList::GetIconIndexFromPath(NullTerminatedString path)
{
    if( PortableFunctions::FileIsDirectory(path) )
    {
        if( !m_folderIconIndex.has_value() )
        {
            HICON hIcon = GetStockIcon(SHSTOCKICONID::SIID_FOLDER, m_iconSize);

            if( hIcon == nullptr )
            {
                m_folderIconIndex = -1;
            }

            else
            {
                m_fileIcons.emplace_back(SIZE_MAX, hIcon);
                Add(hIcon);

                m_folderIconIndex = GetImageCount() - 1;
            }
        }

        return *m_folderIconIndex;
    }

    else
    {
        return GetIconIndexFromExtension(PortableFunctions::PathGetFileExtension(path));
    }
}
