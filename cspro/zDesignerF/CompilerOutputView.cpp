#include "StdAfx.h"
#include "CompilerOutputView.h"
#include <zToolsO/CSProExecutables.h>


// --------------------------------------------------------------------------
// CompilerOutputView
// --------------------------------------------------------------------------

IMPLEMENT_DYNCREATE(CompilerOutputView, LogicView)


CompilerOutputView::CompilerOutputView() noexcept
    :   m_designerDoc(nullptr)
{
}


void CompilerOutputView::OnUpdate(CView* const pSender, const LPARAM lHint, CObject* const pHint)
{
    __super::OnUpdate(pSender, lHint, pHint);

    if( lHint == Hint::DesignerDocLoaded )
        m_designerDoc = assert_cast<DesignerDoc*>(pHint);
}


void CompilerOutputView::InitializeLogicControl()
{
    m_logicCtrl->InitLogicControl(
        false, // show_line_numbers
        false, // show_error_markers
        SCLEX_NULL
    );

    // hide the caret
    m_logicCtrl->SetCaretStyle(Scintilla::CaretStyle::Invisible);

    // the text is read-only
    m_logicCtrl->SetReadOnly(TRUE);
}


void CompilerOutputView::AddLogicError(Logic::ParserMessage parser_message, std::optional<int> adjusted_line_number_for_bookmark/* = std::nullopt*/)
{
    ASSERT(!SO::ContainsNewlineCharacter(parser_message.message_text));

    m_logicErrorLocations.emplace_back(LogicErrorLocation { std::move(parser_message), std::move(adjusted_line_number_for_bookmark) });
}


void CompilerOutputView::ClearLogicErrors()
{
    m_logicErrorLocations.clear();
}


void CompilerOutputView::ProcessLButtonDblClk(const bool control_pressed)
{
    // process the error or warning message that was double-clicked
    const Sci_Position nPos = m_logicCtrl->GetCurrentPos();
    const size_t logic_error_index = static_cast<size_t>(m_logicCtrl->LineFromPosition(nPos));

    // clear whatever was selected by the double-click
    m_logicCtrl->ClearSelections();

    if( logic_error_index >= m_logicErrorLocations.size() )
        return;

    const LogicErrorLocation& logic_error_location = m_logicErrorLocations[logic_error_index];

    // if the control button was held down when double-clicking on an error or warning message in an external file, open the file in CSCode
    if( control_pressed && !logic_error_location.parser_message.compilation_unit_name.empty() )
    {
        CSProExecutables::RunProgramOpeningFile(
            CSProExecutables::Program::CSCode,
            logic_error_location.parser_message.compilation_unit_name
        );
    }

    // let the designer handle processing the error or warning,
    // which could be in main logic, external code, or a CAPI condition/fill
    else
    {
        // create a copy of the struct because going to the logic error may call
        // ClearLogicErrors, which will reset the errors
        const CompilerOutputView::LogicErrorLocation logic_error_location_copy = logic_error_location;
        WindowsDesktopMessage::Send(UWM::Designer::GoToLogicError, m_designerDoc, &logic_error_location_copy);
    }
}



// --------------------------------------------------------------------------
// CompilerOutputView::DoubleClickProcessingLogicCtrl
// --------------------------------------------------------------------------

class CompilerOutputView::DoubleClickProcessingLogicCtrl : public LogicCtrl
{
public:
    DoubleClickProcessingLogicCtrl(CompilerOutputView& compiler_output_view)
        :   m_compilerOutputView(compiler_output_view)
    {
    }

    bool ProcessClicksForReferenceWindow() const override
    {
        return false;
    }

protected:
    DECLARE_MESSAGE_MAP()

    void OnLButtonDblClk(UINT nFlags, CPoint point)
    {
        const bool control_pressed = ( GetKeyState(VK_CONTROL) < 0 );
        __super::OnLButtonDblClk(nFlags, point);
        m_compilerOutputView.ProcessLButtonDblClk(control_pressed);
    }

private:
    CompilerOutputView& m_compilerOutputView;
};


BEGIN_MESSAGE_MAP(CompilerOutputView::DoubleClickProcessingLogicCtrl, LogicCtrl)
    ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()


std::unique_ptr<LogicCtrl> CompilerOutputView::CreateLogicCtrl()
{
    return std::make_unique<DoubleClickProcessingLogicCtrl>(*this);
}
