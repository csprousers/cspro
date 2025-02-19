#include "stdafx.h"
#include "FileSystemVirtualFileMappingHandler.h"
#include <zToolsO/FileIO.h>


namespace
{
    // vfs = virtual file system
    // the URL prefix is used because Android paths begin with /, which resulted in ServeContent's key
    // coming in with a missing starting / (because browsers would turn // into a single /)
    constexpr std::string_view UrlPrefix_sv = "vfs"; 
}


FileSystemVirtualFileMappingHandler::FileSystemVirtualFileMappingHandler(std::string file_path, std::shared_ptr<VirtualFileMappingHandler> virtual_file_mapping_handler)
{
    RegisterSpecialHandler(std::move(file_path), std::move(virtual_file_mapping_handler));
}


VirtualFileMappingHandler* FileSystemVirtualFileMappingHandler::GetSpecialHandler(const std::string& file_path)
{
    const auto& lookup = m_specialHandlers.find(file_path);

    return ( lookup != m_specialHandlers.cend() ) ? lookup->second.get() :
                                                    nullptr;
}


void FileSystemVirtualFileMappingHandler::RegisterSpecialHandler(std::string file_path, std::shared_ptr<VirtualFileMappingHandler> virtual_file_mapping_handler)
{
    ASSERT(file_path == PortableFunctions::PathToNativeSlash(file_path));
    ASSERT(GetSpecialHandler(file_path) == nullptr && virtual_file_mapping_handler != nullptr);

    m_specialHandlers.try_emplace(std::move(file_path), std::move(virtual_file_mapping_handler));
}


std::string FileSystemVirtualFileMappingHandler::CreateUrlForPath(const std::string& file_path) const
{
    return CreateUrl(SO::Concatenate(UrlPrefix_sv, PortableFunctions::PathToForwardSlash(file_path)), false);
}


bool FileSystemVirtualFileMappingHandler::ServeContent(VirtualFileMappingResponse& response, const std::string& key)
{
    if( !SO::StartsWithNoCase(key, UrlPrefix_sv) )
        return ReturnProgrammingError(false);

    const std::string file_path = PortableFunctions::PathToNativeSlash(key.substr(UrlPrefix_sv.length()));
    ASSERT(file_path.empty() || ( ( file_path.front() == '/' ) == OnAndroidOrWasm() ));

    VirtualFileMappingHandler* const special_handler = GetSpecialHandler(file_path);

    if( special_handler != nullptr && special_handler->ServeContent(response) )
        return true;

    // if not a special handler, or if the special handler failed, try to serve the content off the disk
    return ServeFileSystemContent(response, file_path);
}


bool FileSystemVirtualFileMappingHandler::ServeFileSystemContent(VirtualFileMappingResponse& response, const std::string& file_path) const
{
    if( PortableFunctions::FileIsRegular(file_path) )
    {
        try
        {
            response.SetContent(FileIO::Read(file_path),
                                ValueOrDefault(MimeType::GetServerTypeFromFileExtension(PortableFunctions::PathGetFileExtension(file_path))));

            return true;
        }
        catch(...) { }
    }

    return false;
}
