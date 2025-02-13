#include "StdAfx.h"
#include "BuildWnd.h"
#include <zJson/JsonNode.h>
#include <zLogicO/ParserMessage.h>
#include <zEdit2O/ReadOnlyEditCtrl.h>


// --------------------------------------------------------------------------
// BuildWndReadOnlyEditCtrl
//
// this class handles mouse double-clicks to facilicate the action of going
// to the line of an error/warning when double-clicking on the message
// --------------------------------------------------------------------------

class BuildWndReadOnlyEditCtrl : public ReadOnlyEditCtrl
{
public:
    BuildWndReadOnlyEditCtrl(BuildWnd& build_wnd);

protected:
    DECLARE_MESSAGE_MAP()

    void OnLButtonDblClk(UINT nFlags, CPoint point);

private:
    BuildWnd& m_buildWnd;
};


BEGIN_MESSAGE_MAP(BuildWndReadOnlyEditCtrl, ReadOnlyEditCtrl)
    ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()


BuildWndReadOnlyEditCtrl::BuildWndReadOnlyEditCtrl(BuildWnd& build_wnd)
    :   m_buildWnd(build_wnd)
{
}


void BuildWndReadOnlyEditCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    __super::OnLButtonDblClk(nFlags, point);

    // get the line number and then clear whatever was selected by the double-click
    const Sci_Position nPos = GetCurrentPos();
    const int build_wnd_line_number_base0 = LineFromPosition(nPos);

    ClearSelections();

    m_buildWnd.ProgressMessageClick(build_wnd_line_number_base0);
}



// --------------------------------------------------------------------------
// BuildWnd
// --------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(BuildWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()


BuildWnd::BuildWnd()
    :   m_addNewlinesBetweenErrorsAndWarnings(false),
        m_indentMessageLinesAfterFirstLine(false),
        m_sourceLogicSource(nullptr),
        m_currentSeparatorLength(0),
        m_warningCount(0)
{
}


BuildWnd::~BuildWnd()
{
}


int BuildWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if( __super::OnCreate(lpCreateStruct) == -1 )
        return -1;

    m_editCtrl = std::make_unique<BuildWndReadOnlyEditCtrl>(*this);

    if( !m_editCtrl->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, this) )
        return -1;

	return 0;
}


void BuildWnd::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	m_editCtrl->SetWindowPos(nullptr, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
}


void BuildWnd::Initialize(CLogicCtrl* source_logic_ctrl, const std::wstring& source_title, std::wstring action, bool reset_flags/* = false*/)
{
    ASSERT(!action.empty());

    if( reset_flags )
    {
        m_addNewlinesBetweenErrorsAndWarnings = false;
        m_indentMessageLinesAfterFirstLine = false;
    }

    m_sourceLogicSource = source_logic_ctrl;
    m_sourceDocFilename.clear();
    m_currentAction = std::move(action);

    m_messageDetails.clear();
    m_errors.clear();
    m_warningCount = 0;

    // clear any markers in the source buffer
    if( source_logic_ctrl != nullptr )
        source_logic_ctrl->ClearErrorAndWarningMarkers();

    // clear the build window text
    m_editCtrl->ClearReadOnlyText();

    // add a start message
    const std::wstring action_text = m_currentAction + _T(" started: ") + source_title;

    m_currentSeparatorLength = action_text.length();

    AddInfo(SO::Concatenate(action_text, _T("\n"), SO::GetDashedLine(m_currentSeparatorLength), _T("\n")));
}


void BuildWnd::Initialize(CLogicCtrl* source_logic_ctrl, const CDocument* source_doc, std::wstring action, bool reset_flags/* = false*/)
{
    ASSERT(source_doc != nullptr);

    std::wstring filename = CS2WS(source_doc->GetPathName());

    // if there is no path, use the title (without any modified marker)
    const std::wstring source_title = filename.empty() ? SO::TrimLeft(source_doc->GetTitle(), '*') : 
                                                         filename;

    Initialize(source_logic_ctrl, source_title, std::move(action), reset_flags);

    m_sourceDocFilename = std::move(filename);
}


