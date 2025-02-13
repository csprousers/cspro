#pragma once

#include <zEngineO/Compiler/CompilerHelper.h>
#include <zUtilO/CommonStore.h>


class CommonStoreCompilerHelper : public CompilerHelper
{
public:
    CommonStoreCompilerHelper(LogicCompiler& logic_compiler);

    template<typename T>
    std::optional<T> GetConfigValue(const std::wstring& symbol_name, bool issue_warning_if_value_does_not_exist);

protected:
    bool IsCacheable() const override { return true; }

private:
    CommonStore m_commonStore;
};



inline CommonStoreCompilerHelper::CommonStoreCompilerHelper(LogicCompiler& logic_compiler)
    :   CompilerHelper(logic_compiler)
{
    if( !m_commonStore.Open({ CommonStore::TableType::ConfigVariables }) )
        m_compiler->IssueError(MGF::CommonStore_cannot_be_opened_94102);
}


template<typename T>
std::optional<T> CommonStoreCompilerHelper::GetConfigValue(const std::wstring& symbol_name, bool issue_warning_if_value_does_not_exist)
{
    std::optional<std::wstring> value = m_commonStore.SimpleDbMap::GetString(symbol_name);

    if( !value.has_value() )
    {
        if( issue_warning_if_value_does_not_exist )
            m_compiler->IssueWarning(MGF::CommonStore_setting_not_found_94103, symbol_name.c_str());

        return std::nullopt;
    }

    // numeric
    if constexpr(std::is_same_v<T, double>)
    {
        std::optional<double> numeric_value = StringToNumber<std::optional<double>>(*value);

        if( numeric_value.has_value() )
            return *numeric_value;

        m_compiler->IssueWarning(MGF::CommonStore_setting_not_numeric_94104, symbol_name.c_str(), value->c_str());
        return std::nullopt;
    }

    // string
    else
    {
        static_assert(std::is_same_v<T, std::wstring>);
        return value;
    }
}
