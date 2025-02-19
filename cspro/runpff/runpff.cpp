#include "StdAfx.h"
#include "runpff.h"
#include <zToolsO/Utf8.h>
#include <zUtilO/FileDlg.h>
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

    std::string pff_file_path = TC::ToUtf8(cmdInfo.m_strFileName);
    std::unique_ptr<const std::string> optional_command_line_arguments;

    // prompt for a file...
    if( pff_file_path.empty() )
    {
        OpenFileDlg open_file_dlg(0, FileExtensions::Pff, nullptr, FileFilters::Pff);
        open_file_dlg.SetTitle(L"Select CSPro Task File to Run");

        if( open_file_dlg.DoModal() == IDOK )
            pff_file_path = open_file_dlg.GetFilePath();
    }

    // ...or use command line arguments, forwarding any to the program that will be executed
    else
    {
        std::string command_line_arguments = TC::ToUtf8(GetCommandLine());
        const size_t pff_file_path_pos = command_line_arguments.find(pff_file_path);

        if( pff_file_path_pos != std::string::npos )
        {
            size_t command_line_arguments_pos = pff_file_path_pos + pff_file_path.length();

            // skip past any quote in the file path
            if( command_line_arguments_pos < command_line_arguments.length() && command_line_arguments[command_line_arguments_pos] == '"' )
                ++command_line_arguments_pos;

            if( command_line_arguments_pos != command_line_arguments.length() )
                optional_command_line_arguments = std::make_unique<std::string>(SO::Trim(std::string_view(command_line_arguments).substr(command_line_arguments_pos)));
        }
    }

    if( !pff_file_path.empty() )
        PFF::ExecutePff(pff_file_path, optional_command_line_arguments.get());

    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return FALSE;
}
