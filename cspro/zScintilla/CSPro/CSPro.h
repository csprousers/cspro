#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Scintilla { class ILexer5; }


namespace CSProScintilla
{
    // the class name registered by ScintillaWin::Register and then used by CScintillaCtrl
    constexpr const wchar_t* ClassName = L"CSProScintilla";

    // creates a lexer for the given language ID
    __declspec(dllexport) Scintilla::ILexer5* CreateLexer(int lexer_language);

    // the styled text, to match SCI_GETSTYLEDTEXT, returns each character followed by its style, ending in two nulls
    __declspec(dllexport) std::unique_ptr<char[]> GetStyledText(int lexer_language, const std::vector<std::string>& keywords, std::string_view text_sv);
}
