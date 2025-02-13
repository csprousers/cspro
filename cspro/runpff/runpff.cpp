#include "StdAfx.h"
#include "runpff.h"
#include <zUtilO/Filedlg.h>
#include <zUtilO/Interapp.h>
#include <zAppO/PFF.h>


// The one and only RunPffApp object
RunPffApp theApp;


RunPffApp::RunPffApp()
{
    InitializeCSProEnvironment();
}


BOOL RunPffApp::InitInstance()
{
    AfxEnableControlContainer();

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    std::wstring filename = CS2WS(cmdInfo.m_strFileName);
    std::optional<std::wstring> optional_command_line_arguments;

    // prompt for a filename...
    if( filename.empty() )
    {
        CIMSAFileDialog dlgFile(TRUE, FileExtensions::Pff, NULL, OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
                                _T("CSPro Tasks (*.pff)|*.pff|All Files (*.*)|*.*||"));
        dlgFile.m_ofn.lpstrTitle = _T("Select CSPro Task File to Run");

        if( dlgFile.DoModal() == IDOK )
            filename = CS2WS(dlgFile.GetPathName());
    }

    // or use command line arguments, forwarding any to the program that will be executed
    else
    {
        std::wstring command_line_arguments = GetCommandLine();
        size_t filename_pos = command_line_arguments.find(filename);

        if( filename_pos != std::wstring::npos )
        {
            size_t command_line_arguments_pos = filename_pos + filename.length();

            // skip past any quote in the filename
            if( command_line_arguments_pos < command_line_arguments.length() && command_line_arguments[command_line_arguments_pos] == '"' )
                ++command_line_arguments_pos;

            if( command_line_arguments_pos != command_line_arguments.length() )
                optional_command_line_arguments = SO::Trim(wstring_view(command_line_arguments).substr(command_line_arguments_pos));
        }
    }

    if( !filename.empty() )
        PFF::ExecutePff(filename, optional_command_line_arguments);

    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return FALSE;
}
