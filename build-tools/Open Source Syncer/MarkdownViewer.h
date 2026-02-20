#pragma once


class MarkdownViewer
{
public:
    static std::string MarkdownToHtmlDocument(std::string_view title_sv, std::string_view markdown_sv);

    static void ShowHtmlInDialog(SharableString html);
};
