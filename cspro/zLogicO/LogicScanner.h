#pragma once

#include <zAppO/LogicSettings.h>
#include <zToolsO/EscapesAndLogicOperators.h>


namespace Logic
{
    // this class processes text, character by character, keeping track of comments and string literals
    class LogicScanner
    {
    public:
        LogicScanner(const LogicSettings& logic_settings);

        bool InSpecialSection() const { return ( m_region != Region::None ); }

        void ProcessCharacter(TCHAR ch, TCHAR next_ch);

    private:
        enum class Region { None, StringLiteral, VerbatimStringLiteral, Comment };

        const LogicSettings& m_logicSettings;
        Region m_region;
        TCHAR m_quotemarkOrCommentLevel;
        bool m_ignoreNextCh;
    };
}


inline Logic::LogicScanner::LogicScanner(const LogicSettings& logic_settings)
    :   m_logicSettings(logic_settings),
        m_region(Region::None),
        m_quotemarkOrCommentLevel(0),
        m_ignoreNextCh(false)
{
}


inline void Logic::LogicScanner::ProcessCharacter(TCHAR ch, TCHAR next_ch)
{
    if( m_ignoreNextCh )
    {
        ASSERT(_tcschr(_T("/'\""), ch) != nullptr);
        m_ignoreNextCh = false;
    }

    // on a new line, reset the state when in a single line comment or
    // when encountering an invalid string literal that spans multiple lines
    else if( ch == '\n' && ( m_region != Region::Comment || m_quotemarkOrCommentLevel == 0 ) )
    {
        m_region = Region::None;
    }

    // in a comment
    else if( m_region == Region::Comment )
    {
        // single line comments are ended above, so check if a multiline comment is ending
        if( ( m_quotemarkOrCommentLevel > 0 ) &&
            ( m_logicSettings.MeetsVersion(LogicSettings::Version::V8_0) ? ( ch == '*' && next_ch == '/' ) :
                                                                           ( ch == '}' ) ) )
        {
            m_ignoreNextCh = m_logicSettings.MeetsVersion(LogicSettings::Version::V8_0);

            if( --m_quotemarkOrCommentLevel == 0 )
                m_region = Region::None;
        }
    }

    // in a string literal
    else if( m_region != Region::None )
    {
        if( ch == m_quotemarkOrCommentLevel )
        {
            // ignore verbatim string literal escapes: ""
            if( m_region == Region::VerbatimStringLiteral && next_ch == '"' )
            {
                m_ignoreNextCh = true;
            }

            else
            {
                m_region = Region::None;
            }
        }

        else if( m_region == Region::StringLiteral && ch == '\\' && next_ch == m_quotemarkOrCommentLevel )
        {
            m_ignoreNextCh = true;
        }
    }

    // starting a string literal
    else if( is_quotemark(ch) )
    {
        m_region = Region::StringLiteral;
        m_quotemarkOrCommentLevel = ch;
    }

    // starting a verbatim string literal
    else if( ch == VerbatimStringLiteralStartCh1 && next_ch == VerbatimStringLiteralStartCh2 && m_logicSettings.UseVerbatimStringLiterals() )
    {
        m_ignoreNextCh = true;
        m_region = Region::VerbatimStringLiteral;
        m_quotemarkOrCommentLevel = '"';
    }

    // starting a single line comment
    else if( ch == '/' && next_ch == '/' )
    {
        m_region = Region::Comment;
        m_quotemarkOrCommentLevel = 0;
    }

    // starting a multiline comment
    else if( m_logicSettings.MeetsVersion(LogicSettings::Version::V8_0) ? ( ch == '/' && next_ch == '*' ) :
                                                                          ( ch == '{' ) )
    {
        m_region = Region::Comment;
        m_quotemarkOrCommentLevel = 1;
    }
}
