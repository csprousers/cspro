#include "StdAfx.h"
#include "MarkdownViewer.h"
#include <zHtml/HtmlViewDlg.h>
#include <zMarkdown/Markdown.h>


std::string MarkdownViewer::MarkdownToHtmlDocument(const std::string_view title_sv, const std::string_view markdown_sv)
{
    CssProvider css_provider(Html::CSS::Markdown, true);
    return Markdown::ToHtmlDocument(title_sv, markdown_sv, &css_provider);
}


void MarkdownViewer::ShowHtmlInDialog(SharableString html)
{
    HtmlViewDlg dlg;
    dlg.SetInitialHtml(std::move(html));
    dlg.DoModal();
}
