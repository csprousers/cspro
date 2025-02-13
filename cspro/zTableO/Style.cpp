//***************************************************************************
//  File name: Style.cpp
//
//  Description:
//       Style objects implementation
//
//  History:    Date       Author     Comment
//              -----------------------------
//              28 Oct 02   BMD       CSPro 3.0
//              Nov 2003    csc       CFmt, CVarFmt, CTblFmt added
//              Nov 2004    csc+savy  reimplementation
//
//***************************************************************************

#include "StdAfx.h"
#include "Style.h"
#include "LabelSerializer.h"
#include "TllyStat.h"
#include <zUtilO/PortableFont.h>


IMPLEMENT_DYNAMIC(CFmtBase, CObject);
IMPLEMENT_DYNAMIC(CFmt, CFmtBase);
IMPLEMENT_DYNAMIC(CDataCellFmt, CFmt);
IMPLEMENT_DYNAMIC(CTblFmt, CFmtBase);
IMPLEMENT_DYNAMIC(CTblPrintFmt, CFmtBase);
IMPLEMENT_DYNAMIC(CTallyFmt, CFmtBase);
IMPLEMENT_DYNAMIC(CTabSetFmt, CFmtBase);
IMPLEMENT_DYNAMIC(CFmtFont, CObject);
IMPLEMENT_DYNAMIC(CFmtReg, CObject);

/*static*/ CMapStringToPtr CFmtFont::m_mapFont;

/////////////////////////////////////////////////////////////////////////////
//
// helper functions
//
/////////////////////////////////////////////////////////////////////////////
float TwipsToInches(long lTwips)
{
    return (float)lTwips / (float)TWIPS_PER_INCH;
}

float TwipsToCm(long lTwips)
{
    return (float)lTwips * CM_PER_INCH / (float)TWIPS_PER_INCH;
}


long InchesToTwips(float fInches)
{
    return (long)(fInches * (float)TWIPS_PER_INCH);
}


long CmToTwips(float fCm)
{
    return (long)(fCm * (float)TWIPS_PER_INCH / CM_PER_INCH);
}

int PointsToTwips(int iPoints)
{
    return iPoints * TWIPS_PER_POINT;
}

void PointsToTwips(CFmt* pFmt)
{
    LOGFONT lf;
    ASSERT_VALID(pFmt->GetFont());
    pFmt->GetFont()->GetLogFont(&lf);
    lf.lfHeight=PointsToTwips(lf.lfHeight);
    pFmt->SetFont(&lf);
}

int TwipsToPoints(int iTwips)
{
    return iTwips / TWIPS_PER_POINT;
}

int LPToPoints(int iLP, int iLogPixelsY)
// useful for converting LOGFONT height (which is LP units) to points
{
    return MulDiv(iLP, 72, iLogPixelsY);
}


void LPToPoints(CFmt* pFmt, int iLogPixelsY)
{
    LOGFONT lf;
    ASSERT_VALID(pFmt->GetFont());
    pFmt->GetFont()->GetLogFont(&lf);
    lf.lfHeight=LPToPoints(lf.lfHeight, iLogPixelsY);
    pFmt->SetFont(&lf);
}



#include <math.h>
float Round(float fVal, int iPrecision)
{
    ASSERT(iPrecision>=1 && iPrecision<6);
    long lMag = (long) pow(10.0, iPrecision);
    return (float) floor(((float)(0.5f/lMag) + fVal) * lMag)/ (float) lMag;
}


void StripQuotes(CIMSAString& s)
{
    // strip out " or ' delimiters, if present
    int iLen = s.GetLength();
    if (s[0]==_T('"') && s[iLen-1]=='"')  {
        if (iLen>2)  {
            // has contents
            s=s.Mid(1,iLen-2);
        }
        else {
            // null string
            s.Empty();
        }
    }
    else if (s[0]==_T('\'') && s[iLen-1]=='\'')  {
        if (iLen>2)  {
            // has contents
            s=s.Mid(1,iLen-2);
        }
        else {
            // null string
            s.Empty();
        }
    }
}

// old style tally fmt (before CSPro 3.2) for converting older files
class CPre32TallyFmt
{
public:

    // constructor
    CPre32TallyFmt()
    {
        eTotalsPos = TOTALS_POSITION_DEFAULT;          // totals position and presence (default, none, above, below)

        // percents
        ePercentType = PCT_TYPE_DEFAULT;        // percent type (default, total, row, col, cell)
        ePercentPos = PCT_POS_DEFAULT;         // percent position (default, above, below, left, right)

        // counts
        eCounts = TALLY_STATISTIC_DEFAULT;             // include counts (default, yes, no, notappl)

        // statistics stuff
        eNRow = TALLY_STATISTIC_DEFAULT;                //N Row 4 Trevor
        eMin = TALLY_STATISTIC_DEFAULT;                // show minimum value (default, yes, no)
        eMax = TALLY_STATISTIC_DEFAULT;	               // show maximum value (default, yes, no)
        eStdDev = TALLY_STATISTIC_DEFAULT;             // show standard deviation (default, yes, no)
        eVariance = TALLY_STATISTIC_DEFAULT;           // show variances (default, yes, no)
        eMean = TALLY_STATISTIC_DEFAULT;               // show mean value (default, yes, no)
        eMode = TALLY_STATISTIC_DEFAULT;               // show mode value (default, yes, no)
        eStdErr = TALLY_STATISTIC_DEFAULT;             // show standard error (default, yes, no)
        eProportion = TALLY_STATISTIC_DEFAULT;         // show proportion (default, yes, no)
        eNTiles = TALLY_STATISTIC_DEFAULT;             // show n-tiles (default, yes, no)
        eMedian= MEDIAN_TYPE_DEFAULT;                  // median type (default, none, continuous, discrete)
        iTiles = 0;
    }

    TOTALS_POSITION                 eTotalsPos;
    PCT_TYPE                        ePercentType;
    PCT_POS                         ePercentPos;
    TALLY_STATISTIC                 eCounts;
    TALLY_STATISTIC                 eNRow;
    TALLY_STATISTIC	                eMin;
    TALLY_STATISTIC	                eMax;
    TALLY_STATISTIC	                eStdDev;
    TALLY_STATISTIC	                eVariance;
    TALLY_STATISTIC	                eMean;
    TALLY_STATISTIC	                eMode;
    TALLY_STATISTIC	                eStdErr;
    TALLY_STATISTIC	                eProportion;
    TALLY_STATISTIC	                eNTiles;
    MEDIAN_TYPE                     eMedian;
    int				                iTiles;
};

// convert old (pre 3.2) PCT_TYPE from CTallyFmt to new CTallyVarStatFmt enum PctType
PctType OldPctTypeToPctType(PCT_TYPE ePercentType)
{
    PctType pt = (PctType) -1;
    switch (ePercentType) {
        case PCT_TYPE_TOTAL:
            pt = PT_TOTAL;
            break;
        case PCT_TYPE_ROW:
            pt = PT_ROW;
            break;
        case PCT_TYPE_COL:
            pt = PT_COL;
            break;
    }
    return pt;
}

// add counts and percents into the given tally format in correct order based on
// ePercentPos.  Only add non-null stats.
void AddPercentsAndCounts(CTallyFmt* pFmt, PCT_POS ePercentPos, CTallyVarStatFmt* pStatCounts, CTallyVarStatFmt* pStatPct)
{
    if (ePercentPos == PCT_POS_ABOVE_OR_LEFT) {
        // percents first
        if (pStatPct) {
            pFmt->AddStat(pStatPct);
        }
        if (pStatCounts) {
            pFmt->AddStat(pStatCounts);
        }
    }
    else {
        // counts first
        if (pStatCounts) {
            pFmt->AddStat(pStatCounts);
        }
        if (pStatPct) {
            pFmt->AddStat(pStatPct);
        }
    }

}


////////////
//
// foreign key words
//
///////////


/////////////////////////////////////////////////////////////////////////////
//
//                               FOREIGN_KEYS::GetKey
//
/////////////////////////////////////////////////////////////////////////////
LPCTSTR FOREIGN_KEYS::GetKey(LPCTSTR sDefault) const
{
    const KeyMap::CPair* p = m_keys.PLookup(sDefault);
    if (p) {
        return p->value;
    }
    else {
        return sDefault;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                               FOREIGN_KEYS::SetKey
//
/////////////////////////////////////////////////////////////////////////////
void FOREIGN_KEYS::SetKey(LPCTSTR sDefault, LPCTSTR sAlt)
{
    m_keys.SetAt(sDefault, sAlt);
}

/////////////////////////////////////////////////////////////////////////////
//
//                               FOREIGN_KEYS::operator==
//
/////////////////////////////////////////////////////////////////////////////
bool FOREIGN_KEYS::operator==(const FOREIGN_KEYS& f) const
{
    // make sure that all args in this are same is in f
    CString sDef, sAlt;

	POSITION pos = m_keys.GetStartPosition();
    while (pos != NULL)
    {
        m_keys.GetNextAssoc( pos, sDef, sAlt );
        if (f.GetKey(sDef) != sAlt) {
            return false;
        }
    }
    // make sure that all args in f are same as in this
    pos = f.m_keys.GetStartPosition();
    while (pos != NULL)
    {
        f.m_keys.GetNextAssoc( pos, sDef, sAlt );
        if (GetKey(sDef) != sAlt) {
            return false;
        }
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////
//
//                               FOREIGN_KEYS::operator=
//
/////////////////////////////////////////////////////////////////////////////
FOREIGN_KEYS& FOREIGN_KEYS::operator=(const FOREIGN_KEYS& f)
{
    m_keys.RemoveAll();
    CString sDef, sAlt;

	POSITION pos = f.m_keys.GetStartPosition();
    while (pos != NULL)
    {
        f.m_keys.GetNextAssoc( pos, sDef, sAlt );
        m_keys.SetAt(sDef, sAlt);
    }

    return *this;
}

/////////////////////////////////////////////////////////////////////////////
//
//                               FOREIGN_KEYS::Save
//
/////////////////////////////////////////////////////////////////////////////
void FOREIGN_KEYS::Save(CSpecFile& specFile) const
{
    CString sDef, sAlt;

	POSITION pos = m_keys.GetStartPosition();
    while (pos != NULL)
    {
        m_keys.GetNextAssoc( pos, sDef, sAlt );
        specFile.PutLine(TFT_CMD_FOREIGN_KEY, MakeForeignKeyString(sDef,sAlt));
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                               FOREIGN_KEYS::GetAllKeys
//
/////////////////////////////////////////////////////////////////////////////
void FOREIGN_KEYS::GetAllKeys(CStringArray& aDefaults) const
{
    CString sDef, sAlt;

	POSITION pos = m_keys.GetStartPosition();
    while (pos != NULL)
    {
        m_keys.GetNextAssoc( pos, sDef, sAlt );
        aDefaults.Add(sDef);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                               FOREIGN_KEYS::MakeForeignKeyString
//
/////////////////////////////////////////////////////////////////////////////
CIMSAString FOREIGN_KEYS::MakeForeignKeyString(CIMSAString sDefaultString ,CIMSAString sAltString) const
{
    CIMSAString sRet ;
    sDefaultString.Trim();
    sAltString.Trim();
    sRet = sDefaultString;

    if(sDefaultString.CompareNoCase(sAltString) == 0 || sAltString.IsEmpty()){
        sRet.QuoteDelimit();
        return sRet;
    }
    else {
        sRet.QuoteDelimit();
        sAltString.QuoteDelimit();
        sRet += _T(",") + sAltString;
    }
    return sRet;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CFmtFont::SetFont
//
/////////////////////////////////////////////////////////////////////////////

void CFmtFont::SetFont(LOGFONT* pLF)
{
    CFont* pFont=NULL;
    CString sLF = PortableFont(*pLF).GetPre80String();
    if (!m_mapFont.Lookup(sLF, (void*&)pFont)) {
        // font isn't yet in the map, so create it ...
        pFont=new CFont;
        VERIFY(pFont->CreateFontIndirect(pLF));
        m_mapFont.SetAt(sLF, (void*)pFont);
    }
    ASSERT_KINDOF(CFont,pFont);
    SetFont(pFont);
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CFmtBase::CFmtBase
//
/////////////////////////////////////////////////////////////////////////////
CFmtBase::CFmtBase(void)
{
	SetUsedFlag(true);
    Init();
}

CFmtBase::CFmtBase(const CFmtBase& f)
{
	SetUsedFlag(true);
	*this=f;
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CFmtBase::Init
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ void CFmtBase::Init(void)
{
    SetID(FMT_ID_INVALID);
    SetIndex(0);
	SetUsedFlag(true);
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CFmtBase::SetID
//                               CFmtBase::SetIndex
//
/////////////////////////////////////////////////////////////////////////////
bool CFmtBase::SetID(const CIMSAString& sID)
{
    CIMSAString s;
    s=sID;
    s.Trim();
    if (s.CompareNoCase(TFT_FMT_ID_SPANNER)==0) {
        SetID(FMT_ID_SPANNER);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_COLHEAD)==0) {
        SetID(FMT_ID_COLHEAD);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_CAPTION)==0) {
        SetID(FMT_ID_CAPTION);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_STUB)==0) {
        SetID(FMT_ID_STUB);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_DATACELL)==0) {
        SetID(FMT_ID_DATACELL);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_HEADER_LEFT)==0) {
        SetID(FMT_ID_HEADER_LEFT);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_HEADER_CENTER)==0) {
        SetID(FMT_ID_HEADER_CENTER);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_HEADER_RIGHT)==0) {
        SetID(FMT_ID_HEADER_RIGHT);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_FOOTER_LEFT)==0) {
        SetID(FMT_ID_FOOTER_LEFT);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_FOOTER_CENTER)==0) {
        SetID(FMT_ID_FOOTER_CENTER);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_FOOTER_RIGHT)==0) {
        SetID(FMT_ID_FOOTER_RIGHT);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_TITLE)==0) {
        SetID(FMT_ID_TITLE);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_SUBTITLE)==0) {
        SetID(FMT_ID_SUBTITLE);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_PAGENOTE)==0) {
        SetID(FMT_ID_PAGENOTE);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_ENDNOTE)==0) {
        SetID(FMT_ID_ENDNOTE);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_STUBHEAD)==0) {
        SetID(FMT_ID_STUBHEAD);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_STUBHEAD_SEC)==0) {
        SetID(FMT_ID_STUBHEAD_SEC);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_AREA_CAPTION)==0) {
        SetID(FMT_ID_AREA_CAPTION);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_TALLY)==0) {
        SetID(FMT_ID_TALLY);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_TALLY_ROW)==0) {
        SetID(FMT_ID_TALLY_ROW);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_TALLY_COL)==0) {
        SetID(FMT_ID_TALLY_COL);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_TABSET)==0) {
        SetID(FMT_ID_TABSET);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_TABLE)==0) {
        SetID(FMT_ID_TABLE);
    }
    else if (s.CompareNoCase(TFT_FMT_ID_TBLPRINT)==0) {
        SetID(FMT_ID_TBLPRINT);
    }
    else {
        SetID(FMT_ID_INVALID);
        return false;
    }
    return true;
}


