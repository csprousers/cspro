#pragma once

#include <zLogicO/Preprocessor.h>

class CEngineDriver;


class EnginePreprocessor : public Logic::Preprocessor
{
public:
    EnginePreprocessor(Logic::BasicTokenCompiler& compiler, CEngineDriver* pEngineDriver);

protected:
    const TCHAR* GetAppType() override;
    Symbol* FindSymbol(const std::wstring& name, bool search_only_base_symbols) override;
    void SetProperty(Symbol* symbol, const std::wstring& attribute, const std::variant<double, std::wstring>& value) override;

private:
    template<typename T>
    static std::optional<T> ParseValue(const std::variant<double, std::wstring>& value);

private:
    CEngineDriver* const m_pEngineDriver;
    const Logic::SymbolTable& m_symbolTable;
    const size_t m_initialSymbolTableSize;
};
