#include "stdafx.h"
#include "BasicTokenCompiler.h"
#include "ProcDirectory.h"
#include <zUtilO/StdioFileUnicode.h>
#include <zUtilO/TextSource.h>
#include <zEngineO/Messages/EngineMessages.h>


using namespace Logic;


BasicTokenCompiler::BasicTokenCompiler()
{
    ClearSourceBuffer();
}


void BasicTokenCompiler::ClearSourceBuffer()
{
    m_basicTokens.clear();
    m_markIndices.clear();
    m_nextBasicTokenIndex = 0;

    m_compilationUnitName.clear();
    m_capiLogicLocation.reset();
    m_sourceBuffer.reset();
}


void BasicTokenCompiler::SetSourceBuffer(const TextSource& source_text_source)
{
    SetSourceBuffer(std::make_shared<SourceBuffer>(source_text_source.GetText()));
    m_compilationUnitName = source_text_source.GetFilename();
    m_capiLogicLocation.reset();
}


void BasicTokenCompiler::SetSourceBuffer(std::shared_ptr<SourceBuffer> source_buffer, const ProcDirectoryEntry* proc_directory_entry/* = nullptr*/)
{
    ClearSourceBuffer();

    m_sourceBuffer = std::move(source_buffer);

    const std::vector<BasicToken>& basic_tokens = m_sourceBuffer->Tokenize(GetLogicSettings());
    std::vector<BasicToken>::const_iterator range_beginning = basic_tokens.cbegin();
    std::vector<BasicToken>::const_iterator range_ending = basic_tokens.cend();

    if( proc_directory_entry != nullptr )
    {
        range_beginning += proc_directory_entry->first_basic_token_index;
        range_ending = range_beginning + proc_directory_entry->number_basic_tokens;
    }

    m_basicTokens = std::vector<BasicToken>(range_beginning, range_ending);
}


void BasicTokenCompiler::SetCompilationUnitName(std::wstring name)
{
    m_compilationUnitName = std::move(name);
    m_capiLogicLocation.reset();
}


void BasicTokenCompiler::SetCapiLogicLocation(CapiLogicLocation capi_logic_location)
{
    m_compilationUnitName = _T("<CAPI Text>");
    m_capiLogicLocation = std::move(capi_logic_location);
}


void BasicTokenCompiler::MarkInputBufferToRestartLater()
{
    m_markIndices.emplace_back(m_nextBasicTokenIndex);
}


void BasicTokenCompiler::RestartFromMarkedInputBuffer()
{
    ASSERT(!m_markIndices.empty());
    m_nextBasicTokenIndex = m_markIndices.back();
    m_markIndices.pop_back();
}


void BasicTokenCompiler::ClearMarkedInputBuffer()
{
    ASSERT(!m_markIndices.empty());
    m_markIndices.pop_back();
}


void BasicTokenCompiler::MoveNextBasicTokenIndex(int offset_from_next_token_index)
{
    const size_t offset = m_nextBasicTokenIndex + offset_from_next_token_index;

    if( offset < m_basicTokens.size() )
    {
        m_nextBasicTokenIndex = offset;
    }

    else
    {
        ASSERT(false);
    }
}


const BasicToken* BasicTokenCompiler::GetBasicTokenFromOffset(int offset_from_next_token_index) const
{
    const size_t offset = m_nextBasicTokenIndex + offset_from_next_token_index;
    return ( offset < m_basicTokens.size() ) ? &m_basicTokens[offset] : nullptr;
}


size_t BasicTokenCompiler::GetCurrentBasicTokenLineNumber() const
{
    const BasicToken* basic_token = GetCurrentBasicToken();
    return ( basic_token != nullptr ) ? basic_token->line_number : 1;
}


// the most straightforward next token reader
const BasicToken* BasicTokenCompiler::NextBasicToken()
{
    BasicToken* basic_token = nullptr;

    if( m_nextBasicTokenIndex < m_basicTokens.size() )
    {
        basic_token = &m_basicTokens[m_nextBasicTokenIndex];
        ++m_nextBasicTokenIndex;

        // check for unbalanced comments
        if( basic_token->type == BasicToken::Type::UnbalancedComment )
        {
            const bool is_start_comment = ( basic_token->GetTextSV() == GetLogicSettings().GetMultilineCommentStart() );
            IssueError(MGF::unbalanced_multiline_comment_92180, is_start_comment ? _T("start") : _T("end"), basic_token->GetText().c_str());
        }
    }

    return basic_token;
}


