#pragma once

#include "StyleContext.h"


class CSProStyleContext : public Lexilla::StyleContext
{
public:
    // because SCE_CSPRO_DEFAULT is not 0 (to facilitate lexing reports while using the report document's styles),
    // we set the initial style correctly at the beginning of the document
    CSProStyleContext(Sci_PositionU startPos, Sci_PositionU length, int initStyle, Lexilla::LexAccessor &styler_)
        :   StyleContext(startPos,
                         length,
                         ( startPos == 0 && initStyle == 0 ) ? SCE_CSPRO_DEFAULT : initStyle,
                         styler_),
            m_lineState(styler.GetLineState(currentLine - 1))
    {
    }

    void UpdateLineState()
    {
        styler.SetLineState(currentLine, m_lineState);
    }

    void IncrementCommentNestLevel()
    {
        ++m_lineState;
        UpdateLineState();
    }

    int DecrementCommentNestLevel()
    {
        if( ( m_lineState & 0x00FFFFFF ) == 0 )
            return 0;

        --m_lineState;
        UpdateLineState();

        return ( m_lineState & 0x00FFFFFF );
    }

    unsigned char GetReportStyle() const
    {
        return ( m_lineState >> 24 );
    }

    void ClearReportStyle()
    {
        m_lineState &= 0x00FFFFFF;
        UpdateLineState();
    }

    void SetReportStyle(int style)
    {
        assert(( style & 0xFF) == style);
        m_lineState |= ( style << 24 );
        UpdateLineState();
    }

private:
    int m_lineState; // 0xXXYYYYYY; XX = report style; YYYYYY = comment nest level
};
