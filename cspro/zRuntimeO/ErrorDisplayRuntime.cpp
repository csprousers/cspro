#include "StdAfx.h"
#include "ErrorDisplayRuntime.h"


ErrorDisplayRuntime::ErrorDisplayRuntime(SharableString message)
    :   m_message(std::move(message))
{
    const std::string error_html_path = Path::Combine(Html::GetDirectory(Html::Subdirectory::Runtime), "error.html");

    if( PortableFunctions::FileIsRegular(error_html_path) )
    {
        m_url = PortableLocalhost::CreateUniqueFileUrl(error_html_path);
    }

    // if the error file does not exist, display the error message as text
    else
    {
        m_virtualFileMappingHandler = std::make_unique<TextVirtualFileMappingHandler>(m_message);
        PortableLocalhost::CreateVirtualFile(*m_virtualFileMappingHandler);
        m_url = m_virtualFileMappingHandler->GetUrl();
    }
}


ErrorDisplayRuntime::~ErrorDisplayRuntime()
{
}


SharableString ErrorDisplayRuntime::GetDescription() noexcept
{
    return "Error";
}


SharableString ErrorDisplayRuntime::GetUrl() noexcept
{
    return m_url;
}


bool ErrorDisplayRuntime::IsCloseable() noexcept
{
    return true;
}


void ErrorDisplayRuntime::Start()
{
}


SharableString ErrorDisplayRuntime::OnMessage(const std::string_view action_sv, const JsonNode& json_node)
{
    if( action_sv == "getError" )
    {
        const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

        json_writer->BeginObject()
                    .Write(JK::error, m_message)
                    .EndObject();

        return json_writer->ReleaseSharableString();
    }

    return Runtime::OnMessage(action_sv, json_node);
}
