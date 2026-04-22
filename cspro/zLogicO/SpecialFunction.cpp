#include "stdafx.h"
#include "SpecialFunction.h"


const std::vector<SpecialFunction::Definition>& SpecialFunction::GetDefinitions()
{
    static const std::vector<SpecialFunction::Definition> definitions
    {
        {
            "On_Focus",
            nullptr,
            Code::GlobalOnFocus,
        },
        {
            "OnStop",
            "OnStop_global_function.html",
            Code::OnStop,
        },
        {
            "OnKey",
            "OnKey_global_function.html",
            Code::OnKey,
        },
        {
            "OnChar",
            "OnChar_global_function.html",
            Code::OnChar,
        },
        {
            "OnChangeLanguage",
            "OnChangeLanguage_global_function.html",
            Code::OnChangeLanguage,
        },
        {
            "OnSyncMessage",
            "syncmessage_function.html",
            Code::OnSyncMessage,
        },
        {
            "OnRefused",
            "refused_value.html",
            Code::OnRefused,
        },
        {
            "OnSystemMessage",
            "OnSystemMessage_global_function.html",
            Code::OnSystemMessage,
        },
        {
            "OnViewQuestionnaire",
            "OnViewQuestionnaire_global_function.html",
            Code::OnViewQuestionnaire,
        },
        {
            "OnActionInvokerResult",
            "CS_OnActionInvokerResult.html",
            Code::OnActionInvokerResult,
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


const char* ToString(const SpecialFunction::Code special_function)
{
    const std::vector<SpecialFunction::Definition>& definitions = SpecialFunction::GetDefinitions();
    const size_t index = static_cast<size_t>(special_function);
    ASSERT(index < definitions.size());
    return definitions[index].name;
}


const SpecialFunction::Definition* SpecialFunction::Lookup(const std::string_view function_name_sv)
{
    const std::vector<Definition>& definitions = SpecialFunction::GetDefinitions();

    for( const Definition& definition : definitions )
    {
        if( SO::EqualsNoCase(function_name_sv, definition.name) )
            return &definition;
    }

    return nullptr;
}
