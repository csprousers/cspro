#pragma once

#include <zLogicO/Preprocessor.h>

class CEngineDriver;


class EnginePreprocessor : public Logic::Preprocessor
{
public:
    EnginePreprocessor(Logic::BasicTokenCompiler& compiler, CEngineDriver* pEngineDriver);

protected:
    const char* GetAppType() override;
    Symbol* FindSymbol(std::string_view symbol_name_sv, bool search_only_base_symbols) override;
    void SetProperty(Symbol* symbol, const std::string& attribute, const std::variant<double, SharableString>& value) override;

private:
    template<typename T>
    static std::optional<T> ParseValue(const std::variant<double, SharableString>& value);

private:
    CEngineDriver* const m_pEngineDriver;
    const Logic::SymbolTable& m_symbolTable;
    const size_t m_initialSymbolTableSize;
};
