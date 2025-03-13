#include "StdAfx.h"
#include "ProcessorMarkdown.h"
#include <zMarkdown/Markdown.h>


void ProcessorMarkdown::Run(CodeDoc& code_doc)
{
    try
    {
        std::string file_path = code_doc.GetActualOrTempFilePath(FileExtensions::Markdown);
        const std::string title = Path::GetFilename(file_path);

        code_doc.GetHtmlProcessor().DisplayHtml(Markdown::ToHtmlDocument(title, code_doc.GetPrimaryCodeView().GetLogicCtrl()->GetText()),
                                                std::move(file_path));
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
