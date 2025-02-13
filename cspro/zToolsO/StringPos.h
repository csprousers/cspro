#pragma once


class CStringPos
{
public:
    CString m_csText;
    int m_iPos;
    int m_iLen;

    CStringPos()
    {
        Empty();
    }

    CStringPos( CString csText, int iPos, int iLen )
    {
        Init( csText, iPos, iLen );
    }

    void Empty()
    {
        Init( _T(""), CSTRINGPOS_CURPOS, 0 );// RHF Mar 07, 2003
    }

    void Init( CString csText, int iPos, int iLen )
    {
        ASSERT( iPos >= CSTRINGPOS_CURPOS );
        ASSERT( iLen >= 0 );

        m_csText = csText;
        m_iPos = iPos;
        m_iLen = iLen;
    }

private:
    static const int CSTRINGPOS_ENDPOS = -1;
    static const int CSTRINGPOS_CURPOS = -2;
};
