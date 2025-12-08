#include "StdAfx.h"
#include "EngineUI.h"


EngineUIProcessor::EngineUIProcessor(BaseApplicationInterface& base_application_interface)
    :   m_baseApplicationInterface(base_application_interface)
{
}


LRESULT EngineUIProcessor::CaptureImage(EngineUI::CaptureImageNode& capture_image_node)
{
    return m_baseApplicationInterface.CaptureImage(capture_image_node);
}


LRESULT EngineUIProcessor::ColorizeLogic(EngineUI::ColorizeLogicNode& /*colorize_logic_node*/)
{
    return 0;
}


LRESULT EngineUIProcessor::CreateMapUI(EngineUI::CreateMapUINode& create_map_ui_node)
{
    m_baseApplicationInterface.CreateMapUI(create_map_ui_node);
    return ( create_map_ui_node.map_ui != nullptr );
}


LRESULT EngineUIProcessor::CreateUserbar(std::unique_ptr<Userbar>& userbar)
{
    m_baseApplicationInterface.CreateUserbar(userbar);
    return ( userbar != nullptr );
}


LRESULT EngineUIProcessor::EditNote(EngineUI::EditNoteNode& edit_note_node)
{
    edit_note_node.note = m_baseApplicationInterface.EditNote(edit_note_node.note, edit_note_node.title, edit_note_node.case_note);
    return 1;
}


LRESULT EngineUIProcessor::ExecSystemApp(EngineUI::ExecSystemAppNode& exec_system_app_node)
{
    return m_baseApplicationInterface.ExecSystemApp(exec_system_app_node);
}


LRESULT EngineUIProcessor::HtmlDialogsDirectoryQuery(std::string& html_dialogs_directory)
{
    html_dialogs_directory = m_baseApplicationInterface.GetHtmlDialogsDirectory();
    return 1;
}


LRESULT EngineUIProcessor::Prompt(EngineUI::PromptNode& prompt_node)
{
    m_baseApplicationInterface.Prompt(prompt_node);
    return 1;
}


LRESULT EngineUIProcessor::RunPffExecutor(EngineUI::RunPffExecutorNode& run_pff_executor_node)
{
    return m_baseApplicationInterface.RunPffExecutor(run_pff_executor_node);
}


LRESULT EngineUIProcessor::View(const Viewer& viewer)
{
    return m_baseApplicationInterface.View(viewer);
}