bool CFmtBase::SetIndex(const CIMSAString& sIndex)
{
    CIMSAString s;
    s=sIndex;
    s.Trim();
    if (s.CompareNoCase(_T("default"))==0) {
        SetIndex(0);
    }
    else {
        if (!s.IsNumeric()) {
            SetIndex(NONE);
            return false;
        }
        SetIndex((int)s.Val());
    }
    return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CFmtBase::Build
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ bool CFmtBase::Build(CSpecFile& specFile, bool bSilent)  {
    UNREFERENCED_PARAMETER(bSilent);

	CIMSAString sCmd;    // command (left side of =)
    CIMSAString sArg;    // argument (right side of =)
    bool bLineOK;

    // read line by line ...
    while (specFile.GetState() == SF_OK)  {
        specFile.GetLine(sCmd, sArg);
        ASSERT (!sCmd.IsEmpty());
        if (sCmd[0] == '[')  {
            // we are starting a new section ... put back the line and quit
            specFile.UngetLine();
            break;
        }
        bLineOK = false;
        StripQuotes(sArg);

        // identification
        if (!sCmd.CompareNoCase(TFT_CMD_NAME))  {
            CIMSAString s;

            // sArg should be in format id-string,index-string

            s=sArg.GetToken();
            bLineOK=SetID(s);

            if (bLineOK) {
                s=sArg.GetToken();
                bLineOK=SetIndex(s);
            }

            // verify that this is a legal ID (confirm that class derivation matches too) ...
            if (bLineOK) {
                if (IsKindOf(RUNTIME_CLASS(CTabSetFmt)))  {
                    bLineOK=(GetID()==FMT_ID_TABSET);
                }
                else if (IsKindOf(RUNTIME_CLASS(CTblFmt)))  {
                    bLineOK=(GetID()==FMT_ID_TABLE);
                }
                else if (IsKindOf(RUNTIME_CLASS(CTallyFmt)))  {
                    bLineOK=(GetID()==FMT_ID_TALLY ||GetID()==FMT_ID_TALLY_ROW || GetID()==FMT_ID_TALLY_COL);
                }
                else if (IsKindOf(RUNTIME_CLASS(CTblPrintFmt)))  {
                    bLineOK=(GetID()==FMT_ID_TBLPRINT);
                }
                else if (IsKindOf(RUNTIME_CLASS(CDataCellFmt)))  {
                    bLineOK=(GetID()==FMT_ID_DATACELL);
                }
                else if (IsKindOf(RUNTIME_CLASS(CFmt)))  {
                    bLineOK=(GetID()==FMT_ID_SPANNER ||
                        GetID()==FMT_ID_COLHEAD ||
                        GetID()==FMT_ID_CAPTION ||
                        GetID()==FMT_ID_STUB ||
                        GetID()==FMT_ID_HEADER_LEFT ||
                        GetID()==FMT_ID_HEADER_CENTER ||
                        GetID()==FMT_ID_HEADER_RIGHT ||
                        GetID()==FMT_ID_FOOTER_LEFT ||
                        GetID()==FMT_ID_FOOTER_CENTER ||
                        GetID()==FMT_ID_FOOTER_RIGHT ||
                        GetID()==FMT_ID_TITLE ||
                        GetID()==FMT_ID_SUBTITLE ||
                        GetID()==FMT_ID_PAGENOTE ||
                        GetID()==FMT_ID_ENDNOTE ||
                        GetID()==FMT_ID_STUBHEAD ||
                        GetID()==FMT_ID_AREA_CAPTION ||
                        GetID()==FMT_ID_STUBHEAD_SEC);
                }
                else {
                    bLineOK=false;
                    AfxMessageBox(_T("Invalid runtime class ")+sArg);
                    ASSERT(FALSE);
                }
                // we have all the info we need for the base class (it's just ID + index) ... all done!
                return true;
            }
        }

        if (!bLineOK)  {
            // signal unrecognized command
			CIMSAString sMsg;
            sMsg.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
			sMsg += _T("\n") + sCmd + _T("=") + sArg;
			AfxMessageBox(sMsg,MB_ICONEXCLAMATION);
            return false;
        }
    }
    return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CFmtBase::Save
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ bool CFmtBase::Save(CSpecFile& specFile) const  {
    CIMSAString s;

    // section heading
    specFile.PutLine(GetSectionHeading());  // calls derived-class version

    // get ID
    specFile.PutLine(TFT_CMD_NAME, GetIDInfo());

    return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CFmtBase::operator==
//                               CFmtBase::operator!=
//                               CFmtBase::operator=
//
// Note: only compares ID, not index.
//
/////////////////////////////////////////////////////////////////////////////
bool CFmtBase::operator==(const CFmtBase& f) const
{
    return (GetID()==f.GetID());
}


bool CFmtBase::operator!=(const CFmtBase& f) const
{
    return !(operator==(f));
}


void CFmtBase::operator=(const CFmtBase& f)
{
    SetID(f.GetID());
    SetIndex(f.GetIndex());
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CFmtBase::GetIDString()
//
/////////////////////////////////////////////////////////////////////////////
CIMSAString CFmtBase::GetIDString(void) const
{
    CIMSAString sID;
    switch(GetID()) {
    case FMT_ID_SPANNER:
        sID=TFT_FMT_ID_SPANNER;
        break;
    case FMT_ID_COLHEAD:
        sID=TFT_FMT_ID_COLHEAD;
        break;
    case FMT_ID_CAPTION:
        sID=TFT_FMT_ID_CAPTION;
        break;
    case FMT_ID_STUB:
        sID=TFT_FMT_ID_STUB;
        break;
    case FMT_ID_DATACELL:
        sID=TFT_FMT_ID_DATACELL;
        break;
    case FMT_ID_HEADER_LEFT:
        sID=TFT_FMT_ID_HEADER_LEFT;
        break;
    case FMT_ID_HEADER_CENTER:
        sID=TFT_FMT_ID_HEADER_CENTER;
        break;
    case FMT_ID_HEADER_RIGHT:
        sID=TFT_FMT_ID_HEADER_RIGHT;
        break;
    case FMT_ID_FOOTER_LEFT:
        sID=TFT_FMT_ID_FOOTER_LEFT;
        break;
    case FMT_ID_FOOTER_CENTER:
        sID=TFT_FMT_ID_FOOTER_CENTER;
        break;
    case FMT_ID_FOOTER_RIGHT:
        sID=TFT_FMT_ID_FOOTER_RIGHT;
        break;
    case FMT_ID_TITLE:
        sID=TFT_FMT_ID_TITLE;
        break;
    case FMT_ID_SUBTITLE:
        sID=TFT_FMT_ID_SUBTITLE;
        break;
    case FMT_ID_PAGENOTE:
        sID=TFT_FMT_ID_PAGENOTE;
        break;
    case FMT_ID_ENDNOTE:
        sID=TFT_FMT_ID_ENDNOTE;
        break;
    case FMT_ID_STUBHEAD:
        sID=TFT_FMT_ID_STUBHEAD;
        break;
    case FMT_ID_STUBHEAD_SEC:
        sID=TFT_FMT_ID_STUBHEAD_SEC;
        break;
    case FMT_ID_AREA_CAPTION:
        sID=TFT_FMT_ID_AREA_CAPTION;
        break;
    case FMT_ID_TALLY:
        sID=TFT_FMT_ID_TALLY;
        break;
    case FMT_ID_TALLY_ROW:
        sID=TFT_FMT_ID_TALLY_ROW;
        break;
    case FMT_ID_TALLY_COL:
        sID=TFT_FMT_ID_TALLY_COL;
        break;
    case FMT_ID_TABSET:
        sID=TFT_FMT_ID_TABSET;
        break;
    case FMT_ID_TABLE:
        sID=TFT_FMT_ID_TABLE;
        break;
    case FMT_ID_TBLPRINT:
        sID=TFT_FMT_ID_TBLPRINT;
        break;
    default:
        ASSERT(false);
        break;
    }
    return sID;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CFmtBase::GetIndexString()
//
// index 0 is returned as "default"
//
/////////////////////////////////////////////////////////////////////////////
CIMSAString CFmtBase::GetIndexString(void) const
{
    CIMSAString sIndex;
    if (GetIndex()==0) {
        sIndex=_T("default");
    }
    else {
        sIndex.Str(GetIndex());
    }
    return sIndex;
}



/////////////////////////////////////////////////////////////////////////////
//
//                               CFmt::CFmt
//
/////////////////////////////////////////////////////////////////////////////
CFmt::CFmt(void)
{
    Init();
}

CFmt::CFmt(const CFmt& f)
{
    *this=f;
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CFmt::Init
//
/////////////////////////////////////////////////////////////////////////////
void CFmt::Init(void)
{
    // init base class members ...
    CFmtBase::Init();

    // init class members ...
    SetFont((CFont*)NULL);
    SetHorzAlign(HALIGN_DEFAULT);
    SetVertAlign(VALIGN_DEFAULT);
    m_colorText.m_bUseDefault=true;
    m_colorFill.m_bUseDefault=true;
    SetIndent(LEFT, INDENT_DEFAULT);
    SetIndent(RIGHT, INDENT_DEFAULT);
    SetLineLeft(LINE_DEFAULT);
    SetLineRight(LINE_DEFAULT);
    SetLineTop(LINE_DEFAULT);
    SetLineBottom(LINE_DEFAULT);
    SetHidden(HIDDEN_DEFAULT);
    SetSpanCells(SPAN_CELLS_DEFAULT);
    m_custom.m_bIsCustomized=false;
    SetUnits(UNITS_METRIC);

    SetFontExtends(false);
    SetTextColorExtends(false);
    SetFillColorExtends(false);
    SetLinesExtend(false);
    SetIndentationExtends(false);
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CFmt::Build
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ bool CFmt::Build(CSpecFile& specFile, const CString& sVersion, bool bSilent)  {
    CIMSAString sCmd;    // command (left side of =)
    CIMSAString sArg;    // argument (right side of =)
    bool bLineOK;

    // build base class
    CFmtBase::Build(specFile, bSilent);

    // now read line by line ...
    while (specFile.GetState() == SF_OK)  {
        specFile.GetLine(sCmd, sArg, false);      // BMD 21 Sep 2006
        sCmd.Trim();
        sArg.TrimRight();
//        ASSERT (!sCmd.IsEmpty());               // BMD 21 Sep 2006
        if (sCmd[0] == '[')  {
            // we are starting a new section ... put back the line and quit
            specFile.UngetLine();
            break;
        }
        bLineOK = false;
        StripQuotes(sArg);

        // font
        if (!sCmd.CompareNoCase(TFT_CMD_FONT))  {
            if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                SetFont((CFont*)NULL);
            }
            else {
                PortableFont font;
                font.BuildFromPre80String(sArg);
                LOGFONT lf = font;
                // todo: add error checking here

  //              // we always use TWIPS here, so convert height to twips
  //              lf.lfHeight = PointsToTwips(lf.lfHeight);
                SetFont(&lf);
            }
            bLineOK=true;
        }

        // horizontal alignment
        if (!sCmd.CompareNoCase(TFT_CMD_HORZALIGN))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_LEFT)==0)  {
                SetHorzAlign(HALIGN_LEFT);
            }
            else if (sArg.CompareNoCase(TFT_ARG_CENTER)==0)  {
                SetHorzAlign(HALIGN_CENTER);
            }
            else if (sArg.CompareNoCase(TFT_ARG_RIGHT)==0)  {
                SetHorzAlign(HALIGN_RIGHT);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                SetHorzAlign(HALIGN_DEFAULT);
            }
            else  {
                // bad argument
                bLineOK=false;
            }
        }

        // vertical alignment
        if (!sCmd.CompareNoCase(TFT_CMD_VERTALIGN))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_TOP)==0)  {
                SetVertAlign(VALIGN_TOP);
            }
            else if (sArg.CompareNoCase(TFT_ARG_MID)==0)  {
                SetVertAlign(VALIGN_MID);
            }
            else if (sArg.CompareNoCase(TFT_ARG_BOTTOM)==0)  {
                SetVertAlign(VALIGN_BOTTOM);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                SetVertAlign(VALIGN_DEFAULT);
            }
            else  {
                // bad argument
                bLineOK=false;
            }
        }

        // text color
        if (!sCmd.CompareNoCase(TFT_CMD_COLOR_TEXT))  {
            FMT_COLOR color;
            if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                color.m_rgb=rgbBlack;  // just to initialize it
                color.m_bUseDefault=true;
            }
            else {
                color.m_rgb=(COLORREF) atoi64((const csprochar*) sArg);
                color.m_bUseDefault=false;
            }
            SetTextColor(color);
            bLineOK=true;
        }

        // fill color
        if (!sCmd.CompareNoCase(TFT_CMD_COLOR_FILL))  {
            FMT_COLOR color;
            if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                color.m_rgb=rgbBlack;  // just to initialize it
                color.m_bUseDefault=true;
            }
            else {
                color.m_rgb=(COLORREF) atoi64((const csprochar*) sArg);
                color.m_bUseDefault=false;
            }
            SetFillColor(color);
            bLineOK=true;
        }

        // left indentation
        if (!sCmd.CompareNoCase(TFT_CMD_INDENT_LEFT))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                SetIndent(LEFT, INDENT_DEFAULT);
            }
            else {
                /*if (!sArg.IsNumeric())  {
                    bLineOK=false;
                }
                else*/ {
                    float f = (float)sArg.fVal();
                    if (GetUnits()==UNITS_METRIC) {
                        SetIndent(LEFT, CmToTwips(f));
                    }
                    else {
                        SetIndent(LEFT, InchesToTwips(f));
                    }
                }
            }
        }

        // right indentation
        if (!sCmd.CompareNoCase(TFT_CMD_INDENT_RIGHT))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                SetIndent(RIGHT, INDENT_DEFAULT);
            }
            else {
                /*if (!sArg.IsNumeric())  {
                    bLineOK=false;
                }
                else */{
                    float f = (float) sArg.fVal();
                    if (GetUnits()==UNITS_METRIC) {
                        SetIndent(RIGHT, CmToTwips(f));
                    }
                    else {
                        SetIndent(RIGHT, InchesToTwips(f));
                    }
                }
            }
        }

        // top line
        if (!sCmd.CompareNoCase(TFT_CMD_LINE_TOP))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_THICK)==0)  {
                SetLineTop(LINE_THICK);
            }
            else if (sArg.CompareNoCase(TFT_ARG_THIN)==0)  {
                SetLineTop(LINE_THIN);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NONE)==0)  {
                SetLineTop(LINE_NONE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NOTAPPL)==0)  {
                SetLineTop(LINE_NOT_APPL);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                SetLineTop(LINE_DEFAULT);
            }
            else  {
                // bad argument
                bLineOK=false;
            }
        }

        // bottom line
        if (!sCmd.CompareNoCase(TFT_CMD_LINE_BOTTOM))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_THICK)==0)  {
                SetLineBottom(LINE_THICK);
            }
            else if (sArg.CompareNoCase(TFT_ARG_THIN)==0)  {
                SetLineBottom(LINE_THIN);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NONE)==0)  {
                SetLineBottom(LINE_NONE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NOTAPPL)==0)  {
                SetLineBottom(LINE_NOT_APPL);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                SetLineBottom(LINE_DEFAULT);
            }
            else  {
                // bad argument
                bLineOK=false;
            }
        }

        // left line
        if (!sCmd.CompareNoCase(TFT_CMD_LINE_LEFT))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_THICK)==0)  {
                SetLineLeft(LINE_THICK);
            }
            else if (sArg.CompareNoCase(TFT_ARG_THIN)==0)  {
                SetLineLeft(LINE_THIN);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NONE)==0)  {
                SetLineLeft(LINE_NONE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NOTAPPL)==0)  {
                SetLineLeft(LINE_NOT_APPL);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                SetLineLeft(LINE_DEFAULT);
            }
            else  {
                // bad argument
                bLineOK=false;
            }
        }

        // right line
        if (!sCmd.CompareNoCase(TFT_CMD_LINE_RIGHT))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_THICK)==0)  {
                SetLineRight(LINE_THICK);
            }
            else if (sArg.CompareNoCase(TFT_ARG_THIN)==0)  {
                SetLineRight(LINE_THIN);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NONE)==0)  {
                SetLineRight(LINE_NONE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NOTAPPL)==0)  {
                SetLineRight(LINE_NOT_APPL);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                SetLineRight(LINE_DEFAULT);
            }
            else  {
                // bad argument
                bLineOK=false;
            }
        }

        // hidden
        if (!sCmd.CompareNoCase(TFT_CMD_HIDDEN))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0)  {
                SetHidden(HIDDEN_YES);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0)  {
                SetHidden(HIDDEN_NO);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NOTAPPL)==0)  {
                SetHidden(HIDDEN_NOT_APPL);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                SetHidden(HIDDEN_DEFAULT);
            }
            else  {
                // bad argument
                bLineOK=false;
            }
        }

        // span cells
        if (!sCmd.CompareNoCase(TFT_CMD_SPAN_CELLS))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0)  {
                SetSpanCells(SPAN_CELLS_YES);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0)  {
                SetSpanCells(SPAN_CELLS_NO);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NOTAPPL)==0)  {
                SetSpanCells(SPAN_CELLS_NOT_APPL);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                SetSpanCells(SPAN_CELLS_DEFAULT);
            }
            else  {
                // bad argument
                bLineOK=false;
            }
        }

        // custom
        if (!sCmd.CompareNoCase(TFT_CMD_CUSTOM))  {
            bLineOK=true;
            CUSTOM custom;
            if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                custom.m_sCustomText.Empty();
                custom.m_bIsCustomized=false;
            }
            else {
                if (sVersion.CompareNoCase(_T("CSPro 8.0")) < 0) {
                    CIMSAString csTemp = sArg;
                    int p = csTemp.Find(_T("\\n"));
                    while (p >= 0) {
                        csTemp.SetAt(p++,'\r');
                        csTemp.SetAt(p,'\n');
                        p = csTemp.Find(_T("\\n"),p);
                    }
                    custom.m_sCustomText = csTemp;
                }
                else {
                    custom.m_sCustomText = TableLabelSerializer::ParseV8(sArg);
                }

                custom.m_bIsCustomized = true;
            }
            SetCustom(custom);
        }

        // font extends
        if (!sCmd.CompareNoCase(TFT_CMD_EXTEND_FONT)) {
            if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                AfxMessageBox(_T("Default not allowed for TFT_CMD_EXTEND_FONT"));
            }
            else {
                SetFontExtends(sArg.CompareNoCase(TFT_ARG_YES)==0);
            }
            bLineOK=true;
        }

        // text color extends
        if (!sCmd.CompareNoCase(TFT_CMD_EXTEND_TEXT_COLOR)) {
            if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                AfxMessageBox(_T("Default not allowed for TFT_CMD_EXTEND_TEXT_COLOR"));
            }
            else {
                SetTextColorExtends(sArg.CompareNoCase(TFT_ARG_YES)==0);
            }
            bLineOK=true;
        }

        // fill color extends
        if (!sCmd.CompareNoCase(TFT_CMD_EXTEND_FILL_COLOR)) {
            if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                AfxMessageBox(_T("Default not allowed for TFT_CMD_EXTEND_FILL_COLOR"));
            }
            else {
                SetFillColorExtends(sArg.CompareNoCase(TFT_ARG_YES)==0);
            }
            bLineOK=true;
        }

        // lines extend
        if (!sCmd.CompareNoCase(TFT_CMD_EXTEND_LINES)) {
            if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                AfxMessageBox(_T("Default not allowed for TFT_CMD_EXTEND_LINES"));
            }
            else {
                SetLinesExtend(sArg.CompareNoCase(TFT_ARG_YES)==0);
            }
            bLineOK=true;
        }

        // indentation extends
        if (!sCmd.CompareNoCase(TFT_CMD_EXTEND_INDENTATION)) {
            if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                AfxMessageBox(_T("Default not allowed for TFT_CMD_EXTEND_INDENTATION"));
            }
            else {
                SetIndentationExtends(sArg.CompareNoCase(TFT_ARG_YES)==0);
            }
            bLineOK=true;
        }

        if (!bLineOK)  {

            // if we're a datacell format and we just say num decimals, then we're done
            if ((sCmd.CompareNoCase(TFT_CMD_DECIMALS) ==0 || sCmd.CompareNoCase(TFT_CMD_HIDE_ZERO_ROW) ==0) && (GetID()==FMT_ID_DATACELL ||
                GetID()==FMT_ID_COLHEAD || GetID()==FMT_ID_STUB ||
                GetID()==FMT_ID_SPANNER || GetID()==FMT_ID_CAPTION))  {
                specFile.UngetLine();
                return true;
            }
            if (sCmd.IsEmpty()) {         // BMD 21 Sep 2006
                return true;
            }
            // signal unrecognized command
			CIMSAString sMsg;
            sMsg.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
			sMsg += _T("\n") + sCmd + _T("=") + sArg;
			AfxMessageBox(sMsg,MB_ICONEXCLAMATION);

			return false;
        }
    }
    return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CFmt::Save
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ bool CFmt::Save(CSpecFile& specFile) const  {

    // save base class stuff...
    CFmtBase::Save(specFile);

    // font
    CFont* pFont=GetFont();
    if (pFont!=NULL) {
        LOGFONT lf;      // convert twips back to points for storage

        // only put out font if it's non-null, meaning non-default
        if (!pFont->GetLogFont(&lf)) {
            AfxMessageBox(_T("internal error retrieving logfont"));
        }
        else {
//            lf.lfHeight=TwipsToPoints(lf.lfHeight);  // store it in points
            specFile.PutLine(TFT_CMD_FONT, PortableFont(lf).GetPre80String());
        }
    }

	// horizontal alignment
    switch (GetHorzAlign())  {
    case HALIGN_LEFT:
        specFile.PutLine(TFT_CMD_HORZALIGN, TFT_ARG_LEFT);
        break;
    case HALIGN_CENTER:
        specFile.PutLine(TFT_CMD_HORZALIGN, TFT_ARG_CENTER);
        break;
    case HALIGN_RIGHT:
        specFile.PutLine(TFT_CMD_HORZALIGN, TFT_ARG_RIGHT);
        break;
    case HALIGN_DEFAULT:
        // do nothing
        break;
    default:
        ASSERT(FALSE);
    }

    // vertical alignment
    switch (GetVertAlign())  {
    case VALIGN_TOP:
        specFile.PutLine(TFT_CMD_VERTALIGN, TFT_ARG_TOP);
        break;
    case VALIGN_MID:
        specFile.PutLine(TFT_CMD_VERTALIGN, TFT_ARG_MID);
        break;
    case VALIGN_BOTTOM:
        specFile.PutLine(TFT_CMD_VERTALIGN, TFT_ARG_BOTTOM);
        break;
    case VALIGN_DEFAULT:
        // do nothing
        break;
    default:
        ASSERT(FALSE);
    }

    // text color
    if (IsTextColorCustom()) {
        CIMSAString s;
        s.Str((__int64)GetTextColor().m_rgb);
        specFile.PutLine(TFT_CMD_COLOR_TEXT, s);
    }

    // fill color
    if (IsFillColorCustom()) {
        CIMSAString s;
        s.Str((__int64)GetFillColor().m_rgb);
        specFile.PutLine(TFT_CMD_COLOR_FILL, s);
    }

    // left indentation
    if (IsIndentCustom(LEFT)) {
        float f;
        if (GetUnits()==UNITS_METRIC) {
            f = Round(TwipsToCm(GetIndent(LEFT)),2);
        }
        else {
            f = Round(TwipsToInches(GetIndent(LEFT)),2);
        }

        CIMSAString s;
        s.Format(_T("%.2f"), f);
        s.Replace(_T('.'),CIMSAString::GetDecChar());
        specFile.PutLine(TFT_CMD_INDENT_LEFT, s);
    }

    // right indentation
    if (IsIndentCustom(RIGHT)) {
        float f;
        if (GetUnits()==UNITS_METRIC) {
            f = Round(TwipsToCm(GetIndent(RIGHT)),2);
        }
        else {
            f = Round(TwipsToInches(GetIndent(RIGHT)),2);
        }

        CIMSAString s;
        s.Format(_T("%.2f"), f);
        s.Replace(_T('.'),CIMSAString::GetDecChar());
        specFile.PutLine(TFT_CMD_INDENT_RIGHT, s);
    }

    // top line
    switch (GetLineTop())  {
    case LINE_THICK:
        specFile.PutLine(TFT_CMD_LINE_TOP, TFT_ARG_THICK);
        break;
    case LINE_THIN:
        specFile.PutLine(TFT_CMD_LINE_TOP, TFT_ARG_THIN);
        break;
    case LINE_NONE:
        specFile.PutLine(TFT_CMD_LINE_TOP, TFT_ARG_NONE);
        break;
    case LINE_NOT_APPL:
        specFile.PutLine(TFT_CMD_LINE_TOP, TFT_ARG_NOTAPPL);
        ASSERT(GetID()==FMT_ID_DATACELL);
        break;
    case LINE_DEFAULT:
        // do nothing
        break;
    default:
        ASSERT(FALSE);
    }

    // bottom line
    switch (GetLineBottom())  {
    case LINE_THICK:
        specFile.PutLine(TFT_CMD_LINE_BOTTOM, TFT_ARG_THICK);
        break;
    case LINE_THIN:
        specFile.PutLine(TFT_CMD_LINE_BOTTOM, TFT_ARG_THIN);
        break;
    case LINE_NONE:
        specFile.PutLine(TFT_CMD_LINE_BOTTOM, TFT_ARG_NONE);
        break;
    case LINE_NOT_APPL:
        specFile.PutLine(TFT_CMD_LINE_BOTTOM, TFT_ARG_NOTAPPL);
        ASSERT(GetID()==FMT_ID_DATACELL);
        break;
    case LINE_DEFAULT:
        // do nothing
        break;
    default:
        ASSERT(FALSE);
    }

    // left line
    switch (GetLineLeft())  {
    case LINE_THICK:
        specFile.PutLine(TFT_CMD_LINE_LEFT, TFT_ARG_THICK);
        break;
    case LINE_THIN:
        specFile.PutLine(TFT_CMD_LINE_LEFT, TFT_ARG_THIN);
        break;
    case LINE_NONE:
        specFile.PutLine(TFT_CMD_LINE_LEFT, TFT_ARG_NONE);
        break;
    case LINE_NOT_APPL:
        specFile.PutLine(TFT_CMD_LINE_LEFT, TFT_ARG_NOTAPPL);
        ASSERT(GetID()==FMT_ID_DATACELL);
        break;
    case LINE_DEFAULT:
        // do nothing
        break;
    default:
        ASSERT(FALSE);
    }

    // right line
    switch (GetLineRight())  {
    case LINE_THICK:
        specFile.PutLine(TFT_CMD_LINE_RIGHT, TFT_ARG_THICK);
        break;
    case LINE_THIN:
        specFile.PutLine(TFT_CMD_LINE_RIGHT, TFT_ARG_THIN);
        break;
    case LINE_NONE:
        specFile.PutLine(TFT_CMD_LINE_RIGHT, TFT_ARG_NONE);
        break;
    case LINE_NOT_APPL:
        specFile.PutLine(TFT_CMD_LINE_RIGHT, TFT_ARG_NOTAPPL);
        ASSERT(GetID()==FMT_ID_DATACELL);
        break;
    case LINE_DEFAULT:
        // do nothing
        break;
    default:
        ASSERT(FALSE);
    }

    // hidden
    switch(GetHidden()) {
    case HIDDEN_YES:
        specFile.PutLine(TFT_CMD_HIDDEN, TFT_ARG_YES);
        break;
    case HIDDEN_NO:
        specFile.PutLine(TFT_CMD_HIDDEN, TFT_ARG_NO);
        break;
    case HIDDEN_NOT_APPL:
        specFile.PutLine(TFT_CMD_HIDDEN, TFT_ARG_NOTAPPL);
        ASSERT(GetID()==FMT_ID_DATACELL || GetID()==FMT_ID_SPANNER || GetID()==FMT_ID_CAPTION);
        break;
    case HIDDEN_DEFAULT:
        // do nothing
        break;
    default:
        ASSERT(FALSE);
    }

    // span cells
    switch(GetSpanCells()) {
    case SPAN_CELLS_YES:
        specFile.PutLine(TFT_CMD_SPAN_CELLS, TFT_ARG_YES);
        ASSERT(GetID()==FMT_ID_CAPTION || GetID()==FMT_ID_AREA_CAPTION);
        break;
    case SPAN_CELLS_NO:
        specFile.PutLine(TFT_CMD_SPAN_CELLS, TFT_ARG_NO);
        ASSERT(GetID()==FMT_ID_CAPTION || GetID()==FMT_ID_AREA_CAPTION);
        break;
    case SPAN_CELLS_NOT_APPL:
        specFile.PutLine(TFT_CMD_SPAN_CELLS, TFT_ARG_NOTAPPL);
        ASSERT(GetID()!=FMT_ID_CAPTION && GetID()!=FMT_ID_AREA_CAPTION);
        break;
    case SPAN_CELLS_DEFAULT:
        // do nothing
        break;
    default:
        ASSERT(FALSE);
    }

    // custom
    if (IsTextCustom()) {
        specFile.PutLine(TFT_CMD_CUSTOM, TableLabelSerializer::Create(GetCustom().m_sCustomText));
    }

    // font extends
    if (GetFontExtends()) {
        specFile.PutLine(TFT_CMD_EXTEND_FONT, TFT_ARG_YES);
    }

    // text color extends
    if (GetTextColorExtends()) {
        specFile.PutLine(TFT_CMD_EXTEND_TEXT_COLOR, TFT_ARG_YES);
    }

    // fill color extends
    if (GetFillColorExtends()) {
        specFile.PutLine(TFT_CMD_EXTEND_FILL_COLOR, TFT_ARG_YES);
    }

    // lines extend
    if (GetLinesExtend()) {
        specFile.PutLine(TFT_CMD_EXTEND_LINES, TFT_ARG_YES);
    }

    // indentation extends
    if (GetIndentationExtends()) {
        specFile.PutLine(TFT_CMD_EXTEND_INDENTATION, TFT_ARG_YES);
    }

    return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CFmt::operator==
//                               CFmt::operator!=
//                               CFmt::operator=
//
/////////////////////////////////////////////////////////////////////////////
bool CFmt::operator==(const CFmt& f) const
{
    return (CFmtBase::operator==(f) &&
        GetFont()==f.GetFont() &&
        GetHorzAlign()==f.GetHorzAlign() &&
        GetVertAlign()==f.GetVertAlign() &&
        GetTextColor()==f.GetTextColor() &&
        GetFillColor()==f.GetFillColor() &&
        GetIndent(LEFT)==f.GetIndent(LEFT) &&
        GetIndent(RIGHT)==f.GetIndent(RIGHT) &&
        GetLineLeft()==f.GetLineLeft() &&
        GetLineRight()==f.GetLineRight() &&
        GetLineTop()==f.GetLineTop() &&
        GetLineBottom()==f.GetLineBottom() &&
        GetHidden()==f.GetHidden() &&
        GetSpanCells()==f.GetSpanCells() &&
        GetCustom()==f.GetCustom() &&
        GetFontExtends()==f.GetFontExtends() &&
        GetTextColorExtends()==f.GetTextColorExtends() &&
        GetFillColorExtends()==f.GetFillColorExtends() &&
        GetLinesExtend()==f.GetLinesExtend() &&
        GetUnits()==f.GetUnits() &&
        GetIndentationExtends()==f.GetIndentationExtends());
}


bool CFmt::operator!=(const CFmt& f) const
{
    return !(operator==(f));
}


/*V*/ void CFmt::operator=(const CFmt& f)
{
    CFmtBase::operator=(f);
    SetFont(f.GetFont());
    SetHorzAlign(f.GetHorzAlign());
    SetVertAlign(f.GetVertAlign());
    SetTextColor(f.GetTextColor());
    SetFillColor(f.GetFillColor());
    SetIndent(LEFT, f.GetIndent(LEFT));
    SetIndent(RIGHT, f.GetIndent(RIGHT));
    SetLineLeft(f.GetLineLeft());
    SetLineRight(f.GetLineRight());
    SetLineTop(f.GetLineTop());
    SetLineBottom(f.GetLineBottom());
    SetHidden(f.GetHidden());
    SetSpanCells(f.GetSpanCells());
    SetCustom(f.GetCustom());
    SetFontExtends(f.GetFontExtends());
    SetTextColorExtends(f.GetTextColorExtends());
    SetFillColorExtends(f.GetFillColorExtends());
    SetLinesExtend(f.GetLinesExtend());
    SetIndentationExtends(f.GetIndentationExtends());
    SetUnits(f.GetUnits());
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CopyNonDefaultValues
//
// pTarget --   the CFmt that will contain guaranteed non-default values, based on
//              the object's attributes.  This is what gets modified here.
// pDefault --  the default format from the fmt registry (ie, index=0), which is
//              used to resolve DEFAULT values.  (pDefault never has DEFAULT values.)
//
// procedure --
//    for each attribute {
//        if attrib is non-default, then pTarget=this attribute
//        otherwise, pTarget=pDefault for this attribute
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ void CFmt::CopyNonDefaultValues(CFmtBase* pTarget, const CFmtBase* pDefault) const
{
    ASSERT(pDefault->GetIndex()==0);
//    ASSERT(GetID()==pDefault->GetID());
    const CFmt* pFmtDefault=DYNAMIC_DOWNCAST(CFmt,pDefault);
    CFmt* pFmtTarget=DYNAMIC_DOWNCAST(CFmt, pTarget);
	*pFmtTarget=*this;

	// font
    if (GetFont()==NULL) {
        pFmtTarget->SetFont(pFmtDefault->GetFont());
    }

	// horz alignment
    if (GetHorzAlign()==HALIGN_DEFAULT) {
        pFmtTarget->SetHorzAlign(pFmtDefault->GetHorzAlign());
    }

    // vert alignment
    if (GetVertAlign()==VALIGN_DEFAULT) {
        pFmtTarget->SetVertAlign(pFmtDefault->GetVertAlign());
    }

    // text color
    if (!IsTextColorCustom()) {
        pFmtTarget->SetTextColor(pFmtDefault->GetTextColor());
    }

    // fill color
    if (!IsFillColorCustom()) {
        pFmtTarget->SetFillColor(pFmtDefault->GetFillColor());
    }

    // left indentation
    if (GetIndent(LEFT)==INDENT_DEFAULT) {
        pFmtTarget->SetIndent(LEFT,pFmtDefault->GetIndent(LEFT));
    }

    // right indentation
    if (GetIndent(RIGHT)==INDENT_DEFAULT) {
        pFmtTarget->SetIndent(RIGHT,pFmtDefault->GetIndent(RIGHT));
    }

    // lines left
    if (GetLineLeft()==LINE_DEFAULT) {
        pFmtTarget->SetLineLeft(pFmtDefault->GetLineLeft());
    }

    // lines top
    if (GetLineTop()==LINE_DEFAULT) {
        pFmtTarget->SetLineTop(pFmtDefault->GetLineTop());
    }

    // lines right
    if (GetLineRight()==LINE_DEFAULT) {
        pFmtTarget->SetLineRight(pFmtDefault->GetLineRight());
    }

    // lines bottom
    if (GetLineBottom()==LINE_DEFAULT) {
        pFmtTarget->SetLineBottom(pFmtDefault->GetLineBottom());
    }

    // hidden
    if (GetHidden()==HIDDEN_DEFAULT) {
        pFmtTarget->SetHidden(pFmtDefault->GetHidden());
    }

    // span cells
    if (GetSpanCells()==SPAN_CELLS_DEFAULT) {
        pFmtTarget->SetSpanCells(pFmtDefault->GetSpanCells());
    }

// SAVY: this doesn't make sense!!!!!!!!!!!!!!!!!!!!
//    // custom text
//    if (!IsTextCustom()) {
//        pFmtTarget->SetCustom(pFmtDefault->GetCustom());
//    }

    // there are no default units, so we don't do anything with those
    // the extend flags don't have default values either
    ASSERT(!pFmtTarget->ContainsDefaultValues());
//    ASSERT(!fmtTarget.IsTextCustom());
}


/////////////////////////////////////////////////////////////////////////////
//
//                  CFmt::ContainsDefaultValues
//
// Returns true if any of the format's attributes are default
// (LINE_DEFAULT, for example); returns false otherwise.
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ bool CFmt::ContainsDefaultValues(void) const
{
    if (
    	GetFont()!=NULL &&
        GetHorzAlign()!=HALIGN_DEFAULT &&
        GetVertAlign()!=VALIGN_DEFAULT &&
        IsTextColorCustom() &&
        IsFillColorCustom() &&
        GetIndent(LEFT)!=INDENT_DEFAULT &&
        GetIndent(RIGHT)!=INDENT_DEFAULT &&
        GetLineLeft()!=LINE_DEFAULT &&
        GetLineTop()!=LINE_DEFAULT &&
        GetLineRight()!=LINE_DEFAULT &&
        GetLineBottom()!=LINE_DEFAULT &&
        GetHidden()!=HIDDEN_DEFAULT &&
        GetSpanCells()!=SPAN_CELLS_DEFAULT) {
        return false;
    }
    else {
        return true;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CDataCellFmt::CDataCellFmt
//
/////////////////////////////////////////////////////////////////////////////
CDataCellFmt::CDataCellFmt(void)
{
    Init();
}

CDataCellFmt::CDataCellFmt(const CDataCellFmt& f)
{
    *this=f;
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CDataCellFmt::Init
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ void CDataCellFmt::Init(void)
{
    // init base class members ...
    CFmt::Init();
    SetID(FMT_ID_DATACELL);    // csc 3/22/05
    m_eNumDecimals=NUM_DECIMALS_DEFAULT;
	m_iNumJoinSpanners =0;
	SetSpanCells(SPAN_CELLS_NOT_APPL);
	SetZeroHidden(false);
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CDataCellFmt::Build
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ bool CDataCellFmt::Build(CSpecFile& specFile, const CString& sVersion, bool bSilent)  {
    CIMSAString sCmd;    // command (left side of =)
    CIMSAString sArg;    // argument (right side of =)
    bool bLineOK;

    // build base class
    CFmt::Build(specFile, sVersion, bSilent);
   // specFile.UngetLine(); //Chris Unget lines problem please check this happens when you have special cells
    //I feel this call is redundant

    // now read line by line ...
    while (specFile.GetState() == SF_OK)  {
        specFile.GetLine(sCmd, sArg);
        ASSERT (!sCmd.IsEmpty());
        if (sCmd[0] == '[')  {
            // we are starting a new section ... put back the line and quit
            specFile.UngetLine();
            break;
        }
        bLineOK = false;
        StripQuotes(sArg);

        // number of decimals
        if (!sCmd.CompareNoCase(TFT_CMD_DECIMALS))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_NOTAPPL)==0)  {
                SetNumDecimals(NUM_DECIMALS_NOTAPPL);
            }
            else if (sArg.CompareNoCase(TFT_ARG_ZERO)==0)  {
                SetNumDecimals(NUM_DECIMALS_ZERO);
            }
            else if (sArg.CompareNoCase(TFT_ARG_ONE)==0)  {
                SetNumDecimals(NUM_DECIMALS_ONE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_TWO)==0)  {
                SetNumDecimals(NUM_DECIMALS_TWO);
            }
            else if (sArg.CompareNoCase(TFT_ARG_THREE)==0)  {
                SetNumDecimals(NUM_DECIMALS_THREE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_FOUR)==0)  {
                SetNumDecimals(NUM_DECIMALS_FOUR);
            }
            else if (sArg.CompareNoCase(TFT_ARG_FIVE)==0)  {
                SetNumDecimals(NUM_DECIMALS_FIVE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                SetNumDecimals(NUM_DECIMALS_DEFAULT);
            }
            else {
                bLineOK=false;
            }
        }
		if (!sCmd.CompareNoCase(TFT_CMD_HIDE_ZERO_ROW))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0)  {
                SetZeroHidden(true);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0)  {
                SetZeroHidden(false);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                SetZeroHidden(false);
            }
			else if (sArg.CompareNoCase(TFT_ARG_NOTAPPL)==0)  {
                SetZeroHidden(false);
            }
            else {
                bLineOK=false;
            }
        }
		if (!sCmd.CompareNoCase(TFT_CMD_JOIN_SPANNERS))  {
			bLineOK=true;
			int iNumJoinSpanners = (int)sArg.Val();
			if(iNumJoinSpanners >  0){
				SetNumJoinSpanners(iNumJoinSpanners);
			}
		}
        if (!bLineOK)  {
            // signal unrecognized command
			CIMSAString sMsg;
            sMsg.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
			sMsg += _T("\n") + sCmd + _T("=") + sArg;
			AfxMessageBox(sMsg,MB_ICONEXCLAMATION);
        }
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CDataCellFmt::Save
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ bool CDataCellFmt::Save(CSpecFile& specFile) const  {

    // save base class stuff...
    CFmt::Save(specFile);

    // number of decimals
    switch(GetNumDecimals()) {
    case NUM_DECIMALS_NOTAPPL:
        specFile.PutLine(TFT_CMD_DECIMALS, TFT_ARG_NOTAPPL);
        break;
    case NUM_DECIMALS_ZERO:
        specFile.PutLine(TFT_CMD_DECIMALS, TFT_ARG_ZERO);
        break;
    case NUM_DECIMALS_ONE:
        specFile.PutLine(TFT_CMD_DECIMALS, TFT_ARG_ONE);
        break;
    case NUM_DECIMALS_TWO:
        specFile.PutLine(TFT_CMD_DECIMALS, TFT_ARG_TWO);
        break;
    case NUM_DECIMALS_THREE:
        specFile.PutLine(TFT_CMD_DECIMALS, TFT_ARG_THREE);
        break;
    case NUM_DECIMALS_FOUR:
        specFile.PutLine(TFT_CMD_DECIMALS, TFT_ARG_FOUR);
        break;
    case NUM_DECIMALS_FIVE:
        specFile.PutLine(TFT_CMD_DECIMALS, TFT_ARG_FIVE);
        break;
    case NUM_DECIMALS_DEFAULT:
        // do nothing
        break;
    default:
        ASSERT(FALSE);
    }

	 // hide zero row
  if (GetZeroHidden()) {
        specFile.PutLine(TFT_CMD_HIDE_ZERO_ROW, TFT_ARG_YES);
  }
  else {
        specFile.PutLine(TFT_CMD_HIDE_ZERO_ROW, TFT_ARG_NO);
  }
  if(GetNumJoinSpanners()){
        specFile.PutLine(TFT_CMD_JOIN_SPANNERS, GetNumJoinSpanners());
  }
   return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CDataCellFmt::operator==
//                               CDataCellFmt::operator!=
//                               CDataCellFmt::operator=
//
/////////////////////////////////////////////////////////////////////////////
bool CDataCellFmt::operator==(const CDataCellFmt& f) const
{
    return (CFmt::operator==(f) &&
        GetNumDecimals()==f.GetNumDecimals() && GetZeroHidden() == f.GetZeroHidden());
}


bool CDataCellFmt::operator!=(const CDataCellFmt& f) const
{
    return !(operator==(f));
}


void CDataCellFmt::operator=(const CDataCellFmt& f)
{
    CFmt::operator=(f);
    SetNumDecimals(f.GetNumDecimals());
	SetZeroHidden(f.GetZeroHidden());
	SetNumJoinSpanners(f.GetNumJoinSpanners());
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CopyNonDefaultValues
//
// pTarget --   the CFmt that will contain guaranteed non-default values, based on
//              the object's attributes.  This is what gets modified here.
// pDefault --  the default format from the fmt registry (ie, index=0), which is
//              used to resolve DEFAULT values.  (pDefault never has DEFAULT values.)
//
// procedure --
//    for each attribute {
//        if attrib is non-default, then pTarget=this attribute
//        otherwise, pTarget=pDefault for this attribute
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ void CDataCellFmt::CopyNonDefaultValues(CFmtBase* pTarget, const CFmtBase* pDefault) const
{
    ASSERT(pDefault->GetIndex()==0);
    ASSERT(GetID()==pDefault->GetID());

    const CDataCellFmt* pFmtDefault=DYNAMIC_DOWNCAST(CDataCellFmt,pDefault);
    CDataCellFmt* pFmtTarget=DYNAMIC_DOWNCAST(CDataCellFmt, pTarget);

    *pFmtTarget=*this;

    // number of decimals
    if (GetNumDecimals()==NUM_DECIMALS_DEFAULT) {
        pFmtTarget->SetNumDecimals(pFmtDefault->GetNumDecimals());
    }

    // call base class implementation
    CFmt::CopyNonDefaultValues(pTarget, pDefault);

    ASSERT(!pFmtTarget->ContainsDefaultValues());
}


/////////////////////////////////////////////////////////////////////////////
//
//                  CDataCellFmt::ContainsDefaultValues
//
// Returns true if any of the format's attributes are default
// (LINE_DEFAULT, for example); returns false otherwise.
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ bool CDataCellFmt::ContainsDefaultValues(void) const
{
    if (GetNumDecimals()!=NUM_DECIMALS_DEFAULT && !CFmt::ContainsDefaultValues()) {
        return false;
    }
    else {
        return true;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTallyFmt::CTallyFmt
//
/////////////////////////////////////////////////////////////////////////////
CTallyFmt::CTallyFmt(void)
{
    Init();
}

CTallyFmt::CTallyFmt(const CTallyFmt& v)
{
    *this=v;
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTallyFmt::~CTallyFmt
// Destructor
/////////////////////////////////////////////////////////////////////////////
CTallyFmt::~CTallyFmt()
{
	for (int i = 0; i < m_aStats.GetCount(); ++i) {
		SAFE_DELETE(m_aStats[i]);
	}
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTallyFmt::Init
//
/////////////////////////////////////////////////////////////////////////////
void CTallyFmt::Init(void)
{
    // init base class members ...
    CFmtBase::Init();
    SetID(FMT_ID_TALLY);

    // init class members ...
    SetInclUndef(INCLUDE_UNDEF_DEFAULT);
    SetDumpUndef(DUMP_UNDEF_DEFAULT);

	// by default only stats are counts and totals
	ClearStats();
	AddStat(new CTallyVarStatFmtTotal);
	AddStat(new CTallyVarStatFmtCounts);

}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTallyFmt::Build
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ bool CTallyFmt::Build(CSpecFile& specFile, bool bSilent)  {
    CIMSAString sCmd;    // command (left side of =)
    CIMSAString sArg;    // argument (right side of =)
    bool bLineOK;

    // build base class
    CFmtBase::Build(specFile, bSilent);

    ClearStats();

    // now read line by line ...
    while (specFile.GetState() == SF_OK)  {
        specFile.GetLine(sCmd, sArg);
        ASSERT (!sCmd.IsEmpty());
        if (sCmd[0] == '[')  {
            if (sCmd.CompareNoCase(TFT_SECT_TALLY_STAT) == 0) {
                specFile.GetLine(sCmd, sArg);
                StripQuotes(sArg);
                if (sCmd.CompareNoCase(TFT_CMD_TALLY_STAT_TYPE) == 0) {
			        CTallyVarStatFmt* pStat = CTallyVarStatFmtFactory::GetInstance()->Create(sArg);
                    pStat->Build(specFile, bSilent);
                    AddStat(pStat);
                    continue;
                }
                else {
			        CIMSAString sMsg;
                    sMsg.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
			        sMsg += _T("\n") + sCmd + _T("=") + sArg;
			        AfxMessageBox(sMsg,MB_ICONEXCLAMATION);
                }
            }

            // we are starting a new section ... put back the line and quit
            specFile.UngetLine();
            break;
        }

        bLineOK = false;
        StripQuotes(sArg);

        // include undefined values
        if (!sCmd.CompareNoCase(TFT_CMD_INCLUDE_UNDEF))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0) {
                SetInclUndef(INCLUDE_UNDEF_YES);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0) {
                SetInclUndef(INCLUDE_UNDEF_NO);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                SetInclUndef(INCLUDE_UNDEF_DEFAULT);
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // dump undefined values
        if (!sCmd.CompareNoCase(TFT_CMD_DUMP_UNDEF))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0) {
                SetDumpUndef(DUMP_UNDEF_YES);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0) {
                SetDumpUndef(DUMP_UNDEF_NO);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                SetDumpUndef(DUMP_UNDEF_DEFAULT);
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        if (!bLineOK)  {
            // signal unrecognized command
			CIMSAString sMsg;
            sMsg.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
			sMsg += _T("\n") + sCmd + _T("=") + sArg;
			AfxMessageBox(sMsg,MB_ICONEXCLAMATION);
        }
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTallyFmt::BuildPre32
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ bool CTallyFmt::BuildPre32(CSpecFile& specFile, CPre32TallyFmt& oldDefRowTallyFmt, CPre32TallyFmt& oldDefColTallyFmt, bool bSilent)  {
    CIMSAString sCmd;    // command (left side of =)
    CIMSAString sArg;    // argument (right side of =)
    bool bLineOK;

    // build base class
    CFmtBase::Build(specFile, bSilent);


     // use local vars to capture data stored in tally fmt pre 3.2
    CPre32TallyFmt oldFmtVals;

   // now read line by line ...
    while (specFile.GetState() == SF_OK)  {
        specFile.GetLine(sCmd, sArg);
        ASSERT (!sCmd.IsEmpty());
        if (sCmd[0] == '[')  {
            // we are starting a new section ... put back the line and quit
            specFile.UngetLine();
            break;
        }
        bLineOK = false;
        StripQuotes(sArg);

        // totals position
        if (!sCmd.CompareNoCase(TFT_CMD_TOTALS_POS))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_NONE)==0)  {
                oldFmtVals.eTotalsPos = TOTALS_POSITION_NONE;
            }
            else if (sArg.CompareNoCase(TFT_ARG_AFTER)==0)  {
                oldFmtVals.eTotalsPos = TOTALS_POSITION_AFTER;
            }
            else  if (sArg.CompareNoCase(TFT_ARG_BEFORE)==0)  {
                oldFmtVals.eTotalsPos = TOTALS_POSITION_BEFORE;
            }
            else  {
                // bad argument
                bLineOK=false;
            }
        }

        // percent type
        if (!sCmd.CompareNoCase(TFT_CMD_PCT_TYPE))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_NONE)==0) {
                oldFmtVals.ePercentType = PCT_TYPE_NONE;
            }
            else if (sArg.CompareNoCase(TFT_ARG_TOTAL)==0) {
                oldFmtVals.ePercentType = PCT_TYPE_TOTAL;
            }
            else if (sArg.CompareNoCase(TFT_ARG_BYROW)==0){
                oldFmtVals.ePercentType = PCT_TYPE_ROW;
            }
            else if(sArg.CompareNoCase(TFT_ARG_ROW)==0) {//flip for 3.0 version stuff
                oldFmtVals.ePercentType = PCT_TYPE_COL;
            }
            else if (sArg.CompareNoCase(TFT_ARG_BYCOL)==0 ) {
                oldFmtVals.ePercentType = PCT_TYPE_COL;
            }
            else if (sArg.CompareNoCase(TFT_ARG_COL)==0) {//flip for 3.0 version stuff
                oldFmtVals.ePercentType = PCT_TYPE_ROW;
            }
            else if (sArg.CompareNoCase(TFT_ARG_CELL)==0) {
                oldFmtVals.ePercentType = PCT_TYPE_CELL;
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                oldFmtVals.ePercentType = PCT_TYPE_DEFAULT;
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // percent position
        if (!sCmd.CompareNoCase(TFT_CMD_PCT_POS))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_ABOVE)==0) {
                oldFmtVals.ePercentPos = PCT_POS_ABOVE_OR_LEFT;
            }
            else if (sArg.CompareNoCase(TFT_ARG_BELOW)==0) {
                oldFmtVals.ePercentPos = PCT_POS_BELOW_OR_RIGHT;
            }
            /*else if (sArg.CompareNoCase(TFT_ARG_LEFT)==0) {
                SetPercentPos(PCT_POS_LEFT;
            }
            else if (sArg.CompareNoCase(TFT_ARG_RIGHT)==0) {
                SetPercentPos(PCT_POS_RIGHT;
            }*/
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                oldFmtVals.ePercentPos = PCT_POS_DEFAULT;
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // counts
        if (!sCmd.CompareNoCase(TFT_CMD_COUNTS))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0) {
                oldFmtVals.eCounts = TALLY_STATISTIC_YES;
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0) {
                oldFmtVals.eCounts = TALLY_STATISTIC_NO;
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                oldFmtVals.eCounts = TALLY_STATISTIC_DEFAULT;
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // include undefined values
        if (!sCmd.CompareNoCase(TFT_CMD_INCLUDE_UNDEF))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0) {
                SetInclUndef(INCLUDE_UNDEF_YES);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0) {
                SetInclUndef(INCLUDE_UNDEF_NO);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                SetInclUndef(INCLUDE_UNDEF_DEFAULT);
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // dump undefined values
        if (!sCmd.CompareNoCase(TFT_CMD_DUMP_UNDEF))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0) {
                SetDumpUndef(DUMP_UNDEF_YES);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0) {
                SetDumpUndef(DUMP_UNDEF_NO);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                SetDumpUndef(DUMP_UNDEF_DEFAULT);
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // statistics -- N-Row
         if (!sCmd.CompareNoCase(TFT_CMD_STATS_NROW))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0) {
                oldFmtVals.eNRow = TALLY_STATISTIC_YES;
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0) {
                oldFmtVals.eNRow = TALLY_STATISTIC_NO;
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                oldFmtVals.eNRow = TALLY_STATISTIC_DEFAULT;
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }
        // statistics -- min

        if (!sCmd.CompareNoCase(TFT_CMD_STATS_MIN))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0) {
                oldFmtVals.eMin = TALLY_STATISTIC_YES;
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0) {
                oldFmtVals.eMin = TALLY_STATISTIC_NO;
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                oldFmtVals.eMin = TALLY_STATISTIC_DEFAULT;
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // statistics -- max
        if (!sCmd.CompareNoCase(TFT_CMD_STATS_MAX))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0) {
                oldFmtVals.eMax = TALLY_STATISTIC_YES;
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0) {
                oldFmtVals.eMax = TALLY_STATISTIC_NO;
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                oldFmtVals.eMax = TALLY_STATISTIC_DEFAULT;
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // statistics -- std deviation
        if (!sCmd.CompareNoCase(TFT_CMD_STATS_STDDEV))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0) {
                oldFmtVals.eStdDev = TALLY_STATISTIC_YES;
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0) {
                oldFmtVals.eStdDev = TALLY_STATISTIC_NO;
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                oldFmtVals.eStdDev = TALLY_STATISTIC_DEFAULT;
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // statistics -- standard error
        if (!sCmd.CompareNoCase(TFT_CMD_STATS_STDERR))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0) {
                oldFmtVals.eStdErr = TALLY_STATISTIC_YES;
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0) {
                oldFmtVals.eStdErr = TALLY_STATISTIC_NO;
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                oldFmtVals.eStdErr = TALLY_STATISTIC_DEFAULT;
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // statistics -- proportion
        if (!sCmd.CompareNoCase(TFT_CMD_STATS_PROPORTION))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0) {
                oldFmtVals.eProportion = TALLY_STATISTIC_YES;
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0) {
                oldFmtVals.eProportion = TALLY_STATISTIC_NO;
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                oldFmtVals.eProportion = TALLY_STATISTIC_DEFAULT;
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // statistics -- variance
        if (!sCmd.CompareNoCase(TFT_CMD_STATS_VAR))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0) {
                oldFmtVals.eVariance = TALLY_STATISTIC_YES;
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0) {
                oldFmtVals.eVariance = TALLY_STATISTIC_NO;
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                oldFmtVals.eVariance = TALLY_STATISTIC_DEFAULT;
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // statistics -- mode
        if (!sCmd.CompareNoCase(TFT_CMD_STATS_MODE))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0) {
                oldFmtVals.eMode = TALLY_STATISTIC_YES;
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0) {
                oldFmtVals.eMode = TALLY_STATISTIC_NO;
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                oldFmtVals.eMode = TALLY_STATISTIC_DEFAULT;
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // statistics -- median
        if (!sCmd.CompareNoCase(TFT_CMD_STATS_MEDIAN))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_CONTINUOUS)==0) {
                oldFmtVals.eMedian=MEDIAN_TYPE_CONTINUOUS;
            }
            else if (sArg.CompareNoCase(TFT_ARG_DISCRETE)==0) {
                oldFmtVals.eMedian=MEDIAN_TYPE_DISCRETE;
            }
            else if (sArg.CompareNoCase(TFT_ARG_NONE)==0) {
                oldFmtVals.eMedian=MEDIAN_TYPE_NONE;
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                oldFmtVals.eMedian=MEDIAN_TYPE_DEFAULT;
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // statistics -- mean
        if (!sCmd.CompareNoCase(TFT_CMD_STATS_MEAN))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0) {
                oldFmtVals.eMean=TALLY_STATISTIC_YES;
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0) {
                oldFmtVals.eMean=TALLY_STATISTIC_NO;
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                oldFmtVals.eMean=TALLY_STATISTIC_DEFAULT;
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // statistics -- NTiles
        if (!sCmd.CompareNoCase(TFT_CMD_STATS_NTILES))  {
            bLineOK=true;
            CIMSAString sTilesFlag = sArg.GetToken();
            CIMSAString sNumTiles = sArg.GetToken();
            if (sTilesFlag.CompareNoCase(TFT_ARG_YES)==0) {
                int iNumTiles = (int)sNumTiles.Val();
                if(iNumTiles >= 2 && iNumTiles<=10){
                    oldFmtVals.eNTiles=TALLY_STATISTIC_YES;
                    oldFmtVals.iTiles=iNumTiles;
                }
            }
            else if (sTilesFlag.CompareNoCase(TFT_ARG_NO)==0) {
                oldFmtVals.eNTiles=TALLY_STATISTIC_NO;
            }
            else if (sTilesFlag.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                oldFmtVals.eNTiles=TALLY_STATISTIC_DEFAULT;
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        if (!bLineOK)  {
            // signal unrecognized command
			CIMSAString sMsg;
            sMsg.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
			sMsg += _T("\n") + sCmd + _T("=") + sArg;
			AfxMessageBox(sMsg,MB_ICONEXCLAMATION);
        }
    }

    // remove any existing stats
    ClearStats();

    // build the new style tally fmt from the old style data

     if (GetIndex() != 0) {
        // not a default fmt, need to resolve index by looking at defaults
         CPre32TallyFmt* pDefFmt = (GetID() == FMT_ID_TALLY_ROW) ? &oldDefRowTallyFmt : &oldDefColTallyFmt;
        if (oldFmtVals.eTotalsPos == TOTALS_POSITION_DEFAULT) {
            oldFmtVals.eTotalsPos = pDefFmt->eTotalsPos;
        }
        if (oldFmtVals.ePercentType == PCT_TYPE_DEFAULT) {
            oldFmtVals.ePercentType = pDefFmt->ePercentType;
        }
        if (oldFmtVals.ePercentPos == PCT_POS_DEFAULT) {
            oldFmtVals.ePercentPos = pDefFmt->ePercentPos;
        }
        if (oldFmtVals.eCounts == TALLY_STATISTIC_DEFAULT) {
            oldFmtVals.eCounts = pDefFmt->eCounts;
        }
        if (oldFmtVals.eNRow == TALLY_STATISTIC_DEFAULT) {
            oldFmtVals.eNRow = pDefFmt->eNRow;
        }
        if (oldFmtVals.eMin == TALLY_STATISTIC_DEFAULT) {
            oldFmtVals.eMin = pDefFmt->eMin;
        }
        if (oldFmtVals.eMax == TALLY_STATISTIC_DEFAULT) {
            oldFmtVals.eMax = pDefFmt->eMax;
        }
        if (oldFmtVals.eStdDev == TALLY_STATISTIC_DEFAULT) {
            oldFmtVals.eStdDev = pDefFmt->eStdDev;
        }
        if (oldFmtVals.eVariance == TALLY_STATISTIC_DEFAULT) {
            oldFmtVals.eVariance = pDefFmt->eVariance;
        }
        if (oldFmtVals.eMean == TALLY_STATISTIC_DEFAULT) {
            oldFmtVals.eMean = pDefFmt->eMean;
        }
        if (oldFmtVals.eMode == TALLY_STATISTIC_DEFAULT) {
            oldFmtVals.eMode = pDefFmt->eMode;
        }
        if (oldFmtVals.eStdErr == TALLY_STATISTIC_DEFAULT) {
            oldFmtVals.eStdErr = pDefFmt->eStdErr;
        }
        if (oldFmtVals.eMean == TALLY_STATISTIC_DEFAULT) {
            oldFmtVals.eMean = pDefFmt->eMean;
        }
        if (oldFmtVals.eProportion == TALLY_STATISTIC_DEFAULT) {
            oldFmtVals.eProportion = pDefFmt->eProportion;
        }
        if (oldFmtVals.eNTiles == TALLY_STATISTIC_DEFAULT) {
            oldFmtVals.eNTiles = pDefFmt->eNTiles;
        }
        if (oldFmtVals.eMedian == MEDIAN_TYPE_DEFAULT) {
            oldFmtVals.eMedian = pDefFmt->eMedian;
        }
        if (oldFmtVals.iTiles == 0) {
            oldFmtVals.iTiles = pDefFmt->iTiles;
        }
     }
     else {
         // this is a default format - assign the values read in to the appropriate default
         // so we can use it to resolve defaults (as above) we we read in later fmts
        ASSERT(oldFmtVals.eTotalsPos != TOTALS_POSITION_DEFAULT);
        ASSERT(oldFmtVals.ePercentType != PCT_TYPE_DEFAULT);
        ASSERT(oldFmtVals.ePercentPos != PCT_POS_DEFAULT);
        ASSERT(oldFmtVals.eCounts != TALLY_STATISTIC_DEFAULT);
        ASSERT(oldFmtVals.eNRow != TALLY_STATISTIC_DEFAULT);
        ASSERT(oldFmtVals.eMin != TALLY_STATISTIC_DEFAULT);
        ASSERT(oldFmtVals.eMax != TALLY_STATISTIC_DEFAULT);
        ASSERT(oldFmtVals.eStdDev != TALLY_STATISTIC_DEFAULT);
        ASSERT(oldFmtVals.eVariance != TALLY_STATISTIC_DEFAULT);
        ASSERT(oldFmtVals.eMean != TALLY_STATISTIC_DEFAULT);
        ASSERT(oldFmtVals.eMode != TALLY_STATISTIC_DEFAULT);
        ASSERT(oldFmtVals.eStdErr != TALLY_STATISTIC_DEFAULT);
        ASSERT(oldFmtVals.eProportion != TALLY_STATISTIC_DEFAULT);
        ASSERT(oldFmtVals.eNTiles != TALLY_STATISTIC_DEFAULT);
        ASSERT(oldFmtVals.eMedian!= MEDIAN_TYPE_DEFAULT);

        if (GetID() == FMT_ID_TALLY_ROW) {
             oldDefRowTallyFmt = oldFmtVals;
         }
         else {
             ASSERT(GetID() == FMT_ID_TALLY_COL);
             oldDefColTallyFmt = oldFmtVals;
         }
     }

     // create the stats that we will need, will order them later
    CTallyVarStatFmtCounts* pStatCounts = NULL;
    if (oldFmtVals.eCounts == TALLY_STATISTIC_YES) {
        pStatCounts = new CTallyVarStatFmtCounts;
    }
    CTallyVarStatFmtPercent* pStatPct = NULL;
    if (oldFmtVals.ePercentType != PCT_TYPE_NONE) {
        pStatPct = new CTallyVarStatFmtPercent;
        pStatPct->SetPctType(OldPctTypeToPctType(oldFmtVals.ePercentType));
        // make percents interleaved only if there are counts present i.e. not in percent only
        pStatPct->SetInterleaved(pStatCounts ? true : false);
    }
    CTallyVarStatFmtTotalPercent* pStatTotalPct = NULL;
    CTallyVarStatFmtTotal* pStatTotal = NULL;
    if (oldFmtVals.eTotalsPos != TOTALS_POSITION_NONE) {
        if (oldFmtVals.eCounts == TALLY_STATISTIC_YES) {
            pStatTotal = new CTallyVarStatFmtTotal;
        }
        if (oldFmtVals.ePercentType != PCT_TYPE_NONE) {
            pStatTotalPct = new CTallyVarStatFmtTotalPercent;
            pStatTotalPct->SetPctType(OldPctTypeToPctType(oldFmtVals.ePercentType));
        }
    }

    // put them in the correct order
    if (oldFmtVals.eCounts == TALLY_STATISTIC_NO && oldFmtVals.eNRow == TALLY_STATISTIC_YES) {
        // handle NRow separately - only happens with pcts only and is always pcts followed by total
        ASSERT(oldFmtVals.ePercentType != PCT_TYPE_NONE);
        ASSERT(pStatPct);
        ASSERT(pStatTotal == NULL);
        ASSERT(pStatCounts == NULL);
        pStatTotal = new CTallyVarStatFmtTotal;
        switch (oldFmtVals.eTotalsPos) {
            case TOTALS_POSITION_BEFORE:
                ASSERT(pStatTotalPct);
                AddStat(pStatTotalPct);
                AddStat(pStatPct);
                break;
            case TOTALS_POSITION_AFTER:
                ASSERT(pStatTotalPct);
                AddStat(pStatPct);
                AddStat(pStatTotalPct);
                break;
            case TOTALS_POSITION_NONE:
                AddStat(pStatPct);
                break;
            default:
                ASSERT(!_T("Invalid totals pos"));
        }
        AddStat(pStatTotal);
    }
    else {
        // non-row case, have to put totals, total percent, counts and percents in correct order for whichever of themn
        // are present
        if (oldFmtVals.eTotalsPos == TOTALS_POSITION_BEFORE) {
            // totals first
            AddPercentsAndCounts(this, oldFmtVals.ePercentPos, pStatTotal, pStatTotalPct);
            AddPercentsAndCounts(this, oldFmtVals.ePercentPos, pStatCounts, pStatPct);
        }
        else {
            // totals first
            AddPercentsAndCounts(this, oldFmtVals.ePercentPos, pStatCounts, pStatPct);
            AddPercentsAndCounts(this, oldFmtVals.ePercentPos, pStatTotal, pStatTotalPct);
        }
    }

    // the rest of the stats are in fixed order, just add them if they exist
    if (oldFmtVals.eMin == TALLY_STATISTIC_YES) {
        AddStat(new CTallyVarStatFmtMin);
    }
    if (oldFmtVals.eMax == TALLY_STATISTIC_YES) {
        AddStat(new CTallyVarStatFmtMax);
    }
    if (oldFmtVals.eMedian != MEDIAN_TYPE_NONE) {
        ASSERT(oldFmtVals.eMedian == MEDIAN_TYPE_CONTINUOUS || oldFmtVals.eMedian == MEDIAN_TYPE_CONTINUOUS);
        CTallyVarStatFmtMedian* pStatMedian = new CTallyVarStatFmtMedian(-1, -1);
        pStatMedian->GetRangeProps().SetUseValueSet(true);
        pStatMedian->SetContinuous(oldFmtVals.eMedian == MEDIAN_TYPE_CONTINUOUS);
        AddStat(pStatMedian);
    }
    if (oldFmtVals.eMode == TALLY_STATISTIC_YES) {
        AddStat(new CTallyVarStatFmtMode);
    }
    if (oldFmtVals.eMean == TALLY_STATISTIC_YES) {
        AddStat(new CTallyVarStatFmtMean);
    }
    if (oldFmtVals.eStdDev == TALLY_STATISTIC_YES) {
        AddStat(new CTallyVarStatFmtStdDeviation);
    }
    if (oldFmtVals.eVariance == TALLY_STATISTIC_YES) {
        AddStat(new CTallyVarStatFmtVariance);
    }
    if (oldFmtVals.eNTiles == TALLY_STATISTIC_YES) {
        CTallyVarStatFmtNTiles* pStatNTile = new CTallyVarStatFmtNTiles(-1,-1);
        ASSERT(oldFmtVals.iTiles >= 2 && oldFmtVals.iTiles <= 10);
        pStatNTile->SetNumTiles(oldFmtVals.iTiles);
        pStatNTile->GetRangeProps().SetUseValueSet(true);
        AddStat(pStatNTile);
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTallyFmt::Save
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ bool CTallyFmt::Save(CSpecFile& specFile) const  {

    // save base class stuff...
    if(GetID() == FMT_ID_TALLY){// this shld never get saved starting CSPro 3.1
        return true;
    }
    CFmtBase::Save(specFile);

    // include undefined values
    switch(GetInclUndef()) {
    case INCLUDE_UNDEF_YES:
        specFile.PutLine(TFT_CMD_INCLUDE_UNDEF, TFT_ARG_YES);
        break;
    case INCLUDE_UNDEF_NO:
        specFile.PutLine(TFT_CMD_INCLUDE_UNDEF, TFT_ARG_NO);
        break;
    case INCLUDE_UNDEF_DEFAULT:
        // do nothing
        break;
    default:
        ASSERT(FALSE);
    }

    // dump undefined values
    switch(GetDumpUndef()) {
    case DUMP_UNDEF_YES:
        specFile.PutLine(TFT_CMD_DUMP_UNDEF, TFT_ARG_YES);
        break;
    case DUMP_UNDEF_NO:
        specFile.PutLine(TFT_CMD_DUMP_UNDEF, TFT_ARG_NO);
        break;
    case DUMP_UNDEF_DEFAULT:
        // do nothing
        break;
    default:
        ASSERT(FALSE);
    }

    // save the stats
    for (int iStat = 0; iStat < GetStats().GetCount(); ++iStat) {
        GetStats().GetAt(iStat)->Save(specFile);
        specFile.PutLine(_T(""));
    }

    return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTallyFmt::operator==
//                               CTallyFmt::operator!=
//                               CTallyFmt::operator=
//
/////////////////////////////////////////////////////////////////////////////
bool CTallyFmt::operator==(const CTallyFmt& f) const
{
    return CFmtBase::operator==(f) &&
        GetInclUndef()==f.GetInclUndef() &&
        GetDumpUndef() ==f.GetDumpUndef() &&
		CompareStats(f.GetStats());
}


bool CTallyFmt::operator!=(const CTallyFmt& f) const
{
    return !(operator==(f));
}


void CTallyFmt::operator=(const CTallyFmt& f)
{
    CFmtBase::operator=(f);
    SetInclUndef(f.GetInclUndef());
    SetDumpUndef(f.GetDumpUndef());
	CopyStats(f.GetStats());
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CopyNonDefaultValues
//
// pTarget --   the CFmt that will contain guaranteed non-default values, based on
//              the object's attributes.  This is what gets modified here.
// pDefault --  the default format from the fmt registry (ie, index=0), which is
//              used to resolve DEFAULT values.  (pDefault never has DEFAULT values.)
//
// procedure --
//    for each attribute {
//        if attrib is non-default, then pTarget=this attribute
//        otherwise, pTarget=pDefault for this attribute
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ void CTallyFmt::CopyNonDefaultValues(CFmtBase* pTarget, const CFmtBase* pDefault) const
{
    ASSERT(pDefault->GetIndex()==0);
    ASSERT(GetID()==pDefault->GetID());

    const CTallyFmt* pTallyFmtDefault=DYNAMIC_DOWNCAST(CTallyFmt,pDefault);
    CTallyFmt* pTallyFmtTarget=DYNAMIC_DOWNCAST(CTallyFmt, pTarget);

	*pTallyFmtTarget=*this;

    // include undefined
    if (GetInclUndef()==INCLUDE_UNDEF_DEFAULT) {
        pTallyFmtTarget->SetInclUndef(pTallyFmtDefault->GetInclUndef());
    }

    // dump undefined
    if (GetDumpUndef()==DUMP_UNDEF_DEFAULT) {
        pTallyFmtTarget->SetDumpUndef(pTallyFmtDefault->GetDumpUndef());
    }

    pTallyFmtTarget->CopyStats(pTallyFmtDefault->GetStats());

    // sanity checks
    ASSERT(!pTallyFmtTarget->ContainsDefaultValues());
}


/////////////////////////////////////////////////////////////////////////////
//
//                  CTallyFmt::ContainsDefaultValues
//
// Returns true if any of the format's attributes are default
// (LINE_DEFAULT, for example); returns false otherwise.
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ bool CTallyFmt::ContainsDefaultValues(void) const
{
    if (GetInclUndef()!=INCLUDE_UNDEF_DEFAULT &&
        GetDumpUndef()!=DUMP_UNDEF_DEFAULT) {
        return false;
    }
    else {
        return true;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                  CTallyFmt::CompareStats
//
//
/////////////////////////////////////////////////////////////////////////////
bool CTallyFmt::CompareStats(const CArray<CTallyVarStatFmt*>& aStats) const
{
	// check # first
	if (m_aStats.GetCount() != aStats.GetCount()) {
		return false;
	}

	// check individual stats
	for (int iStat = 0; iStat < m_aStats.GetCount(); ++iStat) {
		if (*(m_aStats.GetAt(iStat)) != *(aStats.GetAt(iStat))) {
			return false;
		}
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////
//
//                  CTallyFmt::CopyStats
//
//
/////////////////////////////////////////////////////////////////////////////
void CTallyFmt::CopyStats(const CArray<CTallyVarStatFmt*>& aStats)
{
	// delete existing stats first
	ClearStats();

	// copy individual stats
	for (int iStat = 0; iStat < aStats.GetCount(); ++iStat) {
		m_aStats.Add(aStats.GetAt(iStat)->Clone());
	}
}

/////////////////////////////////////////////////////////////////////////////
//
//                  CTallyFmt::ClearStats
//
//
/////////////////////////////////////////////////////////////////////////////
void CTallyFmt::ClearStats()
{
	// delete existing stats first
	for (int iStat = 0; iStat < m_aStats.GetCount(); ++iStat) {
		SAFE_DELETE(m_aStats[iStat]);
	}
	m_aStats.RemoveAll();
}

/////////////////////////////////////////////////////////////////////////////
//
//                  CTallyFmt::HasCounts
//
// Returns true if any counts are included in the stats for this format
//
/////////////////////////////////////////////////////////////////////////////
bool CTallyFmt::HasCounts() const
{
	for (int iStat = 0; iStat < m_aStats.GetCount(); ++iStat) {
		if (!_tcscmp(m_aStats.GetAt(iStat)->GetType(), _T("Counts"))) {
			return true;
		}
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////
//
//                  CTallyFmt::HasPercents
//
// Returns true if any percents are included in the stats for this format
//
/////////////////////////////////////////////////////////////////////////////
bool CTallyFmt::HasPercents() const
{
	for (int iStat = 0; iStat < m_aStats.GetCount(); ++iStat) {
        if (!_tcscmp(m_aStats.GetAt(iStat)->GetType(), _T("Percents"))) {
			return true;
		}
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////
//
//                  CTallyFmt::GetInterleavedStats
//
// Returns array of pairs of interleaved tally stats (by order) i.e.
// a pair of 3 and 4 means the third and fourth stats should be interleaved.
//
/////////////////////////////////////////////////////////////////////////////
void CTallyFmt::GetInterleavedStats(CArray<InterleavedStatPair>& aInterleavedStats) const
{
    InterleavedStatPair newPair;

	const CArray<CTallyVarStatFmt*>& aStats = GetStats();
	for (int iStat = 0; iStat < aStats.GetSize(); ++iStat) {

		CTallyVarStatFmt* pStat = aStats.GetAt(iStat);
        if (!_tcscmp(pStat->GetType(), _T("Percents"))) {
            CTallyVarStatFmtPercent* pPctStat = static_cast<CTallyVarStatFmtPercent*>(pStat);
            if (pPctStat->GetInterleaved()) {
                if (iStat > 0 && !_tcscmp(aStats.GetAt(iStat-1)->GetType(), _T("Counts"))) {
                    // interleave with counts above
                    newPair.m_first = iStat-1;
                    newPair.m_second = iStat;
                    aInterleavedStats.Add(newPair);
                }
                else if (iStat <  aStats.GetSize() - 1 && !_tcscmp(aStats.GetAt(iStat+1)->GetType(), _T("Counts"))) {
                    // interleave with counts below
                    newPair.m_first = iStat;
                    newPair.m_second = iStat+1;
                    aInterleavedStats.Add(newPair);
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                  CTallyFmt::Reconcile
//
// Reconcile tally fmt on vset change.
/////////////////////////////////////////////////////////////////////////////
bool CTallyFmt::Reconcile(const DictValueSet* pVSet)
{
    bool bChanged = false;
    const CArray<CTallyVarStatFmt*>& aStats = GetStats();
	for (int iStat = 0; iStat < aStats.GetSize(); ++iStat) {
        if (aStats.GetAt(iStat)->ReconcileStatRanges(pVSet)) {
            bChanged = true;
        }
    }

    return bChanged;
}

/////////////////////////////////////////////////////////////////////////////
//
//                  CTallyFmt::AddStat
//
//
/////////////////////////////////////////////////////////////////////////////
void CTallyFmt::AddStat(CTallyVarStatFmt* pStat)
{
	ASSERT(pStat);
	m_aStats.Add(pStat);
}

/////////////////////////////////////////////////////////////////////////////
//
//                  CTallyFmt::MoveStatTo
//
//
/////////////////////////////////////////////////////////////////////////////
void CTallyFmt::MoveStatTo(int iOrigPos, int iNewPos)
{
	ASSERT(iOrigPos >= 0 && iOrigPos < m_aStats.GetCount());
	ASSERT(iNewPos >= 0 && iOrigPos <= m_aStats.GetCount());
	CTallyVarStatFmt* pStat = m_aStats.GetAt(iOrigPos);
	m_aStats.RemoveAt(iOrigPos);
	m_aStats.InsertAt(iNewPos, pStat);
}

/////////////////////////////////////////////////////////////////////////////
//
//                  CTallyFmt::RemoveStatAt
//
//
/////////////////////////////////////////////////////////////////////////////
void CTallyFmt::RemoveStatAt(int iPos)
{
	m_aStats.RemoveAt(iPos);
}

/////////////////////////////////////////////////////////////////////////////
//
//                  CTallyFmt::FindFirstStatPos
//
//
/////////////////////////////////////////////////////////////////////////////
int CTallyFmt::FindFirstStatPos(LPCTSTR sType)
{
    for (int i = 0; i < m_aStats.GetCount(); ++i) {
        if (!_tcscmp(m_aStats.GetAt(i)->GetType(), sType)) {
            return i;
        }
    }
    return NONE;
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTblFmt::CTblFmt
//
/////////////////////////////////////////////////////////////////////////////
CTblFmt::CTblFmt(void)
{
    Init();
}

CTblFmt::CTblFmt(const CTblFmt& f)
{
    *this=f;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTblFmt::Init
//
/////////////////////////////////////////////////////////////////////////////
void CTblFmt::Init(void)
{
    // init base class members ...
    CFmtBase::Init();
    SetID(FMT_ID_TABLE);

    // init class members ...
    SetBorderLeft(LINE_DEFAULT);
    SetBorderTop(LINE_DEFAULT);
    SetBorderRight(LINE_DEFAULT);
    SetBorderBottom(LINE_DEFAULT);
    SetLeadering(LEFT, LEADERING_DEFAULT);
    SetLeadering(RIGHT, LEADERING_DEFAULT);
    SetReaderBreak(READER_BREAK_DEFAULT);

    m_bIncludeSubTitle = false;
    m_bIncludePageNote = false;
    m_bIncludeEndNote = false;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTblFmt::Build
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ bool CTblFmt::Build(CSpecFile& specFile, bool bSilent)  {
    CIMSAString sCmd;    // command (left side of =)
    CIMSAString sArg;    // argument (right side of =)
    FOREIGN_KEYS foreign;
    bool bLineOK=false;

    // build base class
    CFmtBase::Build(specFile, bSilent);

    // now read line by line ...
    while (specFile.GetState() == SF_OK)  {
        specFile.GetLine(sCmd, sArg);
        ASSERT (!sCmd.IsEmpty());
        if (sCmd[0] == '[')  {
            // we are starting a new section ... put back the line and quit
            specFile.UngetLine();
            break;
        }
        bLineOK = false;
        StripQuotes(sArg);

        // border left
        if (!sCmd.CompareNoCase(TFT_CMD_BORDER_LEFT)) {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_THICK)==0) {
                SetBorderLeft(LINE_THICK);
            }
            else if (sArg.CompareNoCase(TFT_ARG_THIN)==0) {
                SetBorderLeft(LINE_THIN);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NONE)==0) {
                SetBorderLeft(LINE_NONE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                SetBorderLeft(LINE_DEFAULT);
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // border top
        if (!sCmd.CompareNoCase(TFT_CMD_BORDER_TOP)) {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_THICK)==0) {
                SetBorderTop(LINE_THICK);
            }
            else if (sArg.CompareNoCase(TFT_ARG_THIN)==0) {
                SetBorderTop(LINE_THIN);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NONE)==0) {
                SetBorderTop(LINE_NONE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                SetBorderTop(LINE_DEFAULT);
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // border right
        if (!sCmd.CompareNoCase(TFT_CMD_BORDER_RIGHT)) {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_THICK)==0) {
                SetBorderRight(LINE_THICK);
            }
            else if (sArg.CompareNoCase(TFT_ARG_THIN)==0) {
                SetBorderRight(LINE_THIN);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NONE)==0) {
                SetBorderRight(LINE_NONE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                SetBorderRight(LINE_DEFAULT);
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // border bottom
        if (!sCmd.CompareNoCase(TFT_CMD_BORDER_BOTTOM)) {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_THICK)==0) {
                SetBorderBottom(LINE_THICK);
            }
            else if (sArg.CompareNoCase(TFT_ARG_THIN)==0) {
                SetBorderBottom(LINE_THIN);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NONE)==0) {
                SetBorderBottom(LINE_NONE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                SetBorderBottom(LINE_DEFAULT);
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // leadering left
        if (!sCmd.CompareNoCase(TFT_CMD_LEADER_LEFT)) {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_DOT)==0) {
                SetLeadering(LEFT, LEADERING_DOT);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DOT_SPACE)==0) {
                SetLeadering(LEFT, LEADERING_DOT_SPACE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DASH)==0) {
                SetLeadering(LEFT, LEADERING_DASH);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DASH_SPACE)==0) {
                SetLeadering(LEFT, LEADERING_DASH_SPACE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NONE)==0) {
                SetLeadering(LEFT, LEADERING_NONE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                SetLeadering(LEFT, LEADERING_DEFAULT);
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // leadering right
        if (!sCmd.CompareNoCase(TFT_CMD_LEADER_RIGHT)) {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_DOT)==0) {
                SetLeadering(RIGHT, LEADERING_DOT);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DOT_SPACE)==0) {
                SetLeadering(RIGHT, LEADERING_DOT_SPACE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DASH)==0) {
                SetLeadering(RIGHT, LEADERING_DASH);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DASH_SPACE)==0) {
                SetLeadering(RIGHT, LEADERING_DASH_SPACE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NONE)==0) {
                SetLeadering(RIGHT, LEADERING_NONE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                SetLeadering(RIGHT, LEADERING_DEFAULT);
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // reader breaks
        if (!sCmd.CompareNoCase(TFT_CMD_READER_BREAK)) {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_NONE)==0) {
                SetReaderBreak(READER_BREAK_NONE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_ONE)==0) {
                SetReaderBreak(READER_BREAK_ONE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_TWO)==0) {
                SetReaderBreak(READER_BREAK_TWO);
            }
            else if (sArg.CompareNoCase(TFT_ARG_THREE)==0) {
                SetReaderBreak(READER_BREAK_THREE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_FOUR)==0) {
                SetReaderBreak(READER_BREAK_FOUR);
            }
            else if (sArg.CompareNoCase(TFT_ARG_FIVE)==0) {
                SetReaderBreak(READER_BREAK_FIVE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_SIX)==0) {
                SetReaderBreak(READER_BREAK_SIX);
            }
            else if (sArg.CompareNoCase(TFT_ARG_SEVEN)==0) {
                SetReaderBreak(READER_BREAK_SEVEN);
            }
            else if (sArg.CompareNoCase(TFT_ARG_EIGHT)==0) {
                SetReaderBreak(READER_BREAK_EIGHT);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NINE)==0) {
                SetReaderBreak(READER_BREAK_NINE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_TEN)==0) {
                SetReaderBreak(READER_BREAK_TEN);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                SetReaderBreak(READER_BREAK_DEFAULT);
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }
         // Include SubTitle
        if (!sCmd.CompareNoCase(TFT_CMD_INCLUDE_SUBTITLE))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0)  {
                SetIncludeSubTitle(true);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0)  {
                SetIncludeSubTitle(false);
            }
            else  {
                // bad argument
                bLineOK=false;
            }
        }
        // Include PageNote
        if (!sCmd.CompareNoCase(TFT_CMD_INCLUDE_PAGENOTE))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0)  {
                SetIncludePageNote(true);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0)  {
                SetIncludePageNote(false);
            }
            else  {
                // bad argument
                bLineOK=false;
            }
        }
        // Include EndNote
        if (!sCmd.CompareNoCase(TFT_CMD_INCLUDE_ENDNOTE))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0)  {
                SetIncludeEndNote(true);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0)  {
                SetIncludeEndNote(false);
            }
            else  {
                // bad argument
                bLineOK=false;
            }
        }
        if (!bLineOK)  {
            // signal unrecognized command
			CIMSAString sMsg;
            sMsg.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
			sMsg += _T("\n") + sCmd + _T("=") + sArg;
			AfxMessageBox(sMsg,MB_ICONEXCLAMATION);
        }
    }
    return true;
}



/////////////////////////////////////////////////////////////////////////////
//
//                               CTblFmt::Save
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ bool CTblFmt::Save(CSpecFile& specFile) const  {
    CIMSAString s;

    // save base class stuff...
    CFmtBase::Save(specFile);

    // left border
    switch(GetBorderLeft())  {
    case LINE_THIN:
        specFile.PutLine(TFT_CMD_BORDER_LEFT, TFT_ARG_THIN);
        break;
    case LINE_THICK:
        specFile.PutLine(TFT_CMD_BORDER_LEFT, TFT_ARG_THICK);
        break;
    case LINE_NONE:
		specFile.PutLine(TFT_CMD_BORDER_LEFT, TFT_ARG_NONE);
        break;
    case LINE_DEFAULT:
        // do nothing
        break;
    default:
        ASSERT(FALSE);
    }

    // top border
    switch(GetBorderTop())  {
    case LINE_THIN:
        specFile.PutLine(TFT_CMD_BORDER_TOP, TFT_ARG_THIN);
        break;
    case LINE_THICK:
        specFile.PutLine(TFT_CMD_BORDER_TOP, TFT_ARG_THICK);
        break;
    case LINE_NONE:
		specFile.PutLine(TFT_CMD_BORDER_TOP, TFT_ARG_NONE);
        break;
    case LINE_DEFAULT:
        // do nothing
        break;
    default:
        ASSERT(FALSE);
    }

    // right border
    switch(GetBorderRight())  {
    case LINE_THIN:
        specFile.PutLine(TFT_CMD_BORDER_RIGHT, TFT_ARG_THIN);
        break;
    case LINE_THICK:
        specFile.PutLine(TFT_CMD_BORDER_RIGHT, TFT_ARG_THICK);
        break;
    case LINE_NONE:
		specFile.PutLine(TFT_CMD_BORDER_RIGHT, TFT_ARG_NONE);
        break;
    case LINE_DEFAULT:
        // do nothing
        break;
    default:
        ASSERT(FALSE);
    }

    // bottom border
    switch(GetBorderBottom())  {
    case LINE_THIN:
        specFile.PutLine(TFT_CMD_BORDER_BOTTOM, TFT_ARG_THIN);
        break;
    case LINE_THICK:
        specFile.PutLine(TFT_CMD_BORDER_BOTTOM, TFT_ARG_THICK);
        break;
    case LINE_NONE:
		specFile.PutLine(TFT_CMD_BORDER_BOTTOM, TFT_ARG_NONE);
        break;
    case LINE_DEFAULT:
        // do nothing
        break;
    default:
        ASSERT(FALSE);
    }

    // leadering left
    switch(GetLeadering(LEFT)) {
    case LEADERING_DOT:
        specFile.PutLine(TFT_CMD_LEADER_LEFT, TFT_ARG_DOT);
        break;
    case LEADERING_DOT_SPACE:
        specFile.PutLine(TFT_CMD_LEADER_LEFT, TFT_ARG_DOT_SPACE);
        break;
    case LEADERING_DASH:
        specFile.PutLine(TFT_CMD_LEADER_LEFT, TFT_ARG_DASH);
        break;
    case LEADERING_DASH_SPACE:
        specFile.PutLine(TFT_CMD_LEADER_LEFT, TFT_ARG_DASH_SPACE);
        break;
    case LEADERING_NONE:
        specFile.PutLine(TFT_CMD_LEADER_LEFT, TFT_ARG_NONE);
        break;
    case LEADERING_DEFAULT:
        // do nothing
        break;
    default:
        ASSERT(FALSE);
    }

    // leadering right
    switch(GetLeadering(RIGHT)) {
    case LEADERING_DOT:
        specFile.PutLine(TFT_CMD_LEADER_RIGHT, TFT_ARG_DOT);
        break;
    case LEADERING_DOT_SPACE:
        specFile.PutLine(TFT_CMD_LEADER_RIGHT, TFT_ARG_DOT_SPACE);
        break;
    case LEADERING_DASH:
        specFile.PutLine(TFT_CMD_LEADER_RIGHT, TFT_ARG_DASH);
        break;
    case LEADERING_DASH_SPACE:
        specFile.PutLine(TFT_CMD_LEADER_RIGHT, TFT_ARG_DASH_SPACE);
        break;
    case LEADERING_NONE:
        specFile.PutLine(TFT_CMD_LEADER_RIGHT, TFT_ARG_NONE);
        break;
    case LEADERING_DEFAULT:
        // do nothing
        break;
    default:
        ASSERT(FALSE);
    }

    // reader breaks
    switch(GetReaderBreak()) {
    case READER_BREAK_NONE:
        specFile.PutLine(TFT_CMD_READER_BREAK, TFT_ARG_NONE);
        break;
    case READER_BREAK_ONE:
        specFile.PutLine(TFT_CMD_READER_BREAK, TFT_ARG_ONE);
        break;
    case READER_BREAK_TWO:
        specFile.PutLine(TFT_CMD_READER_BREAK, TFT_ARG_TWO);
        break;
    case READER_BREAK_THREE:
        specFile.PutLine(TFT_CMD_READER_BREAK, TFT_ARG_THREE);
        break;
    case READER_BREAK_FOUR:
        specFile.PutLine(TFT_CMD_READER_BREAK, TFT_ARG_FOUR);
        break;
    case READER_BREAK_FIVE:
        specFile.PutLine(TFT_CMD_READER_BREAK, TFT_ARG_FIVE);
        break;
    case READER_BREAK_SIX:
        specFile.PutLine(TFT_CMD_READER_BREAK, TFT_ARG_SIX);
        break;
    case READER_BREAK_SEVEN:
        specFile.PutLine(TFT_CMD_READER_BREAK, TFT_ARG_SEVEN);
        break;
    case READER_BREAK_EIGHT:
        specFile.PutLine(TFT_CMD_READER_BREAK, TFT_ARG_EIGHT);
        break;
    case READER_BREAK_NINE:
        specFile.PutLine(TFT_CMD_READER_BREAK, TFT_ARG_NINE);
        break;
    case READER_BREAK_TEN:
        specFile.PutLine(TFT_CMD_READER_BREAK, TFT_ARG_TEN);
        break;
    case READER_BREAK_DEFAULT:
        // do nothing
        break;
    }
    specFile.PutLine(TFT_CMD_INCLUDE_SUBTITLE, (HasSubTitle()?TFT_ARG_YES:TFT_ARG_NO));
    specFile.PutLine(TFT_CMD_INCLUDE_PAGENOTE, (HasPageNote()?TFT_ARG_YES:TFT_ARG_NO));
    specFile.PutLine(TFT_CMD_INCLUDE_ENDNOTE, (HasEndNote()?TFT_ARG_YES:TFT_ARG_NO));
    return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTblFmt::operator==
//                               CTblFmt::operator!=
//                               CTblFmt::operator=
//
/////////////////////////////////////////////////////////////////////////////
bool CTblFmt::operator==(const CTblFmt& f) const
{
    return (CFmtBase::operator==(f) &&
        GetBorderLeft()==f.GetBorderLeft() &&
        GetBorderTop()==f.GetBorderTop() &&
        GetBorderRight()==f.GetBorderRight() &&
        GetBorderBottom()==f.GetBorderBottom() &&
        GetLeadering(LEFT)==f.GetLeadering(LEFT) &&
        GetLeadering(RIGHT)==f.GetLeadering(RIGHT) &&
        GetReaderBreak()==f.GetReaderBreak() &&
        HasSubTitle() == f.HasSubTitle() &&
        HasPageNote() == f.HasPageNote() &&
        HasEndNote() == f.HasEndNote());
}


bool CTblFmt::operator!=(const CTblFmt& f) const
{
    return !(operator==(f));
}


void CTblFmt::operator=(const CTblFmt& f)
{
    CFmtBase::operator=(f);
    SetBorderLeft(f.GetBorderLeft());
    SetBorderTop(f.GetBorderTop());
    SetBorderRight(f.GetBorderRight());
    SetBorderBottom(f.GetBorderBottom());
    SetLeadering(LEFT, f.GetLeadering(LEFT));
    SetLeadering(RIGHT, f.GetLeadering(RIGHT));
    SetReaderBreak(f.GetReaderBreak());
    SetIncludePageNote(f.HasPageNote());
    SetIncludeSubTitle(f.HasSubTitle());
    SetIncludeEndNote(f.HasEndNote());
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CopyNonDefaultValues
//
// pTarget --   the CFmt that will contain guaranteed non-default values, based on
//              the object's attributes.  This is what gets modified here.
// pDefault --  the default format from the fmt registry (ie, index=0), which is
//              used to resolve DEFAULT values.  (pDefault never has DEFAULT values.)
//
// procedure --
//    for each attribute {
//        if attrib is non-default, then pTarget=this attribute
//        otherwise, pTarget=pDefault for this attribute
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ void CTblFmt::CopyNonDefaultValues(CFmtBase* pTarget, const CFmtBase* pDefault) const
{
    ASSERT(pDefault->GetIndex()==0);
    ASSERT(GetID()==pDefault->GetID());

    const CTblFmt* pTblFmtDefault=DYNAMIC_DOWNCAST(CTblFmt,pDefault);
    CTblFmt* pTblFmtTarget=DYNAMIC_DOWNCAST(CTblFmt, pTarget);

    *pTblFmtTarget=*this;

    // border left
    if (GetBorderLeft()==LINE_DEFAULT) {
        pTblFmtTarget->SetBorderLeft(pTblFmtDefault->GetBorderLeft());
    }

    // border top
    if (GetBorderTop()==LINE_DEFAULT) {
        pTblFmtTarget->SetBorderTop(pTblFmtDefault->GetBorderTop());
    }

    // border right
    if (GetBorderRight()==LINE_DEFAULT) {
        pTblFmtTarget->SetBorderRight(pTblFmtDefault->GetBorderRight());
    }

    // border bottom
    if (GetBorderBottom()==LINE_DEFAULT) {
        pTblFmtTarget->SetBorderBottom(pTblFmtDefault->GetBorderBottom());
    }

    // leadering left
    if (GetLeadering(LEFT)==LEADERING_DEFAULT) {
        pTblFmtTarget->SetLeadering(LEFT, pTblFmtDefault->GetLeadering(LEFT));
    }

    // leadering right
    if (GetLeadering(RIGHT)==LEADERING_DEFAULT) {
        pTblFmtTarget->SetLeadering(RIGHT, pTblFmtDefault->GetLeadering(RIGHT));
    }

    // reader breaks
    if (GetReaderBreak()==READER_BREAK_DEFAULT) {
        pTblFmtTarget->SetReaderBreak(pTblFmtDefault->GetReaderBreak());
    }

    // sanity checks
    ASSERT(!pTblFmtTarget->ContainsDefaultValues());
}


/////////////////////////////////////////////////////////////////////////////
//
//                  CTblFmt::ContainsDefaultValues
//
// Returns true if any of the format's attributes are default
// (LINE_DEFAULT, for example); returns false otherwise.
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ bool CTblFmt::ContainsDefaultValues(void) const
{
    if (GetBorderLeft()!=LINE_DEFAULT &&
        GetBorderTop()!=LINE_DEFAULT &&
        GetBorderRight()!=LINE_DEFAULT &&
        GetBorderBottom()!=LINE_DEFAULT &&
        GetLeadering(LEFT)!=LEADERING_DEFAULT &&
        GetLeadering(RIGHT)!=LEADERING_DEFAULT &&
        GetReaderBreak()!=READER_BREAK_DEFAULT) {
        return false;
    }
    else {
        return true;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSetFmt::CTabSetFmt
//
/////////////////////////////////////////////////////////////////////////////
CTabSetFmt::CTabSetFmt(void)
{
    Init();
}

CTabSetFmt::CTabSetFmt(const CTabSetFmt& f)
{
    *this=f;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSetFmt::Init
//
/////////////////////////////////////////////////////////////////////////////
void CTabSetFmt::Init(void)
{
    // init base class members ...
    CFmtBase::Init();
    SetID(FMT_ID_TABSET);

    // gather regional settings, and use them to init class members ...
    csprochar acTmp[10];
    CIMSAString sTmp;
    //CIMSAString sDigitGrouping;
    int iBytes;
    LCID lcid = ::GetUserDefaultLCID();

    // zero before decimal
    iBytes = ::GetLocaleInfo(lcid, LOCALE_ILZERO, acTmp, 9);
    ASSERT(iBytes==2);
    ASSERT(acTmp[0]==_T('1') || acTmp[0]=='0');
    SetZeroBeforeDecimal(acTmp[0]=='1');

    // thousands separator
    iBytes = ::GetLocaleInfo(lcid, LOCALE_STHOUSAND, acTmp, 9);
    SetThousandsSep((LPCTSTR)acTmp);

    // decimal separator
    iBytes = ::GetLocaleInfo(lcid, LOCALE_SDECIMAL, acTmp, 9);
    SetDecimalSep((LPCTSTR)acTmp);

    // digit grouping
    iBytes = ::GetLocaleInfo(lcid, LOCALE_SGROUPING, acTmp, 9);    // ex: 3;0    3 means 3 digits/block     0 means repeating
    ASSERT(iBytes>1);                                              // if not "3;2;0" or "3;0" then use NONE
    sTmp = (csprochar*)acTmp;
    if (sTmp==_T("3;2;0"))  {
        SetDigitGrouping(DIGIT_GROUPING_INDIC);
    }
    else if (sTmp==_T("3;0"))  {
        SetDigitGrouping(DIGIT_GROUPING_THOUSANDS);
    }
    else {
        SetDigitGrouping(DIGIT_GROUPING_NONE);
    }

    // units
    iBytes = ::GetLocaleInfo(lcid, LOCALE_IMEASURE, acTmp, 9);
    ASSERT(iBytes==2);
    ASSERT(acTmp[0]==_T('1') || acTmp[0]=='0');
    if (acTmp[0]=='0')  {
        SetUnits(UNITS_METRIC);
    }
    else {
        SetUnits(UNITS_US);
    }

    // foreign keys
    FOREIGN_KEYS foreign;
    foreign.SetKey(_T("Table"), _T("Table"));
    foreign.SetKey(_T("and"), _T("and"));
    foreign.SetKey(_T("by"), _T("by"));
    foreign.SetKey(_T("for"), _T("for"));
    foreign.SetKey(_T("to"), _T("to"));
    foreign.SetKey(_T("(Weighted)"), _T("(Weighted)"));
    foreign.SetKey(_T("(Percents)"), _T("(Percents)"));
    foreign.SetKey(_T("Percent"), _T("Percent"));
    foreign.SetKey(_T("Total"), _T("Total"));
    foreign.SetKey(_T("Undefined"), _T("Undefined"));
    foreign.SetKey(_T("Frequency"), _T("Frequency"));
    foreign.SetKey(_T("Cumulative"), _T("Cumulative"));
    foreign.SetKey(_T("<Value has no label>"), _T("<Value has no label>"));
    foreign.SetKey(_T("Mean"), _T("Mean"));
    foreign.SetKey(_T("Median"), _T("Median"));
    foreign.SetKey(_T("Mode"), _T("Mode"));
    foreign.SetKey(_T("Minimum"), _T("Minimum"));
    foreign.SetKey(_T("Maximum"), _T("Maximum"));
    foreign.SetKey(_T("Variance"), _T("Variance"));
    foreign.SetKey(_T("Standard Deviation"), _T("Standard Deviation"));
    foreign.SetKey(_T("Proportion"), _T("Proportion"));
    foreign.SetKey(_T("Number"), _T("Number"));
    foreign.SetKey(_T("Percentile"), _T("Percentile"));

    SetAltForeignKeys(foreign); //alternate text

    // other stuff ...
    SetZeroMask(_T("-"));
    SetZRoundMask(_T("*"));
    SetSuppressed(_T("SSS"));
    //SetTitleTemplate("Table %d. %s");
    SetTitleTemplate(_T("Table %s."));
    SetContinuationStr(_T(" (continued)"));
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSetFmt::Build
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ bool CTabSetFmt::Build(CSpecFile& specFile, bool bSilent)  {
    CIMSAString sCmd;    // command (left side of =)
    CIMSAString sArg;    // argument (right side of =)
    CIMSAString sMsg;
    bool bLineOK;

    // build base class
    CFmtBase::Build(specFile, bSilent);

    // now read line by line ...
    while (specFile.GetState() == SF_OK)  {
        specFile.GetLine(sCmd, sArg);
        ASSERT (!sCmd.IsEmpty());
        if (sCmd[0] == '[')  {
            // we are starting a new section ... put back the line and quit
            specFile.UngetLine();
            break;
        }
        bLineOK = false;
        StripQuotes(sArg);

        // thousands separator
        if (!sCmd.CompareNoCase(TFT_CMD_SEP_THOUSANDS))  {
            bLineOK=true;
            if (sArg.GetLength()>3) {
                AfxMessageBox(_T("Warning: thousands separator too long; truncated to 3 characters."), MB_ICONWARNING);
            }
            SetThousandsSep(sArg.Left(3));
        }

        // decimal separator
        if (!sCmd.CompareNoCase(TFT_CMD_SEP_DECIMALS))  {
            bLineOK=true;
            if (sArg.GetLength()>3) {
                AfxMessageBox(_T("Warning: decimal separator too long; truncated to 3 characters."), MB_ICONWARNING);
            }
            SetDecimalSep(sArg.Left(3));
        }

        // zero mask string
        if (!sCmd.CompareNoCase(TFT_CMD_ZERO_MASK))  {
            bLineOK=true;
            if (sArg.GetLength()>10) {
                AfxMessageBox(_T("Warning: zero mask too long; truncated to 10 characters."), MB_ICONWARNING);
            }
            SetZeroMask(sArg.Left(10));
        }

        // rounded zero mask string
        if (!sCmd.CompareNoCase(TFT_CMD_ZROUND_MASK))  {
            bLineOK=true;
            if (sArg.GetLength()>10) {
                AfxMessageBox(_T("Warning: zero round mask too long; truncated to 10 characters."), MB_ICONWARNING);
            }
            SetZRoundMask(sArg.Left(10));
        }

        // suppressed cell mask string
        if (!sCmd.CompareNoCase(TFT_CMD_SUPPRESSED))  {
            bLineOK=true;
            if (sArg.GetLength()>10) {
                AfxMessageBox(_T("Warning: suppressed string too long; truncated to 10 characters."), MB_ICONWARNING);
            }
            SetSuppressed(sArg.Left(10));
        }

        // digit grouping
        if (!sCmd.CompareNoCase(TFT_CMD_DIGIT_GROUPING))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_NONE)==0)  {
                SetDigitGrouping(DIGIT_GROUPING_NONE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_THOUSANDS)==0)  {
                SetDigitGrouping(DIGIT_GROUPING_THOUSANDS);
            }
            else if (sArg.CompareNoCase(TFT_ARG_INDIC)==0)  {
                SetDigitGrouping(DIGIT_GROUPING_INDIC);
            }
            else  {
                // bad argument
                bLineOK=false;
            }
        }

        // show zero before decimal
        if (!sCmd.CompareNoCase(TFT_CMD_ZERO_BEFORE_DEC))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0)  {
                SetZeroBeforeDecimal(true);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0)  {
                SetZeroBeforeDecimal(false);
            }
            else  {
                // bad argument
                bLineOK=false;
            }
        }

        // foreign keys - look first for TFT_CMD_FOREIGN_XXX for backwards compat, newer files (3.2+)
        // just write out Word=default,alt
        //

        if (!sCmd.CompareNoCase(TFT_CMD_FOREIGN_TABLE) ||
            !sCmd.CompareNoCase(TFT_CMD_FOREIGN_AND) ||
            !sCmd.CompareNoCase(TFT_CMD_FOREIGN_BY) ||
            !sCmd.CompareNoCase(TFT_CMD_FOREIGN_FOR) ||
            !sCmd.CompareNoCase(TFT_CMD_FOREIGN_TO) ||
            !sCmd.CompareNoCase(TFT_CMD_FOREIGN_WEIGHTED) ||
            !sCmd.CompareNoCase(TFT_CMD_FOREIGN_PERCENTS) ||
            !sCmd.CompareNoCase(TFT_CMD_FOREIGN_PERCENT) ||
            !sCmd.CompareNoCase(TFT_CMD_FOREIGN_TOTAL) ||
            !sCmd.CompareNoCase(TFT_CMD_FOREIGN_UNDEFINED) ||
            !sCmd.CompareNoCase(TFT_CMD_FOREIGN_FREQUENCY) ||
            !sCmd.CompareNoCase(TFT_CMD_FOREIGN_CUMULATIVE) ||
            !sCmd.CompareNoCase(TFT_CMD_FOREIGN_NOLABEL) ||
            !sCmd.CompareNoCase(TFT_CMD_FOREIGN_CUMULATIVE) ||
            !sCmd.CompareNoCase(TFT_CMD_FOREIGN_CUMULATIVE) ||
            !sCmd.CompareNoCase(TFT_CMD_FOREIGN_KEY)) {

            CIMSAString sDef =sArg.GetToken(_T(","));
            sDef.TrimRight(_T("'"));
            CIMSAString sAlt=sArg.GetToken();
            StripQuotes(sAlt);
            sAlt.Trim();
            if(sAlt.IsEmpty()){
                sAlt = sDef;
            }
            m_foreignAlt.SetKey(sDef, sAlt);
            bLineOK=true;
            continue;
        }

        // title template
        if (!sCmd.CompareNoCase(TFT_CMD_TITLE_TEMPLATE))  {
            SetTitleTemplate(sArg);
            bLineOK=true;
            continue;
        }

        // title continuation string
        if (!sCmd.CompareNoCase(TFT_CMD_CONTINUATION))  {
            SetContinuationStr(sArg);
            bLineOK=true;
            continue;
        }

        // units
        if (!sCmd.CompareNoCase(TFT_CMD_UNITS))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_METRIC)==0)  {
                SetUnits(UNITS_METRIC);
            }
            else if (sArg.CompareNoCase(TFT_ARG_US)==0)  {
                SetUnits(UNITS_US);
            }
            else  {
                // bad argument
                bLineOK=false;
            }
        }

        if (!bLineOK)  {
            // signal unrecognized command
            sArg.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
            sArg += _T("\n") + sCmd;
            AfxMessageBox(sArg);
        }
    }


    return true;
}



/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSetFmt::Save
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ bool CTabSetFmt::Save(CSpecFile& specFile) const  {
    CIMSAString s;

    // save base class stuff...
    CFmtBase::Save(specFile);

    // thousands separator
    s=GetThousandsSep();
    s.QuoteDelimit();
    specFile.PutLine(TFT_CMD_SEP_THOUSANDS, s);

    // decimal separator
    s=GetDecimalSep();
    s.QuoteDelimit();
    specFile.PutLine(TFT_CMD_SEP_DECIMALS, s);

    // zero mask string
    s=GetZeroMask();
    s.QuoteDelimit();
    specFile.PutLine(TFT_CMD_ZERO_MASK, s);

    // rounded zero mask string
    s=GetZRoundMask();
    s.QuoteDelimit();
    specFile.PutLine(TFT_CMD_ZROUND_MASK, s);

    // suppressed cell mask string
    s=GetSuppressed();
    s.QuoteDelimit();
    specFile.PutLine(TFT_CMD_SUPPRESSED, s);

    // digit grouping
    switch(GetDigitGrouping())  {
    case DIGIT_GROUPING_NONE:
        specFile.PutLine(TFT_CMD_DIGIT_GROUPING, TFT_ARG_NONE);
        break;
    case DIGIT_GROUPING_THOUSANDS:
        specFile.PutLine(TFT_CMD_DIGIT_GROUPING, TFT_ARG_THOUSANDS);
        break;
    case DIGIT_GROUPING_INDIC:
        specFile.PutLine(TFT_CMD_DIGIT_GROUPING, TFT_ARG_INDIC);
        break;
    default:
        ASSERT(FALSE);
    }

    // show zero before decimal
    specFile.PutLine(TFT_CMD_ZERO_BEFORE_DEC, (GetZeroBeforeDecimal()?TFT_ARG_YES:TFT_ARG_NO));

    // foreign keys
    const FOREIGN_KEYS& altforeign = GetAltForeignKeys();
    altforeign.Save(specFile);

    // title template
    s=GetTitleTemplate();
    s.QuoteDelimit();
    specFile.PutLine(TFT_CMD_TITLE_TEMPLATE, s);

    // title continuation string
    s=GetContinuationStr();
    s.QuoteDelimit();
    specFile.PutLine(TFT_CMD_CONTINUATION, s);

    // units
    switch(GetUnits())  {
    case UNITS_METRIC:
        specFile.PutLine(TFT_CMD_UNITS, TFT_ARG_METRIC);
        break;
    case UNITS_US:
        specFile.PutLine(TFT_CMD_UNITS, TFT_ARG_US);
        break;
    default:
        ASSERT(FALSE);
    }

    return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSetFmt::operator==
//                               CTabSetFmt::operator!=
//                               CTabSetFmt::operator=
//
/////////////////////////////////////////////////////////////////////////////
bool CTabSetFmt::operator==(const CTabSetFmt& f) const
{
    return (CFmtBase::operator==(f) &&
        GetThousandsSep()==f.GetThousandsSep() &&
        GetDecimalSep()==f.GetDecimalSep() &&
        GetZeroMask()==f.GetZeroMask() &&
        GetZRoundMask()==f.GetZRoundMask() &&
        GetSuppressed()==f.GetSuppressed() &&
        GetDigitGrouping()==f.GetDigitGrouping() &&
        GetZeroBeforeDecimal()==f.GetZeroBeforeDecimal() &&
        GetAltForeignKeys()==f.GetAltForeignKeys() &&
        GetTitleTemplate()==f.GetTitleTemplate() &&
        GetContinuationStr()==f.GetContinuationStr() &&
        GetUnits()==f.GetUnits());
}


bool CTabSetFmt::operator!=(const CTabSetFmt& f) const
{
    return !(operator==(f));
}


void CTabSetFmt::operator=(const CTabSetFmt& f)
{
    CFmtBase::operator=(f);
    SetThousandsSep(f.GetThousandsSep());
    SetDecimalSep(f.GetDecimalSep());
    SetZeroMask(f.GetZeroMask());
    SetZRoundMask(f.GetZRoundMask());
    SetSuppressed(f.GetSuppressed());
    SetDigitGrouping(f.GetDigitGrouping());
    SetZeroBeforeDecimal(f.GetZeroBeforeDecimal());
    SetAltForeignKeys(f.GetAltForeignKeys());
    SetTitleTemplate(f.GetTitleTemplate());
    SetContinuationStr(f.GetContinuationStr());
    SetUnits(f.GetUnits());
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CopyNonDefaultValues
//
// pTarget --   the CFmt that will contain guaranteed non-default values, based on
//              the object's attributes.  This is what gets modified here.
// pDefault --  the default format from the fmt registry (ie, index=0), which is
//              used to resolve DEFAULT values.  (pDefault never has DEFAULT values.)
//
// procedure --
//    since tabsets don't have non-default values, we just return ourselves
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ void CTabSetFmt::CopyNonDefaultValues(CFmtBase* pTarget, const CFmtBase* pDefault) const
{
    ASSERT(pDefault->GetIndex()==0);
    ASSERT(GetID()==pDefault->GetID());

    *pTarget=*this;
}


/////////////////////////////////////////////////////////////////////////////
//
//                  CTabSetFmt::ContainsDefaultValues
//
// Returns true if any of the format's attributes are default
// (LINE_DEFAULT, for example); returns false otherwise.
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ bool CTabSetFmt::ContainsDefaultValues(void) const
{
    return false;
}



/////////////////////////////////////////////////////////////////////////////
//
//                               CTblPrintFmt::CTblPrintFmt
//
/////////////////////////////////////////////////////////////////////////////
CTblPrintFmt::CTblPrintFmt(void)
{
    Init();
}

CTblPrintFmt::CTblPrintFmt(const CTblPrintFmt& f)
{
    *this=f;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTblPrintFmt::Init
//
/////////////////////////////////////////////////////////////////////////////
void CTblPrintFmt::Init(void)
{
    // init base class members ...
    CFmtBase::Init();
    SetID(FMT_ID_TBLPRINT);

    // init class members ...
    SetTblLayout(TBL_LAYOUT_DEFAULT);
    SetPageBreak(PAGE_BREAK_DEFAULT);
    SetCtrHorz(CENTER_TBL_DEFAULT);
    SetCtrVert(CENTER_TBL_DEFAULT);
    SetStartPage(START_PAGE_DEFAULT);
    SetPaperType(PAPER_TYPE_DEFAULT);
    SetPageOrientation(PAGE_ORIENTATION_DEFAULT);
    SetPageMargin(CRect(PAGE_MARGIN_DEFAULT,PAGE_MARGIN_DEFAULT,PAGE_MARGIN_DEFAULT,PAGE_MARGIN_DEFAULT));
    SetHeaderFrequency(HEADER_FREQUENCY_DEFAULT);
    SetUnits(UNITS_METRIC);
    SetPrinterDevice(_T(""));
    SetPrinterDriver(_T(""));
    SetPrinterOutput(_T(""));
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTblPrintFmt::Build
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ bool CTblPrintFmt::Build(CSpecFile& specFile, bool bSilent)  {
    CIMSAString sCmd;    // command (left side of =)
    CIMSAString sArg;    // argument (right side of =)
    CIMSAString sMsg;
    bool bLineOK;

    // build base class
    CFmtBase::Build(specFile, bSilent);

    // now read line by line ...
    while (specFile.GetState() == SF_OK)  {
        specFile.GetLine(sCmd, sArg);
        ASSERT (!sCmd.IsEmpty());
        if (sCmd[0] == '[')  {
            // we are starting a new section ... put back the line and quit
            specFile.UngetLine();
            break;
        }
        bLineOK = false;
        StripQuotes(sArg);

        // table layout
        if (!sCmd.CompareNoCase(TFT_CMD_TBL_LAYOUT))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_LAY_LEFT_STD)==0)  {
                SetTblLayout(TBL_LAYOUT_LEFT_STANDARD);
            }
            else if (sArg.CompareNoCase(TFT_ARG_LAY_LEFT_FACING)==0)  {
                SetTblLayout(TBL_LAYOUT_LEFT_FACING);
            }
            else if (sArg.CompareNoCase(TFT_ARG_LAY_BOTH_STD)==0)  {
                SetTblLayout(TBL_LAYOUT_BOTH_STANDARD);
            }
            else if (sArg.CompareNoCase(TFT_ARG_LAY_BOTH_FACING)==0)  {
                SetTblLayout(TBL_LAYOUT_BOTH_FACING);
            }
            else  {
                // bad argument
                bLineOK=false;
            }
        }

        // page break before table
        if (!sCmd.CompareNoCase(TFT_CMD_PAGE_BREAK))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0)  {
                SetPageBreak(PAGE_BREAK_YES);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0)  {
                SetPageBreak(PAGE_BREAK_NO);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                SetPageBreak(PAGE_BREAK_DEFAULT);
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // center table horizontally
        if (!sCmd.CompareNoCase(TFT_CMD_HORZCENTER))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0)  {
                SetCtrHorz(CENTER_TBL_YES);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0)  {
                SetCtrHorz(CENTER_TBL_NO);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                SetCtrHorz(CENTER_TBL_DEFAULT);
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // center table vertically
        if (!sCmd.CompareNoCase(TFT_CMD_VERTCENTER))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_YES)==0)  {
                SetCtrVert(CENTER_TBL_YES);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NO)==0)  {
                SetCtrVert(CENTER_TBL_NO);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                SetCtrVert(CENTER_TBL_DEFAULT);
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // table starting page number
        if (!sCmd.CompareNoCase(TFT_CMD_START_PAGE))  {
            if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                SetStartPage(START_PAGE_DEFAULT);
                bLineOK=true;
            }
            else {
                if (sArg.IsNumeric()) {
                    int iStartPage=(int)sArg.Val();
                    if (iStartPage>=0) {
                        SetStartPage(iStartPage);
                        bLineOK=true;
                    }
                }
            }
        }

        // paper type
        if (!sCmd.CompareNoCase(TFT_CMD_PAPER_TYPE))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_A4)==0)  {
                SetPaperType(PAPER_TYPE_A4);
            }
            else if (sArg.CompareNoCase(TFT_ARG_A3)==0)  {
                SetPaperType(PAPER_TYPE_A3);
            }
            else if (sArg.CompareNoCase(TFT_ARG_LETTER)==0)  {
                SetPaperType(PAPER_TYPE_LETTER);
            }
            else if (sArg.CompareNoCase(TFT_ARG_LEGAL)==0)  {
                SetPaperType(PAPER_TYPE_LEGAL);
            }
            else if (sArg.CompareNoCase(TFT_ARG_TABLOID)==0)  {
                SetPaperType(PAPER_TYPE_TABLOID);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                SetPaperType(PAPER_TYPE_DEFAULT);
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // page orientation
        if (!sCmd.CompareNoCase(TFT_CMD_PAPER_ORIENT))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_PORTRAIT)==0)  {
                SetPageOrientation(PAGE_ORIENTATION_PORTRAIT);
            }
            else if (sArg.CompareNoCase(TFT_ARG_LANDSCAPE)==0)  {
                SetPageOrientation(PAGE_ORIENTATION_LANDSCAPE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                SetPageOrientation(PAGE_ORIENTATION_DEFAULT);
            }
            else {
                // bad argument
                bLineOK=false;
            }
        }

        // printer device
        if (!sCmd.CompareNoCase(TFT_CMD_PRINTER_DEVICE))  {
            bLineOK=true;
            SetPrinterDevice(sArg);
        }

        // printer driver
        if (!sCmd.CompareNoCase(TFT_CMD_PRINTER_DRIVER))  {
            bLineOK=true;
            SetPrinterDriver(sArg);
        }

        // printer output
        if (!sCmd.CompareNoCase(TFT_CMD_PRINTER_OUTPUT))  {
            bLineOK=true;
            SetPrinterOutput(sArg);
        }

        // top page margin (in local units; convert to twips)
        if (!sCmd.CompareNoCase(TFT_CMD_MARGIN_TOP))  {
            CRect rcPageMargin=GetPageMargin();
            if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                rcPageMargin.top=PAGE_MARGIN_DEFAULT;
                SetPageMargin(rcPageMargin);
                bLineOK=true;
            }
            else {
                float f=(float)sArg.fVal();
                if (GetUnits()==UNITS_METRIC) {
                    rcPageMargin.top=CmToTwips(f);
                }
                else {
                    rcPageMargin.top=InchesToTwips(f);
                }
                SetPageMargin(rcPageMargin);
                bLineOK=true;
            }
        }

        // bottom page margin
        if (!sCmd.CompareNoCase(TFT_CMD_MARGIN_BOTTOM))  {
            CRect rcPageMargin=GetPageMargin();
            if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                rcPageMargin.bottom=PAGE_MARGIN_DEFAULT;
                SetPageMargin(rcPageMargin);
                bLineOK=true;
            }
            else {
                float f=(float)sArg.fVal();
                if (GetUnits()==UNITS_METRIC) {
                    rcPageMargin.bottom=CmToTwips(f);
                }
                else {
                    rcPageMargin.bottom=InchesToTwips(f);
                }
                SetPageMargin(rcPageMargin);
                bLineOK=true;
            }
        }

        // left page margin
        if (!sCmd.CompareNoCase(TFT_CMD_MARGIN_LEFT))  {
            CRect rcPageMargin=GetPageMargin();
            if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                rcPageMargin.left=PAGE_MARGIN_DEFAULT;
                SetPageMargin(rcPageMargin);
                bLineOK=true;
            }
            else {
                float f=(float)sArg.fVal();
                if (GetUnits()==UNITS_METRIC) {
                    rcPageMargin.left=CmToTwips(f);
                }
                else {
                    rcPageMargin.left=InchesToTwips(f);
                }
                SetPageMargin(rcPageMargin);
                bLineOK=true;
            }
        }

        // right page margin
        if (!sCmd.CompareNoCase(TFT_CMD_MARGIN_RIGHT))  {
            CRect rcPageMargin=GetPageMargin();
            if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0) {
                rcPageMargin.right=PAGE_MARGIN_DEFAULT;
                SetPageMargin(rcPageMargin);
                bLineOK=true;
            }
            else {
                float f=(float)sArg.fVal();
                if (GetUnits()==UNITS_METRIC) {
                    rcPageMargin.right=CmToTwips(f);
                }
                else {
                    rcPageMargin.right=InchesToTwips(f);
                }
                SetPageMargin(rcPageMargin);
                bLineOK=true;
            }
        }

        // headers frequency
        if (!sCmd.CompareNoCase(TFT_CMD_HEADER_FREQUENCY))  {
            bLineOK=true;
            if (sArg.CompareNoCase(TFT_ARG_HEADER_TOP_TBL)==0)  {
                SetHeaderFrequency(HEADER_FREQUENCY_TOP_TABLE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_HEADER_TOP_PAGE)==0)  {
                SetHeaderFrequency(HEADER_FREQUENCY_TOP_PAGE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_NONE)==0)  {
                SetHeaderFrequency(HEADER_FREQUENCY_NONE);
            }
            else if (sArg.CompareNoCase(TFT_ARG_DEFAULT)==0)  {
                SetHeaderFrequency(HEADER_FREQUENCY_DEFAULT);
            }
            else  {
                // bad argument
                bLineOK=false;
            }
        }

        if (!bLineOK)  {
            // signal unrecognized command
            sArg.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
            sArg += _T("\n") + sCmd;
            AfxMessageBox(sArg);
        }
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTblPrintFmt::Save
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ bool CTblPrintFmt::Save(CSpecFile& specFile) const
{
    CIMSAString s;

    // save base class stuff...
    CFmtBase::Save(specFile);

    // table layout
    switch(GetTblLayout())  {
    case TBL_LAYOUT_LEFT_STANDARD:
        specFile.PutLine(TFT_CMD_TBL_LAYOUT, TFT_ARG_LAY_LEFT_STD);
        break;
    case TBL_LAYOUT_LEFT_FACING:
        specFile.PutLine(TFT_CMD_TBL_LAYOUT, TFT_ARG_LAY_LEFT_FACING);
        break;
    case TBL_LAYOUT_BOTH_STANDARD:
        specFile.PutLine(TFT_CMD_TBL_LAYOUT, TFT_ARG_LAY_BOTH_STD);
        break;
    case TBL_LAYOUT_BOTH_FACING:
        specFile.PutLine(TFT_CMD_TBL_LAYOUT, TFT_ARG_LAY_BOTH_FACING);
        break;
    case TBL_LAYOUT_DEFAULT:
        // do nothing
        break;
    default:
        ASSERT(FALSE);
    }

    // page break before table
    switch(GetPageBreak()) {
    case PAGE_BREAK_YES:
        specFile.PutLine(TFT_CMD_PAGE_BREAK, TFT_ARG_YES);
        break;
    case PAGE_BREAK_NO:
        specFile.PutLine(TFT_CMD_PAGE_BREAK, TFT_ARG_NO);
        break;
    case PAGE_BREAK_DEFAULT:
        // do nothing
        break;
    default:
        ASSERT(FALSE);
    }

    // center table horizontally
    switch(GetCtrHorz()) {
    case CENTER_TBL_YES:
        specFile.PutLine(TFT_CMD_HORZCENTER, TFT_ARG_YES);
        break;
    case CENTER_TBL_NO:
        specFile.PutLine(TFT_CMD_HORZCENTER, TFT_ARG_NO);
        break;
    case CENTER_TBL_DEFAULT:
        // do nothing
        break;
    default:
        ASSERT(FALSE);
    }

    // center table vertically
    switch(GetCtrVert()) {
    case CENTER_TBL_YES:
        specFile.PutLine(TFT_CMD_VERTCENTER, TFT_ARG_YES);
        break;
    case CENTER_TBL_NO:
        specFile.PutLine(TFT_CMD_VERTCENTER, TFT_ARG_NO);
        break;
    case CENTER_TBL_DEFAULT:
        // do nothing
        break;
    default:
        ASSERT(FALSE);
    }

    // table starting page number
    if (GetStartPage()!=START_PAGE_DEFAULT) {
        specFile.PutLine(TFT_CMD_START_PAGE, GetStartPage());
    }

    // paper type
    switch(GetPaperType()) {
    case PAPER_TYPE_A4:
        specFile.PutLine(TFT_CMD_PAPER_TYPE, TFT_ARG_A4);
        break;
    case PAPER_TYPE_A3:
        specFile.PutLine(TFT_CMD_PAPER_TYPE, TFT_ARG_A3);
        break;
    case PAPER_TYPE_LETTER:
        specFile.PutLine(TFT_CMD_PAPER_TYPE, TFT_ARG_LETTER);
        break;
    case PAPER_TYPE_LEGAL:
        specFile.PutLine(TFT_CMD_PAPER_TYPE, TFT_ARG_LEGAL);
        break;
    case PAPER_TYPE_TABLOID:
        specFile.PutLine(TFT_CMD_PAPER_TYPE, TFT_ARG_TABLOID);
        break;
    case PAPER_TYPE_DEFAULT:
        // do nothing
        break;
    }

    // page orientation
    switch(GetPageOrientation()) {
    case PAGE_ORIENTATION_PORTRAIT:
        specFile.PutLine(TFT_CMD_PAPER_ORIENT, TFT_ARG_PORTRAIT);
        break;
    case PAGE_ORIENTATION_LANDSCAPE:
        specFile.PutLine(TFT_CMD_PAPER_ORIENT, TFT_ARG_LANDSCAPE);
        break;
    case PAGE_ORIENTATION_DEFAULT:
        // do nothing
        break;
    }

    // printer device
    if (!GetPrinterDevice().IsEmpty()) {
        specFile.PutLine(TFT_CMD_PRINTER_DEVICE, GetPrinterDevice());
    }

    // printer driver
    if (!GetPrinterDriver().IsEmpty()) {
        specFile.PutLine(TFT_CMD_PRINTER_DRIVER, GetPrinterDriver());
    }

    // printer output
    if (!GetPrinterOutput().IsEmpty()) {
        specFile.PutLine(TFT_CMD_PRINTER_OUTPUT, GetPrinterOutput());
    }

    // top page margin
    CRect rcPageMargin=GetPageMargin();
    if (rcPageMargin.top!=PAGE_MARGIN_DEFAULT) {
        float f;
        if (GetUnits()==UNITS_METRIC) {
            f = Round(TwipsToCm(rcPageMargin.top),2);
        }
        else {
            f = Round(TwipsToInches(rcPageMargin.top),2);
        }
        s.Format(_T("%.2f"), f);
        s.Replace(_T('.'),CIMSAString::GetDecChar());
        specFile.PutLine(TFT_CMD_MARGIN_TOP, s);
    }

    // bottom page margin
    if (rcPageMargin.bottom!=PAGE_MARGIN_DEFAULT) {
        float f;
        if (GetUnits()==UNITS_METRIC) {
            f = Round(TwipsToCm(rcPageMargin.bottom),2);
        }
        else {
            f = Round(TwipsToInches(rcPageMargin.bottom),2);
        }
        s.Format(_T("%.2f"), f);
        s.Replace(_T('.'),CIMSAString::GetDecChar());
        specFile.PutLine(TFT_CMD_MARGIN_BOTTOM, s);
    }

    // left page margin
    if (rcPageMargin.left!=PAGE_MARGIN_DEFAULT) {
        float f;
        if (GetUnits()==UNITS_METRIC) {
            f = Round(TwipsToCm(rcPageMargin.left),2);
        }
        else {
            f = Round(TwipsToInches(rcPageMargin.left),2);
        }
        s.Format(_T("%.2f"), f);
        s.Replace(_T('.'),CIMSAString::GetDecChar());
        specFile.PutLine(TFT_CMD_MARGIN_LEFT, s);
    }

    // right page margin
    if (rcPageMargin.right!=PAGE_MARGIN_DEFAULT) {
        float f;
        if (GetUnits()==UNITS_METRIC) {
            f = Round(TwipsToCm(rcPageMargin.right),2);
        }
        else {
            f = Round(TwipsToInches(rcPageMargin.right),2);
        }
        s.Format(_T("%.2f"), f);
        s.Replace(_T('.'),CIMSAString::GetDecChar());
        specFile.PutLine(TFT_CMD_MARGIN_RIGHT, s);
    }

    // header frequency
    switch(GetHeaderFrequency()) {
    case HEADER_FREQUENCY_TOP_TABLE:
        specFile.PutLine(TFT_CMD_HEADER_FREQUENCY, TFT_ARG_HEADER_TOP_TBL);
        break;
    case HEADER_FREQUENCY_TOP_PAGE:
        specFile.PutLine(TFT_CMD_HEADER_FREQUENCY, TFT_ARG_HEADER_TOP_PAGE);
        break;
    case HEADER_FREQUENCY_NONE:
        specFile.PutLine(TFT_CMD_HEADER_FREQUENCY, TFT_ARG_NONE);
        break;
    case HEADER_FREQUENCY_DEFAULT:
        // do nothing
        break;
    }

    return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTblPrintFmt::operator==
//                               CTblPrintFmt::operator!=
//                               CTblPrintFmt::operator=
//
/////////////////////////////////////////////////////////////////////////////
bool CTblPrintFmt::operator==(const CTblPrintFmt& f) const
{
    return (CFmtBase::operator==(f) &&
        GetTblLayout()==f.GetTblLayout() &&
        GetPageBreak()==f.GetPageBreak() &&
        GetCtrHorz()==f.GetCtrHorz() &&
        GetCtrVert()==f.GetCtrVert() &&
        GetStartPage()==f.GetStartPage() &&
        GetPaperType()==f.GetPaperType() &&
        GetPageOrientation()==f.GetPageOrientation() &&
        GetPrinterDevice()==f.GetPrinterDevice() &&
        GetPrinterDriver()==f.GetPrinterDriver() &&
        GetPrinterOutput()==f.GetPrinterOutput() &&
        GetPageMargin()==f.GetPageMargin() &&
        GetUnits()==f.GetUnits() &&
        GetHeaderFrequency()==f.GetHeaderFrequency());
}


bool CTblPrintFmt::operator!=(const CTblPrintFmt& f) const
{
    return !(operator==(f));
}


void CTblPrintFmt::operator=(const CTblPrintFmt& f)
{
    CFmtBase::operator=(f);
    SetTblLayout(f.GetTblLayout());
    SetPageBreak(f.GetPageBreak());
    SetCtrHorz(f.GetCtrHorz());
    SetCtrVert(f.GetCtrVert());
    SetStartPage(f.GetStartPage());
    SetPaperType(f.GetPaperType());
    SetPageOrientation(f.GetPageOrientation());
    SetPrinterDevice(f.GetPrinterDevice());
    SetPrinterDriver(f.GetPrinterDriver());
    SetPrinterOutput(f.GetPrinterOutput());
    SetPageMargin(f.GetPageMargin());
    SetHeaderFrequency(f.GetHeaderFrequency());
    SetUnits(f.GetUnits());
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CopyNonDefaultValues
//
// pTarget --   the CFmt that will contain guaranteed non-default values, based on
//              the object's attributes.  This is what gets modified here.
// pDefault --  the default format from the fmt registry (ie, index=0), which is
//              used to resolve DEFAULT values.  (pDefault never has DEFAULT values.)
//
// procedure --
//    for each attribute {
//        if attrib is non-default, then pTarget=this attribute
//        otherwise, pTarget=pDefault for this attribute
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ void CTblPrintFmt::CopyNonDefaultValues(CFmtBase* pTarget, const CFmtBase* pDefault) const
{
    ASSERT(pDefault->GetIndex()==0);
    ASSERT(GetID()==pDefault->GetID());

    const CTblPrintFmt* pTblPrintFmtDefault=DYNAMIC_DOWNCAST(CTblPrintFmt,pDefault);
    CTblPrintFmt* pTblPrintFmtTarget=DYNAMIC_DOWNCAST(CTblPrintFmt, pTarget);

    *pTblPrintFmtTarget=*this;

    // table layout
    if (GetTblLayout()==TBL_LAYOUT_DEFAULT) {
        pTblPrintFmtTarget->SetTblLayout(pTblPrintFmtDefault->GetTblLayout());
    }

    // page break
    if (GetPageBreak()==PAGE_BREAK_DEFAULT) {
        pTblPrintFmtTarget->SetPageBreak(pTblPrintFmtDefault->GetPageBreak());
    }

    // center table horz
    if (GetCtrHorz()==CENTER_TBL_DEFAULT) {
        pTblPrintFmtTarget->SetCtrHorz(pTblPrintFmtDefault->GetCtrHorz());
    }

    // center table vert
    if (GetCtrVert()==CENTER_TBL_DEFAULT) {
        pTblPrintFmtTarget->SetCtrVert(pTblPrintFmtDefault->GetCtrVert());
    }

    // start page
    if (GetStartPage()==START_PAGE_DEFAULT) {
        pTblPrintFmtTarget->SetStartPage(pTblPrintFmtDefault->GetStartPage());
    }

    // paper type
    if (GetPaperType()==PAPER_TYPE_DEFAULT) {
        pTblPrintFmtTarget->SetPaperType(pTblPrintFmtDefault->GetPaperType());
    }

    // page orientation
    if (GetPageOrientation()==PAGE_ORIENTATION_DEFAULT) {
        pTblPrintFmtTarget->SetPageOrientation(pTblPrintFmtDefault->GetPageOrientation());
    }

    // note that printer device does not have a default value

    // note that printer driver does not have a default value

    // note that printer output does not have a default value

    // header frequency
    if (GetHeaderFrequency()==HEADER_FREQUENCY_DEFAULT) {
        pTblPrintFmtTarget->SetHeaderFrequency(pTblPrintFmtDefault->GetHeaderFrequency());
    }

    // page margins -- left
    CRect rcPageMargin(GetPageMargin());
    if (rcPageMargin.left==PAGE_MARGIN_DEFAULT) {
        rcPageMargin.left=pTblPrintFmtDefault->GetPageMargin().left;
    }

    // page margins -- top
    if (rcPageMargin.top==PAGE_MARGIN_DEFAULT) {
        rcPageMargin.top=pTblPrintFmtDefault->GetPageMargin().top;
    }

    // page margins -- right
    if (rcPageMargin.right==PAGE_MARGIN_DEFAULT) {
        rcPageMargin.right=pTblPrintFmtDefault->GetPageMargin().right;
    }

    // page margins -- bottom
    if (rcPageMargin.bottom==PAGE_MARGIN_DEFAULT) {
        rcPageMargin.bottom=pTblPrintFmtDefault->GetPageMargin().bottom;
    }
    pTblPrintFmtTarget->SetPageMargin(rcPageMargin);

    // there are no default units, so we don't do anything with those

    // sanity checks
    ASSERT(!pTblPrintFmtTarget->ContainsDefaultValues());
}


/////////////////////////////////////////////////////////////////////////////
//
//                  CTblPrintFmt::ContainsDefaultValues
//
// Returns true if any of the format's attributes are default
// (LINE_DEFAULT, for example); returns false otherwise.
//
// note that printer device does not have a default value
// note that printer driver does not have a default value
// note that printer output does not have a default value
// note that start page CAN have a default value
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ bool CTblPrintFmt::ContainsDefaultValues(void) const
{
    if (GetTblLayout()!=TBL_LAYOUT_DEFAULT &&
        GetPageBreak()!=PAGE_BREAK_DEFAULT &&
        GetCtrHorz()!=CENTER_TBL_DEFAULT &&
        GetCtrVert()!=CENTER_TBL_DEFAULT &&
        GetPaperType()!=PAPER_TYPE_DEFAULT &&
        GetPageOrientation()!=PAGE_ORIENTATION_DEFAULT &&
        GetHeaderFrequency()!=HEADER_FREQUENCY_DEFAULT &&
        GetPageMargin().left!=PAGE_MARGIN_DEFAULT &&
        GetPageMargin().top!=PAGE_MARGIN_DEFAULT &&
        GetPageMargin().right!=PAGE_MARGIN_DEFAULT &&
        GetPageMargin().bottom!=PAGE_MARGIN_DEFAULT) {
        return false;
    }
    else {
        return true;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CFmtReg::CFmtReg
//                               CFmtReg::~CFmtReg
//
/////////////////////////////////////////////////////////////////////////////
CFmtReg::CFmtReg (void)
{
    CUSTOM custom;
    FMT_COLOR color;
    LOGFONT lf;
    lf.lfHeight = -13;     //PointsToTwips(-13);
    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfItalic = FALSE;
    lf.lfUnderline = FALSE;
    lf.lfStrikeOut = FALSE;
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfOutPrecision = OUT_STROKE_PRECIS;
    lf.lfClipPrecision = CLIP_STROKE_PRECIS;
    lf.lfQuality = DRAFT_QUALITY;
    lf.lfPitchAndFamily = FF_SWISS | VARIABLE_PITCH;
    _tcscpy(lf.lfFaceName, _T("Arial"));

    // create and add default tabset format
    CTabSetFmt* pTabSetFmt = new CTabSetFmt();
    pTabSetFmt->SetID(FMT_ID_TABSET);
    m_aFmt.Add(pTabSetFmt);

    // create and add default table format
    {
    CTblFmt* pTblFmt = new CTblFmt();
    pTblFmt->SetID(FMT_ID_TABLE);
    pTblFmt->SetBorderLeft(LINE_NONE);
    pTblFmt->SetBorderTop(LINE_NONE);
    pTblFmt->SetBorderRight(LINE_NONE);
    pTblFmt->SetBorderBottom(LINE_NONE);
    pTblFmt->SetLeadering(LEFT, LEADERING_NONE);
    pTblFmt->SetLeadering(RIGHT, LEADERING_NONE);
    pTblFmt->SetReaderBreak(READER_BREAK_NONE);
    m_aFmt.Add(pTblFmt);
    }

    // create and add default table print format
    {
    CTblPrintFmt* pTblPrintFmt = new CTblPrintFmt();
    pTblPrintFmt->SetID(FMT_ID_TBLPRINT);
    pTblPrintFmt->SetTblLayout(TBL_LAYOUT_LEFT_STANDARD);
    pTblPrintFmt->SetPageBreak(PAGE_BREAK_YES);
    pTblPrintFmt->SetCtrHorz(CENTER_TBL_NO);
    pTblPrintFmt->SetCtrVert(CENTER_TBL_NO);
    pTblPrintFmt->SetStartPage(0);

    // see if we can get better defaults from the currently selected printer ...
    PRINTDLG pd;
    pd.lStructSize=(DWORD)sizeof(PRINTDLG);
    pTblPrintFmt->SetPaperType(PAPER_TYPE_A4);
    pTblPrintFmt->SetPageOrientation(PAGE_ORIENTATION_PORTRAIT);
    pTblPrintFmt->SetPrinterDevice(_T(""));
    pTblPrintFmt->SetPrinterDriver(_T(""));
    pTblPrintFmt->SetPrinterOutput(_T(""));
    if (AfxGetApp() != nullptr && AfxGetApp()->GetPrinterDeviceDefaults(&pd)) {
        DEVMODE FAR* pDevMode=(DEVMODE FAR*)::GlobalLock(pd.hDevMode);

        // paper size ...
        switch(pDevMode->dmPaperSize) {
        case DMPAPER_A4:
            pTblPrintFmt->SetPaperType(PAPER_TYPE_A4);
            break;
        case DMPAPER_LETTER:
            pTblPrintFmt->SetPaperType(PAPER_TYPE_LETTER);
            break;
        case DMPAPER_LEGAL:
            pTblPrintFmt->SetPaperType(PAPER_TYPE_LEGAL);
            break;
        case DMPAPER_A3:
            pTblPrintFmt->SetPaperType(PAPER_TYPE_A3);
            break;
        case DMPAPER_TABLOID:
            pTblPrintFmt->SetPaperType(PAPER_TYPE_TABLOID);
            break;
        default:
            // we'll just use A4
            break;
        }

        // page orientation ...
        switch(pDevMode->dmOrientation) {
        case DMORIENT_PORTRAIT:
            pTblPrintFmt->SetPageOrientation(PAGE_ORIENTATION_PORTRAIT);
            break;
        case DMORIENT_LANDSCAPE:
            pTblPrintFmt->SetPageOrientation(PAGE_ORIENTATION_LANDSCAPE);
            break;
        default:
            // we'll just use portrait
            break;
        }

        DEVNAMES FAR* pDevNames=(DEVNAMES FAR*)::GlobalLock(pd.hDevNames);
        CString sDriver((LPTSTR)pDevNames + pDevNames->wDriverOffset);
        CString sDevice((LPTSTR)pDevNames + pDevNames->wDeviceOffset);
        CString sOutput((LPTSTR)pDevNames + pDevNames->wOutputOffset);
        pTblPrintFmt->SetPrinterDevice(sDevice);
        pTblPrintFmt->SetPrinterDriver(sDriver);
        pTblPrintFmt->SetPrinterOutput(sOutput);

        ::GlobalUnlock(pd.hDevMode);
        ::GlobalUnlock(pd.hDevNames);
    }

    pTblPrintFmt->SetPageMargin(CRect(1440,1440,1440,1440));
    pTblPrintFmt->SetHeaderFrequency(HEADER_FREQUENCY_TOP_PAGE);
    pTblPrintFmt->SetUnits(pTabSetFmt->GetUnits());
    m_aFmt.Add(pTblPrintFmt);
    }

    // create and add default tally format
    {
    CTallyFmt* pTallyFmt = new CTallyFmt();
    pTallyFmt->SetID(FMT_ID_TALLY);
    pTallyFmt->SetInclUndef(INCLUDE_UNDEF_NO);
    pTallyFmt->SetDumpUndef(DUMP_UNDEF_NO);
    m_aFmt.Add(pTallyFmt);
    }

    // create and add default row tally format
    {//Row tally fmt
    CTallyFmt* pTallyFmt = new CTallyFmt();
    pTallyFmt->SetID(FMT_ID_TALLY_ROW);
    pTallyFmt->SetInclUndef(INCLUDE_UNDEF_NO);
    pTallyFmt->SetDumpUndef(DUMP_UNDEF_NO);
//    pTallyFmt->m_sMedianIntervalList.Empty();
//    pTallyFmt->m_iTiles=0;
    m_aFmt.Add(pTallyFmt);
    }
    {//Column tally fmt
    CTallyFmt* pTallyFmt = new CTallyFmt();
    pTallyFmt->SetID(FMT_ID_TALLY_COL);
    pTallyFmt->SetInclUndef(INCLUDE_UNDEF_NO);
    pTallyFmt->SetDumpUndef(DUMP_UNDEF_NO);
    m_aFmt.Add(pTallyFmt);
    }
    // create default spanner format
    {
    CDataCellFmt* pSpannerFmt = new CDataCellFmt();
    pSpannerFmt->SetNumJoinSpanners(0);
    pSpannerFmt->SetID(FMT_ID_SPANNER);
    pSpannerFmt->SetLineLeft(LINE_THIN);
    pSpannerFmt->SetLineTop(LINE_THIN);
    pSpannerFmt->SetLineRight(LINE_NONE);
    pSpannerFmt->SetLineBottom(LINE_THIN);
    pSpannerFmt->SetVertAlign(VALIGN_MID);
    pSpannerFmt->SetHorzAlign(HALIGN_CENTER);
    pSpannerFmt->SetFont(&lf);

    color.m_rgb=rgbBlack;
    color.m_bUseDefault=false;
    pSpannerFmt->SetTextColor(color);
    color.m_rgb=rgbWhite;
    pSpannerFmt->SetFillColor(color);
    pSpannerFmt->SetIndent(LEFT, 0);
    pSpannerFmt->SetIndent(RIGHT, 0);
    pSpannerFmt->SetHidden(HIDDEN_NO);
    pSpannerFmt->SetZeroHidden(false);
    pSpannerFmt->SetSpanCells(SPAN_CELLS_NOT_APPL);
    custom.m_sCustomText.Empty();
    custom.m_bIsCustomized=false;
    pSpannerFmt->SetCustom(custom);
    pSpannerFmt->SetFontExtends(false);
    pSpannerFmt->SetTextColorExtends(false);
    pSpannerFmt->SetFillColorExtends(false);
    pSpannerFmt->SetLinesExtend(true);
    pSpannerFmt->SetIndentationExtends(false);
    pSpannerFmt->SetUnits(pTabSetFmt->GetUnits());
    pSpannerFmt->SetNumDecimals(NUM_DECIMALS_NOTAPPL);
    m_aFmt.Add(pSpannerFmt);
    }

    // create default column head format
    {
    CDataCellFmt* pColHeadFmt = new CDataCellFmt();
    pColHeadFmt->SetID(FMT_ID_COLHEAD);
    pColHeadFmt->SetLineLeft(LINE_THIN);
    pColHeadFmt->SetLineTop(LINE_THIN);
    pColHeadFmt->SetLineRight(LINE_NONE);
    pColHeadFmt->SetLineBottom(LINE_THIN);
    pColHeadFmt->SetVertAlign(VALIGN_BOTTOM);
    pColHeadFmt->SetHorzAlign(HALIGN_RIGHT);
    pColHeadFmt->SetFont(&lf);

    color.m_rgb=rgbBlack;
    color.m_bUseDefault=false;
    pColHeadFmt->SetTextColor(color);
    color.m_rgb=rgbWhite;
    pColHeadFmt->SetFillColor(color);
    pColHeadFmt->SetIndent(LEFT, 0);
    pColHeadFmt->SetIndent(RIGHT, 0);
    pColHeadFmt->SetHidden(HIDDEN_NO);
    pColHeadFmt->SetZeroHidden(false);
    pColHeadFmt->SetSpanCells(SPAN_CELLS_NOT_APPL);
    custom.m_sCustomText.Empty();
    custom.m_bIsCustomized=false;
    pColHeadFmt->SetCustom(custom);
    pColHeadFmt->SetFontExtends(false);
    pColHeadFmt->SetTextColorExtends(false);
    pColHeadFmt->SetFillColorExtends(false);
    pColHeadFmt->SetLinesExtend(false);
    pColHeadFmt->SetIndentationExtends(true);
    pColHeadFmt->SetUnits(pTabSetFmt->GetUnits());
    pColHeadFmt->SetNumDecimals(NUM_DECIMALS_ZERO);
    m_aFmt.Add(pColHeadFmt);
    }

    // create default caption format
    {
    CDataCellFmt* pCaptionFmt = new CDataCellFmt();
    pCaptionFmt->SetID(FMT_ID_CAPTION);
    pCaptionFmt->SetLineLeft(LINE_NONE);
    pCaptionFmt->SetLineTop(LINE_NONE);
    pCaptionFmt->SetLineRight(LINE_NONE);
    pCaptionFmt->SetLineBottom(LINE_NONE);
    pCaptionFmt->SetVertAlign(VALIGN_MID);
    pCaptionFmt->SetHorzAlign(HALIGN_LEFT);
    pCaptionFmt->SetFont(&lf);

    color.m_rgb=rgbBlack;
    color.m_bUseDefault=false;
    pCaptionFmt->SetTextColor(color);
    color.m_rgb=rgbWhite;
    pCaptionFmt->SetFillColor(color);
    pCaptionFmt->SetIndent(LEFT, CAPTION_LEFT_INDENT);
    pCaptionFmt->SetIndent(RIGHT, 0);
    pCaptionFmt->SetHidden(HIDDEN_NO);
    pCaptionFmt->SetZeroHidden(false);
    pCaptionFmt->SetSpanCells(SPAN_CELLS_NO);
    custom.m_sCustomText.Empty();
    custom.m_bIsCustomized=false;
    pCaptionFmt->SetCustom(custom);
    pCaptionFmt->SetFontExtends(false);
    pCaptionFmt->SetTextColorExtends(false);
    pCaptionFmt->SetFillColorExtends(false);
    pCaptionFmt->SetLinesExtend(false);
    pCaptionFmt->SetIndentationExtends(false);
    pCaptionFmt->SetUnits(pTabSetFmt->GetUnits());
    pCaptionFmt->SetNumDecimals(NUM_DECIMALS_NOTAPPL);
    m_aFmt.Add(pCaptionFmt);
    }

    // create default stub format
    {
    CDataCellFmt* pStubFmt = new CDataCellFmt();
    pStubFmt->SetID(FMT_ID_STUB);
    pStubFmt->SetLineLeft(LINE_NONE);
    pStubFmt->SetLineTop(LINE_NONE);
    pStubFmt->SetLineRight(LINE_NONE);
    pStubFmt->SetLineBottom(LINE_NONE);
    pStubFmt->SetVertAlign(VALIGN_BOTTOM);
    pStubFmt->SetHorzAlign(HALIGN_LEFT);
    pStubFmt->SetIndent(LEFT, 200);
    pStubFmt->SetFont(&lf);

    color.m_rgb=rgbBlack;
    color.m_bUseDefault=false;
    pStubFmt->SetTextColor(color);
    color.m_rgb=rgbWhite;
    pStubFmt->SetFillColor(color);
    pStubFmt->SetIndent(LEFT, 0);
    pStubFmt->SetIndent(RIGHT, 0);
    pStubFmt->SetHidden(HIDDEN_NO);
    pStubFmt->SetZeroHidden(false);
    pStubFmt->SetSpanCells(SPAN_CELLS_NOT_APPL);
    custom.m_sCustomText.Empty();
    custom.m_bIsCustomized=false;
    pStubFmt->SetCustom(custom);
    pStubFmt->SetFontExtends(true);
    pStubFmt->SetTextColorExtends(false);
    pStubFmt->SetFillColorExtends(false);
    pStubFmt->SetLinesExtend(false);
    pStubFmt->SetIndentationExtends(false);
    pStubFmt->SetUnits(pTabSetFmt->GetUnits());
    pStubFmt->SetUnits(pTabSetFmt->GetUnits());
    pStubFmt->SetNumDecimals(NUM_DECIMALS_ZERO);
    m_aFmt.Add(pStubFmt);
    }

    // create and add default data cell format
    {
    CDataCellFmt* pDataCellFmt = new CDataCellFmt();
    pDataCellFmt->SetID(FMT_ID_DATACELL);
    pDataCellFmt->SetLineLeft(LINE_NOT_APPL);
    pDataCellFmt->SetLineTop(LINE_NOT_APPL);
    pDataCellFmt->SetLineRight(LINE_NOT_APPL);
    pDataCellFmt->SetLineBottom(LINE_NOT_APPL);
    pDataCellFmt->SetVertAlign(VALIGN_BOTTOM);
    pDataCellFmt->SetHorzAlign(HALIGN_RIGHT);
    pDataCellFmt->SetFont(&lf);

    color.m_rgb=rgbBlack;
    color.m_bUseDefault=false;
    pDataCellFmt->SetTextColor(color);
    color.m_rgb=rgbWhite;
    pDataCellFmt->SetFillColor(color);
    pDataCellFmt->SetIndent(LEFT, 0);
    pDataCellFmt->SetIndent(RIGHT, 0);
    pDataCellFmt->SetHidden(HIDDEN_NOT_APPL);
    pDataCellFmt->SetZeroHidden(false);
    pDataCellFmt->SetSpanCells(SPAN_CELLS_NOT_APPL);
    custom.m_sCustomText.Empty();
    custom.m_bIsCustomized=false;
    pDataCellFmt->SetCustom(custom);
    pDataCellFmt->SetFontExtends(false);
    pDataCellFmt->SetTextColorExtends(false);
    pDataCellFmt->SetFillColorExtends(false);
    pDataCellFmt->SetLinesExtend(false);
    pDataCellFmt->SetIndentationExtends(false);
    pDataCellFmt->SetNumDecimals(NUM_DECIMALS_ZERO);
    pDataCellFmt->SetUnits(pTabSetFmt->GetUnits());
    m_aFmt.Add(pDataCellFmt);
    }

    // create and add default title format
    {
    CFmt* pTitleFmt = new CFmt();
    pTitleFmt->SetID(FMT_ID_TITLE);
    pTitleFmt->SetLineLeft(LINE_NONE);
    pTitleFmt->SetLineTop(LINE_NONE);
    pTitleFmt->SetLineRight(LINE_NONE);
    pTitleFmt->SetLineBottom(LINE_NONE);
    pTitleFmt->SetVertAlign(VALIGN_TOP);
    pTitleFmt->SetHorzAlign(HALIGN_LEFT);

    LOGFONT lfBold;
    lfBold = lf;
    lfBold.lfHeight = -16;     //PointsToTwips(-13);
    lfBold.lfWeight = FW_BOLD;
    pTitleFmt->SetFont(&lfBold);

    color.m_rgb=rgbBlack;
    color.m_bUseDefault=false;
    pTitleFmt->SetTextColor(color);
    color.m_rgb=rgbWhite;
    pTitleFmt->SetFillColor(color);
    pTitleFmt->SetIndent(LEFT, 0);
    pTitleFmt->SetIndent(RIGHT, 0);
    pTitleFmt->SetHidden(HIDDEN_NO);
    pTitleFmt->SetSpanCells(SPAN_CELLS_NOT_APPL);
    custom.m_sCustomText.Empty();
    custom.m_bIsCustomized=false;
    pTitleFmt->SetCustom(custom);
    pTitleFmt->SetFontExtends(false);
    pTitleFmt->SetTextColorExtends(false);
    pTitleFmt->SetFillColorExtends(false);
    pTitleFmt->SetLinesExtend(false);
    pTitleFmt->SetIndentationExtends(false);
    pTitleFmt->SetUnits(pTabSetFmt->GetUnits());
    m_aFmt.Add(pTitleFmt);
    }

    // create and add default subtitle format
    {
    CFmt* pSubTitleFmt = new CFmt();
    pSubTitleFmt->SetID(FMT_ID_SUBTITLE);
    pSubTitleFmt->SetLineLeft(LINE_NONE);
    pSubTitleFmt->SetLineTop(LINE_NONE);
    pSubTitleFmt->SetLineRight(LINE_NONE);
    pSubTitleFmt->SetLineBottom(LINE_NONE);
    pSubTitleFmt->SetVertAlign(VALIGN_TOP);
    pSubTitleFmt->SetHorzAlign(HALIGN_LEFT);
    pSubTitleFmt->SetFont(&lf);

    color.m_rgb=rgbBlack;
    color.m_bUseDefault=false;
    pSubTitleFmt->SetTextColor(color);
    color.m_rgb=rgbWhite;
    pSubTitleFmt->SetFillColor(color);
    pSubTitleFmt->SetIndent(LEFT, 0);
    pSubTitleFmt->SetIndent(RIGHT, 0);
    pSubTitleFmt->SetHidden(HIDDEN_NO);
    pSubTitleFmt->SetSpanCells(SPAN_CELLS_NOT_APPL);
    custom.m_sCustomText.Empty();
    custom.m_bIsCustomized=false;
    pSubTitleFmt->SetCustom(custom);
    pSubTitleFmt->SetFontExtends(false);
    pSubTitleFmt->SetTextColorExtends(false);
    pSubTitleFmt->SetFillColorExtends(false);
    pSubTitleFmt->SetLinesExtend(false);
    pSubTitleFmt->SetIndentationExtends(false);
    pSubTitleFmt->SetUnits(pTabSetFmt->GetUnits());
    m_aFmt.Add(pSubTitleFmt);
    }

    // create and add default stubhead format
    {
    CFmt* pStubHeadFmt = new CFmt();
    pStubHeadFmt->SetID(FMT_ID_STUBHEAD);
    pStubHeadFmt->SetLineLeft(LINE_NONE);
    pStubHeadFmt->SetLineTop(LINE_THIN);
    pStubHeadFmt->SetLineRight(LINE_THIN);
    pStubHeadFmt->SetLineBottom(LINE_THIN);
    pStubHeadFmt->SetVertAlign(VALIGN_MID);
    pStubHeadFmt->SetHorzAlign(HALIGN_LEFT);
    pStubHeadFmt->SetFont(&lf);

    color.m_rgb=rgbBlack;
    color.m_bUseDefault=false;
    pStubHeadFmt->SetTextColor(color);
    color.m_rgb=rgbWhite;
    pStubHeadFmt->SetFillColor(color);
    pStubHeadFmt->SetIndent(LEFT, 0);
    pStubHeadFmt->SetIndent(RIGHT, 0);
    pStubHeadFmt->SetHidden(HIDDEN_NO);
    pStubHeadFmt->SetSpanCells(SPAN_CELLS_NOT_APPL);
    custom.m_sCustomText.Empty();
    custom.m_bIsCustomized=false;
    pStubHeadFmt->SetCustom(custom);
    pStubHeadFmt->SetFontExtends(false);
    pStubHeadFmt->SetTextColorExtends(false);
    pStubHeadFmt->SetFillColorExtends(false);
    pStubHeadFmt->SetLinesExtend(true);
    pStubHeadFmt->SetIndentationExtends(false);
    pStubHeadFmt->SetUnits(pTabSetFmt->GetUnits());
    m_aFmt.Add(pStubHeadFmt);
    }

    // create and add default secondary stubhead format
    {
    CFmt* pStubHeadSecFmt = new CFmt();
    pStubHeadSecFmt->SetID(FMT_ID_STUBHEAD_SEC);
    pStubHeadSecFmt->SetLineLeft(LINE_THIN);
    pStubHeadSecFmt->SetLineTop(LINE_THIN);
    pStubHeadSecFmt->SetLineRight(LINE_THIN);
    pStubHeadSecFmt->SetLineBottom(LINE_THIN);
    pStubHeadSecFmt->SetVertAlign(VALIGN_MID);
    pStubHeadSecFmt->SetHorzAlign(HALIGN_RIGHT);
    pStubHeadSecFmt->SetFont(&lf);

    color.m_rgb=rgbBlack;
    color.m_bUseDefault=false;
    pStubHeadSecFmt->SetTextColor(color);
    color.m_rgb=rgbWhite;
    pStubHeadSecFmt->SetFillColor(color);
    pStubHeadSecFmt->SetIndent(LEFT, 0);
    pStubHeadSecFmt->SetIndent(RIGHT, 0);
    pStubHeadSecFmt->SetHidden(HIDDEN_NO);
    pStubHeadSecFmt->SetSpanCells(SPAN_CELLS_NOT_APPL);
    custom.m_sCustomText.Empty();
    custom.m_bIsCustomized=false;
    pStubHeadSecFmt->SetCustom(custom);
    pStubHeadSecFmt->SetFontExtends(false);
    pStubHeadSecFmt->SetTextColorExtends(false);
    pStubHeadSecFmt->SetFillColorExtends(false);
    pStubHeadSecFmt->SetLinesExtend(true);
    pStubHeadSecFmt->SetIndentationExtends(false);
    pStubHeadSecFmt->SetUnits(pTabSetFmt->GetUnits());
    m_aFmt.Add(pStubHeadSecFmt);
    }

    // create and add default header format (left)
    {
    CFmt* pHeaderLeftFmt = new CFmt();
    pHeaderLeftFmt->SetID(FMT_ID_HEADER_LEFT);
    pHeaderLeftFmt->SetVertAlign(VALIGN_TOP);
    pHeaderLeftFmt->SetHorzAlign(HALIGN_LEFT);
    pHeaderLeftFmt->SetFont(&lf);

    pHeaderLeftFmt->SetLineLeft(LINE_NONE);
    pHeaderLeftFmt->SetLineTop(LINE_NONE);
    pHeaderLeftFmt->SetLineRight(LINE_NONE);
    pHeaderLeftFmt->SetLineBottom(LINE_NONE);
    color.m_rgb=rgbBlack;
    color.m_bUseDefault=false;
    pHeaderLeftFmt->SetTextColor(color);
    color.m_rgb=rgbWhite;
    pHeaderLeftFmt->SetFillColor(color);
    pHeaderLeftFmt->SetIndent(LEFT, 0);
    pHeaderLeftFmt->SetIndent(RIGHT, 0);
    pHeaderLeftFmt->SetHidden(HIDDEN_NO);
    pHeaderLeftFmt->SetSpanCells(SPAN_CELLS_NOT_APPL);
    custom.m_sCustomText = _T("&F");
    custom.m_bIsCustomized=true;
    pHeaderLeftFmt->SetCustom(custom);
    pHeaderLeftFmt->SetFontExtends(false);
    pHeaderLeftFmt->SetTextColorExtends(false);
    pHeaderLeftFmt->SetFillColorExtends(false);
    pHeaderLeftFmt->SetLinesExtend(false);
    pHeaderLeftFmt->SetIndentationExtends(false);
    pHeaderLeftFmt->SetUnits(pTabSetFmt->GetUnits());
    m_aFmt.Add(pHeaderLeftFmt);
    }

    // create and add default header format (center)
    {
    CFmt* pHeaderCenterFmt = new CFmt();
    pHeaderCenterFmt->SetID(FMT_ID_HEADER_CENTER);
    pHeaderCenterFmt->SetVertAlign(VALIGN_TOP);
    pHeaderCenterFmt->SetHorzAlign(HALIGN_CENTER);
    pHeaderCenterFmt->SetFont(&lf);
    pHeaderCenterFmt->SetLineLeft(LINE_NONE);
    pHeaderCenterFmt->SetLineTop(LINE_NONE);
    pHeaderCenterFmt->SetLineRight(LINE_NONE);
    pHeaderCenterFmt->SetLineBottom(LINE_NONE);
    color.m_rgb=rgbBlack;
    color.m_bUseDefault=false;
    pHeaderCenterFmt->SetTextColor(color);
    color.m_rgb=rgbWhite;
    pHeaderCenterFmt->SetFillColor(color);
    pHeaderCenterFmt->SetIndent(LEFT, 0);
    pHeaderCenterFmt->SetIndent(RIGHT, 0);
    pHeaderCenterFmt->SetHidden(HIDDEN_NO);
    pHeaderCenterFmt->SetSpanCells(SPAN_CELLS_NOT_APPL);
    custom.m_sCustomText.Empty();
    custom.m_bIsCustomized=true;
    pHeaderCenterFmt->SetCustom(custom);
    pHeaderCenterFmt->SetFontExtends(false);
    pHeaderCenterFmt->SetTextColorExtends(false);
    pHeaderCenterFmt->SetFillColorExtends(false);
    pHeaderCenterFmt->SetLinesExtend(false);
    pHeaderCenterFmt->SetIndentationExtends(false);
    pHeaderCenterFmt->SetUnits(pTabSetFmt->GetUnits());
    m_aFmt.Add(pHeaderCenterFmt);
    }

    // create and add default header format (right)
    {
    CFmt* pHeaderRightFmt = new CFmt();
    pHeaderRightFmt->SetID(FMT_ID_HEADER_RIGHT);
    pHeaderRightFmt->SetVertAlign(VALIGN_TOP);
    pHeaderRightFmt->SetHorzAlign(HALIGN_RIGHT);
    pHeaderRightFmt->SetFont(&lf);
    pHeaderRightFmt->SetLineLeft(LINE_NONE);
    pHeaderRightFmt->SetLineTop(LINE_NONE);
    pHeaderRightFmt->SetLineRight(LINE_NONE);
    pHeaderRightFmt->SetLineBottom(LINE_NONE);
    color.m_rgb=rgbBlack;
    color.m_bUseDefault=false;
    pHeaderRightFmt->SetTextColor(color);
    color.m_rgb=rgbWhite;
    pHeaderRightFmt->SetFillColor(color);
    pHeaderRightFmt->SetIndent(LEFT, 0);
    pHeaderRightFmt->SetIndent(RIGHT, 0);
    pHeaderRightFmt->SetHidden(HIDDEN_NO);
    pHeaderRightFmt->SetSpanCells(SPAN_CELLS_NOT_APPL);
    custom.m_sCustomText = _T("&D");
    custom.m_bIsCustomized=true;
    pHeaderRightFmt->SetCustom(custom);
    pHeaderRightFmt->SetFontExtends(false);
    pHeaderRightFmt->SetTextColorExtends(false);
    pHeaderRightFmt->SetFillColorExtends(false);
    pHeaderRightFmt->SetLinesExtend(false);
    pHeaderRightFmt->SetIndentationExtends(false);
    pHeaderRightFmt->SetUnits(pTabSetFmt->GetUnits());
    m_aFmt.Add(pHeaderRightFmt);
    }

    // create and add default footer format (left)
    {
    CFmt* pFooterLeftFmt = new CFmt();
    pFooterLeftFmt->SetID(FMT_ID_FOOTER_LEFT);
    pFooterLeftFmt->SetVertAlign(VALIGN_BOTTOM);
    pFooterLeftFmt->SetHorzAlign(HALIGN_LEFT);
    pFooterLeftFmt->SetFont(&lf);
    pFooterLeftFmt->SetLineLeft(LINE_NONE);
    pFooterLeftFmt->SetLineTop(LINE_NONE);
    pFooterLeftFmt->SetLineRight(LINE_NONE);
    pFooterLeftFmt->SetLineBottom(LINE_NONE);
    color.m_rgb=rgbBlack;
    color.m_bUseDefault=false;
    pFooterLeftFmt->SetTextColor(color);
    color.m_rgb=rgbWhite;
    pFooterLeftFmt->SetFillColor(color);
    pFooterLeftFmt->SetIndent(LEFT, 0);
    pFooterLeftFmt->SetIndent(RIGHT, 0);
    pFooterLeftFmt->SetHidden(HIDDEN_NO);
    pFooterLeftFmt->SetSpanCells(SPAN_CELLS_NOT_APPL);
    custom.m_sCustomText.Empty();
    custom.m_bIsCustomized=true;
    pFooterLeftFmt->SetCustom(custom);
    pFooterLeftFmt->SetFontExtends(false);
    pFooterLeftFmt->SetTextColorExtends(false);
    pFooterLeftFmt->SetFillColorExtends(false);
    pFooterLeftFmt->SetLinesExtend(false);
    pFooterLeftFmt->SetIndentationExtends(false);
    pFooterLeftFmt->SetUnits(pTabSetFmt->GetUnits());
    m_aFmt.Add(pFooterLeftFmt);
    }

    // create and add default footer format (center)
    {
    CFmt* pFooterCenterFmt = new CFmt();
    pFooterCenterFmt->SetID(FMT_ID_FOOTER_CENTER);
    pFooterCenterFmt->SetVertAlign(VALIGN_BOTTOM);
    pFooterCenterFmt->SetHorzAlign(HALIGN_CENTER);
    pFooterCenterFmt->SetFont(&lf);
    pFooterCenterFmt->SetLineLeft(LINE_NONE);
    pFooterCenterFmt->SetLineTop(LINE_NONE);
    pFooterCenterFmt->SetLineRight(LINE_NONE);
    pFooterCenterFmt->SetLineBottom(LINE_NONE);
    color.m_rgb=rgbBlack;
    color.m_bUseDefault=false;
    pFooterCenterFmt->SetTextColor(color);
    color.m_rgb=rgbWhite;
    pFooterCenterFmt->SetFillColor(color);
    pFooterCenterFmt->SetIndent(LEFT, 0);
    pFooterCenterFmt->SetIndent(RIGHT, 0);
    pFooterCenterFmt->SetHidden(HIDDEN_NO);
    pFooterCenterFmt->SetSpanCells(SPAN_CELLS_NOT_APPL);
    custom.m_sCustomText = _T("- &P -");
    custom.m_bIsCustomized=true;
    pFooterCenterFmt->SetCustom(custom);
    pFooterCenterFmt->SetFontExtends(false);
    pFooterCenterFmt->SetTextColorExtends(false);
    pFooterCenterFmt->SetFillColorExtends(false);
    pFooterCenterFmt->SetLinesExtend(false);
    pFooterCenterFmt->SetIndentationExtends(false);
    pFooterCenterFmt->SetUnits(pTabSetFmt->GetUnits());
    m_aFmt.Add(pFooterCenterFmt);
    }

    // create and add default footer format (right)
    {
    CFmt* pFooterRightFmt = new CFmt();
    pFooterRightFmt->SetID(FMT_ID_FOOTER_RIGHT);
    pFooterRightFmt->SetVertAlign(VALIGN_BOTTOM);
    pFooterRightFmt->SetHorzAlign(HALIGN_RIGHT);
    pFooterRightFmt->SetFont(&lf);
    pFooterRightFmt->SetLineLeft(LINE_NONE);
    pFooterRightFmt->SetLineTop(LINE_NONE);
    pFooterRightFmt->SetLineRight(LINE_NONE);
    pFooterRightFmt->SetLineBottom(LINE_NONE);
    color.m_rgb=rgbBlack;
    color.m_bUseDefault=false;
    pFooterRightFmt->SetTextColor(color);
    color.m_rgb=rgbWhite;
    pFooterRightFmt->SetFillColor(color);
    pFooterRightFmt->SetIndent(LEFT, 0);
    pFooterRightFmt->SetIndent(RIGHT, 0);
    pFooterRightFmt->SetHidden(HIDDEN_NO);
    pFooterRightFmt->SetSpanCells(SPAN_CELLS_NOT_APPL);
    custom.m_sCustomText.Empty();
    custom.m_bIsCustomized=true;
    pFooterRightFmt->SetCustom(custom);
    pFooterRightFmt->SetFontExtends(false);
    pFooterRightFmt->SetTextColorExtends(false);
    pFooterRightFmt->SetFillColorExtends(false);
    pFooterRightFmt->SetLinesExtend(false);
    pFooterRightFmt->SetIndentationExtends(false);
    pFooterRightFmt->SetUnits(pTabSetFmt->GetUnits());
    m_aFmt.Add(pFooterRightFmt);
    }

    // create and add default page note format
    {
    CFmt* pPageNoteFmt = new CFmt();
    pPageNoteFmt->SetID(FMT_ID_PAGENOTE);
    pPageNoteFmt->SetLineLeft(LINE_NONE);
    pPageNoteFmt->SetLineTop(LINE_NONE);
    pPageNoteFmt->SetLineRight(LINE_NONE);
    pPageNoteFmt->SetLineBottom(LINE_NONE);
    pPageNoteFmt->SetVertAlign(VALIGN_TOP);
    pPageNoteFmt->SetHorzAlign(HALIGN_LEFT);
    pPageNoteFmt->SetFont(&lf);
    color.m_rgb=rgbBlack;
    color.m_bUseDefault=false;
    pPageNoteFmt->SetTextColor(color);
    color.m_rgb=rgbWhite;
    pPageNoteFmt->SetFillColor(color);
    pPageNoteFmt->SetIndent(LEFT, 0);
    pPageNoteFmt->SetIndent(RIGHT, 0);
    pPageNoteFmt->SetHidden(HIDDEN_NO);
    pPageNoteFmt->SetSpanCells(SPAN_CELLS_NOT_APPL);
    custom.m_sCustomText.Empty();
    custom.m_bIsCustomized=false;
    pPageNoteFmt->SetCustom(custom);
    pPageNoteFmt->SetFontExtends(false);
    pPageNoteFmt->SetTextColorExtends(false);
    pPageNoteFmt->SetFillColorExtends(false);
    pPageNoteFmt->SetLinesExtend(false);
    pPageNoteFmt->SetIndentationExtends(false);
    pPageNoteFmt->SetUnits(pTabSetFmt->GetUnits());
    m_aFmt.Add(pPageNoteFmt);
    }

    // create and add default end note format
    {
    CFmt* pEndNoteFmt = new CFmt();
    pEndNoteFmt->SetID(FMT_ID_ENDNOTE);
    pEndNoteFmt->SetLineLeft(LINE_NONE);
    pEndNoteFmt->SetLineTop(LINE_NONE);
    pEndNoteFmt->SetLineRight(LINE_NONE);
    pEndNoteFmt->SetLineBottom(LINE_NONE);
    pEndNoteFmt->SetVertAlign(VALIGN_TOP);
    pEndNoteFmt->SetHorzAlign(HALIGN_LEFT);
    pEndNoteFmt->SetFont(&lf);

    color.m_rgb=rgbBlack;
    color.m_bUseDefault=false;
    pEndNoteFmt->SetTextColor(color);
    color.m_rgb=rgbWhite;
    pEndNoteFmt->SetFillColor(color);
    pEndNoteFmt->SetIndent(LEFT, 0);
    pEndNoteFmt->SetIndent(RIGHT, 0);
    pEndNoteFmt->SetHidden(HIDDEN_NO);
    pEndNoteFmt->SetSpanCells(SPAN_CELLS_NOT_APPL);
    custom.m_sCustomText.Empty();
    custom.m_bIsCustomized=false;
    pEndNoteFmt->SetCustom(custom);
    pEndNoteFmt->SetFontExtends(false);
    pEndNoteFmt->SetTextColorExtends(false);
    pEndNoteFmt->SetFillColorExtends(false);
    pEndNoteFmt->SetLinesExtend(false);
    pEndNoteFmt->SetIndentationExtends(false);
    pEndNoteFmt->SetUnits(pTabSetFmt->GetUnits());
    m_aFmt.Add(pEndNoteFmt);
    }

    // create and add default area caption
    {
    CFmt* pAreaCaptionFmt = new CFmt();
    pAreaCaptionFmt->SetID(FMT_ID_AREA_CAPTION);
    pAreaCaptionFmt->SetLineLeft(LINE_NONE);
    pAreaCaptionFmt->SetLineTop(LINE_NONE);
    pAreaCaptionFmt->SetLineRight(LINE_NONE);
    pAreaCaptionFmt->SetLineBottom(LINE_NONE);
    pAreaCaptionFmt->SetVertAlign(VALIGN_BOTTOM);
    pAreaCaptionFmt->SetHorzAlign(HALIGN_LEFT);
    pAreaCaptionFmt->SetFont(&lf);
    color.m_rgb=rgbBlack;
    color.m_bUseDefault=false;
    pAreaCaptionFmt->SetTextColor(color);
    color.m_rgb=rgbWhite;
    pAreaCaptionFmt->SetFillColor(color);
    pAreaCaptionFmt->SetIndent(LEFT, 0);
    pAreaCaptionFmt->SetIndent(RIGHT, 0);
    pAreaCaptionFmt->SetHidden(HIDDEN_NO);
    pAreaCaptionFmt->SetSpanCells(SPAN_CELLS_NO);
    custom.m_sCustomText.Empty();
    custom.m_bIsCustomized=false;
    pAreaCaptionFmt->SetCustom(custom);
    pAreaCaptionFmt->SetFontExtends(false);
    pAreaCaptionFmt->SetTextColorExtends(false);
    pAreaCaptionFmt->SetFillColorExtends(false);
    pAreaCaptionFmt->SetLinesExtend(true);
    pAreaCaptionFmt->SetIndentationExtends(false);
    pAreaCaptionFmt->SetUnits(pTabSetFmt->GetUnits());
    m_aFmt.Add(pAreaCaptionFmt);
    }
}


/*V*/ CFmtReg::~CFmtReg (void)
{
    RemoveAll();
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CFmtReg::GetFmt
//
//  returns NULL if not found
//
/////////////////////////////////////////////////////////////////////////////
const CFmtBase* CFmtReg::GetFmt(FMT_ID ID, int iIndex /*=0*/) const
{
    for (int i=0 ; i<m_aFmt.GetSize() ; i++)  {
        CFmtBase* pFmtBase=m_aFmt[i];
        if (ID==pFmtBase->GetID() && iIndex==pFmtBase->GetIndex()) {
            return pFmtBase;
        }
    }
    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CFmtReg::GetFmt
//
//  returns NULL if not found
//
/////////////////////////////////////////////////////////////////////////////
CFmtBase* CFmtReg::GetFmt(FMT_ID ID, int iIndex /*=0*/)
{
    for (int i=0 ; i<m_aFmt.GetSize() ; i++)  {
        CFmtBase* pFmtBase=m_aFmt[i];
        if (ID==pFmtBase->GetID() && iIndex==pFmtBase->GetIndex()) {
            return pFmtBase;
        }
    }
    return NULL;
}

const CFmtBase* CFmtReg::GetFmt(const CIMSAString& sID, const CIMSAString& sIndex) const
{
    int iIndex;
    FMT_ID ID=FMT_ID_INVALID;
    CIMSAString s;

    ASSERT(sIndex.IsNumeric());
    s=sID;
    s.Trim();
    iIndex=(int)sIndex.Val();
    if (s==TFT_FMT_ID_SPANNER) {
        ID=FMT_ID_SPANNER;
    }
    else if (s==TFT_FMT_ID_COLHEAD) {
        ID=FMT_ID_COLHEAD;
    }
    else if (s==TFT_FMT_ID_CAPTION) {
        ID=FMT_ID_CAPTION;
    }
    else if (s==TFT_FMT_ID_STUB) {
        ID=FMT_ID_STUB;
    }
    else if (s==TFT_FMT_ID_DATACELL) {
        ID=FMT_ID_DATACELL;
    }
    else if (s==TFT_FMT_ID_HEADER_LEFT) {
        ID=FMT_ID_HEADER_LEFT;
    }
    else if (s==TFT_FMT_ID_HEADER_CENTER) {
        ID=FMT_ID_HEADER_CENTER;
    }
    else if (s==TFT_FMT_ID_HEADER_RIGHT) {
        ID=FMT_ID_HEADER_RIGHT;
    }
    else if (s==TFT_FMT_ID_FOOTER_LEFT) {
        ID=FMT_ID_FOOTER_LEFT;
    }
    else if (s==TFT_FMT_ID_FOOTER_CENTER) {
        ID=FMT_ID_FOOTER_CENTER;
    }
    else if (s==TFT_FMT_ID_FOOTER_RIGHT) {
        ID=FMT_ID_FOOTER_RIGHT;
    }
    else if (s==TFT_FMT_ID_TITLE) {
        ID=FMT_ID_TITLE;
    }
    else if (s==TFT_FMT_ID_SUBTITLE) {
        ID=FMT_ID_SUBTITLE;
    }
    else if (s==TFT_FMT_ID_PAGENOTE) {
        ID=FMT_ID_PAGENOTE;
    }
    else if (s==TFT_FMT_ID_ENDNOTE) {
        ID=FMT_ID_ENDNOTE;
    }
    else if (s==TFT_FMT_ID_STUBHEAD) {
        ID=FMT_ID_STUBHEAD;
    }
    else if (s==TFT_FMT_ID_STUBHEAD_SEC) {
        ID=FMT_ID_STUBHEAD_SEC;
    }
    else if (s==TFT_FMT_ID_AREA_CAPTION) {
        ID=FMT_ID_AREA_CAPTION;
    }
    else if (s==TFT_FMT_ID_TALLY) {
        ID=FMT_ID_TALLY;
    }
    else if (s==TFT_FMT_ID_TALLY_ROW) {
        ID=FMT_ID_TALLY_ROW;
    }
    else if (s==TFT_FMT_ID_TALLY_COL) {
        ID=FMT_ID_TALLY_COL;
    }
    else if (s==TFT_FMT_ID_TABSET) {
        ID=FMT_ID_TABSET;
    }
    else if (s==TFT_FMT_ID_TABLE) {
        ID=FMT_ID_TABLE;
    }
    else if (s==TFT_FMT_ID_TBLPRINT) {
        ID=FMT_ID_TBLPRINT;
    }
    else {
        ASSERT(FALSE);
    }
    return GetFmt(ID, iIndex);
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CFmtReg::AddFmt
//
/////////////////////////////////////////////////////////////////////////////
bool CFmtReg::AddFmt(CFmtBase* pFmtBase)
{
    // detects duplicate formats (ID+index)
    CFmtBase* pOldFmt=NULL;

    // debug checks ... default fmts cannot have any "default" settings
    if (pFmtBase->GetIndex()==0) {
        bool bFound=false;
        if (pFmtBase->IsKindOf(RUNTIME_CLASS(CFmt))) {
            CFmt* pFmt=DYNAMIC_DOWNCAST(CFmt, pFmtBase);
            ASSERT(pFmt->GetFont()!=NULL);
            ASSERT(pFmt->GetHorzAlign()!=HALIGN_DEFAULT);
            ASSERT(pFmt->GetVertAlign()!=VALIGN_DEFAULT);
            ASSERT(!pFmt->GetTextColor().m_bUseDefault);
            ASSERT(!pFmt->GetFillColor().m_bUseDefault);
            ASSERT(pFmt->GetIndent(LEFT)!=INDENT_DEFAULT);
            ASSERT(pFmt->GetIndent(RIGHT)!=INDENT_DEFAULT);
            ASSERT(pFmt->GetLineLeft()!=LINE_DEFAULT);
            ASSERT(pFmt->GetLineRight()!=LINE_DEFAULT);
            ASSERT(pFmt->GetLineTop()!=LINE_DEFAULT);
            ASSERT(pFmt->GetLineBottom()!=LINE_DEFAULT);
            ASSERT(pFmt->GetHidden()!=HIDDEN_DEFAULT);
            ASSERT(pFmt->GetSpanCells()!=SPAN_CELLS_DEFAULT);
            // ASSERT(!pFmt->GetCustom().m_bIsCustomized); Can now have custom fmt text for headers & footers JH 4/14/05
            bFound=true;
        }
        if (pFmtBase->IsKindOf(RUNTIME_CLASS(CDataCellFmt))) {
            CDataCellFmt* pDataCellFmt=DYNAMIC_DOWNCAST(CDataCellFmt, pFmtBase);
            ASSERT(pDataCellFmt->GetNumDecimals()!=NUM_DECIMALS_DEFAULT);
            bFound=true;
        }
        if (pFmtBase->IsKindOf(RUNTIME_CLASS(CTallyFmt))) {
            CTallyFmt* pTallyFmt=DYNAMIC_DOWNCAST(CTallyFmt, pFmtBase);
            ASSERT(pTallyFmt->GetInclUndef()!=INCLUDE_UNDEF_DEFAULT);
            ASSERT(pTallyFmt->GetDumpUndef()!=DUMP_UNDEF_DEFAULT);
            bFound=true;
        }
        if (pFmtBase->IsKindOf(RUNTIME_CLASS(CTblFmt))) {
            CTblFmt* pTblFmt=DYNAMIC_DOWNCAST(CTblFmt, pFmtBase);
            ASSERT(pTblFmt->GetBorderLeft()!=LINE_DEFAULT);
            ASSERT(pTblFmt->GetBorderRight()!=LINE_DEFAULT);
            ASSERT(pTblFmt->GetBorderTop()!=LINE_DEFAULT);
            ASSERT(pTblFmt->GetBorderBottom()!=LINE_DEFAULT);
            ASSERT(pTblFmt->GetLeadering(LEFT)!=LEADERING_DEFAULT);
            ASSERT(pTblFmt->GetLeadering(RIGHT)!=LEADERING_DEFAULT);
            ASSERT(pTblFmt->GetReaderBreak()!=READER_BREAK_DEFAULT);
            bFound=true;
        }
        if (pFmtBase->IsKindOf(RUNTIME_CLASS(CTabSetFmt))) {
            // no defaults are possible for thousands sep, decimal sep, digit grouping, etc.
            bFound=true;
        }
        if (pFmtBase->IsKindOf(RUNTIME_CLASS(CTblPrintFmt))) {
            CTblPrintFmt* pTblPrintFmt=DYNAMIC_DOWNCAST(CTblPrintFmt, pFmtBase);
            ASSERT(pTblPrintFmt->GetTblLayout()!=TBL_LAYOUT_DEFAULT);
            ASSERT(pTblPrintFmt->GetPageBreak()!=PAGE_BREAK_DEFAULT);
            ASSERT(pTblPrintFmt->GetCtrHorz()!=CENTER_TBL_DEFAULT);
            ASSERT(pTblPrintFmt->GetCtrVert()!=CENTER_TBL_DEFAULT);
            //ASSERT(pTblPrintFmt->GetStartPage()!=START_PAGE_DEFAULT);  start page CAN be default, even after copy non-default!!
            ASSERT(pTblPrintFmt->GetPaperType()!=PAPER_TYPE_DEFAULT);
            ASSERT(pTblPrintFmt->GetPageOrientation()!=PAGE_ORIENTATION_DEFAULT);
            ASSERT(pTblPrintFmt->GetPageMargin()!=CRect(PAGE_MARGIN_DEFAULT,PAGE_MARGIN_DEFAULT,PAGE_MARGIN_DEFAULT,PAGE_MARGIN_DEFAULT));
            ASSERT(pTblPrintFmt->GetHeaderFrequency()!=HEADER_FREQUENCY_DEFAULT);
            bFound=true;
        }
        if (!bFound) {
            // huh? unknown kind of fmt
            ASSERT(FALSE);
        }
    }

    // make sure that runtime type information matches the format ID ...
    if (pFmtBase->IsKindOf(RUNTIME_CLASS(CTallyFmt))) {
        ASSERT(pFmtBase->GetID()==FMT_ID_TALLY ||pFmtBase->GetID()==FMT_ID_TALLY_ROW || pFmtBase->GetID()==FMT_ID_TALLY_COL );
    }
    else if (pFmtBase->IsKindOf(RUNTIME_CLASS(CTblFmt))) {
        ASSERT(pFmtBase->GetID()==FMT_ID_TABLE);
    }
    else if (pFmtBase->IsKindOf(RUNTIME_CLASS(CTabSetFmt))) {
        ASSERT(pFmtBase->GetID()==FMT_ID_TABSET);
    }
    else if (pFmtBase->IsKindOf(RUNTIME_CLASS(CTblPrintFmt))) {
        ASSERT(pFmtBase->GetID()==FMT_ID_TBLPRINT);
    }
    else if (pFmtBase->IsKindOf(RUNTIME_CLASS(CDataCellFmt))) {
        ASSERT(pFmtBase->GetID()==FMT_ID_DATACELL ||
            pFmtBase->GetID()==FMT_ID_COLHEAD ||
            pFmtBase->GetID()==FMT_ID_STUB ||
            pFmtBase->GetID()==FMT_ID_CAPTION ||
            pFmtBase->GetID()==FMT_ID_SPANNER);
    }
    else if (pFmtBase->IsKindOf(RUNTIME_CLASS(CFmt))) {
        ASSERT(pFmtBase->GetID()==FMT_ID_SPANNER ||
            pFmtBase->GetID()==FMT_ID_CAPTION ||
            pFmtBase->GetID()==FMT_ID_HEADER_LEFT ||
            pFmtBase->GetID()==FMT_ID_HEADER_CENTER ||
            pFmtBase->GetID()==FMT_ID_HEADER_RIGHT ||
            pFmtBase->GetID()==FMT_ID_FOOTER_LEFT ||
            pFmtBase->GetID()==FMT_ID_FOOTER_CENTER ||
            pFmtBase->GetID()==FMT_ID_FOOTER_RIGHT ||
            pFmtBase->GetID()==FMT_ID_TITLE ||
            pFmtBase->GetID()==FMT_ID_SUBTITLE ||
            pFmtBase->GetID()==FMT_ID_PAGENOTE ||
            pFmtBase->GetID()==FMT_ID_ENDNOTE ||
            pFmtBase->GetID()==FMT_ID_STUBHEAD ||
            pFmtBase->GetID()==FMT_ID_STUBHEAD_SEC ||
            pFmtBase->GetID()==FMT_ID_AREA_CAPTION);
    }
    else {
        // invalid format type!
        ASSERT(FALSE);
    }

    for (int i=0 ; i<m_aFmt.GetSize() ; i++)  {
        if (pFmtBase->GetID()==m_aFmt[i]->GetID() && pFmtBase->GetIndex()==m_aFmt[i]->GetIndex()) {
            pOldFmt = m_aFmt[i];
            break;
        }
    }


    if (pOldFmt)  {
        // a format with this ID+index already exists in the registry ...

        // if the *s are the same, then do nothing and indicate success
        if (pOldFmt==pFmtBase) {
            return true;
        }

        // otherwise, return false.  We shouldn't mess with dynamic allocations unless all is good.
        ASSERT(FALSE);
        return false;
    }
    m_aFmt.Add(pFmtBase);

    return true;

//        delete pOldFmt;
//        m_aFmt.RemoveAt(i);
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CFmtReg::Remove
//                               CFmtReg::RemoveAll
//
/////////////////////////////////////////////////////////////////////////////
void CFmtReg::Remove(FMT_ID ID, int iIndex)
{
    for (int i=0 ; i<m_aFmt.GetSize() ; i++)  {
        CFmtBase* pFmtBase=m_aFmt[i];
        if (ID==pFmtBase->GetID() && iIndex==pFmtBase->GetIndex()) {
            delete pFmtBase;
            m_aFmt.RemoveAt(i);
            return;
        }
    }

    // format not found!
    ASSERT(FALSE);
}


void CFmtReg::RemoveAll(void)
{
    while (m_aFmt.GetSize() > 0) {
        if(m_aFmt[0] != NULL){
            ASSERT_VALID(m_aFmt[0]);
            delete m_aFmt[0];
        }
        m_aFmt.RemoveAt(0);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CFmtReg::Build
//
// There are 2 versions of this method:
// - build from an XTS file (takes a CSpecFile& parameter)
// - build from a TFT file (takes a const CIMSAString& filename parameter)
//
/////////////////////////////////////////////////////////////////////////////
bool CFmtReg::Build(const CIMSAString& sTFTFile)  {
    CIMSAString s;
    CSpecFile specFile;

    if (!specFile.Open(sTFTFile, CFile::modeRead))  {
        s.Format(_T("Error: could not open file %s"), (const csprochar*)sTFTFile);
        AfxMessageBox(s,MB_ICONEXCLAMATION);
        return false;
    }

    CIMSAString sVersion;

    // check header
    BOOL bHeaderOK = FALSE;
    if (specFile.IsHeaderOK(TFT_SECT_FORMAT_FILE))  {
        bHeaderOK = TRUE;
        if (!specFile.IsVersionOK(sVersion))  {
            AfxMessageBox(_T("bad version"));
            return false;
        }
    }
    else  {
        return false;
    }

    return Build(specFile, sVersion);
}


bool CFmtReg::Build(CSpecFile& specFile, const CIMSAString& sVersion, bool bSilent)  {
    UNREFERENCED_PARAMETER(bSilent);

    CIMSAString sCmd, sArg;

    // add new contents
    bool bLineOK;
    UNITS eUnits=UNITS_METRIC;
    CPre32TallyFmt oldDefRowTallyFmt, oldDefColTallyFmt;

    while (specFile.GetLine(sCmd, sArg) == SF_OK)  {
        bLineOK = false;
        ASSERT (!sCmd.IsEmpty());

        if (!sCmd.CompareNoCase(TFT_SECT_FORMAT_TABSET))  {
            CTabSetFmt* pTabSetFmt = new CTabSetFmt;
            pTabSetFmt->Build(specFile);
            if(pTabSetFmt->GetIndex()==0){//remove the one which comes from the constructor before we add the .xts default fmt
                Remove(pTabSetFmt->GetID(),pTabSetFmt->GetIndex());
            }
            AddFmt(pTabSetFmt);
            eUnits=pTabSetFmt->GetUnits();
            continue;
        }
        if (!sCmd.CompareNoCase(TFT_SECT_FORMAT_TBL))  {
            CTblFmt* pTblFmt = new CTblFmt;
            pTblFmt->Build(specFile);
            if(pTblFmt->GetIndex()==0){//remove the one which comes from the constructor before we add the .xts default fmt
                Remove(pTblFmt->GetID(),pTblFmt->GetIndex());
            }
            AddFmt(pTblFmt);
            continue;
        }
        if (!sCmd.CompareNoCase(TFT_SECT_FORMAT_TALLY))  {
            CTallyFmt* pTallyFmt = new CTallyFmt;
            if (sVersion == _T("CSPro 3.0") || sVersion == _T("CSPro 3.1")) {
                pTallyFmt->BuildPre32(specFile, oldDefRowTallyFmt, oldDefColTallyFmt);
            }
            else {
                pTallyFmt->Build(specFile);
            }
            if(pTallyFmt->GetIndex()==0){//remove the one which comes from the constructor before we add the .xts default fmt
                Remove(pTallyFmt->GetID(),pTallyFmt->GetIndex());
            }
            AddFmt(pTallyFmt);
            continue;
        }
        if (!sCmd.CompareNoCase(TFT_SECT_FORMAT_TBLPRINT))  {
            CTblPrintFmt* pTblPrintFmt = new CTblPrintFmt;
            pTblPrintFmt->SetUnits(eUnits);
            pTblPrintFmt->Build(specFile);

            // if a printer is specified, see if it is available ...
            m_sErrorMsg.Empty();
            bool bHavePrinter=false;
            if (!pTblPrintFmt->GetPrinterDevice().IsEmpty()) {
                HGLOBAL hDevMode=NULL;
                HGLOBAL hDevNames=NULL;
                CString sPrinterDevice(pTblPrintFmt->GetPrinterDevice());
                csprochar* pszPrinterDevice=sPrinterDevice.GetBuffer(sPrinterDevice.GetLength());
                if (GetPrinterDevice(pszPrinterDevice, &hDevNames, &hDevMode)) {
                    AfxGetApp()->SelectPrinter(hDevNames, hDevMode);
                    bHavePrinter=true;
                }
                else {
                    // XTS gave a printer, but we couldn't find it...
                    m_sErrorMsg.Format(_T("Printer %s not found, using default printer "), (LPCTSTR)pTblPrintFmt->GetPrinterDevice());
                }
                sPrinterDevice.ReleaseBuffer();
            }
            else {
                // no printer specified ... use default
                m_sErrorMsg=_T("No printer specified, using default printer ");
            }

            if (!bHavePrinter) {
                // no printer for us yet; use the Win default ...

                CPrintDialog dlg(FALSE);
                if (dlg.GetDefaults()) {
                    // XTS printer invalid, but we can use the Windows default ...
                    DEVNAMES FAR *pDevNames=(DEVNAMES FAR *)::GlobalLock(dlg.m_pd.hDevNames);
                    CString sDriver((LPTSTR)pDevNames + pDevNames->wDriverOffset);
                    CString sDevice((LPTSTR)pDevNames + pDevNames->wDeviceOffset);
                    CString sOutput((LPTSTR)pDevNames + pDevNames->wOutputOffset);
                    pTblPrintFmt->SetPrinterDevice(sDevice);
                    pTblPrintFmt->SetPrinterDriver(sDriver);
                    pTblPrintFmt->SetPrinterOutput(sOutput);
                    ::GlobalUnlock(dlg.m_pd.hDevNames);
                    m_sErrorMsg += sDevice + _T(" instead.");
                }
                else {
                    // XTS printer invalid, but system has no printer installed...
                    m_sErrorMsg.Format(_T("Error: printer %s not found, no default printer found."), (LPCTSTR)pTblPrintFmt->GetPrinterDevice());
                    pTblPrintFmt->SetPrinterDevice(_T(""));
                    pTblPrintFmt->SetPrinterDriver(_T(""));
                    pTblPrintFmt->SetPrinterOutput(_T(""));
                }
            }

            if(pTblPrintFmt->GetIndex()==0){//remove the one which comes from the constructor before we add the .xts default fmt
                Remove(pTblPrintFmt->GetID(),pTblPrintFmt->GetIndex());
            }
            AddFmt(pTblPrintFmt);
            continue;
        }
        if (!sCmd.CompareNoCase(TFT_SECT_FORMAT_DATACELL))  {
            CDataCellFmt* pDataCellFmt = new CDataCellFmt;
            pDataCellFmt->SetUnits(eUnits);  // gotta do it, since CDataCell is : public CFmt
            pDataCellFmt->Build(specFile, sVersion, bSilent);

            // handle some specific error reporting for not-appl lines and hidden
            CIMSAString sError;
            if (pDataCellFmt->GetID()==FMT_ID_DATACELL) {
                if (pDataCellFmt->GetLineLeft()!=LINE_NOT_APPL) {
                    pDataCellFmt->SetLineLeft(LINE_NOT_APPL);
                    sError += _T("- left line for DataCellFmt must be NotApplicable\n");
                }
                if (pDataCellFmt->GetLineTop()!=LINE_NOT_APPL) {
                    pDataCellFmt->SetLineTop(LINE_NOT_APPL);
                    sError += _T("- top line for DataCellFmt must be NotApplicable\n");
                }
                if (pDataCellFmt->GetLineRight()!=LINE_NOT_APPL) {
                    pDataCellFmt->SetLineRight(LINE_NOT_APPL);
                    sError += _T("- right line for DataCellFmt must be NotApplicable\n");
                }
                if (pDataCellFmt->GetLineBottom()!=LINE_NOT_APPL) {
                    pDataCellFmt->SetLineBottom(LINE_NOT_APPL);
                    sError += _T("- bottom line for DataCellFmt must be NotApplicable\n");
                }
                if (pDataCellFmt->GetHidden()!=HIDDEN_NOT_APPL) {
                    pDataCellFmt->SetHidden(HIDDEN_NOT_APPL);
                    sError += _T("- hidden status for DataCellFmt must be NotApplicable\n");
                }
                /*if (pDataCellFmt->GetSpanCells()!=SPAN_CELLS_NOT_APPL) {
                    pDataCellFmt->SetSpanCells(SPAN_CELLS_NOT_APPL);
                    sError += _T("- span cells for DataCellFmt must be NotApplicable\n");
                }*/
            }
            else {
                if (pDataCellFmt->GetLineLeft()==LINE_NOT_APPL) {
                    pDataCellFmt->SetLineLeft(LINE_THIN);
                    sError += _T("- left line for DataCellFmt cannot be NotApplicable\n");
                }
                if (pDataCellFmt->GetLineTop()==LINE_NOT_APPL) {
                    pDataCellFmt->SetLineTop(LINE_THIN);
                    sError += _T("- top line for DataCellFmt cannot be NotApplicable\n");
                }
                if (pDataCellFmt->GetLineRight()==LINE_NOT_APPL) {
                    pDataCellFmt->SetLineRight(LINE_THIN);
                    sError += _T("- right line for DataCellFmt cannot be NotApplicable\n");
                }
                if (pDataCellFmt->GetLineBottom()==LINE_NOT_APPL) {
                    pDataCellFmt->SetLineBottom(LINE_THIN);
                    sError += _T("- bottom line for DataCellFmt cannot be NotApplicable\n");
                }
                if (pDataCellFmt->GetHidden()==HIDDEN_NOT_APPL) {
                    pDataCellFmt->SetHidden(HIDDEN_NO);
                    sError += _T("- hidden status for DataCellFmt cannot be NotApplicable\n");
                }
                /*if (pDataCellFmt->GetSpanCells()!=SPAN_CELLS_NOT_APPL) {
                    pDataCellFmt->SetSpanCells(SPAN_CELLS_NOT_APPL);
                    sError += _T("- span cells for DataCellFmt must be NotApplicable\n");
                }*/
            }
            if (!sError.IsEmpty()) {
                // report
                sError = _T("The following errors were corrected\n")+sError;
                AfxMessageBox(sError,MB_ICONINFORMATION);
            }
            if(pDataCellFmt->GetIndex()==0){//remove the one which comes from the constructor before we add the .xts default fmt
                Remove(pDataCellFmt->GetID(),pDataCellFmt->GetIndex());
            }
            AddFmt(pDataCellFmt);
            continue;
        }
        if (!sCmd.CompareNoCase(TFT_SECT_FORMAT))  {
            CFmt* pFmt = new CFmt;
            pFmt->SetUnits(eUnits);
            pFmt->Build(specFile, sVersion, bSilent);

            // handle some specific error reporting for not-appl hidden
            CIMSAString sError;
            if (pFmt->GetID()==FMT_ID_SPANNER || pFmt->GetID()==FMT_ID_CAPTION) {
                //BMD Spec for caption /spanner hidden
                /*if (pFmt->GetHidden()!=HIDDEN_NOT_APPL) {
                    pFmt->SetHidden(HIDDEN_NOT_APPL);
                    sError += _T("- hidden status for spanners and captions must be NotApplicable\n");
                }*/
            }
            else {
                if (pFmt->GetHidden()==HIDDEN_NOT_APPL) {
                    pFmt->SetHidden(HIDDEN_NO);
                    sError += _T("- hidden status for non-spanner/caption cannot be NotApplicable\n");
                }
            }

            // handle some specific error reporting for not-appl span cells
            if (pFmt->GetID()==FMT_ID_CAPTION || pFmt->GetID()==FMT_ID_AREA_CAPTION) {
                if (pFmt->GetSpanCells()==SPAN_CELLS_NOT_APPL) {
                    pFmt->SetSpanCells(SPAN_CELLS_NOT_APPL);
                    sError += _T("- span cells for captions cannot be NotApplicable\n");
                }
            }
            else {
                if (pFmt->GetSpanCells()!=SPAN_CELLS_NOT_APPL) {
                    pFmt->SetSpanCells(SPAN_CELLS_NOT_APPL);
                    sError += _T("- span cells for non-captions must be NotApplicable\n");
                }
            }

            if (!sError.IsEmpty()) {
                // report
                sError = _T("The following errors were corrected\n")+sError;
                AfxMessageBox(sError,MB_ICONINFORMATION);
            }
            if(pFmt->GetIndex()==0){//remove the one which comes from the constructor before we add the .xts default fmt
                Remove(pFmt->GetID(),pFmt->GetIndex());
            }
            AddFmt(pFmt);
            continue;
        }
        if (sCmd[0] == '[')  {
            specFile.UngetLine();
            break;
        }

        if (!bLineOK)  {
            // signal unrecognized command
            sArg.Format(_T("Unrecognized command at line %d"), specFile.GetLineNumber());
            sArg += _T("\n") + sCmd;
            AfxMessageBox(sArg);
        }
    }
    return true;
}



/////////////////////////////////////////////////////////////////////////////
//
//                               CFmtReg::Save
//
// Saves each entry in the format registry.  Order is:
//   tabset formats
//   table formats
//   tally formats
//   table print formats
//   data cell formats
//   formats (spanner, colhead, etc.)
//
// There are 2 versions of this method:
// - save for an XTS file (takes a CSpecFile& parameter)
// - save for a TFT file (takes a const CIMSAString& filename parameter)
//
/////////////////////////////////////////////////////////////////////////////
bool CFmtReg::Save(CSpecFile& specFile, bool bDefaultOnly) const
{
    // variables for a little debug checking
    int iNumSavedFmts=0;
    int iNumTabSetFmts=0;
    UNITS eUnits=UNITS_METRIC;
    //Savy (R) to clear the unused format 20090401
    (const_cast<CFmtReg*>(this))->ClearUnUsedFmts();
    // put out tabset fmts
    for (int i=0 ; i<m_aFmt.GetSize() ; i++)  {
        if (m_aFmt[i]->IsKindOf(RUNTIME_CLASS(CTabSetFmt))) {
            if (!bDefaultOnly || m_aFmt[i]->GetIndex() == 0) {
                m_aFmt[i]->Save(specFile);
                iNumSavedFmts++;
                iNumTabSetFmts++;
                eUnits=(DYNAMIC_DOWNCAST(CTabSetFmt,m_aFmt[i]))->GetUnits();
                specFile.PutLine(_T(""));
            }
        }
    }

    // put out table fmts
    for (int i=0 ; i<m_aFmt.GetSize() ; i++)  {
        if (m_aFmt[i]->IsKindOf(RUNTIME_CLASS(CTblFmt))) {
            if (!bDefaultOnly || m_aFmt[i]->GetIndex() == 0) {
                m_aFmt[i]->Save(specFile);
                iNumSavedFmts++;
                specFile.PutLine(_T(""));
            }
        }
    }

    // put out tally fmts
    for (int i=0 ; i<m_aFmt.GetSize() ; i++)  {
        if (m_aFmt[i]->IsKindOf(RUNTIME_CLASS(CTallyFmt))) {
            if (!bDefaultOnly || m_aFmt[i]->GetIndex() == 0) {
                m_aFmt[i]->Save(specFile);
                iNumSavedFmts++;
                specFile.PutLine(_T(""));
            }
        }
    }

    // put out table print fmts
    for (int i=0 ; i<m_aFmt.GetSize() ; i++)  {
        if (m_aFmt[i]->IsKindOf(RUNTIME_CLASS(CTblPrintFmt))) {
            if (!bDefaultOnly || m_aFmt[i]->GetIndex() == 0) {
                m_aFmt[i]->Save(specFile);
                iNumSavedFmts++;
                ASSERT((DYNAMIC_DOWNCAST(CTblPrintFmt,m_aFmt[i]))->GetUnits()==eUnits);  // tabset units must be the same as tblprint units
                specFile.PutLine(_T(""));
            }
        }
    }

    // put out data cell fmts
    for (int i=0 ; i<m_aFmt.GetSize() ; i++)  {
        if (m_aFmt[i]->IsKindOf(RUNTIME_CLASS(CDataCellFmt))) {
            if (!bDefaultOnly || m_aFmt[i]->GetIndex() == 0) {
                m_aFmt[i]->Save(specFile);
                iNumSavedFmts++;
                ASSERT((DYNAMIC_DOWNCAST(CDataCellFmt,m_aFmt[i]))->GetUnits()==eUnits);  // tabset units must be the same as tblprint units
                specFile.PutLine(_T(""));
            }
        }
    }

    // put out stock formats
    for (int i=0 ; i<m_aFmt.GetSize() ; i++)  {
        if (m_aFmt[i]->IsKindOf(RUNTIME_CLASS(CFmt)) && !m_aFmt[i]->IsKindOf(RUNTIME_CLASS(CDataCellFmt))) {  // since CDataCellFmt is : public CFmt
            if (!bDefaultOnly || m_aFmt[i]->GetIndex() == 0) {
                m_aFmt[i]->Save(specFile);
                iNumSavedFmts++;
                ASSERT((DYNAMIC_DOWNCAST(CFmt,m_aFmt[i]))->GetUnits()==eUnits);  // tabset units must be the same as tblprint units
                specFile.PutLine(_T(""));
            }
        }
    }

    // sanity checks
    ASSERT(iNumSavedFmts==m_aFmt.GetSize() || bDefaultOnly);
    ASSERT(iNumTabSetFmts==1);

    return true;
}


bool CFmtReg::Save(const CIMSAString& sTFTFile, bool bDefaultOnly) const
{
    CSpecFile specFile;
    CIMSAString s;

    if (!specFile.Open(sTFTFile, CFile::modeWrite))  {
        s.Format(_T("Error: could not open file %s"), (const csprochar*)sTFTFile);
        AfxMessageBox(s,MB_ICONEXCLAMATION);
        return false;
    }

    // save header
    specFile.PutLine(TFT_SECT_FORMAT_FILE);
    specFile.PutLine(CMD_VERSION, CSPRO_VERSION);
    specFile.PutLine(_T(""));

    return Save(specFile, bDefaultOnly);
}
//Savy (R) to clear the unused format 20090401
void CFmtReg::ClearUnUsedFmts()
{
    for (int iFmt=m_aFmt.GetSize()-1 ; iFmt>=0; iFmt--)  {
        CFmtBase* pFmtBase = m_aFmt[iFmt];
        //If it is default ignore
        if (pFmtBase->GetIndex() == 0) {
            continue;
        }
        else{
            if (!pFmtBase->IsUsed()){
                delete pFmtBase;
                m_aFmt.RemoveAt(iFmt);
            }
        }
    }

}
//Savy- Reset Fmt Ref Counts for Non-Default Formats
void CFmtReg::ResetFmtUsedFlag()
{
    for (int iFmt=m_aFmt.GetSize()-1 ; iFmt>=0; iFmt--)  {
        CFmtBase* pFmtBase = m_aFmt[iFmt];
        //If it is default ignore
        if (pFmtBase->GetIndex() == 0) {
            continue;
        }
        else{
            pFmtBase->SetUsedFlag(false);
        }
    }

}
/////////////////////////////////////////////////////////////////////////////
//
//                               CFmtReg::GetNextCustomFmtIndex
//
// Returns an index that can be used to identify a unique CFmt.  Unique means
// that the format's ID/index combination is not currently used in the fmt registry.
//
// The search for indices only looks at formats with the same ID as fmtSearch.
//
/////////////////////////////////////////////////////////////////////////////
int CFmtReg::GetNextCustomFmtIndex(const CFmtBase& fmtSearch) const
{
    int iIndex=NONE;
    for (int iFmt=0 ; iFmt<m_aFmt.GetSize() ; iFmt++) {
        CFmtBase* pFmt=m_aFmt[iFmt];
        if (pFmt->GetID()==fmtSearch.GetID()) {
            if (pFmt->GetIndex()>iIndex) {
                iIndex=pFmt->GetIndex();
            }
        }
    }
    ASSERT(iIndex>=0);  // 0 means that only the default is present, and a default must always be present!
    return iIndex+1;
}

int CFmtReg::GetNextCustomFmtIndex(FMT_ID id) const
{
    int iIndex=NONE;
    for (int iFmt=0 ; iFmt<m_aFmt.GetSize() ; iFmt++) {
        CFmtBase* pFmt=m_aFmt[iFmt];
        if (pFmt->GetID()==id) {
            if (pFmt->GetIndex()>iIndex) {
                iIndex=pFmt->GetIndex();
            }
        }
    }
    ASSERT(iIndex>=0);  // 0 means that only the default is present, and a default must always be present!
    return iIndex+1;
}

///////////////////////////////////////////////////////////////////////////////////
////
////    CFmtReg::SetUnits(UNITS eUnits)
//
// sets units in all CFmts in registry.
// they should all be in synch with CTabSet units (others are just copies) so use this rather
// than calling CTabSet method directly.
///////////////////////////////////////////////////////////////////////////////////
void CFmtReg::SetUnits(UNITS eUnits)
{
    const int iNumFmts = m_aFmt.GetSize();
    for (int i = 0; i < iNumFmts; ++i) {
        CFmtBase* pFmtBase = m_aFmt[i];
        pFmtBase->SetUnits(eUnits);
    }
}

///////////////////////////////////////////////////////////////////////////////////
////
////    CFmtReg::CopyDefaults
// copy only default values from another format registry
///////////////////////////////////////////////////////////////////////////////////
void CFmtReg::CopyDefaults(const CFmtReg& source)
{
    for (int i=0 ; i<source.m_aFmt.GetSize() ; ++i)  {
        CFmtBase* pSrcFmt = source.m_aFmt[i];
        if (pSrcFmt->GetIndex() == 0) {
            CFmtBase* pDestFmt = GetFmt(pSrcFmt->GetID());
            ASSERT(pDestFmt);
            pDestFmt->Assign(*pSrcFmt);
        }
    }
}
void CFmtReg::ReconcileFmtsForPaste(CFmtReg& fmtClipBoard,CMap<CFmtBase*,CFmtBase*,CFmtBase*,CFmtBase*>& /*aMapOldFmts2NewFmts*/)
{
    //Loop through all the clipboard fmts in fmtReg
    for (int i=fmtClipBoard.m_aFmt.GetSize()-1 ; i>=0; i--)  {
        CFmtBase* pClipBoardFmt = fmtClipBoard.m_aFmt[i];
        //If it is default ignore
        if (pClipBoardFmt->GetIndex() == 0) {
            continue;
        }
        //If it is custom
        //Set the ID for this format  correctly remove it from fmtClipBoard
        int iNextIndex = GetNextCustomFmtIndex(pClipBoardFmt->GetID());
        pClipBoardFmt->SetIndex(iNextIndex);
        fmtClipBoard.m_aFmt.RemoveAt(i);
        //and  Add it to "This" FmtReg.
        AddFmt(pClipBoardFmt);

        //SAVY&&& if this works no need of FMT reconciliation in the clip board
        //SAVY&&& If this does not work then you will have to create new objects

        //Create a new format by finding the nextID which we need
        //Add it to the current TabSet fmtReg
        //Add the old pointer and the new pointer to the map
    }

}
///////////////////////////////////////////////////////////////////////////////////
////
////    bool CTallyFmt::Build (CSpecFile& specFile, bool bSilent/*=false*/)
////
///////////////////////////////////////////////////////////////////////////////////
//bool CTallyFmt::Build(CSpecFile& specFile, bool bSilent/*=false*/)
//{
//  UNREFERENCED_PARAMETER(bSilent);
//  CIMSAString sCmd,sArg;
//  while (specFile.GetLine(sCmd, sArg) == SF_OK) {
//        if (sCmd[0] != '[')  {
//          TALLY_STATISTIC eTallyFmt = TALLY_STATISTIC_DEFAULT;
//          sArg.Trim();
//          if(sArg.CompareNoCase("No") ==0){
//              eTallyFmt =TALLY_STATISTIC_NO;
//          }
//          else if(sArg.CompareNoCase("Yes") ==0){
//              eTallyFmt =TALLY_STATISTIC_YES;
//          }
//
//          if(sCmd.CompareNoCase(XTS_CMD_COUNTS) == 0){
//              SetCounts(eTallyFmt);
//          }
//          else if(sCmd.CompareNoCase(XTS_CMD_PERTYPE) == 0){
//              SetPercentType(sArg);
//          }
//          else if(sCmd.CompareNoCase(XTS_CMD_PERPOS) == 0){
//              SetPercentPos(sArg);
//          }
//          else if(sCmd.CompareNoCase(XTS_CMD_TOTPOS) == 0){
//              SetTotalsPos(sArg);
//          }
//          else if (sCmd.CompareNoCase(XTS_CMD_MIN) == 0)  {
//              SetMin(eTallyFmt);
//          }
//          else if (sCmd.CompareNoCase(XTS_CMD_MAX) == 0)  {
//              SetMax(eTallyFmt);
//          }
//          else if (sCmd.CompareNoCase(XTS_CMD_MODE) == 0)  {
//              SetMode(eTallyFmt);
//          }
//          else if (sCmd.CompareNoCase(XTS_CMD_MEAN) == 0)  {
//              SetMean(eTallyFmt);
//          }
//          else if (sCmd.CompareNoCase(XTS_CMD_MEDIAN) == 0)  {
//              SetMedian(sArg);
//          }
//          //TODO nTiles , Prop Median interval list
//          else if (sCmd.CompareNoCase(XTS_CMD_STDDEV) == 0)  {
//              SetStdDev(eTallyFmt);
//          }
//          else if (sCmd.CompareNoCase(XTS_CMD_VARIANCE) == 0)  {
//              SetVariance(eTallyFmt);
//          }
//          else if (sCmd.CompareNoCase(XTS_CMD_STDERR) == 0)  {
//              SetStdErr(eTallyFmt);
//          }
//
//          else {
//              specFile.UngetLine();
//              break;
//          }
//
//      }
//      else {
//          specFile.UngetLine();
//          break;
//      }
//  }
//  return true;
//}
///////////////////////////////////////////////////////////////////////////////////
////
////    CIMSAString CTallyFmt::GetCmdString(const TALLY_STATISTIC eFlag)const
////
///////////////////////////////////////////////////////////////////////////////////
//CIMSAString CTallyFmt::GetCmdString(const TALLY_STATISTIC eFlag)const
//{
//  CIMSAString sRet;
//  switch(eFlag){
//      case TALLY_STATISTIC_YES:
//          sRet="Yes";
//          break;
//      case TALLY_STATISTIC_NO:
//          sRet="No";
//          break;
//      case TALLY_STATISTIC_DEFAULT:
//      default:
//          sRet="";
//          break;
//  }
//  return sRet;
//}
///////////////////////////////////////////////////////////////////////////////////
////
////    CIMSAString CTallyFmt::GetCmdString(const PCT_POS eFlag)const
////
///////////////////////////////////////////////////////////////////////////////////
//CIMSAString CTallyFmt::GetCmdString(const PCT_POS eFlag)const
//{
//  CIMSAString sRet;
//  switch(eFlag){
//      case PCT_POS_LEFT:
//          sRet="Left";
//          break;
//      case PCT_POS_RIGHT:
//          sRet="Right";
//          break;
//      case PCT_POS_ABOVE:
//          sRet="Above";
//          break;
//      case PCT_POS_BELOW:
//          sRet="Below";
//          break;
//      case PCT_POS_DEFAULT:
//      default:
//          sRet="";
//          break;
//  }
//  return sRet;
//}
//
///////////////////////////////////////////////////////////////////////////////////
////
////    CIMSAString CTallyFmt::GetCmdString(const PCT_TYPE eFlag) const
////
///////////////////////////////////////////////////////////////////////////////////
//CIMSAString CTallyFmt::GetCmdString(const PCT_TYPE eFlag) const
//{
//  CIMSAString sRet;
//  switch(eFlag){
//      case PCT_TYPE_NONE:
//          sRet="None";
//          break;
//      case PCT_TYPE_TOTAL:
//          sRet="Total";
//          break;
//      case PCT_TYPE_ROW:
//          sRet="Row";
//          break;
//      case PCT_TYPE_COL:
//          sRet="Column";
//          break;
//      case PCT_TYPE_CELL:
//          sRet="Cell";
//          break;
//      case PCT_TYPE_DEFAULT:
//      default:
//          sRet="";
//          break;
//  }
//  return sRet;
//}
///////////////////////////////////////////////////////////////////////////////////
////
////    CIMSAString CTallyFmt::GetCmdString(const TOTALS_POSITION eFlag)const
////
///////////////////////////////////////////////////////////////////////////////////
//CIMSAString CTallyFmt::GetCmdString(const TOTALS_POSITION eFlag)const
//{
//  CIMSAString sRet;
//  switch(eFlag){
//      case TOTALS_POSITION_NONE:
//          sRet="None";
//          break;
//      case TOTALS_POSITION_BEFORE:
//          sRet="Before";
//          break;
//      case TOTALS_POSITION_AFTER:
//          sRet="After";
//          break;
//      case TOTALS_POSITION_DEFAULT:
//      default:
//          sRet="";
//          break;
//  }
//  return sRet;
//}
//
///////////////////////////////////////////////////////////////////////////////////
////
////    CIMSAString CTallyFmt::GetCmdString(const MEDIAN_TYPE eFlag)const
////
///////////////////////////////////////////////////////////////////////////////////
//CIMSAString CTallyFmt::GetCmdString(const MEDIAN_TYPE eFlag)const
//{
//  CIMSAString sRet;
//  switch(eFlag){
//      case MEDIAN_TYPE_NONE:
//          sRet="None";
//          break;
//      case MEDIAN_TYPE_CONTINUOUS:
//          sRet="Continuous";
//          break;
//      case MEDIAN_TYPE_DISCRETE:
//          sRet="Discrete";
//          break;
//      case MEDIAN_TYPE_DEFAULT:
//      default:
//          sRet="";
//          break;
//  }
//  return sRet;
//}
//
///////////////////////////////////////////////////////////////////////////////////
////
////    void CTallyFmt::SetMedian(CIMSAString sArg)
////
///////////////////////////////////////////////////////////////////////////////////
//void CTallyFmt::SetMedian(CIMSAString sArg)
//{
//  MEDIAN_TYPE eMedianFmt = MEDIAN_TYPE_DEFAULT;
//  sArg.Trim();
//  if(sArg.CompareNoCase("None")==0){
//      eMedianFmt = MEDIAN_TYPE_NONE;
//  }
//  else if(sArg.CompareNoCase("Continuous")==0){
//      eMedianFmt = MEDIAN_TYPE_CONTINUOUS;
//  }
//  else if(sArg.CompareNoCase("Discrete")==0){
//      eMedianFmt = MEDIAN_TYPE_DISCRETE;
//  }
//  SetMedian(eMedianFmt);
//}
//
///////////////////////////////////////////////////////////////////////////////////
////
////    void CTallyFmt::SetPercentType(const CIMSAString& sArg);
////
///////////////////////////////////////////////////////////////////////////////////
//void CTallyFmt::SetPercentType(CIMSAString sArg)
//{
//  PCT_TYPE ePercentType = PCT_TYPE_DEFAULT;
//  sArg.Trim();
//  if(sArg.CompareNoCase("None")==0){
//      ePercentType = PCT_TYPE_NONE;
//  }
//  else if(sArg.CompareNoCase("Total")==0){
//      ePercentType = PCT_TYPE_TOTAL;
//  }
//  else if(sArg.CompareNoCase("Row")==0){
//      ePercentType = PCT_TYPE_ROW;
//  }
//  else if(sArg.CompareNoCase("Column")==0){
//      ePercentType = PCT_TYPE_COL;
//  }
//  else if(sArg.CompareNoCase("Cell")==0){
//      ePercentType = PCT_TYPE_CELL;
//  }
//  SetPercentType(ePercentType);
//}
//
///////////////////////////////////////////////////////////////////////////////////
////
////    void CTallyFmt::SetPercentPos(CIMSAString sArg)
////
///////////////////////////////////////////////////////////////////////////////////
//void CTallyFmt::SetPercentPos(CIMSAString sArg)
//{
//  PCT_POS ePercentPos = PCT_POS_DEFAULT;
//  sArg.Trim();
//  if(sArg.CompareNoCase("Left")==0){
//      ePercentPos = PCT_POS_LEFT;
//  }
//  else if(sArg.CompareNoCase("Right")==0){
//      ePercentPos = PCT_POS_RIGHT;
//  }
//  else if(sArg.CompareNoCase("Above")==0){
//      ePercentPos = PCT_POS_ABOVE;
//  }
//  else if(sArg.CompareNoCase("Below")==0){
//      ePercentPos = PCT_POS_BELOW;
//  }
//  SetPercentPos(ePercentPos);
//}
//
///////////////////////////////////////////////////////////////////////////////////
////
////    void CTallyFmt::SetTotalsPos(const CIMSAString& sArg)
////
///////////////////////////////////////////////////////////////////////////////////
//void CTallyFmt::SetTotalsPos(CIMSAString sArg)
//{
//  TOTALS_POSITION eTotals = TOTALS_POSITION_DEFAULT;
//  sArg.Trim();
//  if(sArg.CompareNoCase("Before")==0){
//      eTotals = TOTALS_POSITION_BEFORE;
//  }
//  else if(sArg.CompareNoCase("After")==0){
//      eTotals = TOTALS_POSITION_AFTER;
//  }
//  else if(sArg.CompareNoCase("None")==0){
//      eTotals = TOTALS_POSITION_NONE;
//  }
//  SetTotalsPos(eTotals);
//}
//
///////////////////////////////////////////////////////////////////////////////////
////
////    bool CTallyFmt::Save (CSpecFile& specFile)const
////
///////////////////////////////////////////////////////////////////////////////////
//bool CTallyFmt::Save(CSpecFile& specFile) const
//{
//  bool bRet = true;
//#ifdef _DEBUG
//  specFile.PutLine(XTS_SECT_TALLYFMT);
//
//  specFile.PutLine(XTS_CMD_COUNTS,GetCmdString(GetCounts()));
//  specFile.PutLine(XTS_CMD_PERTYPE,GetCmdString(GetPercentType()));
//  specFile.PutLine(XTS_CMD_PERPOS,GetCmdString(GetPercentPos()));
//  specFile.PutLine(XTS_CMD_TOTPOS,GetCmdString(GetTotalsPos()));
//
//  specFile.PutLine(XTS_CMD_MIN,GetCmdString(GetMin()));
//  specFile.PutLine(XTS_CMD_MAX,GetCmdString(GetMax()));
//  specFile.PutLine(XTS_CMD_MODE,GetCmdString(GetMode()));
//  specFile.PutLine(XTS_CMD_MEAN,GetCmdString(GetMean()));
//  //TODO interval list
//
//  specFile.PutLine(XTS_CMD_MEDIAN,GetCmdString(GetMedian()));
//
//  //TODO nTiles
//
//  specFile.PutLine(XTS_CMD_STDDEV,GetCmdString(GetStdDev()));
//  specFile.PutLine(XTS_CMD_VARIANCE,GetCmdString(GetVariance()));
//  specFile.PutLine(XTS_CMD_STDERR,GetCmdString(GetStdErr()));
//#else
//  //Do it only if these are other than NotAppl
//  specFile.PutLine(XTS_SECT_TALLYFMT);
//
//  specFile.PutLine(XTS_CMD_COUNTS,GetCmdString(GetCounts()));
//  specFile.PutLine(XTS_CMD_PERTYPE,GetCmdString(GetPercentType()));
//  specFile.PutLine(XTS_CMD_PERPOS,GetCmdString(GetPercentPos()));
//  specFile.PutLine(XTS_CMD_TOTPOS,GetCmdString(GetTotalsPos()));
//
//  specFile.PutLine(XTS_CMD_MIN,GetCmdString(GetMin()));
//  specFile.PutLine(XTS_CMD_MAX,GetCmdString(GetMax()));
//  specFile.PutLine(XTS_CMD_MODE,GetCmdString(GetMode()));
//  specFile.PutLine(XTS_CMD_MEAN,GetCmdString(GetMean()));
//  //TODO interval list
//
//  specFile.PutLine(XTS_CMD_MEDIAN,GetCmdString(GetMedianFlag()));
//
//  //TODO nTiles
//
//  specFile.PutLine(XTS_CMD_STDDEV,GetCmdString(GetStdDev()));
//  specFile.PutLine(XTS_CMD_VARIANCE,GetCmdString(GetVariance()));
//  specFile.PutLine(XTS_CMD_STDERR,GetCmdString(GetStdErr()));
//
//#endif
//
//    specFile.PutLine(" ");
//  return bRet;
//}


#include <winspool.h>

// returns a DEVMODE and DEVNAMES for the printer name specified
BOOL GetPrinterDevice(LPTSTR pszPrinterName, HGLOBAL* phDevNames, HGLOBAL* phDevMode)
{
    // if NULL is passed, then assume we are setting app object's
    // devmode and devnames
    if (phDevMode == NULL || phDevNames == NULL)
        return FALSE;

    // Open printer
    HANDLE hPrinter;
    if (OpenPrinter(pszPrinterName, &hPrinter, NULL) == FALSE)
        return FALSE;

    // obtain PRINTER_INFO_2 structure and close printer
    DWORD dwBytesReturned, dwBytesNeeded;
    GetPrinter(hPrinter, 2, NULL, 0, &dwBytesNeeded);
    PRINTER_INFO_2* p2 = (PRINTER_INFO_2*)GlobalAlloc(GPTR,dwBytesNeeded);
    if (GetPrinter(hPrinter, 2, (LPBYTE)p2, dwBytesNeeded,&dwBytesReturned) == 0) {
       GlobalFree(p2);
       ClosePrinter(hPrinter);
       return FALSE;
    }
    ClosePrinter(hPrinter);

    // Allocate a global handle for DEVMODE
    HGLOBAL  hDevMode = GlobalAlloc(GHND, sizeof(*p2->pDevMode) + p2->pDevMode->dmDriverExtra);
    ASSERT(hDevMode);
    DEVMODE* pDevMode = (DEVMODE*)GlobalLock(hDevMode);
    ASSERT(pDevMode);

    // copy DEVMODE data from PRINTER_INFO_2::pDevMode
    memcpy(pDevMode, p2->pDevMode, sizeof(*p2->pDevMode) + p2->pDevMode->dmDriverExtra);
    GlobalUnlock(hDevMode);

    // Compute size of DEVNAMES structure from PRINTER_INFO_2's data
    DWORD drvNameLen = lstrlen(p2->pDriverName)+1;  // driver name
    DWORD ptrNameLen = lstrlen(p2->pPrinterName)+1; // printer name
    DWORD porNameLen = lstrlen(p2->pPortName)+1;    // port name

    // Allocate a global handle big enough to hold DEVNAMES.
    HGLOBAL hDevNames = GlobalAlloc(GHND,sizeof(DEVNAMES) + (drvNameLen + ptrNameLen + porNameLen)*sizeof(csprochar));
    ASSERT(hDevNames);
    DEVNAMES* pDevNames = (DEVNAMES*)GlobalLock(hDevNames);
    ASSERT(pDevNames);

    // Copy the DEVNAMES information from PRINTER_INFO_2
    // tcOffset = csprochar Offset into structure
    int tcOffset = sizeof(DEVNAMES)/sizeof(csprochar);
    ASSERT(sizeof(DEVNAMES) == tcOffset*sizeof(csprochar));

    pDevNames->wDriverOffset = (WORD)tcOffset;
    memcpy((LPTSTR)pDevNames + tcOffset, p2->pDriverName,drvNameLen*sizeof(csprochar));
    tcOffset += drvNameLen;

    pDevNames->wDeviceOffset = (WORD)tcOffset;
    memcpy((LPTSTR)pDevNames + tcOffset, p2->pPrinterName,ptrNameLen*sizeof(csprochar));
    tcOffset += ptrNameLen;

    pDevNames->wOutputOffset = (WORD)tcOffset;
    memcpy((LPTSTR)pDevNames + tcOffset, p2->pPortName,porNameLen*sizeof(csprochar));
    pDevNames->wDefault = 0;

    GlobalUnlock(hDevNames);
    GlobalFree(p2);   // free PRINTER_INFO_2

    // set the new hDevMode and hDevNames
    *phDevMode = hDevMode;
    *phDevNames = hDevNames;
    return TRUE;
}


CIMSAString GetFormatString(FMT_ID id)
{
    CIMSAString sRet;
     switch(id){
        case FMT_ID_INVALID:
            sRet = _T("Invalid");
            break;
        case FMT_ID_TITLE:
            sRet = _T("Title");
            break;
        case FMT_ID_SUBTITLE:
            sRet = _T("Sub Title");
            break;
        case FMT_ID_STUBHEAD:
            sRet = _T("Stub Head");
            break;
        case FMT_ID_STUBHEAD_SEC:
            sRet = _T("Secondary Stub Head");
            break;
        case FMT_ID_SPANNER:
           sRet = _T("Spanner");
           break;
        case FMT_ID_COLHEAD:
            sRet = _T("Column Head");
            break;
        case FMT_ID_CAPTION:
             sRet = _T("Caption");
            break;
        case FMT_ID_STUB:
            sRet = _T("Stub");
            break;
        case FMT_ID_DATACELL:
           sRet = _T("Data Cell");
            break;
        case FMT_ID_PAGENOTE:
            sRet = _T("Page Note");
            break;
        case FMT_ID_ENDNOTE:
             sRet = _T("End Note");
            break;
        case FMT_ID_AREA_CAPTION:
            sRet = _T("Area Caption");
            break;
        case FMT_ID_HEADER_LEFT:
            sRet = _T("Left Header");
            break;
        case FMT_ID_HEADER_CENTER:
            sRet = _T("Center Header");
            break;
        case FMT_ID_HEADER_RIGHT:
            sRet = _T("Right Header");
            break;
        case FMT_ID_FOOTER_LEFT:
            sRet = _T("Left Footer");
            break;
        case FMT_ID_FOOTER_CENTER:
            sRet = _T("Center Footer");
            break;
        case FMT_ID_FOOTER_RIGHT:
            sRet = _T("Right Footer");
            break;
        case FMT_ID_TALLY:
        case FMT_ID_TALLY_ROW:
        case FMT_ID_TALLY_COL:
            sRet = _T("Tally");
            break;
        case FMT_ID_TABSET:
            sRet = _T("Application");
            break;
        case FMT_ID_TABLE:
            sRet = _T("Table");
            break;
        case FMT_ID_TBLPRINT:
            sRet = _T("Table");
            break;
        default:
            break;
    }
    return sRet;
}
