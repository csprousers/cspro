#pragma once

#include <zLogicO/zLogicO.h>
#include <zLogicO/ParserMessage.h>
#include <zLogicO/SourceBuffer.h>
#include <zToolsO/span.h>

class CompilerMessageIssuer;
class LogicSettings;
class TextSource;
namespace Logic { class BasicTokenCompiler; class PreprocessorEvaluator; struct ProcDirectoryEntry; }


class ZLOGICO_API Logic::BasicTokenCompiler
{
    friend CompilerMessageIssuer;
    friend class Preprocessor;

public:
    BasicTokenCompiler();
    virtual ~BasicTokenCompiler() { }

    void ClearSourceBuffer();
    void SetSourceBuffer(const TextSource& source_text_source);
    void SetSourceBuffer(std::shared_ptr<SourceBuffer> source_buffer, const ProcDirectoryEntry* proc_directory_entry = nullptr);
    void SetCompilationUnitName(std::string name);
    void SetCapiLogicLocation(CapiLogicLocation capi_logic_location);

    size_t GetCurrentBasicTokenLineNumber() const;
    const std::string& GetCurrentCompilationUnitName() const                    { return m_compilationUnitName; }
    const std::optional<CapiLogicLocation>& GetCurrentCapiLogicLocation() const { return m_capiLogicLocation; }
    std::shared_ptr<const SourceBuffer> GetSourceBuffer() const                 { return m_sourceBuffer; }

    virtual const LogicSettings& GetLogicSettings() const = 0;
    virtual std::string GetCurrentProcName() const = 0;

    // --------------------------------------------------------------------------
    // navigation methods
    // --------------------------------------------------------------------------
    void MarkInputBufferToRestartLater();
    void RestartFromMarkedInputBuffer();
    void ClearMarkedInputBuffer();

    void MoveNextBasicTokenIndex(int offset_from_next_token_index);

    const BasicToken* GetCurrentBasicToken() const                                   { return GetBasicTokenFromOffset(-1); }
    const BasicToken* GetPreviousBasicToken() const                                  { return GetBasicTokenFromOffset(-2); }
    const BasicToken* PeekNextBasicToken(int offset_from_next_token_index = 0) const { return GetBasicTokenFromOffset(offset_from_next_token_index); }

    // --------------------------------------------------------------------------
    // next token methods
    // --------------------------------------------------------------------------

    // Gets the next token of any type.
    const BasicToken* NextBasicToken();

    // Checks if the next token is in the list of keywords. If found, the function returns
    // the 1-based index of the selected keyword and the compiler advances past the keyword.
    // If not found, the function returns 0 and the token is not processed.
    template<typename T>
    size_t NextKeyword(const T& keywords);
    size_t NextKeyword(std::initializer_list<const char*> keywords) { return NextKeyword(cs::span<const char* const>(keywords)); }

    // Skips past all tokens until the matching token is located or the end of the source
    // buffer is reached. If the token is found, it is not read or processed.
    bool SkipBasicTokensUntil(TokenCode token_code);

    // Skips past all tokens until the matching text is located or the end of the source
    // buffer is reached.
    bool SkipBasicTokensUntil(std::string_view token_text_sv);

    // Returns an iterator to the remaining tokens, starting with the current token.
    cs::span<const BasicToken> GetBasicTokensSpanFromCurrentToken() const;

    // Returns an iterator to all of the tokens.
    cs::span<const BasicToken> GetBasicTokensSpan() const { return m_basicTokens; }

    // Returns the line of text (including any comments) where the token is located.
    std::string GetBasicTokenLine(const BasicToken& basic_token) const;


    // --------------------------------------------------------------------------
    // message methods
    // --------------------------------------------------------------------------

    // Issues a compiler error and then throws a ParserError exception.
    template<typename... Args>
    [[noreturn]] void IssueError(int message_number, Args const&... args);

    // Reports a compiler error but does not throw an exception.
    template<typename... Args>
    void ReportError(int message_number, Args const&... args);

    // Reports a compiler error (at the extended location) but does not throw an exception.
    template<typename... Args>
    void ReportError(ParserMessage::ExtendedLocation extended_location, int message_number, Args const&... args);

    // Issues a compiler warning.
    template<typename... Args>
    void IssueWarning(int message_number, Args const&... args);

    // Issues a compiler warning of the specified type.
    template<typename... Args>
    void IssueWarning(ParserMessage::Type type, int message_number, Args const&... args);

    virtual void FormatMessageAndProcessParserMessage(ParserMessage& parser_message, va_list parg);

private:
    const BasicToken* GetBasicTokenFromOffset(int offset_from_next_token_index) const;

    template<typename... Args>
    void IssueMessage(ParserMessage& parser_message, int message_number, Args const&... args);

    void IssueMessageWorker(ParserMessage& parser_message, int message_number, ...);
    void IssueMessageWorkerVA(ParserMessage& parser_message, int message_number, va_list parg);

private:
    std::vector<BasicToken> m_basicTokens;
    std::vector<size_t> m_markIndices;
    size_t m_nextBasicTokenIndex;

    std::string m_compilationUnitName;
    std::optional<CapiLogicLocation> m_capiLogicLocation;
    std::shared_ptr<SourceBuffer> m_sourceBuffer;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
size_t Logic::BasicTokenCompiler::BasicTokenCompiler::NextKeyword(const T& keywords)
{
    const BasicToken* const basic_token = NextBasicToken();

    if( basic_token != nullptr )
    {
        const std::string_view token_text_sv = basic_token->GetSV();

        // check if the value is in the list of keywords
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


template<typename... Args>
void Logic::BasicTokenCompiler::IssueMessage(ParserMessage& parser_message, const int message_number, Args const&... args)
{
#ifdef _DEBUG
    ValidateFormatTextArgumentTypes(args...);
#endif

    IssueMessageWorker(parser_message, message_number, args...);
}


template<typename... Args>
[[noreturn]] void Logic::BasicTokenCompiler::IssueError(const int message_number, Args const&... args)
{
    ParserError parser_error;
    IssueMessage(parser_error, message_number, args...);
    throw parser_error;
}


template<typename... Args>
void Logic::BasicTokenCompiler::ReportError(const int message_number, Args const&... args)
{
    ParserError parser_error;
    IssueMessage(parser_error, message_number, args...);
}


template<typename... Args>
void Logic::BasicTokenCompiler::ReportError(ParserMessage::ExtendedLocation extended_location, const int message_number, Args const&... args)
{
    ParserError parser_error;
    parser_error.extended_location = std::move(extended_location);
    IssueMessage(parser_error, message_number, args...);
}


template<typename... Args>
void Logic::BasicTokenCompiler::IssueWarning(const int message_number, Args const&... args)
{
    ParserMessage parser_message(ParserMessage::Type::Warning);
    IssueMessage(parser_message, message_number, args...);
}


template<typename... Args>
void Logic::BasicTokenCompiler::IssueWarning(const ParserMessage::Type type, const int message_number, Args const&... args)
{
    ASSERT(type != ParserMessage::Type::Error);
    ParserMessage parser_message(type);
    IssueMessage(parser_message, message_number, args...);
}
