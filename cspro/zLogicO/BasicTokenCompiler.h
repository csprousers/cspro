#pragma once

#include <zLogicO/zLogicO.h>
#include <zLogicO/ParserMessage.h>
#include <zLogicO/SourceBuffer.h>
#include <zToolsO/span.h>

class CompilerMessageIssuer;
class LogicSettings;
class TextSource;


namespace Logic
{
    class PreprocessorEvaluator;
    struct ProcDirectoryEntry;


    class ZLOGICO_API BasicTokenCompiler
    {
        friend CompilerMessageIssuer;
        friend class Preprocessor;

    public:
        BasicTokenCompiler();
        virtual ~BasicTokenCompiler() { }

        void ClearSourceBuffer();
        void SetSourceBuffer(const TextSource& source_text_source);
        void SetSourceBuffer(std::shared_ptr<SourceBuffer> source_buffer, const ProcDirectoryEntry* proc_directory_entry = nullptr);
        void SetCompilationUnitName(std::wstring name);
        void SetCapiLogicLocation(CapiLogicLocation capi_logic_location);

        size_t GetCurrentBasicTokenLineNumber() const;
        const std::wstring& GetCurrentCompilationUnitName() const                   { return m_compilationUnitName; }
        const std::optional<CapiLogicLocation>& GetCurrentCapiLogicLocation() const { return m_capiLogicLocation; }
        std::shared_ptr<const SourceBuffer> GetSourceBuffer() const                 { return m_sourceBuffer; }

        virtual const LogicSettings& GetLogicSettings() const = 0;
        virtual const std::wstring& GetCurrentProcName() const = 0;

        // --------------------------------------------------------------------------
        // navigation methods
        // --------------------------------------------------------------------------
        void MarkInputBufferToRestartLater();
        void RestartFromMarkedInputBuffer();
        void ClearMarkedInputBuffer();

        void MoveNextBasicTokenIndex(int offset_from_next_token_index);

    private:
        const BasicToken* GetBasicTokenFromOffset(int offset_from_next_token_index) const;

    public:
        const BasicToken* GetCurrentBasicToken() const                                   { return GetBasicTokenFromOffset(-1); }
        const BasicToken* GetPreviousBasicToken() const                                  { return GetBasicTokenFromOffset(-2); }
        const BasicToken* PeekNextBasicToken(int offset_from_next_token_index = 0) const { return GetBasicTokenFromOffset(offset_from_next_token_index); }

        // --------------------------------------------------------------------------
        // next token methods
        // --------------------------------------------------------------------------

        // Gets the next token of any type.
        const BasicToken* NextBasicToken();

        // Checks if the next token is in the list of keywords. If found, the function returns
        // the 1-based index of the selected and the compiler advances past the keyword.
        // If not found, the function returns 0 and the token is not processed.
        template<typename T = const TCHAR*>
        size_t NextKeyword(const std::vector<T>& keywords);

        // Skips past all tokens until the matching token is located or the end of the source
        // buffer is reached. If the token is found, it is not read or processed.
        bool SkipBasicTokensUntil(TokenCode token_code);

        // Skips past all tokens until the matching text is located or the end of the source
        // buffer is reached.
        bool SkipBasicTokensUntil(wstring_view token_text);

        // Returns an iterator to the remaining tokens, starting with the current token.
        cs::span<const BasicToken> GetBasicTokensSpanFromCurrentToken() const;

        // Returns an iterator to all of the tokens.
        cs::span<const BasicToken> GetBasicTokensSpan() const { return m_basicTokens; }

        // Returns the line of text (including any comments) where the token is located.
        std::wstring GetBasicTokenLine(const BasicToken& basic_token) const;


        // --------------------------------------------------------------------------
        // message methods
        // --------------------------------------------------------------------------
    private:
        void IssueMessage(ParserMessage& parser_message, int message_number, va_list parg);
        void IssueMessage(ParserMessage& parser_message, int message_number, ...);

    public:
        // Issues a compiler error and then throws a ParserError exception.
        template<typename... Args>
        [[noreturn]] void IssueError(int message_number, Args... args)
        {
            ParserError parser_error;
            IssueMessage(parser_error, message_number, args...);
            throw parser_error;
        }

        // Reports a compiler error but does not throw an exception.
        template<typename... Args>
        void ReportError(int message_number, Args... args)
        {
            ParserError parser_error;
            IssueMessage(parser_error, message_number, args...);
        }

        // Issues a compiler warning.
        template<typename... Args>
        void IssueWarning(int message_number, Args... args)
        {
            ParserMessage parser_message(ParserMessage::Type::Warning);
            IssueMessage(parser_message, message_number, args...);
        }

        // Issues a compiler warning of the specified type.
        template<typename... Args>
        void IssueWarning(ParserMessage::Type type, int message_number, Args... args)
        {
            ASSERT(type != ParserMessage::Type::Error);
            ParserMessage parser_message(type);
            IssueMessage(parser_message, message_number, args...);
        }

        virtual void FormatMessageAndProcessParserMessage(ParserMessage& parser_message, va_list parg);

    private:
        std::vector<BasicToken> m_basicTokens;
        std::vector<size_t> m_markIndices;
        size_t m_nextBasicTokenIndex;

        std::wstring m_compilationUnitName;
        std::optional<CapiLogicLocation> m_capiLogicLocation;
        std::shared_ptr<SourceBuffer> m_sourceBuffer;
    };
}