void BuildWnd::AddMessage(CompilerMessageType compiler_message_type, const std::wstring* filename, const std::wstring& text, int line_number_base1)
{
    ASSERT(m_editCtrl->GetLineCount() >= 1);

    // space out errors/warnings (if applicable)
    if( m_addNewlinesBetweenErrorsAndWarnings &&
        compiler_message_type != CompilerMessageType::Info &&
        ( !m_errors.empty() || m_warningCount > 0 ) )
    {
        m_editCtrl->AppendReadOnlyText(_T("\n"));
    }

    // clear the filename if it does not come from a different compilation unit
    if( filename != nullptr && ( filename->empty() || SO::EqualsNoCase(m_sourceDocFilename, *filename) ) )
        filename = nullptr;

    MessageDetails& message_details = m_messageDetails.emplace_back(
        MessageDetails
        {
            m_editCtrl->GetLineCount() - 1,
            line_number_base1 - 1,
            ( filename != nullptr ) ? *filename : std::wstring()
        });

    auto get_text_with_message_filename_prefix = [&]()
    {
        if( !message_details.filename.empty() )
            return SO::CreateColonSeparatedString(PortableFunctions::PathGetFilename(message_details.filename), text);

        return text;
    };

    // process the message
    if( compiler_message_type == CompilerMessageType::Info && line_number_base1 <= 0 )
    {
        m_editCtrl->AppendReadOnlyText(get_text_with_message_filename_prefix() + _T("\n"));
    }

    else
    {
        std::wstring type_text;

        if( compiler_message_type == CompilerMessageType::Info )
        {
            type_text = _T("INFO");
        }

        else
        {
            bool is_error = ( compiler_message_type == CompilerMessageType::Error );

            if( is_error )
            {
                type_text = _T("ERROR");
                m_errors.emplace_back(get_text_with_message_filename_prefix());
            }

            else
            {
                ASSERT(compiler_message_type == CompilerMessageType::Warning);
                type_text = _T("WARNING");
                ++m_warningCount;
            }

            // add the error/warning marker
            if( m_sourceLogicSource != nullptr && message_details.filename.empty() && line_number_base1 >= 1 )
                m_sourceLogicSource->AddErrorOrWarningMarker(is_error, line_number_base1 - 1);
        }

        const std::wstring filename_or_line_number =
            !message_details.filename.empty() ? FormatTextCS2WS(_T("(%s): "), PortableFunctions::PathGetFilename(message_details.filename)) :
            ( line_number_base1 >= 1 )        ? FormatTextCS2WS(_T("(%d): "), line_number_base1) :
                                                _T(": ");

        const std::unique_ptr<std::wstring> indented_text = m_indentMessageLinesAfterFirstLine ? GetIndentedMessageIfNecessary(text, type_text.size() + filename_or_line_number.length()) :
                                                                                                 nullptr;
        m_editCtrl->AppendReadOnlyText(SO::Concatenate(type_text,
                                                       filename_or_line_number,
                                                        ( indented_text != nullptr ) ? *indented_text : text,
                                                       _T("\n")));
    }
}


std::unique_ptr<std::wstring> BuildWnd::GetIndentedMessageIfNecessary(wstring_view text_sv, size_t indent_size/* = SO::DefaultSpacesPerTab*/)
{
    size_t first_newline_pos = text_sv.find_first_of(SO::NewlineCharacters);

    if( first_newline_pos == wstring_view::npos )
        return nullptr;

    // ignore any newline characters that preceed the actual message
    if( SO::IsWhitespace(text_sv.substr(0, first_newline_pos)) )
        first_newline_pos = text_sv.find_first_of(SO::NewlineCharacters, first_newline_pos + 1);

    if( first_newline_pos == wstring_view::npos )
        return nullptr;

    const wstring_view text_to_indent_sv = text_sv.substr(first_newline_pos);
    ASSERT(text_to_indent_sv.find_first_of(SO::NewlineCharacters) == 0);

    if( SO::IsWhitespace(text_to_indent_sv) )
        return nullptr;

    // at this point there are non-whitespace characters along with new lines
    auto indented_text = std::make_unique<std::wstring>(text_sv.substr(0, first_newline_pos));
    const TCHAR* space_string = SO::GetRepeatingCharacterString(' ', indent_size);

    SO::ForeachLine(text_to_indent_sv, true,
        [&](wstring_view line_sv)
        {
            if( !SO::IsWhitespace(line_sv) )
            {
                indented_text->append(space_string);
                indented_text->append(line_sv);
            }

            indented_text->push_back('\n');

            return true;
        });

    // remove the last-added newline
    indented_text->pop_back();

    return indented_text;
}


