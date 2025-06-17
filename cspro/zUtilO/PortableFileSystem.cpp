#include "StdAfx.h"
#include "PortableFileSystem.h"
#include <zPlatformO/PlatformInterface.h>


namespace
{
    constexpr std::string_view AndroidContentUriPrefix_sv = "content://";
}



bool PortableFileSystem::IsSharableUri(const std::string_view uri_sv)
{
#ifdef ANDROID
    return SO::StartsWith(uri_sv, AndroidContentUriPrefix_sv);

#else
    uri_sv; // UNREFERENCED_PARAMETER
    return false;

#endif
}


std::string PortableFileSystem::CreateSharableUri(const std::string& path, const bool add_write_permission)
{
    ASSERT(PortableFunctions::FileIsRegular(path));

#ifdef ANDROID
    std::string sharable_uri = PlatformInterface::GetInstance()->GetApplicationInterface()->CreateSharableUri(path, add_write_permission);
    ASSERT(IsSharableUri(sharable_uri));
    return sharable_uri;

#else
    add_write_permission; // UNREFERENCED_PARAMETER
    return path;

#endif
}


bool PortableFileSystem::FileCopy(const std::string& source_path_or_sharable_uri, const std::string& destination_path,
                                  const FileOverwriteFlag file_overwrite_flag)
{
#ifdef ANDROID
    if( IsSharableUri(source_path_or_sharable_uri) )
    {
        if( ( file_overwrite_flag == FileOverwriteFlag::Never || file_overwrite_flag == FileOverwriteFlag::Fail ) &&
            ( PortableFunctions::FileIsRegular(destination_path) ) )
        {
            if( file_overwrite_flag == FileOverwriteFlag::Never )
            {
                return false;
            }

            else if( file_overwrite_flag == FileOverwriteFlag::Fail )
            {
                throw FileIO::Exception::FileCopyFailDestinationExists(source_path_or_sharable_uri, destination_path);
            }
        }

        PlatformInterface::GetInstance()->GetApplicationInterface()->FileCopySharableUri(source_path_or_sharable_uri, destination_path);

        return true;
    }
#endif

    return PortableFunctions::FileCopyWithExceptions(source_path_or_sharable_uri, destination_path, file_overwrite_flag);
}
