#pragma once


class ProcessorMarkdown
{
public:
    static void Run(CodeDoc& code_doc);

    static void SaveAsHtml(CodeDoc& code_doc);
    static void SaveTextTemplateAsHtml(CodeDoc& code_doc);

private:
    template<typename CF>
    static void SaveAsHtml(CodeDoc& code_doc, CF get_html_callback);
};
