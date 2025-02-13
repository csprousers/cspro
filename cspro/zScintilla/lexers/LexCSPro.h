#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include <map>
#include <string>
#include <vector>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "StringCopy.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "CharacterCategory.h"
#include "LexerModule.h"
#include "OptionSet.h"
#include "DefaultLexer.h"


class LexCSPro : public Lexilla::DefaultLexer
{
public:
    static constexpr const char* Name(int language)
    {
        return ( language == SCLEX_CSPRO_LOGIC_V0 )         ? "csprologic_v0" :
               ( language == SCLEX_CSPRO_LOGIC_V8_0 )       ? "csprologic" :
               ( language == SCLEX_CSPRO_MESSAGE_V0 )       ? "cspromessage_v0" :
               ( language == SCLEX_CSPRO_MESSAGE_V8_0 )     ? "cspromessage" :
               ( language == SCLEX_CSPRO_REPORT_V0 )        ? "csproreport_v0" :
               ( language == SCLEX_CSPRO_REPORT_V8_0 )      ? "csproreport" :
               ( language == SCLEX_CSPRO_REPORT_HTML_V0 )   ? "csproreporthtml_v0" :
               ( language == SCLEX_CSPRO_REPORT_HTML_V8_0 ) ? "csproreporthtml" :
               ( language == SCLEX_CSPRO_DOCUMENT )         ? "csprodocument" :
                                                              "";
    }

    static constexpr bool IsV8_0(int language)
    {
        return ( language == SCLEX_CSPRO_LOGIC_V8_0  ||
                 language == SCLEX_CSPRO_MESSAGE_V8_0 ||
                 language == SCLEX_CSPRO_REPORT_V8_0 ||
                 language == SCLEX_CSPRO_REPORT_HTML_V8_0 ||
                 language == SCLEX_CSPRO_DOCUMENT );
    }

protected:
    LexCSPro(int language)
        :   DefaultLexer(LexCSPro::Name(language), language)
    {
        m_lexParameters.isV8_0 = IsV8_0(language);
    }

    bool IsV8_0() const
    {
        return m_lexParameters.isV8_0;
    }

public:
    Sci_Position SCI_METHOD WordListSet(int n, const char* wl) override;

    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, Scintilla::IDocument* pAccess) override;

    void* SCI_METHOD PrivateCall(int, void*) override
    {
        return nullptr;
    }

    int SCI_METHOD LineEndTypesSupported() override
    {
        return SC_LINE_END_TYPE_UNICODE;
    }

    const char* SCI_METHOD PropertyGet(const char* /*key*/) override
    {
        assert(false);
        return nullptr;
    }

private:
    bool IsKeyword(Lexilla::StyleContext& sc, const Lexilla::WordList& keywords)
    {
        sc.GetCurrentLowered(m_keywordCheckBuffer, _countof(m_keywordCheckBuffer));
        return keywords.InList(m_keywordCheckBuffer);
    }

    void SetPostIdentifierState(Lexilla::StyleContext& sc);

private:
    struct LexParameters
    {
        Lexilla::WordList keywords;
        Lexilla::WordList function_namespaces_parent;
        Lexilla::WordList function_namespaces_child;
        Lexilla::WordList dot_notation_functions;
        bool isV8_0;
    };

    LexParameters m_lexParameters;
    char m_keywordCheckBuffer[50];
};


namespace Lexilla
{
    constexpr bool IsAlpha(int ch) noexcept
    {
        // IsAlpha is a more intuitive name for this function
        return IsUpperOrLowerCase(ch);
    }

    constexpr bool is_quotemark(int ch) noexcept // also defined in Tools.h
    {
        return ( ch == '\'' || ch == '"' );
    }

    constexpr bool IsCSProWordChar(int ch) noexcept
    {
        return ( IsAlphaNumeric(ch) || ch == '_' || ch == '$' );
    }

    constexpr bool IsCSProWordStart(int ch) noexcept
    {
        return ( IsAlpha(ch) || ch == '$' );
    }
}
