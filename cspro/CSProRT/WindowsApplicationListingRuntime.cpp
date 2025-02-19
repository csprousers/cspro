#include "StdAfx.h"
#include "WindowsApplicationListingRuntime.h"
#include <zUtilO/MimeType.h>
#include <zUtilF/SystemIcon.h>


WindowsApplicationListingRuntime::~WindowsApplicationListingRuntime()
{
}


bool WindowsApplicationListingRuntime::CanExecute(const APPTYPE app_type)
{
    app_type; return true; // RT_TODO properly handle
}


std::string WindowsApplicationListingRuntime::GetUrlForFileExtension(const std::string& extension)
{
    auto lookup = m_extensionImageVirtualFileMappingHandlers.find(extension);

    if( lookup == m_extensionImageVirtualFileMappingHandlers.cend() )
    {
        std::unique_ptr<VirtualFileMappingHandler> image_virtual_file_mapping_handler;

        std::shared_ptr<const std::vector<std::byte>> png_data = SystemIcon::GetPngForExtension(extension);

        if( png_data != nullptr )
        {
            image_virtual_file_mapping_handler = std::make_unique<DataVirtualFileMappingHandler<std::shared_ptr<const std::vector<std::byte>>>>(std::move(png_data), MimeType::Type::ImagePng);
            PortableLocalhost::CreateVirtualFile(*image_virtual_file_mapping_handler, extension + ".png");
        }

        lookup = m_extensionImageVirtualFileMappingHandlers.try_emplace(extension, std::move(image_virtual_file_mapping_handler)).first;
    }

    if( lookup->second != nullptr )
        return lookup->second->GetUrl();

    return std::string();
}
