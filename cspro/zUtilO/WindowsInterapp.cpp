#include "StdAfx.h"
#include "WindowsInterapp.h"


void OpenContainingFolder(const NullTerminatedString path)
{
    if( PortableFunctions::FileIsDirectory(path) )
    {
        ShellExecute(nullptr, L"explore", L"", nullptr, path.c_str(), SW_SHOW);
    }

    else
    {
        ITEMIDLIST* const pidl = ILCreateFromPath(path.c_str());

        if( pidl != nullptr )
        {
            SHOpenFolderAndSelectItems(pidl, 0, 0, 0);
            ILFree(pidl);
        }
    }
}


void OpenContainingFolder(const std::string_view path_sv)
{
    OpenContainingFolder(TC::ToWide(path_sv));
}


namespace
{
    std::optional<int> DesignerFontZoomLevel;
}

int GetDesignerFontZoomLevel()
{
    if( !DesignerFontZoomLevel.has_value() )
    {
        DesignerFontZoomLevel = AfxGetApp()->GetProfileInt(L"Settings", L"FontZoomLevel", 100);

        if( *DesignerFontZoomLevel < 100 || *DesignerFontZoomLevel > 200 )
            *DesignerFontZoomLevel = 100;
    }

    return *DesignerFontZoomLevel;
}


void SetDesignerFontZoomLevel(const int zoom_level)
{
    AfxGetApp()->WriteProfileInt(L"Settings", L"FontZoomLevel", zoom_level);
    DesignerFontZoomLevel.reset();
}


CString GetDesignerFontName()
{
    return AfxGetApp()->GetProfileString(L"Settings", L"FontName");
}


void SetDesignerFontName(const NullTerminatedString font_name)
{
    AfxGetApp()->WriteProfileString(L"Settings", L"FontName", font_name.c_str());
}
