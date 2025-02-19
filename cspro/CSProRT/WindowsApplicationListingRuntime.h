#pragma once

#include <zRuntimeO/ApplicationListingRuntime.h>

class VirtualFileMappingHandler;


class WindowsApplicationListingRuntime : public ApplicationListingRuntime
{
public:
    using ApplicationListingRuntime::ApplicationListingRuntime;

    ~WindowsApplicationListingRuntime();

    bool CanExecute(APPTYPE app_type) override;

    std::string GetUrlForFileExtension(const std::string& extension) override;

private:
    std::map<std::string, std::unique_ptr<VirtualFileMappingHandler>> m_extensionImageVirtualFileMappingHandlers;
};
