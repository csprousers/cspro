#include "StdAfx.h"
#include "EngineUI.h"
#include <zUtilO/CSProExecutables.h>
#include <zHtml/PortableLocalhost.h>


long EngineUIProcessor::ProcessMessage(WPARAM wParam, LPARAM lParam)
{
    const EngineUI::Type type = static_cast<EngineUI::Type>(wParam);
        
    switch( type )
    {
        case EngineUI::Type::CaptureImage:
        {
            EngineUI::CaptureImageNode* capture_image_node = reinterpret_cast<EngineUI::CaptureImageNode*>(lParam);
            return CaptureImage(*capture_image_node);
        }

        case EngineUI::Type::ColorizeLogic:
        {
            EngineUI::ColorizeLogicNode* colorize_logic_node = reinterpret_cast<EngineUI::ColorizeLogicNode*>(lParam);
            return ColorizeLogic(*colorize_logic_node);
        }

        case EngineUI::Type::CreateMapUI:
        {
            std::unique_ptr<IMapUI>* map_ui = reinterpret_cast<std::unique_ptr<IMapUI>*>(lParam);
            return CreateMapUI(*map_ui);
        }

        case EngineUI::Type::CreateUserbar:
        {
            std::unique_ptr<Userbar>* userbar = reinterpret_cast<std::unique_ptr<Userbar>*>(lParam);
            return CreateUserbar(*userbar);
        }

        case EngineUI::Type::CreateVirtualFileMappingAroundViewHtmlContent:
        {
            EngineUI::CreateVirtualFileMappingAroundViewHtmlContentNode* node = reinterpret_cast<EngineUI::CreateVirtualFileMappingAroundViewHtmlContentNode*>(lParam);
            return CreateVirtualFileMappingAroundViewHtmlContent(*node);
        }

        case EngineUI::Type::EditNote:
        {
            EngineUI::EditNoteNode* edit_note_node = reinterpret_cast<EngineUI::EditNoteNode*>(lParam);
            return EditNote(*edit_note_node);
        }

        case EngineUI::Type::ExecSystemApp:
        {
            EngineUI::ExecSystemAppNode* exec_system_app_node = reinterpret_cast<EngineUI::ExecSystemAppNode*>(lParam);
            return ExecSystemApp(*exec_system_app_node);
        }

        case EngineUI::Type::HtmlDialogsDirectoryQuery:
        {
            std::wstring* html_dialogs_directory = reinterpret_cast<std::wstring*>(lParam);
            return HtmlDialogsDirectoryQuery(*html_dialogs_directory);
        }

        case EngineUI::Type::Prompt:
        {
            EngineUI::PromptNode* prompt_node = reinterpret_cast<EngineUI::PromptNode*>(lParam);
            return Prompt(*prompt_node);
        }

        case EngineUI::Type::RunPffExecutor:
        {
            EngineUI::RunPffExecutorNode* run_pff_executor_node = reinterpret_cast<EngineUI::RunPffExecutorNode*>(lParam);
            return RunPffExecutor(*run_pff_executor_node);
        }

        case EngineUI::Type::View:
        {
            const Viewer* viewer = reinterpret_cast<const Viewer*>(lParam);
            return View(*viewer);
        }

        default:
        {
            return ReturnProgrammingError(0);
        }
    }
}


long EngineUIProcessor::CreateVirtualFileMappingAroundViewHtmlContent(EngineUI::CreateVirtualFileMappingAroundViewHtmlContentNode& node)
{
    // if a directory is not specified, use the application directory (or the CSEntry directory on Android)
    const std::wstring& directory = !node.local_file_server_root_directory.empty() ? node.local_file_server_root_directory :
#ifdef WIN_DESKTOP
                                                                                     CSProExecutables::GetApplicationDirectory();
#else
                                                                                     PlatformInterface::GetInstance()->GetCSEntryDirectory();
#endif

    ASSERT80(PortableFunctions::PathGetDirectory(PortableFunctions::PathEnsureTrailingSlash(directory)) == PortableFunctions::PathEnsureTrailingSlash(directory));

    node.virtual_file_mapping = std::make_unique<VirtualFileMapping>(PortableLocalhost::CreateVirtualHtmlFile(directory,
        [html = std::move(node.html)]()
        {
            return html;
        }));

    return 1;
}
