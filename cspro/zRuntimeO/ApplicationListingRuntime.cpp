#include "StdAfx.h"
#include "ApplicationListingRuntime.h"
#include <zToolsO/DirectoryLister.h>
#include <zAppO/PFF.h>


CREATE_JSON_KEY(applications);
CREATE_JSON_KEY(iconUrl);
CREATE_JSON_KEY(showInApplicationListing);
CREATE_JSON_VALUE(getApplications);
CREATE_JSON_VALUE(runApplication);


ApplicationListingRuntime::ApplicationListingRuntime(std::string directory)
    :   m_url(PortableLocalhost::CreateUniqueFileUrl(Path::Combine(Html::GetDirectory(Html::Subdirectory::Runtime), "applications.html"))),
        m_directory(std::move(directory))
{
}


SharableString ApplicationListingRuntime::GetDescription() noexcept
{
    return FormatText("Application Listing (%s)", PortableFunctions::PathGetFilename(m_directory).c_str());
}


SharableString ApplicationListingRuntime::GetUrl() noexcept
{
    return m_url;
}


bool ApplicationListingRuntime::IsCloseable() noexcept
{
    return true;
}


void ApplicationListingRuntime::Start()
{
    if( !PortableFunctions::FileIsDirectory(m_directory) )
        throw CSProException("The directory does not exist: " + m_directory);
}


SharableString ApplicationListingRuntime::OnMessage(const std::string_view action_sv, const JsonNode& json_node)
{
    if( action_sv == JV::getApplications )
    {
        return GetApplications();
    }

    else if( action_sv == JV::runApplication )
    {
        m_runtimeHost->StartRuntimeAsync(m_runtimeHost->CreateRuntimeForApplication(json_node.Get<std::string>(JK::path)));
        return SharableString();
    }

    return Runtime::OnMessage(action_sv, json_node);
}


SharableString ApplicationListingRuntime::GetApplications()
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::directory, m_directory)
                .BeginArray(JK::applications);

    // read each PFF
    for( const std::string& pff_file_path : DirectoryLister().SetRecursive()
                                                             .SetNameFilter(FileExtensions::CreateWildcard(FileExtensions::Pff))
                                                             .GetPaths(m_directory) )
    {
        PFF pff(UTF8_TODO::GetCString(pff_file_path));

        if( pff.LoadPifFile() && CanExecute(pff.GetAppType()) )
        {
            json_writer->BeginObject()
                        .Write(JK::path, pff_file_path)
                        .Write(JK::description, pff.GetEvaluatedAppDescription())
                        .Write(JK::showInApplicationListing, pff.GetShowInApplicationListing()) // RT_TODO process here instead?
                        .WriteIfNotBlank(JK::iconUrl, GetUrlForFileExtension(PortableFunctions::PathGetFileExtension(UTF8_TODO::GetUtf8(pff.GetAppFName()))))
                        .EndObject();
        }
    }

    json_writer->EndArray()
                .EndObject();

    return json_writer->ReleaseSharableString();
}
