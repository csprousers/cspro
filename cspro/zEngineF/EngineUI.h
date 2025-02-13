#pragma once

#include <zEngineF/zEngineF.h>
#include <zEngineF/EngineUINodes.h>
#include <zPlatformO/PlatformInterface.h>

struct IMapUI;
class Userbar;
class Viewer;


class CLASS_DECL_ZENGINEF EngineUIProcessor
{
public:
    long ProcessMessage(WPARAM wParam, LPARAM lParam);

private:
    // single implementation for Windows and Android
    long CreateVirtualFileMappingAroundViewHtmlContent(EngineUI::CreateVirtualFileMappingAroundViewHtmlContentNode& node);

    // platform-specific
    long CaptureImage(EngineUI::CaptureImageNode& capture_image_node);
    long ColorizeLogic(EngineUI::ColorizeLogicNode& colorize_logic_node);
    long CreateMapUI(std::unique_ptr<IMapUI>& map_ui);
    long CreateUserbar(std::unique_ptr<Userbar>& userbar);
    long EditNote(EngineUI::EditNoteNode& edit_note_node);
    long ExecSystemApp(EngineUI::ExecSystemAppNode& exec_system_app_node);
    long HtmlDialogsDirectoryQuery(std::wstring& html_dialogs_directory);
    long Prompt(EngineUI::PromptNode& prompt_node);
    long RunPffExecutor(EngineUI::RunPffExecutorNode& run_pff_executor_node);
    long View(const Viewer& viewer);


    // platform-specific constructors and data
#ifdef WIN_DESKTOP

public:
    EngineUIProcessor(const PFF* pff, bool engine_runs_on_ui_thread);

private:
    const PFF* const m_pff;
    const bool m_engineRunsOnUIThread;

#else

public:
    EngineUIProcessor(BaseApplicationInterface& base_application_interface);

private:
    BaseApplicationInterface& m_baseApplicationInterface;

#endif
};



template<typename T>
long SendEngineUIMessage(EngineUI::Type engine_ui_type, T& engine_ui_node)
{
#ifdef WIN_DESKTOP
    return WindowsDesktopMessage::Send(WM_IMSA_PORTABLE_ENGINEUI, engine_ui_type, &engine_ui_node);

#else
    BaseApplicationInterface* app_interface = PlatformInterface::GetInstance()->GetApplicationInterface();

    if( app_interface != nullptr )
        return app_interface->RunEngineUIProcessor(static_cast<WPARAM>(engine_ui_type), reinterpret_cast<LPARAM>(&engine_ui_node));

    return 0;

#endif
}
