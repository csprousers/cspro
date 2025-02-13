#include "StdAfx.h"
#include "PortableFileSystem.h"
#include <zPlatformO/PlatformInterface.h>


namespace
{
    constexpr std::wstring_view AndroidContentUriPrefix_sv = _T("content://");
}



bool PortableFileSystem::IsSharableUri(const wstring_view uri_sv)
{    
#ifdef ANDROID
    return SO::StartsWith(uri_sv, AndroidContentUriPrefix_sv);

#else
    UNREFERENCED_PARAMETER(uri_sv);
    return false;

#endif
}


std::wstring PortableFileSystem::CreateSharableUri(const std::wstring& path, const bool add_write_permission)
{
    ASSERT(PortableFunctions::FileIsRegular(path));

#ifdef ANDROID
    std::wstring sharable_uri = PlatformInterface::GetInstance()->GetApplicationInterface()->CreateSharableUri(path, add_write_permission);
    ASSERT(IsSharableUri(sharable_uri));
    return sharable_uri;

#else
    UNREFERENCED_PARAMETER(add_write_permission);
    return path;

#endif
}


void PortableFileSystem::FileCopy(const std::wstring& source_path_or_sharable_uri, const std::wstring& destination_path, const bool overwrite)
{
#ifdef ANDROID
    if( IsSharableUri(source_path_or_sharable_uri) )
    {
        if( !overwrite && PortableFunctions::FileIsRegular(destination_path) )
            throw FileIO::Exception::FileCopyFailDestinationExists(source_path_or_sharable_uri, destination_path);

        PlatformInterface::GetInstance()->GetApplicationInterface()->FileCopySharableUri(source_path_or_sharable_uri, destination_path);
        return;
    }
#endif

    PortableFunctions::FileCopyWithExceptions(source_path_or_sharable_uri, destination_path, overwrite ? PortableFunctions::FileCopyType::CopyIfDifferent :
                                                                                                         PortableFunctions::FileCopyType::FailIfExists);
}
