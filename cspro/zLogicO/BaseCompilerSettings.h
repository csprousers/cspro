#pragma once

#include <zLogicO/BaseCompiler.h>


namespace Logic
{
    class BaseCompilerSettings
    {
        friend class BaseCompiler;

    private:
        BaseCompilerSettings(BaseCompiler& base_compiler)
            :   m_compiler(base_compiler),
                m_savedSuppressErrorReporting(m_compiler.m_suppressErrorReporting),
                m_savedSuppressExceptionsOnInvalidSymbols(m_compiler.m_suppressExceptionsOnInvalidSymbols),
                m_savedPreferredSymbolType(m_compiler.m_preferredSymbolType)
        {
        }

    public:
        ~BaseCompilerSettings()
        {
            m_compiler.m_suppressErrorReporting = m_savedSuppressErrorReporting;
            m_compiler.m_suppressExceptionsOnInvalidSymbols = m_savedSuppressExceptionsOnInvalidSymbols;
            m_compiler.m_preferredSymbolType = m_savedPreferredSymbolType;
        }

        void SuppressErrorReporting()                                          { m_compiler.m_suppressErrorReporting = true; }
        void SuppressExceptionsOnInvalidSymbols()                              { m_compiler.m_suppressExceptionsOnInvalidSymbols = true; }
        void SetNextTokenPreferredSymbolType(SymbolType preferred_symbol_type) { m_compiler.m_preferredSymbolType = preferred_symbol_type; }

    private:
        BaseCompiler& m_compiler;
        bool m_savedSuppressErrorReporting;
        bool m_savedSuppressExceptionsOnInvalidSymbols;
        SymbolType m_savedPreferredSymbolType;
    };
}
