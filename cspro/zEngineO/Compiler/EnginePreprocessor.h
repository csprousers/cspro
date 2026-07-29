#pragma once

#include <zLogicO/Preprocessor.h>


class EnginePreprocessor : public Logic::Preprocessor
{
public:
    EnginePreprocessor(LogicCompiler& compiler, EngineData& engine_data);

protected:
    const char* GetAppType() override;
    Symbol* FindSymbol(std::string_view symbol_name_sv, bool search_only_base_symbols) override;
    void SetProperty(Symbol* symbol, const std::string& attribute, const std::variant<double, SharableString>& value) override;

private:
    template<typename T>
    static std::optional<T> ParseValue(const std::variant<double, SharableString>& value);

private:
    LogicCompiler& m_compiler;
    EngineData& m_engineData;
};
