#include "stdafx.h"
#include "TextConverter.h"
#include "TextConverterDlg.h"


// The one and only TextConverterApp object
TextConverterApp theApp;


TextConverterApp::TextConverterApp()
{
    InitializeCSProEnvironment();
}


BOOL TextConverterApp::InitInstance()
{
    __super::InitInstance();

    AfxEnableControlContainer();

    SetRegistryKey(L"U.S. Census Bureau");

    // 20141015 allow TextConverter to be run from the command line
    if( __argc > 1 )
    {
        bool convert_to_ansi = true;

        for( int i = 1; i < __argc; ++i )
        {
            const std::string argument = TC::ToUtf8(__targv[i]);

            if( SO::StartsWith(argument, "/") ) // a flag
            {
                if( SO::EqualsNoCase(argument, "/utf8") )
                {
                    convert_to_ansi = false;
                }

                else if( SO::EqualsNoCase(argument, "/ansi") )
                {
                    convert_to_ansi = true;
                }
            }

            else if( PortableFunctions::FileIsRegular(argument) )
            {
                Encoding encoding;

                if( GetFileBOM(argument, encoding) )
                {
                    if( encoding == Encoding::Ansi && !convert_to_ansi )
                    {
                        CStdioFileUnicode::ConvertAnsiToUTF8(argument);
                    }

                    else if( encoding == Encoding::Utf8 && convert_to_ansi )
                    {
                        CStdioFileUnicode::ConvertUTF8ToAnsi(argument);
                    }
                }
            }
        }
    }

    else
    {
        TextConverterDlg dlg;
        m_pMainWnd = &dlg;
        dlg.DoModal();
    }

    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return FALSE;
}
