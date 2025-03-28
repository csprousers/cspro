#pragma once

#include <zHtml/VirtualFileMapping.h>

class CodeDoc;


class ProcessorHtml
{
public:
    void DisplayHtmlDialog(CodeDoc& code_doc);

    void DisplayHtml(CodeDoc& code_doc);

    void DisplayHtml(SharableString html, std::string file_path);

private:
    std::unique_ptr<VirtualFileMapping> CreateHtmlVirtualFileMapping(SharableString html, const std::string& directory) const;

private:
    std::unique_ptr<VirtualFileMapping> m_htmlVirtualFileMapping;
};
