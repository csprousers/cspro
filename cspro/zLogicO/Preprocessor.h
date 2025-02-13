#pragma once

#include <zLogicO/zLogicO.h>
#include <zLogicO/BasicTokenCompiler.h>


namespace Logic
{
    class ZLOGICO_API Preprocessor
    {
        friend class StringLiteralParser;

    public:
        Preprocessor(BasicTokenCompiler& compiler);
        virtual ~Preprocessor() { }

        // for preprocessing the entire source buffer prior to compilation
        void ProcessBuffer();

        // for handling a preprocessor line not handed prior to compilation
        void ProcessLineDuringCompilation();

        enum class FunctionCode { AppType, Exists };
        using ParsedToken = std::variant<TokenCode, FunctionCode, double, std::wstring>;

        // methods for subclasses to implement
    protected:
        // returns the application type
        virtual const TCHAR* GetAppType() = 0;

        // returns null if no symbol exists
        virtual Symbol* FindSymbol(const std::wstring& name, bool search_only_base_symbols) = 0;

        // can throw exceptions if the attribute/value is invalid
        virtual void SetProperty(Symbol* symbol, const std::wstring& attribute, const std::variant<double, std::wstring>& value) = 0;

    protected:
        template<typename... Args>
        [[noreturn]] void IssueError(int message_number, Args const&... args);

    private:
        const LogicSettings& GetLogicSettings() const { return m_compiler.GetLogicSettings(); }

        std::tuple<const TCHAR*, std::vector<BasicToken>::const_iterator, std::vector<BasicToken>::const_iterator> GetCommandAndLineTokens(const std::vector<BasicToken>::const_iterator& hash_token_position);

        std::vector<ParsedToken> ParseTokens(const std::vector<BasicToken>::const_iterator& token_itr_begin, const std::vector<BasicToken>::const_iterator& token_itr_end);

        using FunctionArgument = std::variant<double, std::wstring, Symbol*>;
        std::vector<FunctionArgument> ParseFunctionArguments(std::vector<BasicToken>::const_iterator token_itr, std::vector<BasicToken>::const_iterator token_itr_end);

        bool EvaluateCondition(const std::vector<BasicToken>::const_iterator& token_itr_begin, const std::vector<BasicToken>::const_iterator& token_itr_end);

        // returns true if the property was evaluated; returns false if the property should be evaluated as part of normal compilation
        bool ProcessSetProperty(bool currently_preprocessing, const std::vector<BasicToken>::const_iterator& token_itr_begin, const std::vector<BasicToken>::const_iterator& token_itr_end);

    private:
        struct PreprocessorException { };

        BasicTokenCompiler& m_compiler;
        std::optional<std::vector<BasicToken>::const_iterator> m_lastHashTokenPosition;
        size_t m_nextBasicTokenIndexOnError;
    };
}



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename... Args>
[[noreturn]] void Logic::Preprocessor::IssueError(const int message_number, Args const&... args)
{
    // for BasicTokenCompiler::IssueMessage to get the correct line number, set the token iterator index
    if( m_lastHashTokenPosition.has_value() )
        m_compiler.m_nextBasicTokenIndex = *m_lastHashTokenPosition - m_compiler.m_basicTokens.cbegin() + 1;

    // issue the error and then reset the token iterator index
    m_compiler.ReportError(message_number, args...);

    m_compiler.m_nextBasicTokenIndex = m_nextBasicTokenIndexOnError;

    // throw an error that will immediately get caught (but will get get out of any preprocessing operations)
    throw PreprocessorException();
}
