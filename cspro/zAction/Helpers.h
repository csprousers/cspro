#pragma once

#include <zAction/ActionInvoker.h>
#include <zAction/Listener.h>
#include <zMessageO/ExceptionThrowingSystemMessageIssuer.h>


template<typename... Args>
[[noreturn]] void ActionInvoker::Runtime::IssueError(int message_number, Args... args)
{
    ExceptionThrowingSystemMessageIssuer().Issue(MessageType::Error, message_number, args...);
}


template<typename CF>
void ActionInvoker::Runtime::IterateOverListeners(CF callback_function)
{
    const auto& listener_crend = m_listeners->crend();

    for( auto listener_itr = m_listeners->crbegin(); listener_itr != listener_crend; ++listener_itr )
    {
        if( !callback_function(*(*listener_itr)) )
            return;
    }
}


template<typename... Args>
const TCHAR* const GetUniqueKeyFromChoices(const JsonNode<wchar_t>& json_node, const TCHAR* key1, Args const&... key2_and_more)
{
    const TCHAR* selected_key = nullptr;

    for( const TCHAR* const key : std::initializer_list<const TCHAR*> { key1, key2_and_more... } )
    {
        if( json_node.Contains(key) )
        {
            if( selected_key != nullptr )
                throw CSProException(_T("You cannot specify both '%s' and '%s'."), selected_key, key);

            selected_key = key;
        }
    }

    if( selected_key != nullptr )
        return selected_key;

    throw CSProException(_T("You must specify one of: ") + SO::CreateSingleString(cs::span<const TCHAR* const> { key1, key2_and_more... }));
}
