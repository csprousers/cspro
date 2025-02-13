#pragma once

#include <zLogicO/FunctionTable.h>


namespace Logic
{
    std::vector<const TCHAR*> GetChildSymbolNames(const std::variant<SymbolType, FunctionNamespace>& symbol_type_or_function_namespace);

    const TCHAR* LookupChildSymbolName(const wstring_view name_sv, const std::variant<SymbolType, FunctionNamespace>& symbol_type_or_function_namespace);

    constexpr const TCHAR* ValueSetCodes  = _T("codes");
    constexpr const TCHAR* ValueSetLabels = _T("labels");
}


inline std::vector<const TCHAR*> Logic::GetChildSymbolNames(const std::variant<SymbolType, FunctionNamespace>& symbol_type_or_function_namespace)
{
    if( symbol_type_or_function_namespace == SymbolType::ValueSet )
    {
        return
        {
            ValueSetCodes,
            ValueSetLabels
        };
    }

    return { };
}


inline const TCHAR* Logic::LookupChildSymbolName(const wstring_view name_sv, const std::variant<SymbolType, FunctionNamespace>& symbol_type_or_function_namespace)
{
    for( const TCHAR* name : GetChildSymbolNames(symbol_type_or_function_namespace) )
    {
        if( SO::EqualsNoCase(name_sv, name) )
            return name;
    }

    return nullptr;
}
