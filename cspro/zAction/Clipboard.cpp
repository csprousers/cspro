#include "stdafx.h"


ActionInvoker::Result ActionInvoker::Runtime::Clipboard_getText(const JsonNode& /*json_node*/, Caller& /*caller*/)
{
    SharableString text = PortableRunner::Clipboard_GetText();

    if( text.IsSet() )
        return Result::String(std::move(text));

    return Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::Clipboard_putText(const JsonNode& json_node, Caller& /*caller*/)
{
    PortableRunner::Clipboard_PutText(json_node.Get<std::string_view>(JK::text));

    return Result::Undefined();
}
