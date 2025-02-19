#pragma once

#include <zLogicO/FunctionTable.h>


namespace Logic
{
    std::vector<const char*> GetChildSymbolNames(const std::variant<SymbolType, FunctionNamespace>& symbol_type_or_function_namespace);

    const char* LookupChildSymbolName(const std::string_view name_sv, const std::variant<SymbolType, FunctionNamespace>& symbol_type_or_function_namespace);

    constexpr const char* ValueSetCodes  = "codes";
    constexpr const char* ValueSetLabels = "labels";
}



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline std::vector<const char*> Logic::GetChildSymbolNames(const std::variant<SymbolType, FunctionNamespace>& symbol_type_or_function_namespace)
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


inline const char* Logic::LookupChildSymbolName(const std::string_view name_sv, const std::variant<SymbolType, FunctionNamespace>& symbol_type_or_function_namespace)
{
    for( const char* const name : GetChildSymbolNames(symbol_type_or_function_namespace) )
    {
        if( SO::EqualsNoCase(name_sv, name) )
            return name;
    }

    return nullptr;
}
