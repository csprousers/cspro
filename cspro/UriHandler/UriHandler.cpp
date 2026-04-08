#include "stdafx.h"
#include <zToolsO/base64.h>
#include <zToolsO/Utf8.h>


namespace Pre81
{
    constexpr char Base64EncodingIndicator               = '~';

    constexpr std::string_view ProgramProperty_sv        = "program";

    constexpr std::string_view ProgramValueTextViewer_sv = "TextViewer";
    constexpr std::string_view ProgramValueDataViewer_sv = "DataViewer";
}


// The one and only UriHandlerApp object
UriHandlerApp theApp;


class UriHandlerApp::CommandLineProcessor : public CCommandLineInfo
{
public:
    CommandLineProcessor(std::vector<std::string>& command_line_arguments)
        :   m_commandLineArguments(command_line_arguments)
    {
    }

    void ParseParam(const wchar_t* const pszParam, BOOL /*bFlag*/, BOOL /*bLast*/) override
    {
        m_commandLineArguments.emplace_back(TC::ToUtf8(pszParam));
    }

private:
    std::vector<std::string>& m_commandLineArguments;
};


UriHandlerApp::UriHandlerApp()
{
    InitializeCSProEnvironment();
}


BOOL UriHandlerApp::InitInstance()
{
    try
    {
        std::vector<std::string> command_line_arguments;
        CommandLineProcessor command_line_processor(command_line_arguments);
        ParseCommandLine(command_line_processor);

        if( command_line_arguments.empty() )
            throw CSProException("You must specify a valid CSPro URI.");

        // remove any trailing slashes (which can be added by tools like Excel)
        const std::string_view uri_sv = SO::TrimRight(command_line_arguments.front(), Path::SlashChars_sv);

        // make sure that the URI starts with the proper prefix
        if( !CustomUri::UsesCSProScheme(uri_sv) )
            throw CSProException("A CSPro URI must begin with: " + std::string(CustomUri::CSProScheme_sv));

        // prior to CSPro 8.1, URIs looked like: cspro:///program= or cspro://program=
        const std::string_view pre81_path_sv = uri_sv.substr(CustomUri::CSProScheme_sv.length());

        if( SO::StartsWith(pre81_path_sv, Pre81::ProgramProperty_sv) )
        {
            ProcessPre81Path(pre81_path_sv);
        }

        else if( !pre81_path_sv.empty() && pre81_path_sv.front() == '/' &&
                 SO::StartsWith(pre81_path_sv.substr(1), Pre81::ProgramProperty_sv) )
        {
            ProcessPre81Path(pre81_path_sv.substr(1));
        }

        else
        {
            ProcessUri(uri_sv);
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    return FALSE;
}


void UriHandlerApp::ProcessUri(const std::string_view uri_sv)
{
    ASSERT(CustomUri::UsesCSProScheme(uri_sv));

    const std::optional<CustomUri::UriType> uri_type = CustomUri::GetUriType(uri_sv);

    if( uri_type.has_value() )
    {
        switch( *uri_type )
        {
            case CustomUri::UriType::Text:
                HandleTextUri(uri_sv);
                return;

            case CustomUri::UriType::Data:
                HandleDataUri(std::string(uri_sv));
                return;
        }
    }

    throw CSProException("The CSPro URI Handler does not know how to handle the URI: " + std::string(uri_sv));
}


void UriHandlerApp::ProcessPre81Path(const std::string_view pre81_path_sv)
{
    // pre-8.1 URLs looked like:
    // cspro:///program=TextViewer&file=file.csdb
    // cspro:///program=DataViewer&file=file.csdb&dcf=~QzpcVXNlcnNcSW5zZXRlXERlc2t0b3BcRGF0YU1hbmFnZXJcY29ubmVjdGlvbiBzdHJpbmcgLSBkYXRhXENlbnN1cyBEaWN0aW9uYXJ5LmRjZg==&key=010108800220740201
    ASSERT(SO::StartsWith(pre81_path_sv, Pre81::ProgramProperty_sv));

    // split the text by & and then map each property and value
    std::map<std::string, std::string> properties;

    for( std::string_view uri_component_sv : SO::SplitString<std::string_view>(pre81_path_sv, '&') )
    {
        const size_t equals_pos = uri_component_sv.find('=');
        std::string value;

        if( equals_pos != std::string_view::npos )
        {
            value = uri_component_sv.substr(equals_pos + 1);

            // the value might be Base64-encoded
            if( !value.empty() && value.front() == Pre81::Base64EncodingIndicator )
                value = Base64::DecodeToString(value.substr(1));

            uri_component_sv = uri_component_sv.substr(0, equals_pos);
        }

        properties.try_emplace(std::string(uri_component_sv), std::move(value));
    }

    // handle each program
    const auto& program_lookup = properties.find(std::string(Pre81::ProgramProperty_sv));

    if( program_lookup == properties.cend() )
    {
        ASSERT(false);
        throw CSProException("A pre-CSPro 8.1 URI must contain a program name.");
    }

    const std::string program = std::move(program_lookup->second);
    properties.erase(program_lookup);

    if( program == Pre81::ProgramValueTextViewer_sv )
    {
        ProcessPre81TextViewerProperties(properties);
    }

    else if( program == Pre81::ProgramValueDataViewer_sv )
    {
        ProcessPre81DataViewerProperties(properties);
    }

    else
    {
        throw CSProException("The CSPro URI Handler does not know how to handle the program: " + program);
    }
}
