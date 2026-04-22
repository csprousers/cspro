#pragma once

#include <zLogicO/zLogicO.h>
#include <zToolsO/EnumHelpers.h>


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
    };

    // Returns the definitions of all special functions.
    const std::vector<Definition>& GetDefinitions();

    // Returns the definition for the function, matched in a case-insensitive manner,
    // returning null if the function name does not match any special function.
    ZLOGICO_API const Definition* Lookup(std::string_view function_name_sv);
}


template<> constexpr SpecialFunction::Code FirstInEnum<SpecialFunction::Code>() { return SpecialFunction::Code::GlobalOnFocus;         }
template<> constexpr SpecialFunction::Code LastInEnum<SpecialFunction::Code>()  { return SpecialFunction::Code::OnActionInvokerResult; }

ZLOGICO_API const char* ToString(SpecialFunction::Code special_function);
