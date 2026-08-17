#pragma once

#include <zEngineF/zEngineF.h>
#include <zEngineF/EngineUINodes.h>
#include <zPlatformO/PlatformInterface.h>

class TraceHandler;
class Userbar;
class Viewer;


class CLASS_DECL_ZENGINEF EngineUIProcessor
{
public:
    LRESULT ProcessMessage(WPARAM wParam, LPARAM lParam);

private:
    // single implementation for Windows and Android
    static LRESULT CreateTraceHandler(std::unique_ptr<TraceHandler>& trace_handler);
    static LRESULT CreateVirtualFileMappingAroundViewHtmlContent(EngineUI::CreateVirtualFileMappingAroundViewHtmlContentNode& node);

    // platform-specific
    LRESULT CaptureImage(EngineUI::CaptureImageNode& capture_image_node);
    LRESULT ColorizeLogic(EngineUI::ColorizeLogicNode& colorize_logic_node);
    LRESULT CreateMapUI(EngineUI::CreateMapUINode& create_map_ui_node);
    LRESULT CreateUserbar(std::unique_ptr<Userbar>& userbar);
    LRESULT EditNote(EngineUI::EditNoteNode& edit_note_node);
    LRESULT ExecSystemApp(EngineUI::ExecSystemAppNode& exec_system_app_node);
    LRESULT HtmlDialogsDirectoryQuery(std::string& html_dialogs_directory);
    LRESULT Prompt(EngineUI::PromptNode& prompt_node);
    LRESULT RunPffExecutor(EngineUI::RunPffExecutorNode& run_pff_executor_node);
    LRESULT View(const Viewer& viewer);


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
LRESULT SendEngineUIMessage(EngineUI::Type engine_ui_type, T& engine_ui_node)
{
#ifdef WIN_DESKTOP
    return WindowsDesktopMessage::Send(WM_IMSA_PORTABLE_ENGINEUI, engine_ui_type, &engine_ui_node);

#else
    BaseApplicationInterface* const app_interface = PlatformInterface::GetInstance()->GetApplicationInterface();

    if( app_interface != nullptr )
        return app_interface->RunEngineUIProcessor(static_cast<WPARAM>(engine_ui_type), reinterpret_cast<LPARAM>(&engine_ui_node));

    return 0;

#endif
}
