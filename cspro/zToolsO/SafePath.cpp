#include "StdAfx.h"
#include "SafePath.h"


void SafePath::EnsureSafePathForWin32()
{
    // only process file paths that use characters above ASCII 127
    for( const char* path_itr = m_sourcePath; ; ++path_itr )
    {
        const char ch = *path_itr;

        if( ch == '\0' )
            return;

        if( !TC::IsUtf8SingleByte(*path_itr) )
            break;
    }

    const std::wstring wide_path = TC::ToWide(m_sourcePath).c_str();
    const DWORD wide_short_path_length = GetShortPathName(wide_path.c_str(), nullptr, 0);

    if( wide_short_path_length != 0 )
    {
        auto wide_short_path = std::make_unique_for_overwrite<wchar_t[]>(wide_short_path_length);

        if( GetShortPathName(wide_path.c_str(), wide_short_path.get(), wide_short_path_length) != 0 )
        {
            // wide_short_path_length accounts for the null terminator
            m_shortPath = std::make_unique<std::string>(TC::ToUtf8(wide_short_path.get(), wide_short_path_length - 1));
        }
    }
}
