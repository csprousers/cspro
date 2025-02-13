#include "StdAfx.h"
#include "FormBase.h"
#include <zUtilO/TraceMsg.h>


IMPLEMENT_DYNAMIC(CDEFormBase, CObject)


CDEFormBase::CDEFormBase(const CString& name/* = CString()*/, const CString& label/* = CString()*/)
    :   m_bUsed(false),
        m_name(name),
        m_label(label),
        m_iSymbol(0),
        m_iFormFileNumber(-1)
{
}


CDEFormBase::CDEFormBase(const CDEFormBase& rhs) // FORM_TODO remove if no longer derived from CObject
    :   m_bUsed(rhs.m_bUsed),
        m_name(rhs.m_name),
        m_label(rhs.m_label),
        m_cDims(rhs.m_cDims),
        m_iSymbol(rhs.m_iSymbol),
        m_iFormFileNumber(rhs.m_iFormFileNumber)        
{
}


CDEFormBase& CDEFormBase::operator=(const CDEFormBase& rhs) // FORM_TODO remove if no longer derived from CObject
{
    m_bUsed = rhs.m_bUsed;
    m_name = rhs.m_name;
    m_label = rhs.m_label;
    m_cDims = rhs.m_cDims;
    m_iSymbol = rhs.m_iSymbol;
    m_iFormFileNumber = rhs.m_iFormFileNumber;

    return *this;
}


void CDEFormBase::SetDims(CString& cs)
{
    int      x1,y1,x2,y2;
    TCHAR*   pszArgs = cs.GetBuffer(cs.GetLength());
    TCHAR*   pszArg;

    pszArg = strtoken(pszArgs, SPACE_COMMA_STR, NULL);
    x1 = _ttoi (pszArg);

    pszArg = strtoken(NULL, SPACE_COMMA_STR, NULL);
    y1 = _ttoi (pszArg);

    pszArg = strtoken(NULL, SPACE_COMMA_STR, NULL);
    x2 = _ttoi (pszArg);

    pszArg = strtoken(NULL, SPACE_COMMA_STR, NULL);
    y2 = _ttoi (pszArg);

    m_cDims.SetRect (x1,y1, x2,y2);

    cs = strtoken(NULL, _T("\""), NULL);
}

// rather than have two diff types of varss, one to keep track of those classes that
// only need one x,y coord, and another for those that need x1,y1,x2,y2, i'm using
// the same variable, but just referring to it differently, i.e., use this func when
// only one x,y combo needed (the lower vals will be set to 0, the upper will be the
// "active" pair)

void CDEFormBase::SetUpperDims(CString cs)
{
    int x2, y2;
    TCHAR* pszArgs = cs.GetBuffer(cs.GetLength());
    TCHAR* pszArg;

    pszArg = strtoken(pszArgs, SPACE_COMMA_STR, NULL);
    x2 = _ttoi (pszArg);

    pszArg = strtoken(NULL, SPACE_COMMA_STR, NULL);
    y2 = _ttoi (pszArg);

    m_cDims.SetRect (0,0,x2,y2);

    cs = strtoken(NULL, _T("\""), NULL);
}

// given a string, this will print out the values in a nice, comma-delimited string :)

void CDEFormBase::WriteDimsToStr(CString& cs, CRect rect) const
{
    CPoint cp;
    CIMSAString csTemp;

    if (rect.IsRectEmpty())
        cp = m_cDims.TopLeft();
    else
        cp = rect.TopLeft();

    csTemp.Str ((int)cp.x); cs  = csTemp;
    csTemp.Str ((int)cp.y); cs += _T(",") + csTemp;

    if (rect.IsRectEmpty())
        cp = m_cDims.BottomRight();
    else
        cp = rect.BottomRight();

    csTemp.Str ((int)cp.x);      cs += _T(",") + csTemp;
    csTemp.Str ((int)cp.y);      cs += _T(",") + csTemp;
}

// this func to be used in conjunction w/SetUpperDims()

CString CDEFormBase::GetUpperDimsStr() const
{
    const CPoint& cp = m_cDims.BottomRight();
    return FormatText(_T("%d,%d"), (int)cp.x, (int)cp.y);
}


void CDEFormBase::serialize(Serializer& ar) // 20121109
{
    ar & m_name
       & m_label
       & m_cDims
       & m_iSymbol
       & m_iFormFileNumber
       & m_bUsed;
}
