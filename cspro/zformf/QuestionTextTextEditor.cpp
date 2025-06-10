#include "StdAfx.h"
#include "QuestionTextEditor.h"
#include <zToolsO/Encoders.h>


// --------------------------------------------------------------------------
// QuestionTextTextEditor::CustomLogicCtrl
// --------------------------------------------------------------------------

class QuestionTextTextEditor::CustomLogicCtrl : public CLogicCtrl
{
public:
    void ClearSelectionOnSetFocus() { m_clearSelectionOnSetFocus = true; }

protected:
    bool ProcessClicksForReferenceWindow() const override { return false; }

protected:
    DECLARE_MESSAGE_MAP()

    void OnSetFocus(CWnd* pOldWnd);
    void OnContextMenu(CWnd* pWnd, CPoint point);

private:
    bool m_clearSelectionOnSetFocus = false;
};


BEGIN_MESSAGE_MAP(QuestionTextTextEditor::CustomLogicCtrl, CLogicCtrl)
    ON_WM_SETFOCUS()
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()


void QuestionTextTextEditor::CustomLogicCtrl::OnSetFocus(CWnd* const pOldWnd)
{
    __super::OnSetFocus(pOldWnd);

    if( m_clearSelectionOnSetFocus )
    {
        GotoPos(0);
        m_clearSelectionOnSetFocus = false;
    }
}


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
    ASSERT(EditingHtml() || EditingMarkdown());

    const bool use_v8_0_lexers = ( application != nullptr ) ? Lexers::UseV8_0Lexers(*application) :
                                                              true;

    return EditingHtml() ? ( use_v8_0_lexers ? SCLEX_CSPRO_REPORT_HTML_V8_0     : SCLEX_CSPRO_REPORT_HTML_V0 ) :
                           ( use_v8_0_lexers ? SCLEX_CSPRO_REPORT_MARKDOWN_V8_0 : SCLEX_CSPRO_REPORT_MARKDOWN_V0 );
}


