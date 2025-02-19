#include "StdAfx.h"
#include "HomeScreenRuntime.h"
#include <zHtml/PortableLocalhost.h>
#include <zRuntimeO/RuntimeHost.h>


HomeScreenRuntime::HomeScreenRuntime()
    :   m_url(PortableLocalhost::CreateUniqueFileUrl(Path::Combine(Html::GetDirectory(Html::Subdirectory::Runtime), "home.html")))
{
}


SharableString HomeScreenRuntime::GetDescription() noexcept
{
    return "Home";
}


SharableString HomeScreenRuntime::GetUrl() noexcept
{
    return m_url;
}


bool HomeScreenRuntime::IsCloseable() noexcept
{
    return true;
}


void HomeScreenRuntime::Start()
{
}


SharableString HomeScreenRuntime::OnMessage(const std::string_view action_sv, const JsonNode& json_node)
{
    if( action_sv == "open" )
    {
        const std::string_view open_type_sv = json_node.Get<std::string_view>();

        if( open_type_sv == "application" )
        {
            WindowsDesktopMessage::Post(WM_COMMAND, ID_FILE_OPEN_APPLICATION);
            return SharableString();
        }

        else if( open_type_sv == "directory" )
        {
            WindowsDesktopMessage::Post(WM_COMMAND, ID_FILE_OPEN_DIRECTORY);
            return SharableString();
        }
    }

    return Runtime::OnMessage(action_sv, json_node);
}
