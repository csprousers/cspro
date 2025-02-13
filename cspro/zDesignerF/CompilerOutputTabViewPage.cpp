#include "StdAfx.h"
#include "CompilerOutputTabViewPage.h"
#include <zUtilO/CSProExecutables.h>


BEGIN_MESSAGE_MAP(CompilerOutputTabViewPage, COXTabViewPage<ReadOnlyEditCtrl>)
    ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()


CompilerOutputTabViewPage::CompilerOutputTabViewPage(ApplicationChildWnd* application_child_wnd)
    :   m_applicationChildWnd(application_child_wnd)
{
    ASSERT(application_child_wnd != nullptr);
}


void CompilerOutputTabViewPage::AddLogicError(Logic::ParserMessage parser_message, std::optional<int> adjusted_line_number_for_bookmark/* = std::nullopt*/)
{
    ASSERT(!SO::ContainsNewlineCharacter(parser_message.message_text));

    m_logicErrorLocations.emplace_back(LogicErrorLocation { std::move(parser_message), std::move(adjusted_line_number_for_bookmark) });
}

void CompilerOutputTabViewPage::ClearLogicErrors()
{
    m_logicErrorLocations.clear();
}


void CompilerOutputTabViewPage::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    const bool control_pressed = ( GetKeyState(VK_CONTROL) < 0 );

    ReadOnlyEditCtrl::OnLButtonDblClk(nFlags, point);

    // process the error or warning message that was double-clicked
    const Sci_Position nPos = GetCurrentPos();
    const size_t logic_error_index = static_cast<size_t>(LineFromPosition(nPos));

    // clear whatever was selected by the double-click
    ClearSelections();

    if( logic_error_index >= m_logicErrorLocations.size() )
        return;

    const LogicErrorLocation& logic_error_location = m_logicErrorLocations[logic_error_index];

    // if the control button was held down when double-clicking on an error or warning message in an external file, open the file in CSCode
    if( control_pressed && !logic_error_location.parser_message.compilation_unit_name.empty() )
    {
        CSProExecutables::RunProgramOpeningFile(CSProExecutables::Program::CSCode, logic_error_location.parser_message.compilation_unit_name);
    }

    // let the designer handle processing the error or warning, which could be in main logic, external code, or a CAPI condition/fill
    // (create a copy of the struct because going to the logic error may call ClearLogicErrors, which will reset the errors)
    else
    {
        const CompilerOutputTabViewPage::LogicErrorLocation logic_error_location_copy = logic_error_location;
        WindowsDesktopMessage::Send(UWM::Designer::GoToLogicError, m_applicationChildWnd, &logic_error_location_copy);
    }
}