template<typename T/* = const TCHAR**/>
size_t BasicTokenCompiler::NextKeyword(const std::vector<T>& keywords)
{
    const BasicToken* basic_token = NextBasicToken();

    if( basic_token != nullptr )
    {
        wstring_view token_text_sv = basic_token->GetTextSV();

        // check if the value is in the list of strings
        size_t index = 1;

        for( const auto& keyword : keywords )
        {
            if( SO::EqualsNoCase(token_text_sv, keyword) )
                return index;

            ++index;
        }

        // no match, so reset the token index back to where it had been
        --m_nextBasicTokenIndex;
    }

    return 0;
}

template ZLOGICO_API size_t BasicTokenCompiler::NextKeyword(const std::vector<const TCHAR*>& keywords);
template ZLOGICO_API size_t BasicTokenCompiler::NextKeyword(const std::vector<std::wstring>& keywords);


bool BasicTokenCompiler::SkipBasicTokensUntil(TokenCode token_code)
{
    for( ; m_nextBasicTokenIndex < m_basicTokens.size(); ++m_nextBasicTokenIndex )
    {
        if( m_basicTokens[m_nextBasicTokenIndex].token_code == token_code )
            return true;
    }

    return ( m_nextBasicTokenIndex == m_basicTokens.size() );
}


bool BasicTokenCompiler::SkipBasicTokensUntil(wstring_view token_text)
{
    for( ; m_nextBasicTokenIndex < m_basicTokens.size(); ++m_nextBasicTokenIndex )
    {
        if( SO::EqualsNoCase(token_text, m_basicTokens[m_nextBasicTokenIndex].GetTextSV()) )
            return true;
    }

    return false;
}


cs::span<const BasicToken> BasicTokenCompiler::GetBasicTokensSpanFromCurrentToken() const
{
    ASSERT(m_nextBasicTokenIndex >= 1);

    size_t start_token_index = m_nextBasicTokenIndex - 1;

    return cs::span<const BasicToken>(m_basicTokens.data() + start_token_index, m_basicTokens.size() - start_token_index);
}


std::wstring BasicTokenCompiler::GetBasicTokenLine(const BasicToken& basic_token) const
{
    // the basic token contains a pointer to the buffer, so we can read backwards
    // and forwards to get the entire line
    const TCHAR* buffer_start_position = m_sourceBuffer->GetBuffer();
    const TCHAR* line_start = basic_token.token_text - 1;
    const TCHAR* line_end = basic_token.token_text + basic_token.token_length;

    while( line_start >= buffer_start_position && !is_crlf(*line_start) )
        --line_start;

    ++line_start;

    while( *line_end != 0 && !is_crlf(*line_end) )
        ++line_end;

    return std::wstring(line_start, line_end - line_start);
}


void BasicTokenCompiler::IssueMessage(ParserMessage& parser_message, int message_number, va_list parg)
{
    ASSERT(std::holds_alternative<std::monostate>(parser_message.extended_location));

    parser_message.message_number = message_number;
    parser_message.compilation_unit_name = m_compilationUnitName;
    parser_message.proc_name = GetCurrentProcName();

    if( m_capiLogicLocation.has_value() )
        parser_message.extended_location = *m_capiLogicLocation;

    const BasicToken* basic_token = GetCurrentBasicToken();

    if( basic_token != nullptr )
    {
        parser_message.line_number = basic_token->line_number;
        parser_message.position_in_line = basic_token->position_in_line;
    }

    FormatMessageAndProcessParserMessage(parser_message, parg);
}


void BasicTokenCompiler::IssueMessage(ParserMessage& parser_message, int message_number, ...)
{
    va_list parg;
    va_start(parg, message_number);

    IssueMessage(parser_message, message_number, parg);

    va_end(parg);
}


void BasicTokenCompiler::FormatMessageAndProcessParserMessage(ParserMessage& parser_message, va_list/* parg*/)
{
    // a derived class will use the message file
    ASSERT(false);
    parser_message.message_text = FormatTextCS2WS(_T("Message %d"), parser_message.message_number);
}
