#include "stdafx.h"


ActionInvoker::Result ActionInvoker::Runtime::Clipboard_getText(const JsonNode<wchar_t>& /*json_node*/, Caller& /*caller*/)
{
    std::optional<std::wstring> text = PortableRunner::ClipboardGetText();

    if( text.has_value() )
        return Result::String(std::move(*text));

    return Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::Clipboard_putText(const JsonNode<wchar_t>& json_node, Caller& /*caller*/)
{
    PortableRunner::ClipboardPutText(json_node.Get<std::wstring>(JK::text));

    return Result::Undefined();
}
