#include "StdAfx.h"
#include "WindowsInterapp.h"


void OpenContainingFolder(NullTerminatedString path)
{
    if( PortableFunctions::FileIsDirectory(path) )
    {
        ShellExecute(nullptr, _T("explore"), nullptr, nullptr, path.c_str(), SW_SHOW);
    }

    else
    {
        ITEMIDLIST* pidl = ILCreateFromPath(path.c_str());

        if( pidl != nullptr )
        {
            SHOpenFolderAndSelectItems(pidl, 0, 0, 0);
            ILFree(pidl);
        }
    }
}


namespace
{
    std::optional<int> DesignerFontZoomLevel;
}

int GetDesignerFontZoomLevel()
{
    if( !DesignerFontZoomLevel.has_value() )
    {
        DesignerFontZoomLevel = AfxGetApp()->GetProfileInt(_T("Settings"), _T("FontZoomLevel"), 100);

        if( *DesignerFontZoomLevel < 100 || *DesignerFontZoomLevel > 200 )
            *DesignerFontZoomLevel = 100;
    }

    return *DesignerFontZoomLevel;
}

void SetDesignerFontZoomLevel(int iFontZoomLevel)
{
    AfxGetApp()->WriteProfileInt(_T("Settings"), _T("FontZoomLevel"), iFontZoomLevel);
    DesignerFontZoomLevel.reset();
}


CString GetDesignerFontName()
{
    return AfxGetApp()->GetProfileString(_T("Settings"), _T("FontName"));
}

void SetDesignerFontName(const TCHAR* font_name)
{
    AfxGetApp()->WriteProfileString(_T("Settings"), _T("FontName"), font_name);
}
