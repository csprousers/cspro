#pragma once

#define MINIZ_HEADER_FILE_ONLY

#include <external/miniz/miniz.c>


namespace ZipHelpers
{
    // some zip files files created on Windows use backslashes, others use forward slashes;
    // this function we will check against both
    inline int LocateFile(mz_zip_archive* zip_archive, wstring_view path_in_zip)
    {
        int file_index;

        auto locate_file = [&](wstring_view this_path_in_zip) -> bool
        {
            file_index = mz_zip_reader_locate_file(zip_archive, UTF8Convert::WideToUTF8(this_path_in_zip).c_str(), nullptr, 0);
            return ( file_index >= 0 );
        };

        ( locate_file(path_in_zip) ) ||
        ( path_in_zip.find('\\') != wstring_view::npos && locate_file(PortableFunctions::PathToForwardSlash<std::wstring>(path_in_zip)) ) ||
        ( path_in_zip.find('/') != wstring_view::npos && locate_file(PortableFunctions::PathToBackwardSlash<std::wstring>(path_in_zip)) );

        return file_index;
    }


#ifdef _DEBUG
    inline int CompareStringIgnoreCaseAndPathSlashWorker(const TCHAR* str1, const TCHAR* str2)
#else
    inline int CompareStringIgnoreCaseAndPathSlash(const TCHAR* str1, const TCHAR* str2)
#endif
    {
        for( ; true; ++str1, ++str2 )
        {
            if( *str1 == 0 )
                return ( *str2 == 0 ) ? 0 : -1;

            if( *str2 == 0 )
                return 1;

            if( *str1 != *str2 )
            {
                if( ( *str1 == '/' || *str1 == '\\' ) && ( *str2 == '/' || *str2 == '\\' ) )
                    continue;

                if( std::towupper(*str1) == std::towupper(*str2) )
                    continue;

                return ( *str1 < *str2 ) ? -1 : 1;
            }
        }
    }

#ifdef _DEBUG
    inline int CompareStringIgnoreCaseAndPathSlash(const TCHAR* str1, const TCHAR* str2)
    {
        int result = CompareStringIgnoreCaseAndPathSlashWorker(str1, str2);
        int cstring_result = PortableFunctions::PathToForwardSlash<CString>(str1).CompareNoCase(PortableFunctions::PathToForwardSlash<CString>(str2));
        ASSERT(( result == 0 && cstring_result == 0 ) || ( result != 0 && ( result < 0 ) == ( cstring_result < 0 ) ));
        return result;
    }
#endif
}
