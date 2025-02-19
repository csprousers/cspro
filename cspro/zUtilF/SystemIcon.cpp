#include "StdAfx.h"
#include "SystemIcon.h"
#include "IconToPngConverter.h"
#include <zToolsO/CaseInsensitiveComparer.h>
#include <zDesignerF/UWM.h>
#include <atlimage.h>
#include <comdef.h>
#include <commoncontrols.h>


_COM_SMARTPTR_TYPEDEF(IImageList, __uuidof(IImageList));


namespace
{
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
    HICON GetSystemIcon(const std::string& extension, int icon_size)
    {
        // construct a fake filename with the given extension to query for the icon
        const std::string fake_filename = PortableFunctions::PathAppendFileExtension("a", extension);

        // get the system icon index (from https://devblogs.microsoft.com/oldnewthing/20140120-00/?p=2043)
        SHFILEINFO shell_file_info;

        if( SHGetFileInfo(TC::ToWide(fake_filename).c_str(), 0, &shell_file_info, sizeof(shell_file_info), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX) != 0 )
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
        logo_png = IconToPngConverter::GetPngFromIcon((HICON)AfxGetMainWnd()->SendMessage(UWM::Designer::GetDesignerIcon));

    return logo_png;
}


std::shared_ptr<const std::vector<std::byte>> SystemIcon::GetPngForExtension(const std::string& extension)
{
    static std::map<std::string, std::shared_ptr<const std::vector<std::byte>>, cs::case_insensitive_less> extension_png_map;

    ASSERT(extension.empty() || extension.front() != '.');

    // return the PNG for the icon if it has already been loaded
    const auto& icon_png_lookup = extension_png_map.find(extension);

    if( icon_png_lookup != extension_png_map.cend() )
        return icon_png_lookup->second;

    // otherwise lookup the icon and create a PNG for it
    HICON hIcon = GetSystemIcon(extension, SHIL_JUMBO);
    std::unique_ptr<const std::vector<std::byte>> extension_png = IconToPngConverter::GetPngFromIcon(hIcon);

    return extension_png_map.try_emplace(extension, std::move(extension_png)).first->second;
}


std::shared_ptr<const std::vector<std::byte>> SystemIcon::GetPngForFolder()
{
    static std::shared_ptr<const std::vector<std::byte>> folder_png;

    if( folder_png == nullptr )
    {
        HICON hIcon = GetStockIcon(SHSTOCKICONID::SIID_FOLDER, SHIL_JUMBO);
        folder_png = IconToPngConverter::GetPngFromIcon(hIcon);
    }

    return folder_png;
}


std::shared_ptr<const std::vector<std::byte>> SystemIcon::GetPngForPath(const std::string& path)
{
    return PortableFunctions::FileIsDirectory(path) ? GetPngForFolder() :
                                                      GetPngForExtension(PortableFunctions::PathGetFileExtension(path));
}



// --------------------------------------------------------------------------
// SystemIcon::ImageList
// --------------------------------------------------------------------------

SystemIcon::ImageList::ImageList()
    :   m_iconSize(SHIL_SMALL)
{
}


SystemIcon::ImageList::~ImageList()
{
    for( const auto& [extension, hIcon] : m_fileIcons )
        DestroyIcon(hIcon);
}


BOOL SystemIcon::ImageList::Create(const int cx, const int cy, const UINT nFlags, const int nInitial/* = 0*/, const int nGrow/* = 2*/)
{
    ASSERT(cx == cy && ( cx == 16 || cx == 32));

    m_iconSize = ( cx == 16 ) ? SHIL_SMALL :
                                SHIL_LARGE;

    return CImageList::Create(cx, cy, nFlags, nInitial, nGrow);
}


int SystemIcon::ImageList::GetIconIndexFromExtension(const std::string& extension)
{
    const auto& file_icon_lookup = std::find_if(m_fileIcons.cbegin(), m_fileIcons.cend(),
                                                [&](const auto& extension_and_icon) { return SO::EqualsNoCase(std::get<0>(extension_and_icon), extension); });

    if( file_icon_lookup != m_fileIcons.cend() )
        return static_cast<int>(std::distance(m_fileIcons.cbegin(), file_icon_lookup));

    HICON hIcon = GetSystemIcon(extension, m_iconSize);

    if( hIcon == nullptr )
        return -1;

    // add the icon to the image list
    m_fileIcons.emplace_back(extension, hIcon);
    Add(hIcon);

    return GetImageCount() - 1;
}


int SystemIcon::ImageList::GetIconIndexFromPath(const std::string& path)
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
                // add the folder as a string that cannot possibly be an extension
                m_fileIcons.emplace_back(Path::NativeSlashString, hIcon);
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
