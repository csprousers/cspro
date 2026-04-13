#pragma once


class LexerPercentEncoding : public Lexilla::DefaultLexer
{
protected:
    using DefaultLexer::DefaultLexer;

public:
    static ILexer5* CreateLexer();

    const char* SCI_METHOD PropertyGet(const char* key) override;
    int SCI_METHOD LineEndTypesSupported() override;
    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, Scintilla::IDocument* pAccess) override;

protected:
    // also called by LexCSProPropertyString
    void LexPE(Lexilla::StyleContext& sc, bool is_property_string);
};
