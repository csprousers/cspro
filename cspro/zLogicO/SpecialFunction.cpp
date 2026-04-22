#include "stdafx.h"
#include "SpecialFunction.h"
#include <zToolsO/EnumHelpers.h>


template<> constexpr SpecialFunction::Code FirstInEnum<SpecialFunction::Code>() { return SpecialFunction::Code::GlobalOnFocus;         }
template<> constexpr SpecialFunction::Code LastInEnum<SpecialFunction::Code>()  { return SpecialFunction::Code::OnActionInvokerResult; }


const std::vector<SpecialFunction::Definition>& SpecialFunction::GetDefinitions() noexcept
{
    static const std::vector<SpecialFunction::Definition> definitions
    {
        {
            "On_Focus",
            nullptr,
            Code::GlobalOnFocus,
            SymbolType::WorkVariable,
        },
        {
            "OnStop",
            "OnStop_global_function.html",
            Code::OnStop,
            SymbolType::WorkVariable,
        },
        {
            "OnKey",
            "OnKey_global_function.html",
            Code::OnKey,
            SymbolType::WorkVariable,
        },
        {
            "OnChar",
            "OnChar_global_function.html",
            Code::OnChar,
            SymbolType::WorkVariable,
        },
        {
            "OnChangeLanguage",
            "OnChangeLanguage_global_function.html",
            Code::OnChangeLanguage,
            SymbolType::WorkVariable,
        },
        {
            "OnSyncMessage",
            "syncmessage_function.html",
            Code::OnSyncMessage,
            SymbolType::WorkString,
        },
        {
            "OnRefused",
            "refused_value.html",
            Code::OnRefused,
            SymbolType::WorkVariable,
        },
        {
            "OnSystemMessage",
            "OnSystemMessage_global_function.html",
            Code::OnSystemMessage,
            SymbolType::WorkVariable,
        },
        {
            "OnViewQuestionnaire",
            "OnViewQuestionnaire_global_function.html",
            Code::OnViewQuestionnaire,
            SymbolType::WorkVariable,
        },
        {
            "OnActionInvokerResult",
            "CS_OnActionInvokerResult.html",
            Code::OnActionInvokerResult,
            SymbolType::WorkString,
        },
    };

#ifdef _DEBUG
    Code code = FirstInEnum<Code>();

    std::for_each(definitions.cbegin(), definitions.cend(),
        [&](const Definition& definition)
        {
            ASSERT(definition.code == code);
            IncrementEnum(code);
        });

    ASSERT(definitions.size() == ( 1 + static_cast<size_t>(LastInEnum<Code>()) ));
#endif

    return definitions;
}


const SpecialFunction::Definition* SpecialFunction::Lookup(const std::string_view function_name_sv) noexcept
{
    const std::vector<Definition>& definitions = SpecialFunction::GetDefinitions();

    for( const Definition& definition : definitions )
    {
        if( SO::EqualsNoCase(function_name_sv, definition.name) )
            return &definition;
    }

    return nullptr;
}


bool SpecialFunction::Definition::ValidateParameters(const std::vector<SymbolType>& parameter_symbol_types) const noexcept
{
    constexpr SymbolType numeric_type = SymbolType::WorkVariable;
    constexpr SymbolType string_type = SymbolType::WorkString;

    size_t number_numerics = 0;
    size_t number_strings = 0;

    for( const SymbolType symbol_type : parameter_symbol_types )
    {
        if( symbol_type == numeric_type )
        {
            ++number_numerics;
        }

        else if( symbol_type == string_type )
        {
            ++number_strings;
        }

        else
        {
            // all special functions use only numeric/string parameters
            return false;
        }
    }

    switch( code )
    {
        // OnSyncMessage has two string parameters
        case Code::OnSyncMessage:
            return ( number_strings == 2 );

        // OnActionInvokerResult has three string parameters
        case Code::OnActionInvokerResult:
            return ( number_strings == 3 );

        // OnSystemMessage has at least one parameter (up to two numeric parameters and up to one string parameter)
        case Code::OnSystemMessage:
            return ( !parameter_symbol_types.empty() &&
                     number_numerics <= 2 &&
                     number_strings <= 1 );

        // OnRefused doesn't have any parameters
        case Code::OnRefused:
            return parameter_symbol_types.empty();

        // OnViewQuestionnaire has one optional string parameter
        case Code::OnViewQuestionnaire:
            return ( parameter_symbol_types.empty() ||
                     number_strings == 1 );

        // others functions are only valid if there are only numeric parameters
        default:
            return ( number_strings == 0 );
    }
}
