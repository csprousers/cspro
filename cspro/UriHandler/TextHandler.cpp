#include "stdafx.h"
#include "UriHandler.h"
#include <zToolsO/FileIO.h>


namespace Pre81
{
    constexpr std::string_view TextViewerPropertyFile_sv = "file";
}


void UriHandlerApp::HandleTextUri(const std::string_view uri_sv)
{
    ASSERT(CustomUri::UsesCSProScheme(uri_sv, CustomUri::UriType::Text));

    const std::string file_path = CustomUri::ConvertTextUriToFilePath(uri_sv);

    if( !PortableFunctions::FileIsRegular(file_path) )
        throw FileIO::Exception::FileNotFound(file_path);

    ViewFileInTextViewer(file_path);
}


void UriHandlerApp::ProcessPre81TextViewerProperties(const std::map<std::string, std::string>& properties)
{
    const auto& file_lookup = properties.find(std::string(Pre81::TextViewerPropertyFile_sv));

    if( file_lookup == properties.cend() )
        throw CSProException("The CSPro URI that launches Text Viewer must contain the property: " + std::string(Pre81::TextViewerPropertyFile_sv));

    HandleTextUri(CustomUri::CreateTextUri(file_lookup->second));
}
