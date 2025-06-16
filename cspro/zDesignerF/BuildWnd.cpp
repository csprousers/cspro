#include "StdAfx.h"
#include "BuildWnd.h"
#include <zToolsO/Utf8.h>
#include <zJson/JsonNode.h>
#include <zLogicO/ParserMessage.h>
#include <zEditO/ReadOnlyEditCtrl.h>


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


void BuildWndReadOnlyEditCtrl::OnLButtonDblClk(const UINT nFlags, const CPoint point)
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
        m_currentSeparatorWideLength(0),
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


void BuildWnd::OnSize(const UINT nType, const int cx, const int cy)
{
    __super::OnSize(nType, cx, cy);

    m_editCtrl->SetWindowPos(nullptr, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
}


void BuildWnd::Initialize(CLogicCtrl* const source_logic_ctrl, const std::string& source_title, std::string action, const bool reset_flags/* = false*/)
{
    ASSERT(!action.empty());

    if( reset_flags )
    {
        m_addNewlinesBetweenErrorsAndWarnings = false;
        m_indentMessageLinesAfterFirstLine = false;
    }

    m_sourceLogicSource = source_logic_ctrl;
    m_sourceDocFilePath.clear();
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
    const std::string action_text = SO::Concatenate(m_currentAction, " started: ", source_title);

    m_currentSeparatorWideLength = SO::WideLength(action_text);

    AddInfo(SO::Concatenate(action_text, "\n", SO::GetDashedLine(m_currentSeparatorWideLength), "\n"));
}


void BuildWnd::Initialize(CLogicCtrl* const source_logic_ctrl, const CDocument* const source_doc, std::string action, const bool reset_flags/* = false*/)
{
    ASSERT(source_doc != nullptr);

    std::string file_path = TC::ToUtf8(source_doc->GetPathName());

    // if there is no path, use the title (without any modified marker)
    const std::string source_title = file_path.empty() ? std::string(SO::TrimLeft(TC::ToUtf8(source_doc->GetTitle()), '*')) :
                                                         file_path;

    Initialize(source_logic_ctrl, source_title, std::move(action), reset_flags);

    m_sourceDocFilePath = std::move(file_path);
}


void BuildWnd::AddMessage(const CompilerMessageType compiler_message_type, const std::string* file_path, const std::string& text, const int line_number_base1)
{
    ASSERT(m_editCtrl->GetLineCount() >= 1);

    // space out errors/warnings (if applicable)
    if( m_addNewlinesBetweenErrorsAndWarnings &&
        compiler_message_type != CompilerMessageType::Info &&
        ( !m_errors.empty() || m_warningCount > 0 ) )
    {
        m_editCtrl->AppendReadOnlyText("\n");
    }

    // clear the filename if it does not come from a different compilation unit
    if( file_path != nullptr && ( file_path->empty() || SO::EqualsNoCase(m_sourceDocFilePath, *file_path) ) )
        file_path = nullptr;

    MessageDetails& message_details = m_messageDetails.emplace_back(
        MessageDetails
        {
            m_editCtrl->GetLineCount() - 1,
            line_number_base1 - 1,
            ( file_path != nullptr ) ? *file_path : std::string()
        });

    auto get_text_with_message_filename_prefix = [&]()
    {
        if( !message_details.file_path.empty() )
            return SO::CreateColonSeparatedString(PortableFunctions::PathGetFilename(message_details.file_path), text);

        return text;
    };

    // process the message
    if( compiler_message_type == CompilerMessageType::Info && line_number_base1 <= 0 )
    {
        m_editCtrl->AppendReadOnlyText(get_text_with_message_filename_prefix() + "\n");
    }

    else
    {
        std::string type_text;

        if( compiler_message_type == CompilerMessageType::Info )
        {
            type_text = "INFO";
        }

        else
        {
            const bool is_error = ( compiler_message_type == CompilerMessageType::Error );

            if( is_error )
            {
                type_text = "ERROR";
                m_errors.emplace_back(get_text_with_message_filename_prefix());
            }

            else
            {
                ASSERT(compiler_message_type == CompilerMessageType::Warning);
                type_text = "WARNING";
                ++m_warningCount;
            }

            // add the error/warning marker
            if( m_sourceLogicSource != nullptr && message_details.file_path.empty() && line_number_base1 >= 1 )
                m_sourceLogicSource->AddErrorOrWarningMarker(is_error, line_number_base1 - 1);
        }

        const std::string file_path_or_line_number =
            !message_details.file_path.empty() ? FormatText("(%s): ", PortableFunctions::PathGetFilename(message_details.file_path).c_str()) :
            ( line_number_base1 >= 1 )         ? FormatText("(%d): ", line_number_base1) :
                                                 ": ";

        const std::unique_ptr<std::string> indented_text = m_indentMessageLinesAfterFirstLine ? GetIndentedMessageIfNecessary(text, file_path_or_line_number, type_text.size()) :
                                                                                                nullptr;
        m_editCtrl->AppendReadOnlyText(SO::Concatenate(type_text,
                                                       file_path_or_line_number,
                                                       ( indented_text != nullptr ) ? *indented_text : text,
                                                       "\n"));
    }
}


std::unique_ptr<std::string> BuildWnd::GetIndentedMessageIfNecessary(const std::string_view text_sv, const std::string& indent_text1, const size_t intent_text2_length)
{
    size_t first_newline_pos = text_sv.find_first_of(SO::Newline_crlf_sv);

    if( first_newline_pos == std::string_view::npos )
        return nullptr;

    // ignore any newline characters that preceed the actual message
    if( SO::IsWhitespace(text_sv.substr(0, first_newline_pos)) )
        first_newline_pos = text_sv.find_first_of(SO::Newline_crlf_sv, first_newline_pos + 1);

    if( first_newline_pos == wstring_view::npos )
        return nullptr;

    const std::string_view text_to_indent_sv = text_sv.substr(first_newline_pos);
    ASSERT(text_to_indent_sv.find_first_of(SO::Newline_crlf_sv) == 0);

    if( SO::IsWhitespace(text_to_indent_sv) )
        return nullptr;

    // at this point there are non-whitespace characters along with new lines
    auto indented_text = std::make_unique<std::string>(text_sv.substr(0, first_newline_pos));
    const size_t indent_size = SO::WideLength(indent_text1) + intent_text2_length;
    const std::string_view space_string_sv = SO::GetRepeatingCharacterString(' ', indent_size);

    SO::ForeachLine(text_to_indent_sv, true,
        [&](const std::string_view line_sv)
        {
            if( !SO::IsWhitespace(line_sv) )
            {
                indented_text->append(space_string_sv);
                indented_text->append(line_sv);
            }

            indented_text->push_back('\n');
        });

    // remove the last-added newline
    indented_text->pop_back();

    return indented_text;
}


void BuildWnd::AddMessage(const CompilerMessageType compiler_message_type, const CSProException& exception, const bool dynamic_cast_exception_to_add_details)
{
    if( dynamic_cast_exception_to_add_details )
    {
        const CSProExceptionWithFilePath* const exception_with_file_path = dynamic_cast<const CSProExceptionWithFilePath*>(&exception);

        if( exception_with_file_path != nullptr )
        {
            AddMessage(compiler_message_type, exception_with_file_path->GetFilePath(), exception_with_file_path->what());
            return;
        }

        const JsonParseException* const json_parse_exception = dynamic_cast<const JsonParseException*>(&exception);

        if( json_parse_exception != nullptr )
        {
            AddMessage(compiler_message_type, json_parse_exception->what(), json_parse_exception->GetLineNumber());
            return;
        }
    }

    AddMessage(compiler_message_type, exception.what());
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

    std::string message = had_errors ? FormatText("\n%s\n%s failed with %d error%s",
                                                  SO::GetDashedLine(m_currentSeparatorWideLength),
                                                  m_currentAction.c_str(),
                                                  static_cast<int>(m_errors.size()), PluralizeWord(m_errors.size())) :
                                       FormatText("%s%s\n%s successful at %s",
                                                  had_warnings ? "\n" : "",
                                                  SO::GetDashedLine(m_currentSeparatorWideLength),
                                                  m_currentAction.c_str(),
                                                  UTF8_TODO::GetUtf8(CTime::GetCurrentTime().Format(_T("%X"))).c_str());

    if( m_warningCount > 0 )
    {
        message.append(FormatText(" %s %d warning%s",
                                  had_errors ? "and" : "with",
                                  m_warningCount, PluralizeWord(m_warningCount)));
    }

    AddInfo(message);
}


void BuildWnd::ProgressMessageClick(const int build_wnd_line_number_base0)
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
    CLogicCtrl* const found_logic_ctrl = ( !lookup->file_path.empty() )    ? ActivateDocumentAndGetLogicCtrl(&lookup->file_path) :
                                         ( m_sourceLogicSource != nullptr) ? ActivateDocumentAndGetLogicCtrl(m_sourceLogicSource) :
                                                                             nullptr;

    if( found_logic_ctrl != nullptr )
    {
        if( found_logic_ctrl == m_sourceLogicSource && lookup->compiled_buffer_line_number_base0 >= 0 )
            found_logic_ctrl->GotoLine(lookup->compiled_buffer_line_number_base0);

        found_logic_ctrl->SetFocus();
    }
}
