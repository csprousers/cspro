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


constexpr const TCHAR* SpecialFunctionNames[] =
{
    _T("On_Focus"),
    _T("OnStop"),
    _T("OnKey"),
    _T("OnChar"),
    _T("OnChangeLanguage"),
    _T("OnSyncMessage"),
    _T("OnRefused"),
    _T("OnSystemMessage"),
    _T("OnViewQuestionnaire"),
    _T("OnActionInvokerResult"),
};

static_assert(_countof(SpecialFunctionNames) == ( 1 + static_cast<size_t>(LastInEnum<SpecialFunction>()) ));


constexpr const TCHAR* ToString(SpecialFunction special_function)
{
    const size_t index = static_cast<size_t>(special_function);
    ASSERT(index >= 0 && index < _countof(SpecialFunctionNames));
    return SpecialFunctionNames[index];
}
