#include "StdAfx.h"
#include "CSDocCompilerWorker.h"
#include <zMarkdown/Markdown.h>


// --------------------------------------------------------------------------
// CSDocCompilerWorker
// --------------------------------------------------------------------------

std::string CSDocCompilerWorker::MarkdownEndHandler(const std::string& inner_text)
{
    return Markdown::ToHtml(inner_text);
}
