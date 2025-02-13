#pragma once


// the definitions here are not in zToolsO/Encoders.h or zLogicO/SourceBuffer.cpp
// so as to make them accessible to the Scintilla lexers


namespace Encoders
{
    constexpr const wchar_t* EscapeRepresentations = L"\'\"\\\a\b\f\n\r\t\v";
    constexpr const wchar_t* EscapeSequences       = L"\'\"\\abfnrtv";
}


namespace Logic
{
    constexpr const wchar_t* OperatorCharacters = L"+*/%^()[]&|!,;@.#$:=<>-";

    static constexpr wchar_t VerbatimStringLiteralStartCh1 = '@';
    static constexpr wchar_t VerbatimStringLiteralStartCh2 = '"';
}
