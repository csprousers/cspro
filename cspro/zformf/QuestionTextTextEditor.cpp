#include "StdAfx.h"
#include "QuestionTextEditor.h"


// --------------------------------------------------------------------------
// QuestionTextTextEditor::CustomLogicCtrl
// --------------------------------------------------------------------------

class QuestionTextTextEditor::CustomLogicCtrl : public CLogicCtrl
{
protected:
    bool ProcessClicksForReferenceWindow() const override { return false; }

protected:
    DECLARE_MESSAGE_MAP()

    void OnContextMenu(CWnd* pWnd, CPoint point);
};


BEGIN_MESSAGE_MAP(QuestionTextTextEditor::CustomLogicCtrl, CLogicCtrl)
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()


void QuestionTextTextEditor::CustomLogicCtrl::OnContextMenu(CWnd* const pWnd, const CPoint point)
{
    // use CQSFEView's context menu when possible
    CWnd* const pParent = GetParent();

    if( ::IsWindow(pParent->GetSafeHwnd()) )
    {
        pParent->SendMessage(WM_CONTEXTMENU, reinterpret_cast<WPARAM>(m_hWnd), MAKELPARAM(point.x, point.y));
    }

    else
    {
        __super::OnContextMenu(pWnd, point);
    }
}



// --------------------------------------------------------------------------
// QuestionTextTextEditor
// --------------------------------------------------------------------------

QuestionTextTextEditor::QuestionTextTextEditor()
    :   m_logicCtrl(std::make_unique<CustomLogicCtrl>()),
        m_format(CapiText::Format::ReportMarkdown)
{
}


QuestionTextTextEditor::~QuestionTextTextEditor()
{
}


int QuestionTextTextEditor::GetLexerLanguage(const Application* const application) const
{
    ASSERT(m_format == CapiText::Format::ReportHtml || m_format == CapiText::Format::ReportMarkdown);

    const bool use_v8_0_lexers = ( application != nullptr ) ? Lexers::UseV8_0Lexers(*application) :
                                                              true;

    return ( m_format == CapiText::Format::ReportHtml ) ? ( use_v8_0_lexers ? SCLEX_CSPRO_REPORT_HTML_V8_0     : SCLEX_CSPRO_REPORT_HTML_V0 ) :
                                                          ( use_v8_0_lexers ? SCLEX_CSPRO_REPORT_MARKDOWN_V8_0 : SCLEX_CSPRO_REPORT_MARKDOWN_V0 );
}


void QuestionTextTextEditor::Initialize(CWnd* const pParent, const std::string& /*application_file_path*/)
{
    m_logicCtrl->ReplaceCEdit(pParent, true, true, GetLexerLanguage(nullptr));
}


void QuestionTextTextEditor::Destroy()
{
}


void QuestionTextTextEditor::UpdateForFormat(const Application* const application, const CapiText::Format format)
{
    m_format = format;
    m_logicCtrl->ToggleLexer(GetLexerLanguage(application));
}


bool QuestionTextTextEditor::IsDirty()
{
    return m_logicCtrl->IsModified();
}


void QuestionTextTextEditor::UpdateFillErrorDisplay(const std::map<std::string, CapiEditorViewModel::SyntaxCheckResult>& /*fill_syntax_check_results*/)
{
    // MARKDOWN_TODO ?
}


bool QuestionTextTextEditor::HasContent()
{
    return ( m_logicCtrl->GetTextLength() > 0 );
}


SharableString QuestionTextTextEditor::GetContent()
{
    return m_logicCtrl->GetText();
}


void QuestionTextTextEditor::ClearContent()
{
    m_logicCtrl->ClearAll();
    m_logicCtrl->SetSavePoint();
    m_logicCtrl->SetModified(false);
}


void QuestionTextTextEditor::SetContent(const SharableString& text)
{
    m_logicCtrl->SetTextAndSetSavePoint(text.GetString());
    m_logicCtrl->SetModified(false);
}


void QuestionTextTextEditor::Copy()
{
    m_logicCtrl->CopySelection();
}


bool QuestionTextTextEditor::CanCopy()
{
    return !m_logicCtrl->GetSelectionEmpty();
}


void QuestionTextTextEditor::Cut()
{
    m_logicCtrl->Cut();
}


bool QuestionTextTextEditor::CanCut()
{
    return !m_logicCtrl->GetSelectionEmpty();
}


void QuestionTextTextEditor::Paste(const bool with_formatting)
{
    ASSERT(with_formatting);

    return m_logicCtrl->Paste();
}


bool QuestionTextTextEditor::CanPaste()
{
    return IsClipboardFormatAvailable(CF_TEXT);
}


void QuestionTextTextEditor::SelectAll()
{
    m_logicCtrl->SelectAll();
}


void QuestionTextTextEditor::Undo()
{
    m_logicCtrl->Undo();
}


void QuestionTextTextEditor::Redo()
{
    m_logicCtrl->Redo();
}


void QuestionTextTextEditor::Bold()
{
    // MARKDOWN_TODO
}


void QuestionTextTextEditor::Italic()
{
    // MARKDOWN_TODO
}


void QuestionTextTextEditor::Underline()
{
    // MARKDOWN_TODO
}


void QuestionTextTextEditor::SetForeColor(const COLORREF color)
{
    // MARKDOWN_TODO
}


void QuestionTextTextEditor::UnorderedList()
{
    // MARKDOWN_TODO
}


void QuestionTextTextEditor::OrderedList()
{
    // MARKDOWN_TODO
}


void QuestionTextTextEditor::InsertImage(const std::string& image_url)
{
    // MARKDOWN_TODO
}


void QuestionTextTextEditor::InsertTable(const int rows, const int columns)
{
    // MARKDOWN_TODO
}


void QuestionTextTextEditor::InsertLink(const std::string& text, const std::string& url)
{
    // MARKDOWN_TODO
}
