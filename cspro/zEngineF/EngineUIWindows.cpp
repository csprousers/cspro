#include "StdAfx.h"
#include "EngineUI.h"
#include "PromptFuncDlg.h"
#include "RuntimeNoteDlg.h"
#include "ViewHtmlDlg.h"
#include "WindowsUserbar.h"
#include <zToolsO/WinRegistry.h>
#include <zUtilO/Viewers.h>
#include <zHtml/PortableLocalhost.h>
#include <zAppO/Properties/ApplicationProperties.h>
#include <zEngineO/PffExecutor.h>
#include <zEngineO/SystemApp.h>
#include <zMapping/WindowsMapUI.h>
#include <zMapping/WindowsMapUISingleThread.h>
#include <zEdit2O/ScintillaColorizer.h>


EngineUIProcessor::EngineUIProcessor(const PFF* pff, const bool engine_runs_on_ui_thread)
    :   m_pff(pff),
        m_engineRunsOnUIThread(engine_runs_on_ui_thread)
{
}


long EngineUIProcessor::CaptureImage(EngineUI::CaptureImageNode& /*capture_image_node*/)
{
    return 0; // COMPONENTS_TODO_RESTORE_FOR_CSPRO81 return ReturnProgrammingError(0);
}


long EngineUIProcessor::ColorizeLogic(EngineUI::ColorizeLogicNode& colorize_logic_node)
{
    if( m_pff == nullptr )
        return ReturnProgrammingError(0);

    const int lexer_language = Lexers::GetLexer_Logic(*m_pff->GetApplication());
    ScintillaColorizer colorizer(lexer_language, colorize_logic_node.logic);
    colorize_logic_node.html = colorizer.GetHtml(ScintillaColorizer::HtmlProcessorType::ContentOnly);

    return 1;
}


long EngineUIProcessor::CreateMapUI(std::unique_ptr<IMapUI>& map_ui)
{
    if( m_pff == nullptr )
        return ReturnProgrammingError(0);

    const auto& mapping_properties = m_pff->GetApplication()->GetApplicationProperties().GetMappingProperties();

    if( m_engineRunsOnUIThread )
    {
        map_ui = std::make_unique<WindowsMapUISingleThread>(mapping_properties);
    }

    else
    {
        map_ui = std::make_unique<WindowsMapUI>(mapping_properties);
    }

    return 1;
}


long EngineUIProcessor::CreateUserbar(std::unique_ptr<Userbar>& userbar)
{
    userbar = std::make_unique<WindowsUserbar>();
    return 1;
}


long EngineUIProcessor::EditNote(EngineUI::EditNoteNode& edit_note_node)
{
    RuntimeNoteDlg edit_note_dlg;

    edit_note_dlg.SetTitle(edit_note_node.title);

    ASSERT(edit_note_node.note.Find('\r') == -1);
    CString note_text_with_crlf = edit_note_node.note;
    note_text_with_crlf.Replace(_T("\n"), _T("\r\n"));
    edit_note_dlg.SetNote(note_text_with_crlf);

    if( edit_note_dlg.DoModal() == IDOK )
    {
        edit_note_node.note = edit_note_dlg.GetNote();

        // only use \n escapes, not \r\n
        edit_note_node.note.Remove('\r');
    }

    return 0;
}


long EngineUIProcessor::ExecSystemApp(EngineUI::ExecSystemAppNode& exec_system_app_node)
{
    ASSERT(exec_system_app_node.evaluated_call.find(exec_system_app_node.package_name) == 0);
    std::wstring evaluated_call = exec_system_app_node.evaluated_call;

    // remove any quotes around the executable name
    std::wstring exe_name = UnescapeCommandLineArgument(exec_system_app_node.package_name);

    // if the executable file doesn't exist (for example, chrome.exe), look it up in the registry
    if( !PortableFunctions::FileIsRegular(exe_name) )
    {
        std::wstring key_name = SO::Concatenate(_T("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\"), exec_system_app_node.package_name);

        WinRegistry registry;

        if( registry.Open(HKEY_LOCAL_MACHINE, key_name) && registry.ReadString(_T(""), exe_name) )
        {
            evaluated_call = EscapeCommandLineArgument(exe_name) +
                             exec_system_app_node.evaluated_call.substr(exec_system_app_node.package_name.length());
        }

        registry.Close();
    }

    // RunProgram must run in the engine thread
    exec_system_app_node.function_to_run_in_engine_thread =
        [command = std::move(evaluated_call)]() mutable
        {
            int return_code;
            return RunProgram(std::move(command), &return_code, SW_SHOWNA, true, true);
        };

    return 1;
}


long EngineUIProcessor::HtmlDialogsDirectoryQuery(std::wstring& html_dialogs_directory)
{
    if( m_pff == nullptr )
        return 0;

    html_dialogs_directory = CS2WS(m_pff->GetHtmlDialogsDirectory());
    return 1;
}


long EngineUIProcessor::Prompt(EngineUI::PromptNode& prompt_node)
{
    CPromptFunctionDlg prompt_dlg(prompt_node.title, prompt_node.initial_value,
        prompt_node.multiline, prompt_node.numeric, prompt_node.password, prompt_node.upper_case);

    if( prompt_dlg.DoModal() == IDOK )
        prompt_node.return_value = prompt_dlg.GetResponse();

    return 1;
}


long EngineUIProcessor::RunPffExecutor(EngineUI::RunPffExecutorNode& run_pff_executor_node)
{
    try
    {
        return run_pff_executor_node.pff_executor->Execute(run_pff_executor_node.pff);
    }

    catch(...)
    {
        run_pff_executor_node.thrown_exception = std::current_exception();
        return 0;
    }
}


long EngineUIProcessor::View(const Viewer& viewer)
{
    const Viewer::Data& data = viewer.GetData();
    ASSERT(data.use_embedded_viewers);

    std::wstring url =
        [&]()
        {
            if( data.content_type == Viewer::Data::Type::Filename )
            {
                // if these contents are tied to the file system, we will start a local file server to serve the HTML and any other files
                return !data.local_file_server_root_directory.empty() ? PortableLocalhost::CreateFilenameUrl(data.content) :
                                                                        std::wstring();
            }

            else
            {
                ASSERT(data.content_type == Viewer::Data::Type::HtmlUrl);
                return data.content;
            }
        }();

    // show the dialog
    if( !url.empty() )
    {
        ViewHtmlDlg dlg(viewer);
        dlg.SetInitialUrl(std::move(url));
        dlg.DoModal();

        return 1;
    }

    // if we don't know how to view a file, return 0 and the file will be opened in an external viewer
    else
    {
        ASSERT(data.content_type == Viewer::Data::Type::Filename && !FileExtensions::IsFilenameHtml(data.content));
        return 0;
    }
}
