#pragma once

#include <zAction/ActionInvoker.h>
#include <zAction/Listener.h>
#include <zMessageO/ExceptionThrowingSystemMessageIssuer.h>


template<typename... Args>
[[noreturn]] void ActionInvoker::Runtime::IssueError(const int message_number, Args const&... args)
{
    try
    {
        ExceptionThrowingSystemMessageIssuer().Issue(MessageType::Error, message_number, args...);
    }

    catch( const CSProException& exception )
    {
        // rethrow as an ActionInvoker::Exception
        throw ActionInvoker::Exception(exception.what(), std::string(), std::nullopt);
    }
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
const char* const GetUniqueKeyFromChoices(const JsonNode& json_node, const char* const key1, Args const&... key2_and_more)
{
    const char* selected_key = nullptr;

    for( const char* const key : std::initializer_list<const char*> { key1, key2_and_more... } )
    {
        if( json_node.Contains(key) )
        {
            if( selected_key != nullptr )
                throw CSProException("You cannot specify both '%s' and '%s'.", selected_key, key);

            selected_key = key;
        }
    }

    if( selected_key != nullptr )
        return selected_key;

    throw CSProException("You must specify one of: " + SO::CreateSingleString(cs::span<const char* const> { key1, key2_and_more... }));
}
