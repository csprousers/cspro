#pragma once


// --------------------------------------------------------------------------
// HT = HTML tags shared between:
// CSDocCompilerWorker +
// CSDocCompilerWorker::MarkdownCreator
// --------------------------------------------------------------------------

namespace HT
{
    constexpr std::string_view ParagraphDiv_sv[2] = { "<div class=\"paragraph\">", "</div>\n" };
    constexpr const char* Bold[2]                 = { "<b>", "</b>" };
    constexpr const char* Italic[2]               = { "<i>", "</i>" };
    constexpr const char* Subheader[2]            = { "<div class=\"subheader_size subheader\">", "</div>" };
};
