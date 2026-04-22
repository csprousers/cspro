#pragma once

#include <zLogicO/zLogicO.h>
#include <zLogicO/SymbolType.h>


namespace SpecialFunction
{
    enum class Code : int
    {
        GlobalOnFocus,
        OnStop,
        OnKey,
        OnChar,
        OnChangeLanguage,
        OnSyncMessage,
        OnRefused,
        OnSystemMessage,
        OnViewQuestionnaire,
        OnActionInvokerResult,
    };

    struct Definition
    {
        const char* name;
        const char* const help_filename;
        Code code;
        SymbolType returns;

        // Validates the parameters, returning true when valid.
        ZLOGICO_API bool ValidateParameters(const std::vector<SymbolType>& parameter_symbol_types) const noexcept;
    };

    // Returns the definitions of all special functions.
    ZLOGICO_API const std::vector<Definition>& GetDefinitions() noexcept;

    // Returns the definition for the function, matched in a case-insensitive manner,
    // returning null if the function name does not match any special function.
    ZLOGICO_API const Definition* Lookup(std::string_view function_name_sv) noexcept;
}
