#include "StdAfx.h"
#include "WindowsInterapp.h"


template<bool ThrowExceptionOnError/* = true*/>
std::conditional_t<ThrowExceptionOnError, void, bool> OpenFileInAssociatedApplication(const InterfaceString& file_path)
{
    const HINSTANCE result = ShellExecute(nullptr, L"open", EscapeCommandLineArgument(file_path.GetString()).c_str(), nullptr, nullptr, SW_SHOW);
    const bool success = ( reinterpret_cast<INT_PTR>(result) >= 32 );

    if constexpr(ThrowExceptionOnError)
    {
         if( !success )
             throw CSProException("Error opening: %s", file_path.c_str_utf8());
    }

    else
    {
        return success;
    }
}

template CLASS_DECL_ZUTILO void OpenFileInAssociatedApplication<true>(const InterfaceString& file_path);
template CLASS_DECL_ZUTILO bool OpenFileInAssociatedApplication<false>(const InterfaceString& file_path);


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