void QuestionTextTextEditor::Initialize(CWnd* const pParent, const std::string& /*application_file_path*/)
{
    m_logicCtrl->ReplaceCEdit(pParent, true, true, GetLexerLanguage(nullptr));

    // turn off modifications for many events to prevent CQSFEView::OnEditorChangeText
    // from being called for events such as clearing markers
    m_logicCtrl->SetModEventMask(Scintilla::ModificationFlags::InsertText |
                                 Scintilla::ModificationFlags::DeleteText);

    // errors will be reported using annotations
    m_logicCtrl->StyleSetFore(SCE_CSPRO_ERROR_ANNOTATION, RGB(150, 0, 64));
    m_logicCtrl->StyleSetBack(SCE_CSPRO_ERROR_ANNOTATION, RGB(255, 240, 240));
    m_logicCtrl->AnnotationSetVisible(Scintilla::AnnotationVisible::Boxed);
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


void QuestionTextTextEditor::ClearCompilationResults()
{
    m_logicCtrl->AnnotationClearAll();
}


void QuestionTextTextEditor::CompileFillsAndLogic(CapiEditorViewModel& view_model, const CapiText& capi_text)
{
    QuestionTextTextEditor::ClearCompilationResults();

    try
    {
        std::optional<CapiEditorViewModel::SyntaxCheckError> check_errors = view_model.CheckSyntax(&capi_text);

        if( !check_errors.has_value() )
            return;

        // sort the errors by line number
        if( check_errors->size() > 1 )
        {
            std::sort(check_errors->begin(), check_errors->end(),
                [&](const Logic::ParserMessage& pm1, const Logic::ParserMessage& pm2)
                {
                    return ( pm1.line_number < pm2.line_number );
                });
        }

        // add annotations, grouping all errors per-line into a single annotation
        auto parser_messages_itr = check_errors->cbegin();
        const auto& parser_messages_end = check_errors->cend();
        ASSERT(parser_messages_itr != parser_messages_end);
        bool additional_messages_exist = false;

        do
        {
            const size_t line_number = parser_messages_itr->line_number;
            const std::string& first_message_text = parser_messages_itr->message_text;
            std::string message_text = first_message_text;

            while( ( additional_messages_exist = ( ++parser_messages_itr != parser_messages_end ) ) &&
                   ( line_number == parser_messages_itr->line_number ) )
            {
                // don't add duplicate messages
                if( parser_messages_itr->message_text != first_message_text )
                {
                    message_text.push_back('\n');
                    message_text.append(parser_messages_itr->message_text);
                }
            }

            m_logicCtrl->AnnotationSetStyle(line_number - 1, SCE_CSPRO_ERROR_ANNOTATION);
            m_logicCtrl->AnnotationSetText(line_number - 1, message_text.c_str());

        } while( additional_messages_exist );
    }
    catch(...) { ASSERT(false); }
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
    m_logicCtrl->EmptyUndoBuffer();
    m_logicCtrl->SetModified(false);

    // this is a solution to a problem described by ChatGPT as:
    //
    //   If you set text via SCI_SETTEXT while the control does not have focus, and then
    //   the user clicks on the control for the first time, Scintilla will often select
    //   all the text when it receives focus.
    //
    //   Later, when the user clicks the control to give it focus:
    //     * Scintilla interprets the mouse event as “focus + click”
    //     * If there’s a full range selected (0 to length), it may treat it as a
    //       focus event and auto-select the entire line or buffer
    //
    //   This is similar to how some single-line edit controls select-all-on-focus to
    //   make editing easier — but it's not always desirable in a code editor scenario.
    assert_cast<CustomLogicCtrl&>(*m_logicCtrl).ClearSelectionOnSetFocus();
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
    EditingHtml() ? WrapSelection("<strong>", "</strong>") :
                    WrapSelection("**", "**");
}


void QuestionTextTextEditor::Italic()
{
    EditingHtml() ? WrapSelection("<em>", "</em>") :
                    WrapSelection("*", "*");
}


void QuestionTextTextEditor::Underline()
{
    WrapSelection("<u>", "</u>");
}


void QuestionTextTextEditor::SetForeColor(const COLORREF color)
{
    const std::string color_text = PortableColor::FromCOLORREF(color).ToString();
    WrapSelection(FormatText("<span style=\"color:%s\">", color_text.c_str()), "</span>");
}


void QuestionTextTextEditor::UnorderedList()
{
    EditingHtml() ? WrapSelection("<ul>\n    <li>", "</li>\n</ul>") :
                    WrapSelection("- ", nullptr);
}


void QuestionTextTextEditor::OrderedList()
{
    EditingHtml() ? WrapSelection("<ol>\n    <li>", "</li>\n</ol>") :
                    WrapSelection("1. ", nullptr);
}


void QuestionTextTextEditor::InsertImage(const std::string& image_url)
{
    if( EditingHtml() )
    {
        WrapSelection(FormatText("<img src=\"%s\">", Encoders::ToHtmlTagValue(image_url).c_str()), nullptr);
    }

    else
    {
        WrapSelection(FormatText("![](%s)", ToMarkdownUrl(image_url).c_str()), nullptr);
    }
}


void QuestionTextTextEditor::InsertTable(const int rows, const int columns)
{
    // for HTML, match what Summernote creates
    if( EditingHtml() )
    {
        std::string html = "<table class=\"table table-bordered\">\n    <tbody>\n";

        for( int r = 0; r < rows; ++r )
        {
            html.append("        <tr>");

            for( int c = 0; c < columns; ++c )
                html.append("<td> </td>");

            html.append("</tr>\n");
        }

        html.append("    </tbody>\n</table>");

        WrapSelection(html, nullptr);
    }

    // for Markdown, an additional row will be added for the header
    else
    {
        auto create_row = [&](const char* const cell_text)
        {
            std::string row_markdown = "| ";

            for( int c = 0; c < columns; ++c )
                row_markdown.append(cell_text).append(" | ");

            row_markdown.back() = '\n';

            return row_markdown;
        };

        const std::string row_markdown = create_row("   ");

        std::string markdown = row_markdown + create_row("---");

        for( int r = 0; r < rows; ++r )
            markdown.append(row_markdown);

        WrapSelection(markdown, nullptr);
    }
}


void QuestionTextTextEditor::InsertLink(const std::string& text, const std::string& url)
{
    if( EditingHtml() )
    {
        const std::string link = FormatText("<a href=\"%s\">%s</a>", url.c_str(),
                                                                     Encoders::ToHtml(text).c_str());
        WrapSelection(link, nullptr);
    }

    else
    {
        const std::string link = FormatText("[%s](%s)", Encoders::ToMarkdown(text).c_str(),
                                                        ToMarkdownUrl(url).c_str());
        WrapSelection(link, nullptr);
    }
}


std::string QuestionTextTextEditor::ToMarkdownUrl(std::string url)
{
    // because ) closes the URL, replace it with its percent-encoded equivalent
    return SO::Replace(url, ")", "%29");
}


void QuestionTextTextEditor::WrapSelection(const cs::string_view_sz start_text_sv, const char* const end_text)
{
    const Sci_Position start_pos = m_logicCtrl->GetSelectionStart();
    const Sci_Position end_pos = m_logicCtrl->GetSelectionEnd();

    m_logicCtrl->BeginUndoAction();

    if( end_text != nullptr )
        m_logicCtrl->InsertText(end_pos, end_text);

    m_logicCtrl->InsertText(start_pos, start_text_sv.c_str());
    m_logicCtrl->EndUndoAction();

    // adjust the selection to account for the inserted start text
    m_logicCtrl->SetSelection(start_pos + start_text_sv.length(), end_pos + start_text_sv.length());
}
