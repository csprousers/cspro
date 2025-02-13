#include "stdafx.h"
#include <zToolsO/base64.h>
#include <zToolsO/Utf8Convert.h>


void HandleDataViewerUri(const std::map<std::wstring, std::wstring>& properties);
void HandleTextViewerUri(const std::map<std::wstring, std::wstring>& properties);


namespace
{
    constexpr wstring_view UriPrefix              = _T("cspro:///");
    constexpr wstring_view UriPrefixFromExcel     = _T("cspro://");

    constexpr TCHAR Base64EncodingIndicator       = '~';

    constexpr wstring_view ProgramProperty        = _T("program");
    constexpr wstring_view ProgramValueDataViewer = _T("DataViewer");
    constexpr wstring_view ProgramValueTextViewer = _T("TextViewer");


    class CommandLineProcessor : public CCommandLineInfo
    {
    public:
        CommandLineProcessor(std::vector<std::wstring>& command_line_arguments)
            :   m_commandLineArguments(command_line_arguments)
        {
        }

        void ParseParam(const TCHAR* pszParam, BOOL /*bFlag*/, BOOL /*bLast*/) override
        {
            m_commandLineArguments.emplace_back(pszParam);
        }

    private:
        std::vector<std::wstring>& m_commandLineArguments;
    };
}


class UriHandlerApp : public CWinApp
{
public:
    UriHandlerApp()
    {
        InitializeCSProEnvironment();
    }

    BOOL InitInstance() override
    {
        try
        {
            std::vector<std::wstring> command_line_arguments;
            CommandLineProcessor command_line_processor(command_line_arguments);
            ParseCommandLine(command_line_processor);

            if( command_line_arguments.empty() )
                throw CSProException("You must specify a valid CSPro URI.");

            wstring_view uri_sv = command_line_arguments.front();

            // remove any trailing slashes (which can be added by programs like Excel)
            uri_sv = SO::TrimRight(uri_sv, PortableFunctions::PathSlashChars);

            // make sure that the URI starts with the proper prefix
            size_t uri_prefix_length = SO::StartsWith(uri_sv, UriPrefix)          ? UriPrefix.length() :
                                       SO::StartsWith(uri_sv, UriPrefixFromExcel) ? UriPrefixFromExcel.length() :
                                                                                    SIZE_MAX;

            if( uri_prefix_length == SIZE_MAX )
                throw CSProException(_T("A CSPro URI must begin with: %s"), std::wstring(UriPrefix).c_str());

            // split the URI by & and then map each property and value
            std::map<std::wstring, std::wstring> properties;

            for( wstring_view uri_component : SO::SplitString<wstring_view>(uri_sv.substr(uri_prefix_length), '&') )
            {
                size_t equals_pos = uri_component.find('=');
                std::wstring value;

                if( equals_pos != wstring_view::npos )
                {
                    value = uri_component.substr(equals_pos + 1);

                    // the value might be base64-encoded
                    if( !value.empty() && value.front() == Base64EncodingIndicator )
                        value = Base64::Decode(value.substr(1));

                    uri_component = uri_component.substr(0, equals_pos);
                }

                properties.try_emplace(uri_component, std::move(value));
            }


            // handle each program
            const auto& program_lookup = properties.find(ProgramProperty);

            if( program_lookup == properties.cend() )
                throw CSProException("A CSPro URI must contain a program name.");

            if( program_lookup->second == ProgramValueDataViewer )
            {
                HandleDataViewerUri(properties);
            }

            else if( program_lookup->second == ProgramValueTextViewer )
            {
                HandleTextViewerUri(properties);
            }

            else
            {
                throw CSProException(_T("The CSPro URI handler does not know how to handle the program %s."),
                                     program_lookup->second.c_str());
            }
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(exception);
        }

        return FALSE;
    }
};


UriHandlerApp theApp;
