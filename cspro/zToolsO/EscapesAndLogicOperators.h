#pragma once


// the definitions here are not in zToolsO/Encoders.h or zLogicO/SourceBuffer.cpp
// so as to make them accessible to the Scintilla lexers

namespace EncoderEscapes
{
    constexpr const char* Representations = "\'\"\\\a\b\f\n\r\t\v";
    constexpr const char* Sequences       = "\'\"\\abfnrtv";
}


namespace Logic
{
    constexpr const char* OperatorCharacters = "+*/%^()[]&|!,;@.#$:=<>-";

    static constexpr char VerbatimStringLiteralStartCh1 = '@';
    static constexpr char VerbatimStringLiteralStartCh2 = '"';
}