void BuildWnd::AddMessage(CompilerMessageType compiler_message_type, const CSProException& exception, bool dynamic_cast_exception_to_add_details)
{
    if( dynamic_cast_exception_to_add_details )
    {
        const CSProExceptionWithFilename* exception_with_filename = dynamic_cast<const CSProExceptionWithFilename*>(&exception);

        if( exception_with_filename != nullptr )
        {
            AddMessage(compiler_message_type, exception_with_filename->GetFilename(), exception_with_filename->GetErrorMessage());
            return;
        }

        const JsonParseException* json_parse_exception = dynamic_cast<const JsonParseException*>(&exception);

        if( json_parse_exception != nullptr )
        {
            AddMessage(compiler_message_type, json_parse_exception->GetErrorMessage(), json_parse_exception->GetLineNumber());
            return;
        }
    }

    AddMessage(compiler_message_type, exception.GetErrorMessage());
}


void BuildWnd::AddMessage(const Logic::ParserMessage& parser_message)
{
    const CompilerMessageType compiler_message_type = ( parser_message.type == Logic::ParserMessage::Type::Error ) ? CompilerMessageType::Error :
                                                                                                                     CompilerMessageType::Warning;
    AddMessage(compiler_message_type, parser_message.message_text, parser_message.line_number);
}


void BuildWnd::Finalize()
{
    const bool had_errors = !m_errors.empty();
    const bool had_warnings = ( m_warningCount > 0 );

    std::wstring message = had_errors ? FormatTextCS2WS(_T("\n%s\n%s failed with %d error%s"),
                                                        SO::GetDashedLine(m_currentSeparatorLength),
                                                        m_currentAction.c_str(),
                                                        static_cast<int>(m_errors.size()), PluralizeWord(m_errors.size())) :
                                        FormatTextCS2WS(_T("%s%s\n%s successful at %s"),
                                                        had_warnings ? _T("\n") : _T(""),
                                                        SO::GetDashedLine(m_currentSeparatorLength),
                                                        m_currentAction.c_str(),
                                                        CTime::GetCurrentTime().Format(_T("%X")).GetString());

    if( m_warningCount > 0 )
    {
        SO::AppendFormat(message, _T(" %s %d warning%s"), had_errors ? _T("and") : _T("with"),
                                  m_warningCount, PluralizeWord(m_warningCount));
    }

    AddInfo(message);
}


void BuildWnd::ProgressMessageClick(int build_wnd_line_number_base0)
{
    const auto& lookup = std::find_if(m_messageDetails.cbegin(), m_messageDetails.cend(),
        [&](const MessageDetails& message_details)
        {
            return ( message_details.build_wnd_line_number_base0 >= build_wnd_line_number_base0 );
        });

    if( lookup == m_messageDetails.cend() )
        return;

    // make sure that the document used for this compilation is the active document,
    // taking into account the fact that the message could have originated in a different file
    CLogicCtrl* found_logic_ctrl = ( !lookup->filename.empty() )     ? ActivateDocumentAndGetLogicCtrl(&lookup->filename) :
                                   ( m_sourceLogicSource != nullptr) ? ActivateDocumentAndGetLogicCtrl(m_sourceLogicSource) :
                                                                       nullptr;

    if( found_logic_ctrl != nullptr )
    {
        if( found_logic_ctrl == m_sourceLogicSource && lookup->compiled_buffer_line_number_base0 >= 0 )
            found_logic_ctrl->GotoLine(lookup->compiled_buffer_line_number_base0);

        found_logic_ctrl->SetFocus();
    }
}
