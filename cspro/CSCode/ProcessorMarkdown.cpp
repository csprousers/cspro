#include "StdAfx.h"
#include "ProcessorMarkdown.h"
#include <zToolsO/FileIO.h>
#include <zMarkdown/Markdown.h>


std::string ProcessorMarkdown::CreateHtml(CodeDoc& code_doc, const bool link_to_css)
{
    return Markdown::ToHtmlDocument(Path::GetFilenameWithoutExtension(code_doc.GetFilePath()),
                                    code_doc.GetPrimaryCodeView().GetLogicCtrl()->GetText(),
                                    std::make_unique<CssProvider>(Html::CSS::Markdown, link_to_css).get());
}


void ProcessorMarkdown::Run(CodeDoc& code_doc)
{
    try
    {
        code_doc.GetHtmlProcessor().DisplayHtml(CreateHtml(code_doc, true),
                                                code_doc.GetActualOrTempFilePath(FileExtensions::Markdown));
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void ProcessorMarkdown::SaveAsHtml(CodeDoc& code_doc)
{
    const std::string suggested_file_path = code_doc.GetPathName().IsEmpty() ? std::string() :
                                                                               Path::ReplaceExtension(code_doc.GetFilePath(), FileExtensions::HTML);

    SaveFileDlg save_file_dlg(0, FileExtensions::HTML, suggested_file_path, FileFilters::HTML);
    save_file_dlg.SetTitle(L"Save Markdown as HTML");

    if( save_file_dlg.DoModal() != IDOK )
        return;

    try
    {
        FileIO::WriteText(save_file_dlg.GetFilePath(),
                          CreateHtml(code_doc, false),
                          false);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
