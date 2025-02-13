#include "stdafx.h"
#include "PortableRunner.h"


std::optional<std::wstring> ActionInvoker::PortableRunner::ClipboardGetText()
{
    throw CSProException("Not implemented: PortableRunner::ClipboardGetText");
}


void ActionInvoker::PortableRunner::ClipboardPutText(const std::wstring& /*text*/)
{
    throw CSProException("Not implemented: PortableRunner::ClipboardPutText");
}


std::vector<std::tuple<std::wstring, std::wstring>> ActionInvoker::PortableRunner::SystemShowSelectDocumentDialog(const std::vector<std::wstring>& /*mime_types*/, bool /*multiple*/)
{
    throw CSProException("Not implemented: PortableRunner::SystemShowSelectDocumentDialog");
}
