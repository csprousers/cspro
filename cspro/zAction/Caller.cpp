#include "stdafx.h"
#include "Caller.h"
#include <zUtilO/SpecialDirectoryLister.h>


std::wstring ActionInvoker::Caller::EvaluateAbsolutePath(std::wstring path)
{
    if( PathIsRelative(path.c_str()) )
    {
        const std::wstring& root_directory = GetRootDirectory();

        if( !root_directory.empty() )
            return MakeFullPath(root_directory, std::move(path));
    }

    NormalizePathSlash(path);
    return path;
}


std::wstring ActionInvoker::Caller::EvaluateAbsolutePath(std::wstring path, const bool allow_special_directories)
{
    if( allow_special_directories && SpecialDirectoryLister::IsSpecialDirectory(path) )
    {
        NormalizePathSlash(path);
        return path;
    }

    return EvaluateAbsolutePath(std::move(path));
}
