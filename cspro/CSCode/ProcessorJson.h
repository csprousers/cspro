#pragma once


class ProcessorJson
{
public:
    static void FormatJson(CodeView& code_view, bool compress_mode);

    static void ValidateJson(CodeView& code_view);

    static void ValidateSpecFile(CodeView& code_view);

    static void DowngradeSpecFile(CodeView& code_view);
};
