#pragma once

#include <zRuntimeO/zRuntimeO.h>
#include <zRuntimeO/Runtime.h>

enum APPTYPE;


class ZRUNTIMEO_API ApplicationListingRuntime : public Runtime
{
public:
    ApplicationListingRuntime(std::string directory);

    // Runtime overrides
protected:
    SharableString GetDescription() noexcept override;
    SharableString GetUrl() noexcept override;
    bool IsCloseable() noexcept override;
    void Start() override;
    SharableString OnMessage(std::string_view action_sv, const JsonNode& json_node) override;

    // methods for subclasses to override
protected:
    // return true if the application type is supported
    virtual bool CanExecute(APPTYPE app_type) = 0;

    // return a URL to an image representing the file extension (or a blank string if not supported)
    virtual std::string GetUrlForFileExtension(const std::string& extension) = 0;

private:
    SharableString GetApplications();

private:
    SharableString m_url;
    std::string m_directory;
};
