#include "StdAfx.h"
#include "ProcessorMarkdown.h"
#include <zToolsO/FileIO.h>
#include <zViewO/MarkdownViewInput.h>
#include <zDesignerF/TextTemplatePreviewer.h>


void ProcessorMarkdown::Run(CodeDoc& code_doc)
{
    try
    {
        std::string markdown_file_path = code_doc.GetActualOrTempFilePath(FileExtensions::Markdown);

        SharableString html = MarkdownViewInput::ToViewableHtml(markdown_file_path,
                                                                code_doc.GetPrimaryCodeView().GetLogicCtrl()->GetText());

        code_doc.GetHtmlProcessor().DisplayHtml(std::move(html), std::move(markdown_file_path));
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void ProcessorMarkdown::SaveAsHtml(CodeDoc& code_doc)
{
    SaveAsHtml(code_doc,
        [](const std::string& markdown_file_path, const std::string_view markdown_sv)
        {
            return SharableString(MarkdownViewInput::ToSaveableHtml(markdown_file_path, markdown_sv));
        });
}


void ProcessorMarkdown::SaveReportAsHtml(CodeDoc& code_doc)
{
    // save Markdown reports to HTML using the output of the text template preview
    SaveAsHtml(code_doc,
        [&](const std::string& markdown_file_path, const std::string_view markdown_sv)
        {
            TextTemplatePreviewer text_template_previewer(markdown_file_path,
                                                          markdown_sv,
                                                          code_doc.GetLanguageSettings().GetOrCreateLogicSettings(),
                                                          "saving");

            return text_template_previewer.GetHtml();
        });
}


template<typename CF>
void ProcessorMarkdown::SaveAsHtml(CodeDoc& code_doc, const CF get_html_callback)
{
    try
    {
        const SharableString html = get_html_callback(code_doc.GetActualOrTempFilePath(FileExtensions::Markdown),
                                                      code_doc.GetPrimaryCodeView().GetLogicCtrl()->GetText());

        const std::string suggested_file_path = code_doc.GetPathName().IsEmpty() ? std::string() :
                                                                                   Path::ReplaceExtension(code_doc.GetFilePath(), FileExtensions::HTML);

        SaveFileDlg save_file_dlg(0, FileExtensions::HTML, suggested_file_path, FileFilters::HTML);
        save_file_dlg.SetTitle(L"Save Markdown as HTML");

        if( save_file_dlg.DoModal() != IDOK )
            return;

        FileIO::WriteText(save_file_dlg.GetFilePath(), *html, true);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
