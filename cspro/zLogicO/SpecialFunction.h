#pragma once

#include <zToolsO/EnumHelpers.h>


enum class SpecialFunction : int
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

template<> constexpr SpecialFunction FirstInEnum<SpecialFunction>() { return SpecialFunction::GlobalOnFocus;         }
template<> constexpr SpecialFunction LastInEnum<SpecialFunction>()  { return SpecialFunction::OnActionInvokerResult; }


constexpr const char* SpecialFunctionNames[] =
{
    "On_Focus",
    "OnStop",
    "OnKey",
    "OnChar",
    "OnChangeLanguage",
    "OnSyncMessage",
    "OnRefused",
    "OnSystemMessage",
    "OnViewQuestionnaire",
    "OnActionInvokerResult",
};

static_assert(_countof(SpecialFunctionNames) == ( 1 + static_cast<size_t>(LastInEnum<SpecialFunction>()) ));


constexpr const char* ToString(SpecialFunction special_function)
{
    const size_t index = static_cast<size_t>(special_function);
    ASSERT(index >= 0 && index < _countof(SpecialFunctionNames));
    return SpecialFunctionNames[index];
}
