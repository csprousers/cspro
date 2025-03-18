#pragma once


class ProcessorMarkdown
{
public:
    static void Run(CodeDoc& code_doc);

    static void SaveAsHtml(CodeDoc& code_doc);

private:
    static std::string CreateHtml(CodeDoc& code_doc, bool link_to_css);
};
