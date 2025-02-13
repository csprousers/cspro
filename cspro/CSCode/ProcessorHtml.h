#pragma once

#include <zHtml/VirtualFileMapping.h>

class CodeDoc;


class ProcessorHtml
{
public:
    void DisplayHtmlDialog(CodeDoc& code_doc);

    void DisplayHtml(CodeDoc& code_doc);

private:
    std::unique_ptr<VirtualFileMapping> CreateHtmlVirtualFileMapping(CodeDoc& code_doc, const std::wstring& filename) const;

private:
    std::unique_ptr<VirtualFileMapping> m_htmlVirtualFileMapping;
};
