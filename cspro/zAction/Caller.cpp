#include "stdafx.h"
#include "Caller.h"
#include <zUtilO/SpecialDirectoryLister.h>


std::string ActionInvoker::Caller::EvaluateAbsolutePath(std::string path)
{
    if( Path::IsRelative(path) )
    {
        const std::string& root_directory = GetRootDirectory();

        if( !root_directory.empty() )
            return MakeFullPath(root_directory, std::move(path));
    }

    return PortableFunctions::MakePathToNativeSlash(path);
}


std::string ActionInvoker::Caller::EvaluateAbsolutePath(std::string path, const bool allow_special_directories)
{
    if( allow_special_directories && SpecialDirectoryLister::IsSpecialDirectory(path) )
        return PortableFunctions::MakePathToNativeSlash(path);

    return EvaluateAbsolutePath(std::move(path));
}
