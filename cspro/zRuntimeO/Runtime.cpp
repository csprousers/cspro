#include "StdAfx.h"
#include "Runtime.h"


bool Runtime::IsSuspendable() noexcept
{
    return IsCloseable();
}


void Runtime::OnSuspend() noexcept
{
}


void Runtime::OnClose() noexcept
{
    ASSERT(IsCloseable());
    m_runtimeHost->CloseRuntimeAsync();
}


void Runtime::OnActivate() noexcept
{
    m_runtimeHost->NavigateToAsync(GetUrl());
}


SharableString Runtime::OnMessage(const std::string_view action_sv, const JsonNode& json_node)
{
    if( action_sv == "close" )
    {
        m_runtimeHost->CloseRuntimeAsync();
        return SharableString();
    }

    throw CSProException("Unknown message: " + json_node.GetNodeAsString());
}
