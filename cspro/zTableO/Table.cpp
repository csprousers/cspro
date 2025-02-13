//***************************************************************************
//  File name: Table.cpp
//
//  Description:
//       Table objects implementation
//
//  History:    Date       Author     Comment
//              -----------------------------
//              19 Nov 02   BMD       CSPro 3.0
//
//***************************************************************************
#include "StdAfx.h"
#include "Table.h"
#include "LabelSerializer.h"
#include "TllyStat.h"
#include <zUtilO/ArrUtil.h>
#include <zUtilO/FileUtil.h>
#include <zUtilF/ProgressDlg.h>


IMPLEMENT_DYNAMIC(CTblBase,     CObject);
IMPLEMENT_DYNAMIC(CTblOb,       CTblBase);
IMPLEMENT_DYNAMIC(CSpecialCell, CTblBase);
IMPLEMENT_DYNAMIC(CTabValue,    CTblBase);
IMPLEMENT_DYNAMIC(CTabVar,      CTblBase);
IMPLEMENT_DYNAMIC(CTable,       CTblBase);
IMPLEMENT_DYNAMIC(CTabSet,      CObject);
IMPLEMENT_DYNAMIC(DOMAINVAR,    CObject);
//IMPLEMENT_DYNAMIC(CDataCell,    CTblOb);
IMPLEMENT_DYNAMIC(CSource,      CObject);
IMPLEMENT_DYNAMIC(CTabData,     CObject);
IMPLEMENT_DYNAMIC(CUnitSpec,    CObject);
IMPLEMENT_DYNAMIC(CConsolidate, CObject);
IMPLEMENT_DYNAMIC(CConSpec,     CObject);
IMPLEMENT_DYNAMIC(CTabLevel,    CObject);
//IMPLEMENT_DYNAMIC(CTallyFmt,    CObject);


/////////////////////////////////////////////////////////////////////////////
//
//                                   CTabOb
//
/////////////////////////////////////////////////////////////////////////////
//
//                               CTblOb::CTblOb
//
/////////////////////////////////////////////////////////////////////////////

CTblOb::CTblOb()
{
    m_sText.Empty();

 //   m_aGrdViewInfo.RemoveAll();
  //  m_aPrtViewInfo.RemoveAll();
//    m_bCustom   = false;
//    m_iStyleNum = 1;
//    m_aszMin.RemoveAll();
//    m_aszCurr.RemoveAll();
//    m_aszMin.Add(CSize(0,0));
//    m_szMax     = CSize(0,0);
//    m_aszCurr.Add(CSize(0,0));
    m_pFmt=NULL;
}
/*
/////////////////////////////////////////////////////////////////////////////
//
//                               CDataCell::CDataCell
//
/////////////////////////////////////////////////////////////////////////////

CDataCell::CDataCell(double fData /*=0) : CTblOb()
{
    m_bDirty = true;
    m_fData = fData;
    m_uNumDecimals=0;
    m_pCellFmt=NULL;
}

CDataCell::CDataCell(CDataCell& d) : CTblOb(d)
{
    *this = d;
}


CDataCell& CDataCell::operator=(CDataCell& d)
{
    CTblOb::operator=(d);
    m_bDirty = d.IsDirty();
    m_fData = d.GetData();
    m_uNumDecimals=d.GetNumDecimals();
    m_pCellFmt=d.GetDataCellFmt();
    return *this;
}

CString CDataCell::GetText() const  // overridden from base
{
    CDataCell* p = const_cast<CDataCell*>(this);
    if (p->IsDirty()) {
        p->FormatData();
    }
    return CTblOb::GetText();
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CDataCell::FormatData
//
/////////////////////////////////////////////////////////////////////////////
void CDataCell::FormatData()
{
    CDataCellFmt* pCellFmt = GetDataCellFmt();

    // Handle zero value
    if (m_fData == 0) {
        m_sText = pCellFmt->GetZeroMask();
        return;
    }

    // Break up value into integer part and decimal part
    double dDec, dInt;
    __int64 kDec, kInt;

    dDec = modf(m_fData, &dInt);
    int k = 1;
    for (UINT i = 0 ; i < m_uNumDecimals ; i++) {
        k *= 10;
    }
    if (m_fData < 0) {
        kInt = (__int64) -dInt;
        kDec = (__int64) (-dDec * k + 0.5);
    }
    else {
        kInt = (__int64) dInt;
        kDec = (__int64) (dDec * k + 0.5);
    }
    if (kDec >= k) {
        kInt++;
    }

    // Handle rounded to zero value
    if (kInt == 0 && kDec == 0) {
        m_sText = pCellFmt->GetZRoundMask();
        return;
    }

    // If negative output hyphen
    csprochar pszTemp[30];
    csprochar* pszStart = m_sText.GetBuffer(30);
    if (m_fData < 0) {
        *pszStart = HYPHEN;
        pszStart++;
    }

    // Get integer part
    if (kInt > 0 || m_uNumDecimals == 0 || pCellFmt->GetZeroBeforeDecimal()) {
        i64toa(kInt, pszTemp);
        // Format integer part
        int l = strlen(pszTemp);
        if (pCellFmt->GetDigitGrouping() == NO_GROUPING || l <= 3) {
            memmove(pszStart, pszTemp, l);
            pszStart += l;
        }
        else {
            csprochar cThSep = pCellFmt->GetThousandsSep();
            csprochar* pszGroup = pszTemp;
            if (pCellFmt->GetDigitGrouping() == THREE_PER_BLOCK) {
                int m = l % 3;
                if (m > 0) {
                    memmove(pszStart, pszGroup, m);
                    pszStart += m;
                    pszGroup += m;
                    l -= m;
                    *pszStart = cThSep;
                    pszStart++;
                }
                while (l > 0) {
                    memmove(pszStart, pszGroup, 3);
                    pszStart += 3;
                    pszGroup += 3;
                    l -= 3;
                    if (l > 0) {
                        *pszStart = cThSep;
                        pszStart++;
                    }
                }
            }
            else {
                int m = (l - 3) % 2;
                if (m > 0) {
                    memmove(pszStart, pszGroup, m);
                    pszStart += m;
                    pszGroup += m;
                    l -= m;
                    *pszStart = cThSep;
                    pszStart++;
                }
                while (l > 3) {
                    memmove(pszStart, pszGroup, 2);
                    pszStart += 2;
                    pszGroup += 2;
                    l -= 2;
                    *pszStart = cThSep;
                    pszStart++;
                }
                memmove(pszStart, pszGroup, 3);
                pszStart += 3;
            }
        }
    }

    // Get decimal part
    if (m_uNumDecimals > 0) {
        i64toc(kDec, pszTemp, m_uNumDecimals, TRUE);

        // Format decimal part
        *pszStart = pCellFmt->GetDecimalSep();
        pszStart++;
        memmove(pszStart, pszTemp, m_uNumDecimals);
        pszStart += m_uNumDecimals;
    }

    *pszStart = EOS;
    m_sText.ReleaseBuffer(-1);
    SetDirty(false);
}
*/
/////////////////////////////////////////////////////////////////////////////
//
//                                   CConsolidate
//
/////////////////////////////////////////////////////////////////////////////
//
//                           CConsolidate::CConsolidate
//
/////////////////////////////////////////////////////////////////////////////

CConsolidate::CConsolidate()
{
    m_aStructure.SetSize(0,16);
    m_bStandard = true;
    m_iStandard = -1;
}

CConsolidate::CConsolidate(CConsolidate& c)
{
    m_aStructure.SetSize(0,16);
    m_bStandard = c.m_bStandard;
    m_iStandard = c.m_iStandard;
    for (int i = 0 ; i < c.m_aStructure.GetSize() ; i++) {
        m_aStructure.Add(c.m_aStructure[i]);
    }
    for (int k = 0 ; k < c.m_aConSpec.GetSize() ; k++) {
        m_aConSpec.Add(c.m_aConSpec[k]);
    }
}

void CConsolidate::operator= (CConsolidate& c)
{
    Reset();
    m_bStandard = c.m_bStandard;
    m_iStandard = c.m_iStandard;
    for (int i = 0 ; i < c.m_aStructure.GetSize() ; i++) {
        m_aStructure.Add(c.m_aStructure[i]);
    }
    for (int k = 0 ; k < c.m_aConSpec.GetSize() ; k++) {
        m_aConSpec.Add(c.m_aConSpec[k]);
    }
}


CConsolidate::~CConsolidate()
{
    RemoveAllConSpecs();
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CConsolidate::Reset
//
/////////////////////////////////////////////////////////////////////////////

void CConsolidate::Reset()
{
    m_aStructure.RemoveAll();
    RemoveAllConSpecs();
    m_bStandard = true;
    m_iStandard = -1;
}



/////////////////////////////////////////////////////////////////////////////
//
//                           CConsolidate::GenerateStandard
//
/////////////////////////////////////////////////////////////////////////////

void CConsolidate::GenerateStandard()
{
    RemoveAllConSpecs();
    CConSpec* pConSpec = new CConSpec(m_aStructure.GetSize());
    pConSpec->SetAreaLevel(_T("TOTAL"));
    m_aConSpec.Add(pConSpec);
    for (int i = 0 ; i < m_iStandard ; i++ ) {
        pConSpec = new CConSpec(m_aStructure.GetSize());
        pConSpec->SetAreaLevel(m_aStructure[i]);
        for (int j = 0 ; j <= i ; j++) {
            CONITEM item = pConSpec->GetAction(j);
            item.level = j;
            pConSpec->SetAction(j, item);
        }
        m_aConSpec.Add(pConSpec);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CConsolidate::Reconcile
//
/////////////////////////////////////////////////////////////////////////////

void CConsolidate::Reconcile()
{
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CConsolidate::Build
//
/////////////////////////////////////////////////////////////////////////////

bool CConsolidate::Build(CSpecFile& specFile, bool bSilent /*= false*/)
{
    CString sCmd, sArg, sError;
    bool bResult = true;

    while (specFile.GetLine(sCmd, sArg) == SF_OK) {
        if (sCmd[0] != '[')  {
            if (sCmd.CompareNoCase(XTS_CMD_NAMES) == 0)  {
                CIMSAString sTemp = sArg;
                while (!sTemp.IsEmpty()) {
                    CString sName = sTemp.GetToken();
                    if (CIMSAString::IsName(sName)) {
                        m_aStructure.Add(sName);
                    }
                    else {
                        if (!bSilent) {
                            sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                            sError += _T("\n") + sCmd + _T("=") + sArg;
                            AfxMessageBox(sError);
                        }
                        bResult = false;
                    }
                }
                if (m_aStructure.GetSize() == 0) {
                    if (!bSilent) {
                        sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                        sError += _T("\n") + sCmd + _T("=") + sArg;
                        AfxMessageBox(sError);
                    }
                    bResult = false;
                }
            }
            else if (sCmd.CompareNoCase(XTS_CMD_STANDARD) == 0)  {
                if (sArg.CompareNoCase(XTS_ARG_YES) == 0) {
                    m_bStandard = true;
                }
                else if (sArg.CompareNoCase(XTS_ARG_NO) == 0) {
                    m_bStandard = false;
                }
                else {
                    m_bStandard = true;
                    if (!bSilent) {
                        sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                        sError += _T("\n") + sCmd + _T("=") + sArg;
                        AfxMessageBox(sError);
                    }
                    bResult = false;
                }
            }
            else if (sCmd.CompareNoCase(XTS_CMD_LOWEST_LEVEL) == 0) {
                int i;
                for (i = 0 ; i < m_aStructure.GetSize() ; i++) {
                    if (sArg.CompareNoCase(_T("ALL LEVELS")) == 0) {
                        m_iStandard = 0;
                        break;
                    }
                    else if (sArg.CompareNoCase(m_aStructure[i]) == 0) {
                        m_iStandard = i + 1;
                        break;
                    }
                }
                if (i >= m_aStructure.GetSize()) {
                    if (!bSilent) {
                        sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                        sError += _T("\n") + sCmd + _T("=") + sArg;
                        AfxMessageBox(sError);
                    }
                    bResult = false;
                }
            }
        }
        else if(sCmd.CompareNoCase(XTS_SECT_CONSPEC)==0) {
            CConSpec* pConSpec = new CConSpec(m_aStructure.GetSize());
            bResult = pConSpec->Build(specFile, &m_aStructure, bSilent);
            m_aConSpec.Add(pConSpec);
        }
        else {
            specFile.UngetLine();
            break;
        }
    }

    return bResult;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CConsolidate::Save
//
/////////////////////////////////////////////////////////////////////////////

void CConsolidate::Save(CSpecFile& specFile)
{
    specFile.PutLine (_T(" "));
    specFile.PutLine (XTS_SECT_AREASTRUCT);
    // Output Area Structure Names
    CString sValue = m_aStructure[0];
    for (int i = 1 ; i < m_aStructure.GetSize() ; i++) {
        sValue += _T(",") + m_aStructure[i];
    }
    specFile.PutLine (XTS_CMD_NAMES, sValue);
    // Output Specifications for Standard
    if (m_bStandard) {
        sValue = XTS_ARG_YES;
    }
    else {
        sValue = XTS_ARG_NO;
    }
    specFile.PutLine (XTS_CMD_STANDARD, sValue);
    if (m_iStandard > 0) {
        specFile.PutLine (XTS_CMD_LOWEST_LEVEL, m_aStructure[m_iStandard - 1]);
    }
    else {
        specFile.PutLine (XTS_CMD_LOWEST_LEVEL, _T("ALL LEVELS"));
    }
    // Output Each ConSpec
    for (int i = 0 ; i < m_aConSpec.GetSize() ; i++) {
        GetConSpec(i)->Save(specFile, &m_aStructure);
    }
    specFile.PutLine (_T(" "));
}

/////////////////////////////////////////////////////////////////////////////
//
//                           CConsolidate::RemoveAllConSpecs
//
/////////////////////////////////////////////////////////////////////////////

void CConsolidate::RemoveAllConSpecs()
{
    CConSpec* pConSpec;
    while (m_aConSpec.GetSize() > 0) {
        pConSpec = m_aConSpec[0];
        m_aConSpec.RemoveAt(0);
        delete pConSpec;
     }
}


/////////////////////////////////////////////////////////////////////////////
//
//                                   CConSpec
//
/////////////////////////////////////////////////////////////////////////////
//
//                                CConSpec::CConSpec
//
/////////////////////////////////////////////////////////////////////////////

CConSpec::CConSpec(CConSpec* c)
{
    m_sAreaLevel = c->m_sAreaLevel;
    for (int i = 0 ; i < c->GetNumActions() ; i++) {
        m_aActions.Add(c->m_aActions[i]);
    }
}

CConSpec::CConSpec(int iNum)
{
    for (int i = 0 ; i < iNum ; i++) {
        CONITEM cItem;
        cItem.level = CON_NONE;
        cItem.lower = CON_NONE;
        cItem.upper = CON_NONE;
        cItem.replace = CON_NONE;
        m_aActions.Add(cItem);
    }
}


CConSpec::~CConSpec()
{
    m_aActions.RemoveAll();
}

/////////////////////////////////////////////////////////////////////////////
//
//                                CConSpec::Build
//
/////////////////////////////////////////////////////////////////////////////

bool CConSpec::Build(CSpecFile& specFile, CStringArray* pAreaStruct, bool bSilent /*= false*/)
{
    CString sCmd, sArg, sError;
    bool bResult = true;

    while (specFile.GetLine(sCmd, sArg) == SF_OK) {
        if (sCmd[0] != '[')  {
            if (sCmd.CompareNoCase(XTS_CMD_NAME) == 0)  {
                if (CIMSAString::IsName(sArg)) {
                    m_sAreaLevel = sArg;
                }
                else {
                    if (!bSilent) {
                        sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                        sError += _T("\n") + sCmd + _T("=") + sArg;
                        AfxMessageBox(sError);
                    }
                    bResult = false;
                }
            }
            else if (sCmd.CompareNoCase(XTS_CMD_LEVEL) == 0)  {
                CIMSAString sTemp = sArg;
                CString sName = sTemp.GetToken();
                if (CIMSAString::IsName(sName)) {
                    int i;
                    for (i = 0 ; i < pAreaStruct->GetSize() ; i++) {
                        if (sName.CompareNoCase(pAreaStruct->GetAt(i)) == 0) {
                            break;
                        }
                    }
                    if (i < pAreaStruct->GetSize()) {
                        m_aActions[i].level = i;
                        csprochar cFound = _T(' ');
                        // Get Lower
                        CIMSAString sValue = sTemp.GetToken(_T(":="), &cFound);
                        if (sValue.IsEmpty()) {
                            continue;
                        }
                        m_aActions[i].lower = (int)sValue.Val();
                        if (cFound == ':') {
                            // Get Upper
                            sValue = sTemp.GetToken(_T("="));
                            if (sValue.IsEmpty()) {
                                m_aActions[i].upper = m_aActions[i].lower;
                            }
                            else {
                                int iVal = (int) sValue.Val();
                                if (iVal < m_aActions[i].lower) {
                                    if (!bSilent) {
                                        sError.Format(_T("Upper value (%d) < lower value (%d), Upper ignored"), iVal, m_aActions[i].lower);
                                        AfxMessageBox(sError);
                                    }
                                    m_aActions[i].upper = m_aActions[i].lower;
                                }
                                else {
                                    m_aActions[i].upper = iVal;
                                }
                            }
                        }
                        else {
                            m_aActions[i].upper = m_aActions[i].lower;
                        }
                        // Get Replace
                        if (!sTemp.IsEmpty()) {
                            m_aActions[i].replace = (int)sTemp.Val();
                        }
                    }
                    else {
                        if (!bSilent) {
                            sError.Format(_T("Unknown area name at line %d:"), specFile.GetLineNumber());
                            sError += _T("\n") + sCmd + _T("=") + sArg;
                            AfxMessageBox(sError);
                        }
                        bResult = false;
                    }
                }
                else {
                    if (!bSilent) {
                        sError.Format(_T("Bad area level name at line %d:"), specFile.GetLineNumber());
                        sError += _T("\n") + sCmd + _T("=") + sArg;
                        AfxMessageBox(sError);
                    }
                    bResult = false;
                }
            }
        }
        else {
            specFile.UngetLine();
            break;
        }
    }

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
//
//                                CConSpec::Save
//
/////////////////////////////////////////////////////////////////////////////

void CConSpec::Save(CSpecFile& specFile, CStringArray* pAreaStruct)
{
    specFile.PutLine (_T(" "));
    specFile.PutLine (XTS_SECT_CONSPEC);
    specFile.PutLine (XTS_CMD_NAME, m_sAreaLevel);
    for (int i = 0 ; i < m_aActions.GetSize() ; i++) {
        if (m_aActions[i].level == CON_NONE) {
            continue;
        }
        CString sValue = pAreaStruct->GetAt(m_aActions[i].level);
        if (m_aActions[i].lower != CON_NONE || m_aActions[i].replace != CON_NONE) {
            CIMSAString sTemp;
            sValue += _T(",");
            if (m_aActions[i].lower != CON_NONE) {
                sTemp.Str(m_aActions[i].lower);
                sValue += sTemp;
                if (m_aActions[i].upper != m_aActions[i].lower) {
                    sTemp.Str(m_aActions[i].upper);
                    sValue += _T(":") + sTemp;
                }
            }
            if (m_aActions[i].replace != CON_NONE) {
                sTemp.Str(m_aActions[i].replace);
                sValue += _T("=") + sTemp;
            }
        }
        specFile.PutLine (XTS_CMD_LEVEL, sValue);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                                   CTabData
//
/////////////////////////////////////////////////////////////////////////////
//
//                               CTabData::CTabData
//
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
//
//                                   CTabValue
//
/////////////////////////////////////////////////////////////////////////////
//
//                               CTabValue::Build
//
/////////////////////////////////////////////////////////////////////////////


bool CTabValue::Build(CSpecFile& specFile, const CFmtReg& reg, const CString& sVersion, bool bSilent)
{
    CString sCmd;
    CIMSAString sArg;
    bool bResult = true;

    while (specFile.GetLine(sCmd, sArg, false) == SF_OK) {  // BMD 25 May 2006
        if (sCmd[0] != '[')  {
            if (sCmd.CompareNoCase(XTS_CMD_LABEL) == 0)  {
                SetText(TableLabelSerializer::Parse(sArg, sVersion));
            }
            else if (sCmd.CompareNoCase(XTS_CMD_FORMAT)==0) {
                // find this format in the fmt registry...
                CString sIDString;
                CIMSAString sIndexString;
                CFmt* pFmt=NULL;
                sIDString = sArg.GetToken();
                sIndexString = sArg.GetToken();
                if (sIndexString.IsNumeric()) {
                    pFmt=DYNAMIC_DOWNCAST(CFmt,reg.GetFmt(sIDString,sIndexString));
                }
                SetFmt(pFmt);
                if (NULL==pFmt) {
                    m_sError.Format(_T("Unrecognized format label %s at line %d:"), (LPCTSTR)sArg, specFile.GetLineNumber());
                    m_sError += _T("\n") + sCmd + _T("=") + sArg;
                    m_sError += _T("\nUsing default format instead.");
                    AfxMessageBox(m_sError);
                }
            }
            else if (sCmd.CompareNoCase(XTS_CMD_TABVAL_TYPE)==0){
                sArg.Trim();
                if(sArg.CompareNoCase(XTS_ARG_DICT_TABVAL)==0){
                    m_eTabValType = DICT_TABVAL;
                }
                else if(sArg.CompareNoCase(XTS_ARG_NROW)==0){
                    m_eTabValType = STAT_NROW_TABVAL;
                }
                else if(sArg.CompareNoCase(XTS_ARG_MIN)==0){
                    m_eTabValType = STAT_MIN_TABVAL;
                }
                else if(sArg.CompareNoCase(XTS_ARG_MAX)==0){
                    m_eTabValType = STAT_MAX_TABVAL;
                }
                else if(sArg.CompareNoCase(XTS_ARG_MEAN)==0){
                    m_eTabValType = STAT_MEAN_TABVAL;
                }
                else if(sArg.CompareNoCase(XTS_ARG_MEDIAN)==0){
                    m_eTabValType = STAT_MEDIAN_TABVAL;
                }
                else if(sArg.CompareNoCase(XTS_ARG_MODE)==0){
                    m_eTabValType = STAT_MODE_TABVAL;
                }
                else if(sArg.CompareNoCase(XTS_ARG_STDDEV)==0){
                    m_eTabValType = STAT_STDDEV_TABVAL;
                }
                else if(sArg.CompareNoCase(XTS_ARG_STDERR)==0){
                    m_eTabValType = STAT_STDERR_TABVAL;
                }
                else if(sArg.CompareNoCase(XTS_ARG_VAR)==0){
                    m_eTabValType = STAT_VARIANCE_TABVAL;
                }
                else if(sArg.CompareNoCase(XTS_ARG_PERCENT)==0){
                    m_eTabValType = STAT_PCT_TABVAL;
                }
                else if(sArg.CompareNoCase(XTS_ARG_NTILE)==0){
                    m_eTabValType = STAT_NTILE_TABVAL;
                }
                else if(sArg.CompareNoCase(XTS_ARG_TOTAL)==0){
                    m_eTabValType = STAT_TOTAL_TABVAL;
                }
                else if(sArg.CompareNoCase(XTS_ARG_RDRBRK_TABVAL)==0){
                    m_eTabValType = RDRBRK_TABVAL;
                }
                else if(sArg.CompareNoCase(XTS_ARG_SPECIAL_TABVAL)==0){
                      m_eTabValType = SPECIAL_TABVAL;
                }
                else if(sArg.CompareNoCase(XTS_ARG_SPECIAL_MISSING)==0){
                      m_eTabValType = SPECIAL_MISSING_TABVAL;
                }
                else if(sArg.CompareNoCase(XTS_ARG_SPECIAL_REFUSED)==0){
                      m_eTabValType = SPECIAL_REFUSED_TABVAL;
                }
                else if(sArg.CompareNoCase(XTS_ARG_SPECIAL_DEFAULT)==0){
                      m_eTabValType = SPECIAL_DEFAULT_TABVAL;
                }
                else if(sArg.CompareNoCase(XTS_ARG_SPECIAL_NOTAPPL)==0){
                      m_eTabValType = SPECIAL_NOTAPPL_TABVAL;
                }
                else if(sArg.CompareNoCase(XTS_ARG_SPECIAL_UNDEFINED)==0){
                      m_eTabValType = SPECIAL_UNDEFINED_TABVAL;
                }

                else {
                    m_eTabValType = INVALID_TABVAL;
                    m_sError.Format(_T("Unrecognized Tab Value type %s at line %d: ."), (LPCTSTR)sArg, specFile.GetLineNumber());
                    m_sError += _T("\n") + sCmd + _T("=") + sArg;
                    AfxMessageBox(m_sError);
                }
            }
            else if (sCmd.CompareNoCase(XTS_CMD_TABVAL_STAT_INDEX) == 0) {
                if(!sArg.IsNumeric()){
                    m_sError.Format(_T("Unrecognized format label %s at line %d:"), (LPCTSTR)sArg, specFile.GetLineNumber());
                    m_sError += _T("\n") + sCmd + _T("=") + sArg;
                    m_sError += _T("\n ignoring tab val stat index");
                    AfxMessageBox(m_sError);
                    m_nStatId = -1;
                }
                else {
                    m_nStatId = (int)sArg.Val();
                }
            }
            else if (sCmd.CompareNoCase(XTS_ARG_PRNTVSZ) == 0 || sCmd.CompareNoCase(XTS_ARG_GRIDVSZ) == 0 )  {
                specFile.UngetLine();
                BuildStateInfo(specFile,bSilent);
            }
            else if (SO::IsBlank(sCmd)) {  // BMD 25 May 2006
            }
            else {
                // Unrecognized command at line %d:
                if (!bSilent) {
                    m_sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                    m_sError += _T("\n") + sCmd + _T("=") + sArg;
                    AfxMessageBox(m_sError);
                }
                bResult = false;
            }
        }
        else {
            if (sCmd.CompareNoCase(XTS_SECT_SPECIAL) == 0) {
                CSpecialCell special;
                special.Build(specFile, reg, bSilent);
                if(special.GetPanel() !=-1 && special.GetCol() != -1 && special.GetFmt()){
                    m_aSpecialCell.Add(special);
                }
            }
            else {
                specFile.UngetLine();
                break;
            }
        }
    }

    return bResult;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTabValue::Save
//
/////////////////////////////////////////////////////////////////////////////
void CTabValue::Save(CSpecFile& specFile)
{
    specFile.PutLine (_T(" "));
    specFile.PutLine (XTS_SECT_VALUE);
    specFile.PutLine (XTS_CMD_LABEL, TableLabelSerializer::Create(m_sText));

    switch(m_eTabValType){
        case DICT_TABVAL:
            specFile.PutLine (XTS_CMD_TABVAL_TYPE, XTS_ARG_DICT_TABVAL);
            break;
        case STAT_TOTAL_TABVAL:
            specFile.PutLine (XTS_CMD_TABVAL_TYPE, XTS_ARG_TOTAL);
            break;
        case STAT_NROW_TABVAL:
            specFile.PutLine (XTS_CMD_TABVAL_TYPE, XTS_ARG_NROW);
            break;
        case STAT_MIN_TABVAL:
            specFile.PutLine (XTS_CMD_TABVAL_TYPE, XTS_ARG_MIN);
            break;
        case STAT_MAX_TABVAL:
            specFile.PutLine (XTS_CMD_TABVAL_TYPE, XTS_ARG_MAX);
            break;
        case STAT_MEAN_TABVAL:
            specFile.PutLine (XTS_CMD_TABVAL_TYPE, XTS_ARG_MEAN);
            break;
        case STAT_MEDIAN_TABVAL:
            specFile.PutLine (XTS_CMD_TABVAL_TYPE, XTS_ARG_MEDIAN);
            break;
        case STAT_NTILE_TABVAL:
            specFile.PutLine (XTS_CMD_TABVAL_TYPE, XTS_ARG_NTILE);
            break;
        case STAT_MODE_TABVAL:
            specFile.PutLine (XTS_CMD_TABVAL_TYPE, XTS_ARG_MODE);
            break;
        case STAT_PCT_TABVAL:
            specFile.PutLine (XTS_CMD_TABVAL_TYPE, XTS_ARG_PERCENT);
            break;
        case STAT_VARIANCE_TABVAL:
            specFile.PutLine (XTS_CMD_TABVAL_TYPE, XTS_ARG_VAR);
            break;
        case STAT_STDERR_TABVAL:
            specFile.PutLine (XTS_CMD_TABVAL_TYPE, XTS_ARG_STDERR);
            break;
        case STAT_STDDEV_TABVAL:
            specFile.PutLine (XTS_CMD_TABVAL_TYPE, XTS_ARG_STDDEV);
            break;
        case RDRBRK_TABVAL:
            specFile.PutLine (XTS_CMD_TABVAL_TYPE, XTS_ARG_RDRBRK_TABVAL);
            break;
        case SPECIAL_TABVAL:
            specFile.PutLine (XTS_CMD_TABVAL_TYPE, XTS_ARG_SPECIAL_TABVAL);
            break;
        case SPECIAL_MISSING_TABVAL:
            specFile.PutLine (XTS_CMD_TABVAL_TYPE, XTS_ARG_SPECIAL_MISSING);
            break;
        case SPECIAL_REFUSED_TABVAL:
            specFile.PutLine (XTS_CMD_TABVAL_TYPE, XTS_ARG_SPECIAL_REFUSED);
            break;
        case SPECIAL_DEFAULT_TABVAL:
            specFile.PutLine (XTS_CMD_TABVAL_TYPE, XTS_ARG_SPECIAL_DEFAULT);
            break;
        case SPECIAL_NOTAPPL_TABVAL:
            specFile.PutLine (XTS_CMD_TABVAL_TYPE, XTS_ARG_SPECIAL_NOTAPPL);
            break;
        case SPECIAL_UNDEFINED_TABVAL:
            specFile.PutLine (XTS_CMD_TABVAL_TYPE, XTS_ARG_SPECIAL_UNDEFINED);
            break;
    }

    specFile.PutLine(XTS_CMD_TABVAL_STAT_INDEX, m_nStatId);
    CFmtBase* pFmt=GetFmt();
    if (NULL!=pFmt) {
        if (CFmtReg::IsCustomFmtID(*pFmt)) {
            //            CString sFmt ;
            //            sFmt = pFmt->GetIDString()+ "," + pFmt->GetIndexString();
            //            specFile.PutLine(XTS_CMD_FORMAT, sFmt);
            specFile.PutLine(XTS_CMD_FORMAT, pFmt->GetIDInfo());
        }
    }
    SaveStateInfo(specFile);

    for(int iIndex=0; iIndex<m_aSpecialCell.GetSize();iIndex++){
        m_aSpecialCell[iIndex].Save(specFile);
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CSpecialCell* CTabValue::FindSpecialCell(int iPanel,int iOffSetCol)
//
/////////////////////////////////////////////////////////////////////////////////
CSpecialCell* CTabValue::FindSpecialCell(int iPanel,int iOffSetCol)
{
    CSpecialCell* pRet = NULL;

    for(int iIndex =0; iIndex < m_aSpecialCell.GetSize(); iIndex++){
        if(m_aSpecialCell[iIndex].GetPanel() == iPanel && m_aSpecialCell[iIndex].GetCol() == iOffSetCol){
            pRet = &m_aSpecialCell[iIndex];
            break;
        }
    }

    return pRet;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CSpecialCell::Save(CSpecFile& specFile)
//
/////////////////////////////////////////////////////////////////////////////////
void CSpecialCell::Save(CSpecFile& specFile)
{
    specFile.PutLine (_T(" "));
    specFile.PutLine (XTS_SECT_SPECIAL);

    specFile.PutLine (XTS_CMD_PANEL, m_iRowPanel);
    specFile.PutLine (XTS_CMD_OFFSETCOL, m_iCol);

    CFmtBase* pFmt=GetFmt();
    ASSERT(pFmt); //Special cells always have data cell format
    if (NULL!=pFmt) {
        if (CFmtReg::IsCustomFmtID(*pFmt)) {
            specFile.PutLine(XTS_CMD_FORMAT, pFmt->GetIDInfo());
        }
        else {
            ASSERT(FALSE); //special cells do  have custom datacell fmt  right ?
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CSpecialCell::Build(CSpecFile& specFile, const CFmtReg& reg, bool bSilent)
//
/////////////////////////////////////////////////////////////////////////////////
bool CSpecialCell::Build(CSpecFile& specFile, const CFmtReg& reg, bool bSilent)
{
    CString sCmd;
    CIMSAString sArg;
    bool bResult = true;

    while (specFile.GetLine(sCmd, sArg) == SF_OK) {
        if (sCmd[0] != '[')  {
            if (sCmd.CompareNoCase(XTS_CMD_PANEL) == 0)  {
                if(!sArg.IsNumeric()){
                    m_sError.Format(_T("Unrecognized format label %s at line %d:"), (LPCTSTR)sArg, specFile.GetLineNumber());
                    m_sError += _T("\n") + sCmd + _T("=") + sArg;
                    m_sError += _T("\n ignoring special cell format");
                    AfxMessageBox(m_sError);
                    SetPanel(-1);
                }
                else {
                    SetPanel((int)sArg.Val());
                }
            }
            else if (sCmd.CompareNoCase(XTS_CMD_OFFSETCOL) == 0)  {
                if(!sArg.IsNumeric()){
                    m_sError.Format(_T("Unrecognized format label %s at line %d:"), (LPCTSTR)sArg, specFile.GetLineNumber());
                    m_sError += _T("\n") + sCmd + _T("=") + sArg;
                    m_sError += _T("\n ignoring special cell format");
                    AfxMessageBox(m_sError);
                    SetCol(-1);
                }
                else {
                    SetCol((int)sArg.Val());
                }
            }
            else if (sCmd.CompareNoCase(XTS_CMD_FORMAT)==0) {
                // find this format in the fmt registry...
                CString sIDString,sIndexString,sArgTemp;
                sArgTemp = sArg;
                sIDString = sArg.GetToken();
                sIndexString = sArg.GetToken();
                CFmt* pFmt=DYNAMIC_DOWNCAST(CFmt,reg.GetFmt(sIDString,sIndexString));
                SetFmt(pFmt);

                if (NULL==pFmt) {
                    m_sError.Format(_T("Unrecognized format label %s at line %d:"), (LPCTSTR)sArg, specFile.GetLineNumber());
                    m_sError += _T("\n") + sCmd + _T("=") + sArgTemp;
                    m_sError += _T("\n ignoring special cell format");
                    AfxMessageBox(m_sError);
                }
            }
            else {
                // Unrecognized command at line %d:
                if (!bSilent) {
                    m_sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                    m_sError += _T("\n") + sCmd + _T("=") + sArg;
                    AfxMessageBox(m_sError);
                }
                bResult = false;
            }
        }
        else {
            specFile.UngetLine();
            break;
        }
    }

    return bResult;
}
/////////////////////////////////////////////////////////////////////////////
//
//                                   CTabVar
//
/////////////////////////////////////////////////////////////////////////////
//
//                               CTabVar::CTabVar
//
/////////////////////////////////////////////////////////////////////////////

CTabVar::CTabVar()
{
    m_sVarName = _T("");
    m_VarType = VT_CUSTOM;
    m_pParent = NULL;
    m_iDictItemOcc = NONE;
//  m_bDisplayVar = true;
    m_pTallyFmt = NULL;
}

CTabVar::CTabVar(const DictValueSet* pVSet ,const CTallyFmt& tblTallyFmt)
{
    //Basic Init vars
    m_sVarName = _T("");
    m_VarType = VT_CUSTOM;
    m_pParent = NULL;
    m_iDictItemOcc = NONE;
//  m_bDisplayVar = true;
    m_pTallyFmt = NULL;
    //End Init vars
    m_sVarName = pVSet->GetName();
    SetText(pVSet->GetLabel());
    m_VarType = VT_DICT;

    // add tab vals for each of the statistics
    for (int iStat = 0; iStat < tblTallyFmt.GetStats().GetCount(); ++iStat) {
        tblTallyFmt.GetStats().GetAt(iStat)->GetTabVals(m_aTabValue, iStat, pVSet, NULL, tblTallyFmt.GetStats(), NULL);
    }
    for (int iVar = 0; iVar < m_aTabValue.GetCount(); ++iVar) {
        m_aTabValue.GetAt(iVar)->SetParentVar(this);
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CTabVar::CTabVar(CString sDictItemName,CStringArray& arrVals,const CTallyFmt& tblTallyFmt)
//
/////////////////////////////////////////////////////////////////////////////////
CTabVar::CTabVar(const CDictItem* pDictItem,CStringArray& arrVals,const CTallyFmt& tblTallyFmt)
{
    UNREFERENCED_PARAMETER(tblTallyFmt);
    // From what I can tell, this only gets called for system total
    // although once upon a time it may have been used in other cases
    // with new scheme for stats we should restrict it to system total only
    ASSERT(pDictItem->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) == 0);
    ASSERT(arrVals.IsEmpty());

    //Basic Init vars
    m_sVarName = _T("");
    m_VarType = VT_CUSTOM;
    m_pParent = NULL;
    m_iDictItemOcc = NONE;
//  m_bDisplayVar = true;
    m_pTallyFmt = NULL;
    //End Init vars
    CTabValue* val;
    m_sVarName = pDictItem->GetName();
    SetText(pDictItem->GetLabel());
    m_VarType = VT_DICT;


    // System total always has just total
    val = new CTabValue();
    val->SetText(_T("Total"));
    val->SetTabValType(STAT_TOTAL_TABVAL);
    val->SetParentVar(this);
    m_aTabValue.Add(val);
}

//Savy (R) sampling app 20081208
void CTabVar::Init(const CDictItem* pDictItem,CStringArray& arrVals,const CTallyFmt& tblTallyFmt)
{
    UNREFERENCED_PARAMETER(tblTallyFmt);

    // From what I can tell, this only gets called for system total
    // although once upon a time it may have been used in other cases
    // with new scheme for stats we should restrict it to system total only
    ASSERT(pDictItem->GetName().CompareNoCase(WORKVAR_STAT_NAME) == 0);
    ASSERT(arrVals.IsEmpty());

    //Basic Init vars
    m_sVarName = _T("");
    m_VarType = VT_CUSTOM;
    m_pParent = NULL;
    m_iDictItemOcc = NONE;
//  m_bDisplayVar = true;
    m_pTallyFmt = NULL;
    //End Init vars
    CTabValue* val;
    m_sVarName = pDictItem->GetName();
    SetText(pDictItem->GetLabel());
    m_VarType = VT_DICT;


    // System total always has just total
    val = new CTabValue();
    val->SetText(_T("R"));
    val->SetTabValType(STAT_SAMPLING_R_STABVAL);
    val->SetParentVar(this);
    m_aTabValue.Add(val);

    val = new CTabValue();
    val->SetText(_T("SE"));
    val->SetTabValType(STAT_SAMPLING_SE_STABVAL);
    val->SetParentVar(this);
    m_aTabValue.Add(val);

    val = new CTabValue();
    val->SetText(_T("N-UNWE"));
    val->SetTabValType(STAT_SAMPLING_N_UNWE_STABVAL);
    val->SetParentVar(this);
    m_aTabValue.Add(val);

    val = new CTabValue();
    val->SetText(_T("N-WEIG"));
    val->SetTabValType(STAT_SAMPLING_N_WEIG_STABVAL);
    val->SetParentVar(this);
    m_aTabValue.Add(val);

    //Savy (R) sampling app 20081209
    val = new CTabValue();
    val->SetText(_T("SER"));
    val->SetTabValType(STAT_SAMPLING_SER_STABVAL);
    val->SetParentVar(this);
    m_aTabValue.Add(val);

    val = new CTabValue();
    val->SetText(_T("SD"));
    val->SetTabValType(STAT_SAMPLING_SD_STABVAL);
    val->SetParentVar(this);
    m_aTabValue.Add(val);

    val = new CTabValue();
    val->SetText(_T("DEFT"));
    val->SetTabValType(STAT_SAMPLING_DEFT_STABVAL);
    val->SetParentVar(this);
    m_aTabValue.Add(val);

    val = new CTabValue();
    val->SetText(_T("ROH"));
    val->SetTabValType(STAT_SAMPLING_ROH_STABVAL);
    val->SetParentVar(this);
    m_aTabValue.Add(val);

    val = new CTabValue();
    val->SetText(_T("SE/R"));
    val->SetTabValType(STAT_SAMPLING_SE_R_STABVAL);
    val->SetParentVar(this);
    m_aTabValue.Add(val);

    val = new CTabValue();
    val->SetText(_T("R-2SE"));
    val->SetTabValType(STAT_SAMPLING_R_N2SE_STABVAL);
    val->SetParentVar(this);
    m_aTabValue.Add(val);

    val = new CTabValue();
    val->SetText(_T("R+2SE"));
    val->SetTabValType(STAT_SAMPLING_R_P2SE_STABVAL);
    val->SetParentVar(this);
    m_aTabValue.Add(val);

    val = new CTabValue();
    val->SetText(_T("SAMP_BASE"));
    val->SetTabValType(STAT_SAMPLING_SAMP_BASE_STABVAL);
    val->SetParentVar(this);
    m_aTabValue.Add(val);

    val = new CTabValue();
    val->SetText(_T("B"));
    val->SetTabValType(STAT_SAMPLING_B_STABVAL);
    val->SetParentVar(this);
    m_aTabValue.Add(val);
}

CTabVar::~CTabVar()
{
    while (m_aCrossVar.GetSize() > 0) {
        CTabVar* var = m_aCrossVar.GetAt(0);
        delete var;
        m_aCrossVar.RemoveAt(0);
    }
    while (m_aTabValue.GetSize() > 0) {
        CTabValue* val = m_aTabValue.GetAt(0);
        delete val;
        m_aTabValue.RemoveAt(0);
    }
    /*if(m_pTallyFmt){format registry will take care
//      delete m_pTallyFmt;
    }*/
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTabVar::AddChildVar
//
/////////////////////////////////////////////////////////////////////////////

void CTabVar::AddChildVar(CTabVar* pTabVar)
{
    pTabVar->SetParent(this);
    m_aCrossVar.Add(pTabVar);
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTabVar::InsertSibling
//
/////////////////////////////////////////////////////////////////////////////

void CTabVar::InsertSibling(CTabVar* pTabVar, bool bAfter /*= false*/)
{
    ASSERT(pTabVar != NULL);

    CTabVar* pParent = GetParent();
    for (int i = 0 ; i < pParent->GetNumChildren() ; i++) {
        if (pParent->GetChild(i) == this) {
            if (bAfter) {
                i++;
            }
            pTabVar->SetParent(pParent);
            pParent->m_aCrossVar.InsertAt(i, pTabVar);
            break;
        }
    }
}

void CTabVar::UpdateFmtFlag()
{
    //Caption format
    CFmtBase* pFmtBase = GetFmt();
    if(pFmtBase!=NULL){
        pFmtBase->SetUsedFlag(true);
    }
    //Tally format mean, mode..
    CTallyFmt* pTallyFmt = GetTallyFmt();
    if(pTallyFmt!=NULL){
        pTallyFmt->SetUsedFlag(true);

    }
    //To set the row/col format
    for(int iValue = 0; iValue < GetArrTabVals().GetCount();++iValue){
        CTabValue* pTabVal = GetArrTabVals().GetAt(iValue);
        CFmtBase* pFmtBase = pTabVal->GetFmt();
        if(pFmtBase!=NULL){
            pFmtBase->SetUsedFlag(true);
        }
        //Special cell
        for(int iCell = 0; iCell < pTabVal->GetSpecialCellArr().GetCount();++iCell){
            CSpecialCell arrSpecialCell = pTabVal->GetSpecialCellArr().GetAt(iCell);
            CFmtBase* pFmtBase = arrSpecialCell.GetFmt();
            if(pFmtBase!=NULL && pFmtBase->GetIndex()!=0){
                pFmtBase->SetUsedFlag(true);
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabVar::ReconcileName (const CString& sOldName, const CString& sNewName)
//// change the unique name, m_sVarName, for the current tabvar and all its
// children underneath (recurse!)
/////////////////////////////////////////////////////////////////////////////////
bool CTabVar::ReconcileName (const CString& sOldName, const CString& sNewName)
{
    bool bRet = false;

    if (GetName() == sOldName) {
        SetName (sNewName);
        bRet = true;
    }

    CTabVar* pChild = NULL;

    for (int i=0; i < GetNumChildren(); i++) {

        pChild = GetChild (i);

        bRet = bRet | pChild->ReconcileName (sOldName, sNewName);
    }

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTabVar::Remove
//
/////////////////////////////////////////////////////////////////////////////

void CTabVar::Remove()
{
    ASSERT(m_pParent != NULL);
    for (int i = 0 ; i < m_pParent->GetNumChildren() ; i++) {
        if (m_pParent->GetChild(i) == this) {
            m_pParent->m_aCrossVar.RemoveAt(i);
            break;
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTabVar::CutVar
//
/////////////////////////////////////////////////////////////////////////////


CTabVar* CTabVar::CutVar(const int iIndex)
{
    ASSERT(iIndex >=0);
    CTabVar* pTabVar = m_aCrossVar[iIndex];
    m_aCrossVar.RemoveAt(iIndex);
    return pTabVar;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabVar::RemoveAllPrtViewInfoRecursive()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabVar::RemoveAllPrtViewInfoRecursive()
{
    RemoveAllPrtViewInfo();
    for(int iIndex =0; iIndex < GetNumChildren(); iIndex++){
        GetChild(iIndex)->RemoveAllPrtViewInfoRecursive();
    }
    for(int iTabVal =0; iTabVal < GetNumValues(); iTabVal++){
        GetValue(iTabVal)->RemoveAllPrtViewInfo();
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTabVar::GetTotValues
//
/////////////////////////////////////////////////////////////////////////////


int CTabVar::GetTotValues(bool bIgnoreRdrBrks /*= false*/){
    int iNum = 0;

    if (GetNumChildren() > 0) {
        for (int i = 0 ; i < GetNumChildren() ; i++) {
            iNum += GetChild(i)->GetTotValues();
        }
        iNum *= GetNumValues(bIgnoreRdrBrks);
    }
    else {
        iNum = GetNumValues(bIgnoreRdrBrks);
    }

    return iNum;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTabVar::Build
//
/////////////////////////////////////////////////////////////////////////////


bool CTabVar::Build(CSpecFile& specFile, const CFmtReg& reg, const CString& sVersion, bool bSilent)
{
    CString sCmd;
    CIMSAString sArg;
    bool bResult = true;

    while (specFile.GetLine(sCmd, sArg) == SF_OK) {
        if (sCmd[0] != '[')  {
            if (sCmd.CompareNoCase(XTS_CMD_NAME) == 0)  {
                SetName(sArg);
            }
            else if (sCmd.CompareNoCase(XTS_CMD_LABEL) == 0)  {
                SetText(TableLabelSerializer::Parse(sArg, sVersion));
            }
            else if (sCmd.CompareNoCase(XTS_CMD_ITM_OCC) == 0)  {
                sArg.Trim();
                if(!sArg.IsEmpty()){
                    m_iDictItemOcc = (int) sArg.Val();
                    m_iDictItemOcc--; //Make it zero based for using internally
                    if(m_iDictItemOcc < -1 ){
                        m_iDictItemOcc = -1;
                    }
                }
            }
            else if (sCmd.CompareNoCase(XTS_CMD_VARTYPE) == 0)  {
                if (sArg.CompareNoCase(XTS_ARG_DICT) == 0) {
                    m_VarType = VT_DICT;
                }
                else if (sArg.CompareNoCase(XTS_ARG_SOURCE) == 0) {
                    m_VarType = VT_SOURCE;
                }
                else if (sArg.CompareNoCase(XTS_ARG_CUSTOM) == 0) {
                    m_VarType = VT_CUSTOM;
                }
                else {
                    m_VarType = VT_CUSTOM;
                    if (!bSilent) {
                        m_sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                        m_sError += _T("\n") + sCmd + _T("=") + sArg;
                        AfxMessageBox(m_sError);
                    }
                    bResult = false;
                }
            }
             else if (sCmd.CompareNoCase(XTS_CMD_FORMAT)==0) {
                // find this format in the fmt registry...
                CString sIDString,sIndexString;
                CFmtBase* pFmt=NULL;
                sIDString = sArg.GetToken();
                sIndexString = sArg.GetToken();
                if (CIMSAString::IsNumeric(sIndexString)) {
                    pFmt=DYNAMIC_DOWNCAST(CFmtBase,reg.GetFmt(sIDString,sIndexString));
                }
                if (NULL==pFmt) {
                    m_sError.Format(_T("Unrecognized format label %s at line %d:"), (LPCTSTR)sArg, specFile.GetLineNumber());
                    m_sError += _T("\n") + sCmd + _T("=") + sArg;
                    m_sError += _T("\nUsing default format instead.");
                    AfxMessageBox(m_sError);
                }
                else {
                  CTallyFmt* pTallyFmt=DYNAMIC_DOWNCAST(CTallyFmt,pFmt);
                  CFmt* pVarFmt=DYNAMIC_DOWNCAST(CFmt,pFmt);
                  if(pTallyFmt){
                      SetTallyFmt(pTallyFmt);
                  }
                  else {
                      ASSERT(pVarFmt);
                      SetFmt(pVarFmt);
                  }
                }
            }
            else if (sCmd.CompareNoCase(XTS_CMD_TOTTYPE) == 0)  {
                if (sArg.CompareNoCase(XTS_ARG_COUNT) == 0) {
                   // m_TotalType = TT_COUNT;
                }
                else if (sArg.CompareNoCase(XTS_ARG_PERCENT) == 0) {
                 //   m_TotalType = TT_PERCENT;
                }
                else if (sArg.CompareNoCase(XTS_ARG_BOTH) == 0) {
                  //  m_TotalType = TT_BOTH;
                }
                else {
                  //  m_TotalType = TT_COUNT;
                    if (!bSilent) {
                        m_sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                        m_sError += _T("\n") + sCmd + _T("=") + sArg;
                        AfxMessageBox(m_sError);
                    }
                    bResult = false;
                }
            }
            else if (sCmd.CompareNoCase(XTS_CMD_TOTPOS) == 0)  {
                if (sArg.CompareNoCase(XTS_ARG_NONE) == 0) {
//                    m_TotalPos = TP_NONE;
                }
                else if (sArg.CompareNoCase(XTS_ARG_BEFORE) == 0) {
        //            m_TotalPos = TP_BEFORE;
                }
                else if (sArg.CompareNoCase(XTS_ARG_AFTER) == 0) {
              //      m_TotalPos = TP_AFTER;
                }
                else {
                  //  m_TotalPos = TP_AFTER;
                    if (!bSilent) {
                        m_sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                        m_sError += _T("\n") + sCmd + _T("=") + sArg;
                        AfxMessageBox(m_sError);
                    }
                    bResult = false;
                }
            }
            else if (sCmd.CompareNoCase(XTS_CMD_PERTYPE) == 0)  {
                if (sArg.CompareNoCase(XTS_ARG_COL) == 0) {
//                    m_PercentType = PT_COL;
                }
                else if (sArg.CompareNoCase(XTS_ARG_ROW) == 0) {
                //    m_PercentType = PT_ROW;
                }
                else if (sArg.CompareNoCase(XTS_ARG_TOTAL) == 0) {
                 //   m_PercentType = PT_TOTAL;
                }
                else {
                 //   m_PercentType = PT_COL;
                    if (!bSilent) {
                        m_sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                        m_sError += _T("\n") + sCmd + _T("=") + sArg;
                        AfxMessageBox(m_sError);
                    }
                    bResult = false;
                }
            }
            else if (sCmd.CompareNoCase(XTS_CMD_PERPOS) == 0)  {
                if (sArg.CompareNoCase(XTS_ARG_RIGHT) == 0) {
                 //   m_PercentPos = PP_RIGHT;
                }
                else if (sArg.CompareNoCase(XTS_ARG_LEFT) == 0) {
                  //  m_PercentPos = PP_LEFT;
                }
                else if (sArg.CompareNoCase(XTS_ARG_ABOVE) == 0) {
                  //  m_PercentPos = PP_ABOVE;
                }
                else if (sArg.CompareNoCase(XTS_ARG_BELOW) == 0) {
                  //  m_PercentPos = PP_BELOW;
                }
                else {
                //    m_PercentPos = PP_RIGHT;
                    if (!bSilent) {
                        m_sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                        m_sError += _T("\n") + sCmd + _T("=") + sArg;
                        AfxMessageBox(m_sError);
                    }
                    bResult = false;
                }
            }
            else if (sCmd.CompareNoCase(XTS_CMD_STATS) == 0)  {
                CString sArgSave = sArg;
                while (sArg.GetLength() > 0) {
                    CString sStat = sArg.GetToken();
                    if (sStat.CompareNoCase(XTS_ARG_MIN) == 0) {
//                        m_bMin = true;
                    }
                    else if (sStat.CompareNoCase(XTS_ARG_MAX) == 0) {
                  //      m_bMax = true;
                    }
                    else if (sStat.CompareNoCase(XTS_ARG_STD) == 0) {
                     //   m_bStd = true;
                    }
                    else if (sStat.CompareNoCase(XTS_ARG_VAR) == 0) {
                     //   m_bVar = true;
                    }
                    else if (sStat.CompareNoCase(XTS_ARG_MEAN) == 0) {
                      //  m_bMean = true;
                    }
                    else if (sStat.CompareNoCase(XTS_ARG_MEDIAN) == 0) {
                     //   m_bMedian = true;
                    }
                    else if (sStat.CompareNoCase(XTS_ARG_MODE) == 0) {
                     //   m_bMode = true;
                    }
                    else {
                        if (!bSilent) {
                            m_sError.Format(_T("Unrecognized statistic at line %d:"), specFile.GetLineNumber());
                            m_sError += _T("\n") + sCmd + _T("=") + sArgSave;
                            m_sError += _T("\nStatistic = ") + sStat;
                            AfxMessageBox(m_sError);
                        }
                        bResult = false;
                    }
                }
            }
            else if (sCmd.CompareNoCase(XTS_CMD_NTILES) == 0)  {
               /* m_iTiles = (int) sArg.Val();
                if (m_iTiles < 3 || m_iTiles > 10) {
                    if (!bSilent) {
                        m_sError.Format("Invalid nTiles (must be between 3 to 10) at line %d:", specFile.GetLineNumber());
                        m_sError += "\n" + sCmd + "=" + sArg;
                        AfxMessageBox(m_sError);
                    }
                    m_iTiles = 0;
                    bResult = false;
                }*/
            }
            else if (sCmd.CompareNoCase(XTS_ARG_PRNTVSZ) == 0 || sCmd.CompareNoCase(XTS_ARG_GRIDVSZ) == 0 )  {
                specFile.UngetLine();
                this->BuildStateInfo(specFile,bSilent);
            }

            else {
                // Unrecognized command at line %d:
                if (!bSilent) {
                    m_sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                    m_sError += _T("\n") + sCmd + _T("=") + sArg;
                    AfxMessageBox(m_sError);
                }
                bResult = false;
            }
        }
        else {
            if(sCmd.CompareNoCase(XTS_SECT_TALLYFMT)==0){
                ASSERT(m_pTallyFmt);
                m_pTallyFmt = new CTallyFmt();
                m_pTallyFmt->Build(specFile, bSilent);
            }
            else if (sCmd.CompareNoCase(XTS_SECT_VAR) == 0) {
                CTabVar* pVar = new CTabVar();
                pVar->SetParent(this);
                pVar->Build(specFile, reg, sVersion, bSilent);
                AddChildVar(pVar);
            }
            else if (sCmd.CompareNoCase(XTS_SECT_ENDVAR) == 0) {
                break;
            }
            else if (sCmd.CompareNoCase(XTS_SECT_VALUE) == 0) {
                CTabValue* pValue = new CTabValue();
                pValue->Build(specFile, reg, sVersion, bSilent);
                m_aTabValue.Add(pValue);
            }
            else {
                m_sError.Format(_T("Invalid section heading at line %d:"), specFile.GetLineNumber()); // Invalid section heading at line %d:
                m_sError += _T("\n") + sCmd;
                if (!bSilent) {
                    AfxMessageBox(m_sError);
                    specFile.SkipSection();
                }
                bResult = false;
            }
        }
    }

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTabVar::Save
//
/////////////////////////////////////////////////////////////////////////////


void CTabVar::Save(CSpecFile& specFile)
{
    CString sValue;
    specFile.PutLine (_T(" "));
    specFile.PutLine (XTS_SECT_VAR);
    specFile.PutLine (XTS_CMD_NAME, m_sVarName);
    specFile.PutLine (XTS_CMD_LABEL, TableLabelSerializer::Create(m_sText));
    if(m_iDictItemOcc > -1) {
        CIMSAString sItemOcc;
        sItemOcc.Str(m_iDictItemOcc+1);//Make it one based for saving
        specFile.PutLine (XTS_CMD_ITM_OCC, sItemOcc);
    }
    // Save Var Type
    if (m_VarType == VT_DICT) {
        sValue = XTS_ARG_DICT;
    }
    else if (m_VarType == VT_SOURCE) {
        sValue = XTS_ARG_SOURCE;
    }
    else {
        sValue = XTS_ARG_CUSTOM;
    }
    specFile.PutLine (XTS_CMD_VARTYPE, sValue);

    SaveStateInfo(specFile);

    //save the table fmt if  it is not default
    CFmt* pFmt=DYNAMIC_DOWNCAST(CFmt,GetFmt());
    if (NULL!=pFmt) {
        bool bCustomFmt=CFmtReg::IsCustomFmtID(*pFmt);
        if(bCustomFmt){
//          CString sFmt ;
//          sFmt = pFmt->GetIDString()+ "," + pFmt->GetIndexString();
//          specFile.PutLine(XTS_CMD_FORMAT, sFmt);
            specFile.PutLine(XTS_CMD_FORMAT, pFmt->GetIDInfo());
        }
    }
    //Save the table tallyfmt if it is not default
    if (NULL!=m_pTallyFmt) {
        bool bCustomFmt=CFmtReg::IsCustomFmtID(*m_pTallyFmt);
        if(bCustomFmt){
//          CString sFmt ;
//          sFmt = m_pTallyFmt->GetIDString()+ "," + m_pTallyFmt->GetIndexString();
//          specFile.PutLine(XTS_CMD_FORMAT, sFmt);
            specFile.PutLine(XTS_CMD_FORMAT, m_pTallyFmt->GetIDInfo());
        }
    }

    // Save Values
    for (int v = 0 ; v < m_aTabValue.GetSize() ; v++) {
        m_aTabValue[v]->Save(specFile);
    }
    // Save Children
    for (int c = 0 ; c < GetNumChildren() ; c++) {
        GetChild(c)->Save(specFile);
    }

    specFile.PutLine(_T(" "));
    specFile.PutLine(XTS_SECT_ENDVAR);
}


/////////////////////////////////////////////////////////////////////////////
//
//                                   CTabData
//
/////////////////////////////////////////////////////////////////////////////
//
//                               CTabData::CTabData
//
/////////////////////////////////////////////////////////////////////////////
CTabData::CTabData()
{
    m_sTableName = _T("");
    m_sAreaLevel = _T("");
    m_sAreaLabel = _T("");
    m_sAreaLevel = _T("");
    m_aAreaCodes.RemoveAll();
    m_iRows = 0;
    m_iCols = 0;
    m_lRecodedCluster = -1;
    m_aDataCells.RemoveAll();
}

CTabData::~CTabData()
{
    m_aAreaCodes.RemoveAll();
    m_aDataCells.RemoveAll();
}



/////////////////////////////////////////////////////////////////////////////
//
//                               CTabData::Build
//
/////////////////////////////////////////////////////////////////////////////

bool CTabData::Build(CSpecFile& specFile, bool bSilent /*= false*/)
{
    CString sCmd;
    CIMSAString sArg;
    CIMSAString sCell;
    double iData;
    bool bResult = true;
    while (specFile.GetLine(sCmd, sArg,false) == SF_OK) {
        sCmd.Trim();
        CString sTemp = sArg;
        sTemp.Trim();
        bool bBlank = sTemp.CompareNoCase(_T("")) == 0 && sCmd.CompareNoCase(_T("")) ==0;
        if (sCmd[0] != '[')  {
            if (sCmd.CompareNoCase(XTS_CMD_TABLENAME) == 0)  {
                sArg.Trim();
                SetTableName(sArg);
            }
            else if (sCmd.CompareNoCase(XTS_CMD_SIZE) == 0)  {
                sArg.Trim();
                CIMSAString sTemp;
                sTemp = sArg.GetToken();
                m_iRows = (int) sTemp.Val();
                sTemp = sArg.GetToken();
                m_iCols = (int) sTemp.Val();
            }
            else if(sCmd.CompareNoCase(XTS_CMD_AREANAME) == 0)  {
                SetAreaLabel(sArg);
            }
            else if(sCmd.CompareNoCase(XTS_CMD_BREAKKEY) == 0)  {
                sArg.Trim();
                SetBreakKey(sArg);
            }
            else if (sCmd.CompareNoCase(XTS_CMD_ROW) == 0)  {
                sArg.Trim();
                while (!sArg.IsEmpty())  {
                    sCell = sArg.GetToken(_T(" "));    // only space separators because of comma decimal
                    if (!sCell.IsNumericU())  {    // BMD 21 Jun 2006
                        if (!bSilent) {
                            CString sError;
                            sError.Format(_T("Data cell is not numeric:  %s"), (LPCTSTR)sCell);
                            AfxMessageBox(sError);
                        }
                        bResult = false;
                    }
                    iData = (double) sCell.fVal();
                    //CDataCell d(iData);
                    m_aDataCells.Add(iData);
                }
          }
        else if (bBlank){//do nothing
        }
        else {
                if (!bSilent) {
                    sArg.Trim();
                    m_sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                    m_sError += _T("\n") + sCmd + _T("=") + sArg;
                    AfxMessageBox(m_sError);
                }
                bResult = false;
            }
        }
        else {
            specFile.UngetLine();
            break;
        }
    }
    if (m_aDataCells.GetSize() != m_iRows * m_iCols) {
        if (!bSilent) {
            m_sError.Format(_T("Number of cells not = rows x cols\nCells = %d   Rows = %d   Cols = %d"),
                m_aDataCells.GetSize(), m_iRows, m_iCols);
            m_sError += _T("\n") + sCmd + _T("=") + sArg;
            AfxMessageBox(m_sError);
        }
        bResult = false;
    }
    return bResult;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTabData::Save
//
/////////////////////////////////////////////////////////////////////////////

void CTabData::Save(CSpecFile& specFile)
{
    specFile.PutLine(_T(" "));

    specFile.PutLine(XTS_SECT_CELLS);
    specFile.PutLine(XTS_CMD_TABLENAME,m_sTableName);

    if(m_sBreakKey.GetLength() >0){
       // specFile.PutLine(XTS_SECT_AREA);
      //  specFile.PutLine(XTS_CMD_AREALEVEL,m_sAreaLevel);
        specFile.PutLine(XTS_CMD_AREANAME,m_sAreaLabel);
        specFile.PutLine(XTS_CMD_BREAKKEY,m_sBreakKey);
        //specFile.PutLine(XTS_CMD_AREACODES);
    }

    CIMSAString sRows, sCols;
    sRows.Str(m_iRows);
    sCols.Str(m_iCols);
    specFile.PutLine(XTS_CMD_SIZE,sRows + _T(",") + sCols);
    // Put out data cells row by row
    CString s;
    csprochar pszTemp[10];
    GetPrivateProfileString(_T("intl"), _T("sDecimal"), _T("."), pszTemp, 10, _T("WIN.INI"));
    csprochar cDecimal = pszTemp[0];
    int i = 0;
    for (int r = 0 ; r < m_iRows ; r++) {
        int iLen = _tcslen(XTS_CMD_ROW)+3;
        CString sData;
        for (int c = 0 ; c < m_iCols; c++)  {
            csprochar pszTemp[30];
            s = dtoa(m_aDataCells[i++], pszTemp, 9, cDecimal, false);
            if (iLen + s.GetLength() + 10 > MAX_XTS_LINE)  {
                sData = sData.Left(sData.GetLength()-2);  // strip the trailing   ", " [here because blocks can screw us up inside the loop]
                specFile.PutLine(XTS_CMD_ROW, sData);
                sData.Empty();
                iLen = _tcslen(XTS_CMD_ROW)+3;
            }
            sData += s + _T("  ");        // don't us commas as separators because of comma decimal
            iLen += s.GetLength() + 2;
        }
        //ASSERT(sData.GetLength() > 2);
        if(sData.GetLength() > 2)
            sData = sData.Left(sData.GetLength()-2);  // strip the trailing   ", " [here because blocks can screw us up inside the loop]
        specFile.PutLine(XTS_CMD_ROW, sData);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                                   CTable
//
/////////////////////////////////////////////////////////////////////////////
//
//                               CTable::CTable
//
/////////////////////////////////////////////////////////////////////////////

CTable::CTable()
{
    m_sNum = _T("");
    m_sName = _T("");
    m_iBreakLevel = -1;
    m_pRowRoot = new CTabVar();
    m_pColRoot = new CTabVar();
    m_dFrqMean =0;
    m_dFrqMinCode =0;
    m_dFrqMaxCode =0;
    m_dFrqModeCount=0 ;
    m_dFrqModeCode =0;
    m_dFrqMedianCode =0;
    m_dFrqMedianInt=0 ;
    m_dFrqStdDev =0;
    m_dFrqVariance=0 ;
    m_iTotalCategories=0;
    m_bAlphaFreqStats = false;
    m_bSaveFreqStats = false;
    m_bHasFreqStats = false;
    m_bHasFreqNTiles = false;
    m_bDirty = false;
    m_pTblPrintFmt=NULL;
    m_pFmtReg=NULL;
    m_tAreaCaption.SetText(AREA_TOKEN);
    m_bGenerateLogic = true;
    m_bExcludeForRun = false;
}

CTable::CTable(const CString& sNum)
{
    ASSERT(!SO::IsBlank(sNum));
    m_sNum = sNum;
    m_sName = CIMSAString::MakeName(_T("TABLE") + sNum);
    m_iBreakLevel = -1;
    m_pRowRoot = new CTabVar();
    m_pColRoot = new CTabVar();

    m_dFrqMean =0;
    m_dFrqMinCode =0;
    m_dFrqMaxCode =0;
    m_dFrqModeCount=0 ;
    m_dFrqModeCode =0;
    m_dFrqMedianCode =0;
    m_dFrqMedianInt=0 ;
    m_dFrqStdDev =0;
    m_dFrqVariance=0 ;
    m_iTotalCategories=0;
    m_bAlphaFreqStats = false;
    m_bSaveFreqStats = false;
    m_bHasFreqStats = false;
    m_bHasFreqNTiles = false;
    m_bDoSaveinTBW = true;
    m_bDirty = false;
    m_pTblPrintFmt=NULL;
    m_pFmtReg=NULL;
    GenerateTitle();
    m_tAreaCaption.SetText(AREA_TOKEN);
    m_bGenerateLogic = true;
    m_bExcludeForRun = false;
}

CTable::~CTable()
{
    delete m_pRowRoot;
    delete m_pColRoot;
    RemoveAllData();
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTable::RemoveAllData
//
/////////////////////////////////////////////////////////////////////////////
void CTable::RemoveAllData()
{
    CTabData* pTabData = NULL;
    for(int iIndex = 0; iIndex < m_aTabData.GetSize();iIndex++) {
        pTabData = m_aTabData.GetAt(iIndex);
        delete pTabData;

    }
    m_aTabData.RemoveAll();
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTable::GenerateTitle
//
/////////////////////////////////////////////////////////////////////////////


void CTable::GenerateTitle()
{
    // NOTE: Need to generalize to more than two levels or cross!!!
    //
    bool bHasSystemTotalInRow =false;
    bool bHasSystemTotalInCol = false;
    //Savy (R) sampling app 20081210
    bool bHasSystemStatRow = false;

    if (m_pRowRoot->GetNumChildren() > 0 && m_pColRoot->GetNumChildren() > 0) {
        if(m_pRowRoot->GetChild(0)->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) == 0){
            bHasSystemTotalInRow = true;
        }
        if(m_pColRoot->GetNumChildren() > 0 ){
            if(m_pColRoot->GetChild(0)->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) == 0){
                bHasSystemTotalInCol = true;
            }
        }
    //Savy (R) sampling app 20081210
        if(m_pColRoot->GetNumChildren() > 0 ){
            if(m_pColRoot->GetChild(0)->GetName().CompareNoCase(WORKVAR_STAT_NAME) == 0){
                bHasSystemStatRow = true;
            }
        }
    }
    CFmt* pFmt=DYNAMIC_DOWNCAST(CFmt,m_tTitle.GetFmt());
    if (pFmt && pFmt->IsTextCustom()) {
        return;
    }
    CString sAnd(_T(" and "));
    CString sTable(_T("Table "));
    CString sBy(_T(" by "));
    CString sTitle;
    if(m_pFmtReg) {
        CTabSetFmt* pTabSetFmt = DYNAMIC_DOWNCAST(CTabSetFmt,m_pFmtReg->GetFmt(FMT_ID_TABSET));
        if(pTabSetFmt){
            //CString sTitle = _T("Table ") + m_sNum + _T(" ");
            sTitle.Format(pTabSetFmt->GetTitleTemplate(), (LPCTSTR)m_sNum);
            sTitle += _T(" ");
            const FOREIGN_KEYS& altForeign = pTabSetFmt->GetAltForeignKeys();
            sTable = CString(altForeign.GetKey(_T("Table"))) + _T(" ");
            sAnd = _T(" ") + CString(altForeign.GetKey(_T("and"))) + _T(" ");
            sBy = _T(" ") + CString(altForeign.GetKey(_T("by"))) + _T(" ");
        }
    }
    else {
        sTitle = sTable + m_sNum + _T(" ");
    }
    // Row variable names
    for (int r = 0 ; r < m_pRowRoot->GetNumChildren() ; r++) {
        if (r > 0) {
            sTitle += _T(", ");
        }
        CTabVar* pTabVar = m_pRowRoot->GetChild(r);
        if(pTabVar->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) == 0){
            continue;
        }
        //Savy (R) sampling app 20081210
        if(pTabVar->GetName().CompareNoCase(WORKVAR_STAT_NAME) == 0){
            continue;
        }
        sTitle += pTabVar->GetText();
        for (int cr = 0 ; cr < pTabVar->GetNumChildren() ; cr++) {
            CTabVar* pCrossVar = pTabVar->GetChild(cr);
            if (cr == 0) {
                sTitle += sAnd;
            }
            else {
                sTitle += _T(", ");
            }
            sTitle += pCrossVar->GetText();
        }
    }
    // By
    if (m_pRowRoot->GetNumChildren() > 0 && m_pColRoot->GetNumChildren() > 0) {
        //Savy (R) sampling app 20081210
        //if(!(bHasSystemTotalInRow || bHasSystemTotalInCol)){
        if(!(bHasSystemTotalInRow || bHasSystemTotalInCol || bHasSystemStatRow)){
            sTitle += sBy;
        }
    }
    // Column variable names
    for (int c = 0 ; c < m_pColRoot->GetNumChildren() ; c++) {
        if (c > 0) {
            sTitle += _T(", ");
        }
        CTabVar* pTabVar = m_pColRoot->GetChild(c);
        if(pTabVar->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) == 0){
            continue;
        }
        //Savy (R) sampling app 20081210
        if(pTabVar->GetName().CompareNoCase(WORKVAR_STAT_NAME) == 0){
            continue;
        }
        sTitle += pTabVar->GetText();
        for (int cc = 0 ; cc < pTabVar->GetNumChildren() ; cc++) {
            CTabVar* pCrossVar = pTabVar->GetChild(cc);
            if (cc == 0) {
                sTitle += sAnd;
            }
            else {
                sTitle += _T(", ");
            }
            sTitle += pCrossVar->GetText();
        }
    }
    m_tTitle.SetText(sTitle);
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTable::Build
//
/////////////////////////////////////////////////////////////////////////////
bool CTable::Build(CSpecFile& specFile, const CFmtReg& reg, const CString& sVersion, bool bSilent)
{
    CString sCmd;
    CIMSAString sArg;
    bool bResult = true;

    while (specFile.GetLine(sCmd, sArg) == SF_OK) {
        if (sCmd[0] != '[')  {
            if (sCmd.CompareNoCase(XTS_CMD_NAME) == 0)  {
                SetName(sArg);
            }
            /*else if (sCmd.CompareNoCase(XTS_CMD_TITLE) == 0)  {
                m_tTitle.SetText(sArg);
            }*/
            else if (sCmd.CompareNoCase(XTS_CMD_FORMAT) == 0)  {
                // title format ... find it in the fmt registry...
                CString sIDString,sIndexString;
                sIDString = sArg.GetToken();
                sIndexString = sArg.GetToken();
                CTblFmt* pTblFmt=DYNAMIC_DOWNCAST(CTblFmt,reg.GetFmt(sIDString,sIndexString));
                CTblPrintFmt* pTblPrintFmt=DYNAMIC_DOWNCAST(CTblPrintFmt,reg.GetFmt(sIDString,sIndexString));
                if(pTblFmt){
                    SetFmt(pTblFmt);
                }
                else {
                    SetTblPrintFmt(pTblPrintFmt);
                }
                if (!pTblFmt && !pTblPrintFmt) {
                    m_sError.Format(_T("Unrecognized format label %s at line %d:"), (LPCTSTR)sArg, specFile.GetLineNumber());
                    m_sError += _T("\n") + sCmd + _T("=") + sArg;
                    m_sError += _T("\nUsing default format instead.");
                    AfxMessageBox(m_sError);
                }
            }
            else if (sCmd.CompareNoCase(XTS_CMD_NUM) == 0)  {
                m_sNum = sArg;
            }
            else if(sCmd.CompareNoCase(XTS_CMD_TBL_GENLOGIC) == 0) {
                if((sArg.CompareNoCase(XTS_ARG_NO)==0)){
                    m_bGenerateLogic = false;
                }
            }
            else if(sCmd.CompareNoCase(XTS_CMD_TBL_EXCLUDE_FOR_RUN) == 0) {
                if((sArg.CompareNoCase(XTS_ARG_YES)==0)){
                    m_bExcludeForRun = true;
                }
            }
            else if (sCmd.CompareNoCase(XTS_CMD_BREAKLEVEL) == 0)  {
                m_iBreakLevel = (int) sArg.Val();
            }
            else {
                // Unrecognized command at line %d:
                if (!bSilent) {
                    m_sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                    m_sError += _T("\n") + sCmd + _T("=") + sArg;
                    AfxMessageBox(m_sError);
                }
                bResult = false;
            }
        }
        else {
            if (sCmd.CompareNoCase(XTS_SECT_TITLE)==0) {
                GetTitle()->Build(specFile, reg, sVersion);
            }
            else if (sCmd.CompareNoCase(XTS_SECT_SUBTITLE)==0) {
                GetSubTitle()->Build(specFile, reg, sVersion);
            }
            else if (sCmd.CompareNoCase(XTS_SECT_PAGE_NOTE)==0) {
                GetPageNote()->Build(specFile, reg, sVersion);
            }
            else if (sCmd.CompareNoCase(XTS_SECT_END_NOTE)==0) {
                GetEndNote()->Build(specFile, reg, sVersion);
            }
            else if (sCmd.CompareNoCase(XTS_SECT_HEADER_LEFT)==0) {
                GetHeader(0)->Build(specFile, reg, sVersion);
            }
            else if (sCmd.CompareNoCase(XTS_SECT_HEADER_CENTER)==0) {
                GetHeader(1)->Build(specFile, reg, sVersion);
            }
            else if (sCmd.CompareNoCase(XTS_SECT_HEADER_RIGHT)==0) {
                GetHeader(2)->Build(specFile, reg, sVersion);
            }
            else if (sCmd.CompareNoCase(XTS_SECT_FOOTER_LEFT)==0) {
                GetFooter(0)->Build(specFile, reg, sVersion);
            }
            else if (sCmd.CompareNoCase(XTS_SECT_FOOTER_CENTER)==0) {
                GetFooter(1)->Build(specFile, reg, sVersion);
            }
            else if (sCmd.CompareNoCase(XTS_SECT_FOOTER_RIGHT)==0) {
                GetFooter(2)->Build(specFile, reg, sVersion);
            }
            else if (sCmd.CompareNoCase(XTS_SECT_STUBHEAD)==0) {
                GetStubhead(0)->Build(specFile, reg, sVersion);
            }
            else if (sCmd.CompareNoCase(XTS_SECT_AREA_CAPTION)==0) {
                GetAreaCaption()->Build(specFile, reg, sVersion);
            }
            else if (sCmd.CompareNoCase(XTS_SECT_ONEROWCOL_TOTAL)==0) {
                GetOneRowColTotal()->Build(specFile, reg, sVersion);
            }
            else if (sCmd.CompareNoCase(XTS_SECT_AREA_CAPTION)==0) {
                GetAreaCaption()->Build(specFile, reg, sVersion);
            }
            else if (sCmd.CompareNoCase(XTS_SECT_STUBHEAD_SEC)==0) {
                GetStubhead(1)->Build(specFile, reg, sVersion);
            }
            else if (sCmd.CompareNoCase(XTS_SECT_TABLE) == 0 || sCmd.CompareNoCase(XTS_SECT_LEVEL) == 0) {
                specFile.UngetLine();
                break;
            }
            else if (sCmd.CompareNoCase(XTS_SECT_ROWS) == 0) {
                while (specFile.GetLine(sCmd, sArg) == SF_OK) {
                    if (sCmd.CompareNoCase(XTS_SECT_VAR) == 0)  {
                        CTabVar* pVar = new CTabVar();
                        pVar->SetParent(m_pRowRoot);
                        pVar->Build(specFile, reg, sVersion, bSilent);
                        m_pRowRoot->AddChildVar(pVar);
                    }
                    else {
                        specFile.UngetLine();
                        break;
                    }
                }
            }
            else if (sCmd.CompareNoCase(XTS_SECT_COLS) == 0) {
                while (specFile.GetLine(sCmd, sArg) == SF_OK) {
                    if (sCmd.CompareNoCase(XTS_SECT_VAR) == 0)  {
                        CTabVar* pVar = new CTabVar();
                        pVar->SetParent(m_pColRoot);
                        pVar->Build(specFile, reg, sVersion, bSilent);
                        m_pColRoot->AddChildVar(pVar);
                    }
                    else {
                        specFile.UngetLine();
                        break;
                    }
                }
            }
            else if (sCmd.CompareNoCase(XTS_SECT_UNITSPECS) == 0) {
                while (specFile.GetLine(sCmd, sArg) == SF_OK) {
                    if (sCmd.CompareNoCase(XTS_SECT_UNIT) == 0)  {
                        CUnitSpec unitSpec;
                        unitSpec.Build(specFile, bSilent);
                        if(unitSpec.GetSubTableString().CompareNoCase(_T("Entire Table")) ==0){
                            m_tableUnit = unitSpec;
                        }else {
                            m_aUnitSpec.Add(unitSpec);
                        }
                    }
                    else if(sCmd.CompareNoCase(XTS_SECT_ENDUNITSPECS) == 0){
                        break; //Done with the unitspec build
                    }
                    else {
                        specFile.UngetLine();
                        break;
                    }
                }
            }
            else if (sCmd.CompareNoCase(XTS_SECT_DATA) == 0) {
                while (specFile.GetLine(sCmd, sArg) == SF_OK) {
                    if (sCmd.CompareNoCase(XTS_SECT_AREA) == 0)  {
                        CTabData* pData = new CTabData();
                        while (specFile.GetLine(sCmd, sArg) == SF_OK) {
                            if (sCmd.CompareNoCase(XTS_CMD_AREALEVEL) == 0)  {
                                pData->SetAreaLevel(sArg);
                            }
                            else if (sCmd.CompareNoCase(XTS_CMD_AREANAME) == 0)  {
                                pData->SetAreaLabel(sArg);
                            }
                            else if (sCmd.CompareNoCase(XTS_CMD_AREACODES) == 0)  {
                            }
                            else if (sCmd.CompareNoCase(XTS_SECT_CELLS) == 0)  {
                                break;
                            }
                            else {
                                if (!bSilent) {
                                    m_sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                                    m_sError += _T("\n") + sCmd + _T("=") + sArg;
                                    AfxMessageBox(m_sError);
                                }
                                bResult = false;
                            }
                        }
                        pData->Build(specFile, bSilent);
                        m_aTabData.Add(pData);
                    }
                    else if (sCmd.CompareNoCase(XTS_SECT_CELLS) == 0)  {
                        CTabData* pData = new CTabData();
                        pData->Build(specFile, bSilent);
                        m_aTabData.Add(pData);
                    }
                    else if (sCmd.CompareNoCase(XTS_SECT_FRQSTATS) == 0)  {
                        BuildFrqStats(specFile, bSilent);
                    }
                    else {
                        specFile.UngetLine();
                        break;
                    }
                }
            }
            else if (sCmd.CompareNoCase(XTS_SECT_SPECIAL_VALUES) == 0) {
                while (specFile.GetLine(sCmd, sArg) == SF_OK) {
                    if(sCmd.CompareNoCase(XTS_CMD_SPECVAL_USECUSTOM) ==0){
                        m_custSpecValSettings.SetUseCustomSpecVal(sArg.CompareNoCase(XTS_ARG_YES)==0);
                    }
                    else if (sCmd.CompareNoCase(XTS_CMD_SPECVAL_NOTAPPL) == 0) {
                        m_custSpecValSettings.SetUseSpecValNotAppl(sArg.CompareNoCase(XTS_ARG_YES)==0);
                    }
                    else if (sCmd.CompareNoCase(XTS_CMD_SPECVAL_MISSING) == 0) {
                        m_custSpecValSettings.SetUseSpecValMissing(sArg.CompareNoCase(XTS_ARG_YES)==0);
                    }
                    else if (sCmd.CompareNoCase(XTS_CMD_SPECVAL_REFUSED) == 0) {
                        m_custSpecValSettings.SetUseSpecValRefused(sArg.CompareNoCase(XTS_ARG_YES)==0);
                    }
                    else if (sCmd.CompareNoCase(XTS_CMD_SPECVAL_DEFAULT) == 0) {
                        m_custSpecValSettings.SetUseSpecValDefault(sArg.CompareNoCase(XTS_ARG_YES)==0);
                    }
                    else if (sCmd.CompareNoCase(XTS_CMD_SPECVAL_UNDEFINED) == 0) {
                        m_custSpecValSettings.SetUseSpecValUndefined(sArg.CompareNoCase(XTS_ARG_YES)==0);
                    }
                    else {
                        specFile.UngetLine();
                        break;
                    }
                } // while
            }
            else if (sCmd.CompareNoCase(XTS_SECT_POSTCALC) == 0)  {
                while (specFile.GetLine(sCmd, sArg) == SF_OK) {
                    if (sCmd.CompareNoCase(XTS_CMD_POSTCALC) == 0) {
                        m_aPostCalc.Add(sArg);
                    }
                    else {
                        specFile.UngetLine();
                        break;
                    }
                }
            }
            else {
                m_sError.Format(_T("Invalid section heading at line %d:"), specFile.GetLineNumber()); // Invalid section heading at line %d:
                m_sError += _T("\n") + sCmd;
                if (!bSilent) {
                    AfxMessageBox(m_sError);
                    specFile.SkipSection();
                }
                bResult = false;
            }
        }
    }

    return bResult;
}

///////////////////////////////////////////////////////////////////////
//
//    CTblOb::Build
//
// Builds auxiliary table objects that have their own sections:
// - page notes
// - end notes
// - headers (l/c/r)
// - footers (l/c/r)
//
// Each of these sections has several key only:
// - label (a string)
// - format (a string indicating a CFmtBase ID) -- optional
// - sizing (current, min, max) -- optional
//
///////////////////////////////////////////////////////////////////////
bool CTblOb::Build(CSpecFile& specFile, const CFmtReg& reg, const CString& sVersion)
{
    CString sCmd;     // command (left side of =)
    CIMSAString sArg; // argument (right side of =)
    CString sError;

    // init...
    m_sText.Empty();
    m_pFmt=NULL;


    // read line by line ...
    while (specFile.GetState() == SF_OK)  {
        specFile.GetLine(sCmd, sArg);
        ASSERT (!sCmd.IsEmpty());
        if (sCmd[0] != '[')  {
            // strip out " or ' delimiters, if present
            int iLen = sArg.GetLength();
            if (sArg[0]==_T('"') && sArg[iLen-1]=='"')  {
                if (iLen>2)  {
                    // has contents
                    sArg = sArg.Mid(1,iLen-2);
                }
                else {
                    // null string
                    sArg.Empty();
                }
            }
            else if (sArg[0]==_T('\'') && sArg[iLen-1]=='\'')  {
                if (iLen>2)  {
                    // has contents
                    sArg = sArg.Mid(1,iLen-2);
                }
                else {
                    // null string
                    sArg.Empty();
                }
            }

            // table layout
            if (!sCmd.CompareNoCase(XTS_CMD_LABEL))  {
                SetText(TableLabelSerializer::Parse(sArg, sVersion));
            }
            else if (!sCmd.CompareNoCase(XTS_CMD_FORMAT))  {
                if (!sArg.IsEmpty()) {
                    //Parse conten here to get the id string an index string with the new getfmt
                    CString sIDString,sIndexString;
                    sIDString = sArg.GetToken();
                    sIndexString = sArg.GetToken();
                    CFmt* pFmt=DYNAMIC_DOWNCAST(CFmt,reg.GetFmt(sIDString,sIndexString));
                    if (NULL==pFmt) {
                        sError.Format(_T("Unrecognized format label %s at line %d:"), (LPCTSTR)sArg, specFile.GetLineNumber());
                        sError += _T("\n") + sCmd + _T("=") + sArg;
                        sError += _T("\nUsing default format instead.");
                        AfxMessageBox(sError);
                    }
                    else {
                        SetFmt(pFmt);
                    }
                }
            }
            else if (sCmd.CompareNoCase(XTS_ARG_PRNTVSZ) == 0 || sCmd.CompareNoCase(XTS_ARG_GRIDVSZ) == 0 )  {
                specFile.UngetLine();
                BuildStateInfo(specFile);
            }
            else {
                sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                sError += _T("\n") + sCmd + _T("=") + sArg;
                sError += _T("\n\nIgnoring this component.");
                AfxMessageBox(sError);
                return false;
            }
        }
        else {
            // we are starting a new section ... put back the line and quit
            specFile.UngetLine();
            break;
        }
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTable::Save
//
/////////////////////////////////////////////////////////////////////////////

void CTable::Save(CSpecFile& specFile)
{
    SaveBegin(specFile);

    if (specFile.GetFileName().Right(3).CompareNoCase(FileExtensions::TableSpec) != 0) {
        for (int t = 0 ; t < m_aTabData.GetSize() ; t++) {
            SaveTabData(specFile, t);
        }
    }

    SaveEnd(specFile);
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTable::SaveBegin
//
/////////////////////////////////////////////////////////////////////////////
void CTable::SaveBegin(CSpecFile& specFile)
{
    specFile.PutLine(_T(" "));
    specFile.PutLine(XTS_SECT_TABLE);
    specFile.PutLine(XTS_CMD_NAME, m_sName);
    specFile.PutLine(XTS_CMD_NUM, m_sNum);
    if (!m_bGenerateLogic) {
        specFile.PutLine(XTS_CMD_TBL_GENLOGIC ,XTS_ARG_NO);
    }
    if (m_bExcludeForRun) {
        specFile.PutLine(XTS_CMD_TBL_EXCLUDE_FOR_RUN ,XTS_ARG_YES);
    }
    if (m_iBreakLevel >= 0) {
        CIMSAString sBreakLevel;
        sBreakLevel.Str(m_iBreakLevel);
        specFile.PutLine(XTS_CMD_BREAKLEVEL, sBreakLevel);
    }
    specFile.PutLine(_T(" "));

    //save the table tablefmt is it is not default
    CFmtBase* pFmt=GetFmt();
    if (NULL!=pFmt) {
        bool bCustomFmt=CFmtReg::IsCustomFmtID(*pFmt);
        if (bCustomFmt){
//          CString sFmt ;
//          sFmt = pFmt->GetIDString()+ "," + pFmt->GetIndexString();
//          specFile.PutLine(XTS_CMD_FORMAT, sFmt);
            specFile.PutLine(XTS_CMD_FORMAT, pFmt->GetIDInfo());
        }
    }
    //Save the table printfmt if it is not default
    if (NULL!=m_pTblPrintFmt) {
        bool bCustomFmt=CFmtReg::IsCustomFmtID(*m_pTblPrintFmt);
        if (bCustomFmt){
//          CString sFmt ;
//          sFmt = m_pTblPrintFmt->GetIDString()+ "," + m_pTblPrintFmt->GetIndexString();
//          specFile.PutLine(XTS_CMD_FORMAT, sFmt);
            specFile.PutLine(XTS_CMD_FORMAT, m_pTblPrintFmt->GetIDInfo());
        }
    }
    ///////////////////////////////////////////////////////////////////////////////////////////
    // save the auxiliary table objects (page notes, end notes, headers, footers, stubheads, area captions) ...
    GenerateTitle();
    GetTitle()->Save(specFile, XTS_SECT_TITLE);
    GetSubTitle()->Save(specFile, XTS_SECT_SUBTITLE);
    GetPageNote()->Save(specFile, XTS_SECT_PAGE_NOTE);
    GetEndNote()->Save(specFile, XTS_SECT_END_NOTE);
    GetHeader(0)->Save(specFile, XTS_SECT_HEADER_LEFT);
    GetHeader(1)->Save(specFile, XTS_SECT_HEADER_CENTER);
    GetHeader(2)->Save(specFile, XTS_SECT_HEADER_RIGHT);
    GetFooter(0)->Save(specFile, XTS_SECT_FOOTER_LEFT);
    GetFooter(1)->Save(specFile, XTS_SECT_FOOTER_CENTER);
    GetFooter(2)->Save(specFile, XTS_SECT_FOOTER_RIGHT);
    GetStubhead(0)->Save(specFile, XTS_SECT_STUBHEAD);
    GetStubhead(1)->Save(specFile, XTS_SECT_STUBHEAD_SEC);
    GetAreaCaption()->Save(specFile, XTS_SECT_AREA_CAPTION);
    GetOneRowColTotal()->Save(specFile, XTS_SECT_ONEROWCOL_TOTAL);

    ///////////////////////////////////////////////////////////////////////////////////////////
    // rows ...
    if (m_pRowRoot->GetNumChildren() > 0) {
        specFile.PutLine(_T(" "));
        specFile.PutLine(XTS_SECT_ROWS);
        for (int r = 0 ; r < m_pRowRoot->GetNumChildren() ; r++) {
            m_pRowRoot->GetChild(r)->Save(specFile);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    // cols ...
    if (m_pColRoot->GetNumChildren() > 0) {
        specFile.PutLine(_T(" "));
        specFile.PutLine(XTS_SECT_COLS);
        for (int c = 0 ; c < m_pColRoot->GetNumChildren() ; c++) {
            m_pColRoot->GetChild(c)->Save(specFile);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    // table Unit ...
    bool bAtLeastOne = false;
    m_tableUnit.SetSubTableString(_T("Entire Table"));
    if(m_tableUnit.IsUnitPresent()){
        specFile.PutLine(XTS_SECT_UNITSPECS);
        m_tableUnit.Save(specFile);
        bAtLeastOne = true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    // unit specs ...
    if(m_aUnitSpec.GetSize() >  0) {
        for(int iIndex =0; iIndex < m_aUnitSpec.GetSize(); iIndex++){
            CUnitSpec& unitSpec=m_aUnitSpec[iIndex];
            if(unitSpec.IsUnitPresent()){
                if(!bAtLeastOne){
                    bAtLeastOne = true;
                    specFile.PutLine(_T(" "));
                    specFile.PutLine(XTS_SECT_UNITSPECS);
                }
                unitSpec.Save(specFile);
            }
        }
    }//End Save unit Specs
    if(bAtLeastOne){
        specFile.PutLine(XTS_SECT_ENDUNITSPECS);
        specFile.PutLine(_T(" "));
    }

    // special values
    if (m_custSpecValSettings.GetUseCustomSpecVal()) {
        specFile.PutLine(XTS_SECT_SPECIAL_VALUES);
        specFile.PutLine(XTS_CMD_SPECVAL_USECUSTOM ,XTS_ARG_YES);

        if (m_custSpecValSettings.GetUseSpecValNotAppl()) {
            specFile.PutLine(XTS_CMD_SPECVAL_NOTAPPL ,XTS_ARG_YES);
        }
        if (m_custSpecValSettings.GetUseSpecValMissing()) {
            specFile.PutLine(XTS_CMD_SPECVAL_MISSING ,XTS_ARG_YES);
        }
        if (m_custSpecValSettings.GetUseSpecValRefused()) {
            specFile.PutLine(XTS_CMD_SPECVAL_REFUSED ,XTS_ARG_YES);
        }
        if (m_custSpecValSettings.GetUseSpecValDefault()) {
            specFile.PutLine(XTS_CMD_SPECVAL_DEFAULT ,XTS_ARG_YES);
        }
        if (m_custSpecValSettings.GetUseSpecValUndefined()) {
            specFile.PutLine(XTS_CMD_SPECVAL_UNDEFINED ,XTS_ARG_YES);
        }
        specFile.PutLine(_T(" "));
    }
    //Save PostCalc Logic
    if(m_aPostCalc.GetSize() > 0 ) {
        specFile.PutLine(XTS_SECT_POSTCALC);
        for(int iIndex =0;iIndex < m_aPostCalc.GetSize();iIndex++){
            specFile.PutLine(XTS_CMD_POSTCALC,m_aPostCalc[iIndex]);
        }
    }

    if (specFile.GetFileName().Right(3).CompareNoCase(FileExtensions::TableSpec) != 0) {
        specFile.PutLine(_T(" "));
        specFile.PutLine(XTS_SECT_DATA);

        // actual data gets saved in CTable::SaveTabData
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTable::SaveEnd
//
/////////////////////////////////////////////////////////////////////////////
void CTable::SaveEnd(CSpecFile& specFile)
{
    if (specFile.GetFileName().Right(3).CompareNoCase(FileExtensions::TableSpec) != 0) {
        if(m_bSaveFreqStats) {
            this->SaveFrqStats(specFile);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTable::SaveTabData
//
/////////////////////////////////////////////////////////////////////////////
void CTable::SaveTabData(CSpecFile& specFile, int iTabData)
{
    m_aTabData[iTabData]->SetTableName(m_sName);
    m_aTabData[iTabData]->SetNumRows(GetNumRows(true));
    m_aTabData[iTabData]->SetNumCols(GetNumCols(true));
    m_aTabData[iTabData]->Save(specFile);
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTable::GetNumRows
//
/////////////////////////////////////////////////////////////////////////////
int CTable::GetNumRows(bool bIgnoreRdrBrks/* = false*/) //for supporting datacells
{
    CTabVar* pRowVar = GetRowRoot();
    int iRows = 0;
    for (int i = 0 ; i < pRowVar->GetNumChildren() ; i++) {
        iRows += pRowVar->GetChild(i)->GetTotValues(bIgnoreRdrBrks);
    }
    return iRows;
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTable::GetNumCols
//
/////////////////////////////////////////////////////////////////////////////


int CTable::GetNumCols(bool bIgnoreRdrBrks /*= false*/) //for supporting datacells
{
    CTabVar* pColVar = GetColRoot();
    int iCols = 0;
    for (int i = 0 ; i < pColVar->GetNumChildren() ; i++) {
        iCols += pColVar->GetChild(i)->GetTotValues(bIgnoreRdrBrks);
    }
    return iCols;
}



/////////////////////////////////////////////////////////////////////////////
//
//                                  CTabLevel
//
/////////////////////////////////////////////////////////////////////////////
//
//                               CTabLevel::Save
//
/////////////////////////////////////////////////////////////////////////////


void CTabLevel::Save(CSpecFile& specFile)
{
    specFile.PutLine (_T(" "));
    specFile.PutLine (XTS_SECT_LEVEL);
    for (int i = 0 ; i < m_aTable.GetSize() ; i ++) {
        specFile.PutLine(XTS_CMD_TABLE, m_aTable[i]->GetName());
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                                   CTabSet
//
/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSet::CTabSet
//
/////////////////////////////////////////////////////////////////////////////

CTabSet::CTabSet()
{
    // initialize ptrs to null
    m_pWorkDict = NULL;
    m_pConsolidate = new CConsolidate();
//    m_pAppFmt = NULL;
    SetNumLevels(0);
    m_aPrintTable.SetSize(0);
    m_eSpecType = CROSSTAB_SPEC; //default
    m_bGenLogic = true;
    m_pTabSetFmt = NULL;
    m_bSelectedPrinterChanged = false;
    m_pAreaNameFile = NULL;
    m_sInputDataFilename = XTS_DEFAULT_INPUTDATAFILENAME; // 20090915 GHM
    //Savy (R) sampling app 20081202
    m_bIsSamplingErrApp = false;
}



CTabSet::CTabSet(std::shared_ptr<const CDataDict> pDataDict)
{
    // Get dictionary and initialize
    m_pDataDict = pDataDict;
    m_pWorkDict = NULL;
    m_bGenLogic = true;
    m_bSelectedPrinterChanged = false;
    m_pAreaNameFile = NULL;
    m_sInputDataFilename = XTS_DEFAULT_INPUTDATAFILENAME; // 20090915 GHM
    //Savy (R) sampling app 20081202
    m_bIsSamplingErrApp = false;
    SetNumLevels(pDataDict->GetNumLevels());
    m_pConsolidate = new CConsolidate();

    // Initialize default format specs

// SAVYFOO -- these are handled in CTabSetFmt
/*    csprochar pszTemp[10];
    GetPrivateProfileString("intl", "sThousand", ",", pszTemp, 10, "win.ini");
    m_sThousandSep = pszTemp;
    GetPrivateProfileString("intl", "sDecimal", ".", pszTemp, 10, "win.ini");
    m_sDecimalSep = pszTemp;
    m_bZeroBeforeDec = (GetPrivateProfileInt("intl", "iLZero", 1, "win.ini") == 1);
    m_sZeroMask = "-";
    m_sZeroRound = "(z)";
    m_sSuppress = "*";*/
    m_pTabSetFmt = NULL;
//    m_pAppFmt = NULL;

    // Add empty table
// SAVYFOO -- use CTblPrintFmt::m_sTitleTemplate for determining the table title
    CTable* pTable = new CTable(_T("1"));
    AddTable(pTable);
}

CTabSet::~CTabSet()
{
    delete m_pConsolidate;
    /*if(m_pTabSetFmt){// fmt reg deletes the stuff
        delete m_pTabSetFmt;
    }*/
    while (m_aPrintTable.GetSize() > 0) {
        CTable* pTable = m_aPrintTable[0];
        delete pTable;
        m_aPrintTable.RemoveAt(0);
    }
    while (m_aTabLevel.GetSize() > 0) {
        CTabLevel* pTabLevel = m_aTabLevel[0];
        delete pTabLevel;
        m_aTabLevel.RemoveAt(0);
    }
    if(m_pAreaNameFile){
        CloseAreaNameFile();
    }
}


void CTabSet::SetNumLevels(int levels)
{
    ASSERT(m_aTabLevel.IsEmpty());

     for( int l = 0; l < levels; ++l )
        m_aTabLevel.Add(new CTabLevel());
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSet::IsUniqueTabNum
//
/////////////////////////////////////////////////////////////////////////////

bool CTabSet::IsUniqueTabNum(const CString& sNum)
{
    bool bResult = true;
    for (int i = 0 ; i < m_aPrintTable.GetSize() ; i++) {
        if (m_aPrintTable[i]->GetNum() == sNum) {
            bResult = false;
            break;
        }
    }
    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSet::GetNextTabNum
//
/////////////////////////////////////////////////////////////////////////////

CString CTabSet::GetNextTabNum(const CString& sNum)
{
    CString sNextNum = sNum;
    int last = sNextNum.GetLength() - 1;
    csprochar c = sNextNum.GetAt(last);

    if (c >= _T('0')&& c <= '9') {
        int i;
        for (i = last - 1; i >= 0 ; i--) {
            if (sNextNum.GetAt(i) < _T('0')|| sNextNum.GetAt(i) > '9') {
                break;
            }
        }
        i++;
        CIMSAString sNumStr = sNextNum.Mid(i);
        int n = (int) sNumStr.Val();
        n++;
        sNumStr.Str(n);
        sNextNum = sNextNum.Left(i) + sNumStr;
    }
    else {
        c++;
        if (c < 'A') {
            c = _T('A');
        }
        else if (c > _T('Z') && c < 'a') {
            c = _T('a');
        }
        else if (c > 'z') {
            c--;
        }
        sNextNum.SetAt(last, c);
    }

    if (!IsUniqueTabNum(sNextNum)) {
        sNextNum = sNum + _T("A");
        while (!IsUniqueTabNum(sNextNum)) {
            sNextNum += _T("A");
        }
    }
    bool bProcess = true;
    while(bProcess){
        bProcess = false;
        CString sTableName = _T("TABLE")+sNextNum;
        for (int i = 0 ; i < GetNumTables() ; i++) {
            CTable* pTable = GetTable(i);
            if (sTableName.CompareNoCase(pTable->GetName()) ==0) {
                bProcess = true;
                break;
            }
        }
        if(bProcess){
            sNextNum += _T("A");
            while (!IsUniqueTabNum(sNextNum)) {
                sNextNum += _T("A");
            }
        }
        else{
            break;
        }
    }

    return sNextNum;
}
/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSet::Renumber
//
/////////////////////////////////////////////////////////////////////////////

void CTabSet::Renumber()
{
    CIMSAString sNum;
    for (int i = 0 ; i < m_aPrintTable.GetSize() ; i++) {
        sNum.Str(i);
        m_aPrintTable[i]->SetNum(sNum);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSet::SearchTable
//
/////////////////////////////////////////////////////////////////////////////

CTable* CTabSet::SearchTable(const CString& sTableName, int& iCtabLevel ) {
    CTable*     pTable=NULL;
    iCtabLevel = -1;
    for( int iLevel=0; iLevel < GetNumLevels(); iLevel++ ) {
        CTabLevel* pCtabLevel=GetLevel(iLevel);

        if( (pTable = pCtabLevel->SearchTable(sTableName)) != NULL ) {
            iCtabLevel = iLevel;
            return pTable;
        }
    }

    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTabLevel::SearchTable
//
/////////////////////////////////////////////////////////////////////////////

CTable* CTabLevel::SearchTable(const CString& sTableName ) {
    for( int iTable=0; iTable < GetNumTables(); iTable++ ) {
        CTable*     pTable=GetTable(iTable);

        if( pTable->GetName().CompareNoCase( sTableName ) == 0 ) {
            return pTable;
        }
    }

    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSet::AddTable
//
/////////////////////////////////////////////////////////////////////////////

void CTabSet::AddTable(CTable* pTable, int iLevel /*= NONE*/)
{
    ASSERT(pTable != NULL);
    ASSERT(iLevel >= -1 && iLevel < 3);

    m_aPrintTable.Add(pTable);
    if (iLevel == NONE) {
        m_aTabLevel[0]->AddTable(pTable);
    }
    else {
        m_aTabLevel[iLevel]->AddTable(pTable);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSet::InsertTable
//
/////////////////////////////////////////////////////////////////////////////

void CTabSet::InsertTable(CTable* pTable, CTable* pRefTable, int iLevel /*= NONE*/)
{
    ASSERT(pTable != NULL);
    ASSERT(pRefTable != NULL);
    ASSERT(iLevel >= -1 && iLevel < 3);

    bool bDone = false;
    if (iLevel == NONE) {
        for (int i = 0 ; i < m_aPrintTable.GetSize() ; i++) {
            if (m_aPrintTable[i] == pRefTable) {
                m_aPrintTable.InsertAt(i, pTable);
                m_aTabLevel[0]->AddTable(pTable);
                bDone = true;
                break;
            }
        }
    }
    else {
        for (int i = 0 ; i < m_aTabLevel[iLevel]->GetNumTables() ; i++) {
            if (m_aTabLevel[iLevel]->GetTable(i) == pRefTable) {
                m_aTabLevel[iLevel]->InsertTable(i, pTable);
                m_aPrintTable.Add(pTable);
                bDone = true;
                break;
            }
        }
    }
    ASSERT(bDone);    // pRefTable not found
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSet::DeleteTable
//
/////////////////////////////////////////////////////////////////////////////

void CTabSet::DeleteTable(CTable* pTable)
{
    bool bPrintDone = false;
    for (int i = 0 ; i < m_aPrintTable.GetSize() ; i++) {
        if (m_aPrintTable[i] == pTable) {
            m_aPrintTable.RemoveAt(i);
            bPrintDone = true;
            break;
        }
    }
    ASSERT(bPrintDone);   // pTable not found in print tables
    bool bLevelDone = false;
    for (int l = 0 ; l < m_aTabLevel.GetSize() ; l++) {
        for (int j = 0 ; j < m_aTabLevel[l]->GetNumTables() ; j++) {
            if (m_aTabLevel[l]->GetTable(j) == pTable) {
                m_aTabLevel[l]->DeleteTable(j);
                bLevelDone = true;
                break;
            }
        }
        if (bLevelDone) {
            break;
        }
    }
    ASSERT(bLevelDone);   // pTable not found in level tables
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSet::UpdateFmtFlag
//
/////////////////////////////////////////////////////////////////////////////

void CTabSet::UpdateFmtFlag(CTable* pTable)
{
    //Title format
    CTblOb* pTitleOb = pTable->GetTitle();
    CFmtBase* pTitleFmt = pTitleOb->GetFmt();
    if(pTitleFmt!=NULL){
        pTitleFmt->SetUsedFlag(true);
    }
    //Area Caption format
    CTblOb* pAreaCaption = pTable->GetAreaCaption();
    CFmtBase* pAreaCaptionFmt = pAreaCaption->GetFmt();
    if(pAreaCaptionFmt!=NULL){
        pAreaCaptionFmt->SetUsedFlag(true);
    }

    //StubHead format  - Primary and Secondary
    for(int iStubHead=0; iStubHead < 2; iStubHead++){
        CTblOb* pStubHead = pTable->GetStubhead(iStubHead);
        CFmtBase* pStubFmt = pStubHead->GetFmt();
        if(pStubFmt!=NULL){
            pStubFmt->SetUsedFlag(true);
        }
    }

    //One row table
    CTblOb* pOneRowTotal= pTable->GetOneRowColTotal();
    if (pOneRowTotal && pOneRowTotal->GetFmt()) {
        pOneRowTotal->GetFmt()->SetUsedFlag(true);
    }

    //Printer format
    CTblPrintFmt* pTblPrintFmt = pTable->GetTblPrintFmt();
    if(pTblPrintFmt != NULL && pTblPrintFmt->GetIndex()!= 0){
        pTblPrintFmt->SetUsedFlag(true);
    }
    // Table format
    CTblFmt* pTblFmt = pTable->GetDerFmt();
    if(pTblFmt != NULL && pTblFmt->GetIndex()!= 0){
        pTblFmt->SetUsedFlag(true);
    }
    // header and footer format
    for(int iIndex = 0; iIndex < 3; iIndex++){
        CTblOb* pTbl = pTable->GetHeader(iIndex);
        CFmtBase* pFmtBase = pTbl->GetFmt();
        if(pFmtBase!=NULL){
            pFmtBase->SetUsedFlag(true);
        }
        //footer
        pTbl = pTable->GetFooter(iIndex);
        pFmtBase = pTbl->GetFmt();
        if(pFmtBase!=NULL){
            pFmtBase->SetUsedFlag(true);
        }

    }

    //page note
    CTblOb* pPageNoteOb = pTable->GetPageNote();
    CFmtBase* pPageNoteFmt = pPageNoteOb->GetFmt();
    if(pPageNoteFmt!=NULL){
        pPageNoteFmt->SetUsedFlag(true);
    }
    //End note
    CTblOb* pEndNoteOb = pTable->GetEndNote();
    CFmtBase* pEndNoteFmt = pEndNoteOb->GetFmt();
    if(pEndNoteFmt!=NULL){
        pEndNoteFmt->SetUsedFlag(true);
    }
    //Subtitle
    CTblOb* pSubTitleOb = pTable->GetSubTitle();
    CFmtBase* pSubTitleFmt = pSubTitleOb->GetFmt();
    if(pSubTitleFmt!=NULL){
        pSubTitleFmt->SetUsedFlag(true);
    }

    //Savy (R) To set the used format
    //Row Variables
    CTabVar* pRowVar = pTable->GetRowRoot();
    for(int iRow = 0; iRow < pRowVar->GetNumChildren();iRow++){
        CTabVar* pCrossVar = pRowVar->GetChild(iRow);
        pCrossVar->UpdateFmtFlag();
        for(int iChildVar = 0; iChildVar < pCrossVar->GetNumChildren();iChildVar++){
            CTabVar* pChildVar = pCrossVar->GetChild(iChildVar);
            pChildVar->UpdateFmtFlag();
        }
    }
    //row end


    //Column Variables
    CTabVar* pColVar =  pTable->GetColRoot();
    for (int iCol = 0; iCol < pColVar->GetNumChildren(); iCol++) {
        CTabVar* pCrossVar = pColVar->GetChild(iCol);
        pCrossVar->UpdateFmtFlag();
        for(int iChildVar = 0; iChildVar < pCrossVar->GetNumChildren();iChildVar++){
            CTabVar* pChildVar = pCrossVar->GetChild(iChildVar);
            pChildVar->UpdateFmtFlag();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSet::MovePrintTable
//
/////////////////////////////////////////////////////////////////////////////

void CTabSet::MovePrintTable(CTable* pTable, CTable* pRefTable, bool bAfter /*= false*/)
{
    bool bFound = false;
    for (int i = 0 ; i < m_aPrintTable.GetSize() ; i++) {
        if (m_aPrintTable[i] == pTable) {
            m_aPrintTable.RemoveAt(i);
            bFound = true;
            break;
        }
    }
    ASSERT(bFound);     // pTable not found
    bFound = false;
    for (int i = 0 ; i < m_aPrintTable.GetSize() ; i++) {
        if (m_aPrintTable[i] == pRefTable) {
            if (bAfter) {
                i++;
            }
            m_aPrintTable.InsertAt(i, pTable);
            bFound = true;
            break;
        }
    }
    ASSERT(bFound);     // pRefTable not found
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSet::MoveLevelTable
//
/////////////////////////////////////////////////////////////////////////////

void CTabSet::MoveLevelTable(CTable* pTable, CTable* pRefTable, bool bAfter /*= false*/)
{
    bool bFound = false;
    for (int l = 0 ; l < m_aTabLevel.GetSize() ; l++) {
        for (int i = 0 ; i < m_aTabLevel[l]->GetNumTables() ; i++) {
            if (m_aTabLevel[l]->GetTable(i) == pTable) {
                m_aTabLevel[l]->DeleteTable(i);
                bFound = true;
                break;
            }
        }
        if (bFound) {
            break;
        }
    }
    ASSERT(bFound);     // pTable not found
    bFound = false;
    for (int l = 0 ; l < m_aTabLevel.GetSize() ; l++) {
        for (int i = 0 ; i < m_aTabLevel[l]->GetNumTables() ; i++) {
            if (m_aTabLevel[l]->GetTable(i) == pRefTable) {
                if (bAfter) {
                    i++;
                }
                m_aTabLevel[l]->InsertTable(i, pTable);
                bFound = true;
                break;
            }
        }
        if (bFound) {
            break;
        }
    }
    ASSERT(bFound);     // pRefTable not found
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSet::Open
//
/////////////////////////////////////////////////////////////////////////////

bool CTabSet::Open(const CString& sSpecFilePath, bool bSilent /*=false*/)
{
    CSpecFile specFile;
    bool bResult = false;

    if (specFile.Open(sSpecFilePath, CFile::modeRead)) {
        // Set up progress bar
        std::shared_ptr<ProgressDlg> dlgProgress;
        int len = (int)specFile.GetLength() / 100;
        if (len == 0) {
            len = 1;
        }
        if (!bSilent) {
            dlgProgress = ProgressDlgFactory::Instance().Create();
            dlgProgress->SetStatus(_T("Checking table specs ... please wait"));
            dlgProgress->SetStep(1);
            dlgProgress->SetRange(0, len);   // avoid probs with DD's > 65K bytes
        }
        // Read spec file
        CString sCmd, sArg;
        specFile.GetLine(sCmd, sArg);
        if (specFile.GetState() == SF_EOF) {
            AfxMessageBox (_T("Empty tabulation spec file"));
        }
        else if (sCmd.CompareNoCase(XTS_SECT_TABSPEC) != 0)  {
            AfxMessageBox (_T("Not a CSPro XTS file"));
        }
        else {
            CString sVersion = CSPRO_VERSION;
            if (!specFile.IsVersionOK(sVersion)) {
                if (!IsValidCSProVersion(sVersion, 3.0)) {
                    AfxMessageBox (_T("Incorrect XTS file version"));
                }
                else {
                    bResult = Build(specFile, dlgProgress, sVersion, bSilent);
                    //Now we are doing in the build
                    if(bResult && sVersion.CompareNoCase(_T("CSPro 3.0")) == 0){
                        Reconcile30TallyFmts();
                        //m_fmtReg.Reconcile30PercentsinTallyFmts();
                    }
                    if (bResult && (sVersion.CompareNoCase(_T("CSPro 3.0")) || sVersion.CompareNoCase(_T("CSPro 3.1")))) {
                        Number3031TabVals();
                    }
                    FixSysTotalVarTallyFmts();
                }
            }
            else {
                bResult = Build(specFile, dlgProgress, sVersion, bSilent);
                FixSysTotalVarTallyFmts();
            }
        }
        // Finalize the progress bar
        if (!bSilent) {
            if (dlgProgress->CheckCancelButton() || !bResult)  {
                AfxMessageBox(_T("XTS file not loaded."), MB_OK | MB_ICONINFORMATION);
                bResult = false;
            }
            else  {
                dlgProgress->SetPos((int)specFile.GetLength() / 100);
            }
        }
        specFile.Close();
    }

    return bResult;
}

void CTabSet::Reconcile30TallyFmts()
{
    //Call this only if the version is 3.0 used for backward compatibility
    CTallyFmt* pOldTallyFmt = DYNAMIC_DOWNCAST(CTallyFmt,m_fmtReg.GetFmt(FMT_ID_TALLY));
    ASSERT(pOldTallyFmt);
    //Copy the old defaults to the new row fmt
    CTallyFmt* pRowTallyFmt = DYNAMIC_DOWNCAST(CTallyFmt,m_fmtReg.GetFmt(FMT_ID_TALLY_ROW));
    ASSERT(pRowTallyFmt);
    *pRowTallyFmt = *pOldTallyFmt;
    pRowTallyFmt->SetID(FMT_ID_TALLY_ROW);
    //Copy the old defaults to the new col fmt
    CTallyFmt* pColTallyFmt = DYNAMIC_DOWNCAST(CTallyFmt,m_fmtReg.GetFmt(FMT_ID_TALLY_COL));
    ASSERT(pColTallyFmt);
    *pColTallyFmt = *pOldTallyFmt;
    pColTallyFmt->SetID(FMT_ID_TALLY_COL);

    //for each table
    for(int iTbl =0; iTbl < GetNumTables(); iTbl++){
        CTable* pTbl = GetTable(iTbl);
        //for each tabvar --> in row
            //if fmtID is FMT_ID_TALLY chage it to FMT_ID_TALLY_ROW
        CTabVar* pTabVar = pTbl->GetRowRoot();
        for(int iChild =0; iChild < pTabVar->GetNumChildren(); iChild++){
            CTabVar* pChild = pTabVar->GetChild(iChild);
            if(pChild->GetTallyFmt() && pChild->GetTallyFmt()->GetID() == FMT_ID_TALLY){
                pChild->GetTallyFmt()->SetID(FMT_ID_TALLY_ROW);
            }
            for(int iGrandChild =0; iGrandChild < pChild->GetNumChildren(); iGrandChild++){
                CTabVar* pGrandChild = pChild->GetChild(iGrandChild);
                if(pGrandChild->GetTallyFmt() && pGrandChild->GetTallyFmt()->GetID() == FMT_ID_TALLY){
                    pGrandChild->GetTallyFmt()->SetID(FMT_ID_TALLY_ROW);
                }
            }
        }
        //for each tabvar --> in col
            //if fmtID is FMT_ID_TALLY chage it to FMT_ID_TALLY_COL
        pTabVar = pTbl->GetColRoot();
        for(int iChild =0; iChild < pTabVar->GetNumChildren(); iChild++){
            CTabVar* pChild = pTabVar->GetChild(iChild);
            if(pChild->GetTallyFmt() && pChild->GetTallyFmt()->GetID() == FMT_ID_TALLY){
                pChild->GetTallyFmt()->SetID(FMT_ID_TALLY_COL);
            }
            for(int iGrandChild =0; iGrandChild < pChild->GetNumChildren(); iGrandChild++){
                CTabVar* pGrandChild = pChild->GetChild(iGrandChild);
                if(pGrandChild->GetTallyFmt() && pGrandChild->GetTallyFmt()->GetID() == FMT_ID_TALLY){
                    pGrandChild->GetTallyFmt()->SetID(FMT_ID_TALLY_COL);
                }
            }
        }
    }
}

void CTabSet::Number3031TabVals()
{
    for(int iTbl =0; iTbl < GetNumTables(); iTbl++) {
        CTable* pTbl = GetTable(iTbl);
        Number3031TabVals(pTbl, pTbl->GetRowRoot());
        Number3031TabVals(pTbl, pTbl->GetColRoot());
    }
}

 void CTabSet::Number3031TabVals(CTable* pTbl, CTabVar* pTabVar)
{
     // if the tab vals don't have id's (becuase the file is from 3.1 or 3.0)
    // then assign id's to the tab vals.  We can do this easily since each stat
    // type appears only once in 3.0 and 3.1.  Same is not true in 3.2 which is why
    // we need the stat id in 3.2.
    if (pTabVar->GetArrTabVals().GetCount() > 0) {

        CTallyFmt* pVarTallyFmt = pTabVar->GetTallyFmt();
        ASSERT(pTbl->GetFmtRegPtr());
        FMT_ID eFmtID = FMT_ID_INVALID;
        pVarTallyFmt ? eFmtID= pVarTallyFmt->GetID() : eFmtID = (pTbl->IsRowVar(pTabVar) ? FMT_ID_TALLY_ROW: FMT_ID_TALLY_COL);
        ASSERT(eFmtID != FMT_ID_INVALID && eFmtID != FMT_ID_TALLY);
        CTallyFmt* pTableTallyFmt = DYNAMIC_DOWNCAST(CTallyFmt,pTbl->GetFmtRegPtr()->GetFmt(eFmtID));
        if(!pVarTallyFmt){
            pVarTallyFmt = pTableTallyFmt;
        }

        for (int iVal = 0; iVal < pTabVar->GetArrTabVals().GetCount(); ++iVal) {
            CTabValue* pTabVal = pTabVar->GetArrTabVals().GetAt(iVal);
            CString sStatType;
            if (pTabVal->GetStatId() == -1) {
                switch (pTabVal->GetTabValType()) {
                    case DICT_TABVAL:
                    case SPECIAL_TABVAL:
                    case SPECIAL_MISSING_TABVAL:
                    case SPECIAL_REFUSED_TABVAL:
                    case SPECIAL_NOTAPPL_TABVAL:
                    case SPECIAL_DEFAULT_TABVAL:
                    case SPECIAL_UNDEFINED_TABVAL:
                        sStatType = _T("Counts");
                        break;
                    case STAT_TOTAL_TABVAL:
                        // check for % only in which case the total is actually % total and nrow is total
                        if (pVarTallyFmt->FindFirstStatPos(_T("Counts")) == NONE) {
                            sStatType = _T("Total Percent");
                        }
                        else {
                            sStatType = _T("Total");
                        }
                        break;
                    case INVALID_TABVAL:
                    case RDRBRK_TABVAL:
                        // leave it blank
                        break;
                    case STAT_MIN_TABVAL:
                        sStatType = _T("Minimum");
                        break;
                    case STAT_MAX_TABVAL:
                        sStatType = _T("Maximum");
                        break;
                    case STAT_MEAN_TABVAL:
                        sStatType = _T("Mean");
                        break;
                    case STAT_MODE_TABVAL:
                        sStatType = _T("Mode");
                        break;
                   case STAT_STDDEV_TABVAL:
                        sStatType = _T("Standard Deviation");
                        break;
                   case STAT_STDERR_TABVAL:
                        sStatType = _T("StdErr");
                        break;
                    case STAT_MEDIAN_TABVAL:
                        sStatType = _T("Median");
                        break;
                    case STAT_VARIANCE_TABVAL:
                        sStatType = _T("Variance");
                        break;
                    case STAT_PCT_TABVAL:
                        sStatType = _T("Percents");
                        break;
                    case STAT_NTILE_TABVAL:
                        sStatType = _T("N-tiles");
                        break;
                    case STAT_NROW_TABVAL:
                        sStatType = _T("Total");
                        break;
                    //Savy (R) sampling app 20081202
                    case STAT_SAMPLING_R_STABVAL:
                        sStatType = _T("R");
                        break;
                    case STAT_SAMPLING_SE_STABVAL:
                        sStatType = _T("SE");
                        break;
                    case STAT_SAMPLING_N_UNWE_STABVAL:
                        sStatType = _T("N-UNWE");
                        break;
                    case STAT_SAMPLING_N_WEIG_STABVAL:
                        sStatType = _T("N-WEIG");
                        break;
                    case STAT_SAMPLING_SER_STABVAL:
                        sStatType = _T("SER");
                        break;
                    case STAT_SAMPLING_SD_STABVAL:
                        sStatType = _T("SD");
                        break;
                    case STAT_SAMPLING_DEFT_STABVAL:
                        sStatType = _T("DEFT");
                        break;
                    case STAT_SAMPLING_ROH_STABVAL:
                        sStatType = _T("ROH");
                        break;
                    case STAT_SAMPLING_SE_R_STABVAL:
                        sStatType = _T("SE-R");
                        break;
                    case STAT_SAMPLING_R_N2SE_STABVAL:
                        sStatType = _T("R-2SE");
                        break;
                    case STAT_SAMPLING_R_P2SE_STABVAL:
                        sStatType = _T("R+2SE");
                        break;
                    case STAT_SAMPLING_SAMP_BASE_STABVAL:
                        sStatType = _T("SAMP_BASE");
                        break;
                    case STAT_SAMPLING_B_STABVAL:
                        sStatType = _T("B");
                        break;

                }

                int iStat = pVarTallyFmt->FindFirstStatPos(sStatType);
                pTabVal->SetStatId(iStat);
            }
        }
    }

    // recurse on children
    for(int iIndex =0; iIndex < pTabVar->GetNumChildren();iIndex++){
        CTabVar* pChildVar = pTabVar->GetChild(iIndex);
        Number3031TabVals(pTbl, pChildVar);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSet::FixSysTotalVarTallyFmts
//
// In new var tally fmt scheme, sys total variables must have
// var tally fmts that are different from default since they
// can't contain counts.  This routine is called when loading legacy files
// to add in the appropriate fmts.
//
/////////////////////////////////////////////////////////////////////////////
void CTabSet::FixSysTotalVarTallyFmts()
{
    for(int iTbl =0; iTbl < GetNumTables(); iTbl++) {
        CTable* pTbl = GetTable(iTbl);
        FixSysTotalVarTallyFmts(pTbl->GetRowRoot(), FMT_ID_TALLY_ROW);
        FixSysTotalVarTallyFmts(pTbl->GetColRoot(), FMT_ID_TALLY_COL);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSet::FixSysTotalVarTallyFmts
//
/////////////////////////////////////////////////////////////////////////////
void CTabSet::FixSysTotalVarTallyFmts(CTabVar* pTabVar, FMT_ID eFmtID)
{
    if (pTabVar->GetNumChildren() == 1) {

        CTabVar* pChildVar = pTabVar->GetChild(0);
        if (pChildVar->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) ==0) {

            // if we have default fmt then need to fix it - newer files should
            // have non-default already
            if (pTabVar->GetTallyFmt() == 0 || pTabVar->GetTallyFmt()->GetIndex() == 0) {

                CTallyFmt* pDefTableTallyFmt= DYNAMIC_DOWNCAST(CTallyFmt,m_fmtReg.GetFmt(eFmtID));

                ASSERT(pDefTableTallyFmt);

                // make special fmt for sys total that contains only total stat (no freqs)
                CTallyFmt* pSysTotalVarFmt = new CTallyFmt;
                pSysTotalVarFmt->Init();
                pSysTotalVarFmt->SetID(pDefTableTallyFmt->GetID());
                pSysTotalVarFmt->SetIndex(m_fmtReg.GetNextCustomFmtIndex(*pDefTableTallyFmt));
                pSysTotalVarFmt->ClearStats();
                pSysTotalVarFmt->AddStat(CTallyVarStatFmtFactory::GetInstance()->Create(_T("Total")));
                m_fmtReg.AddFmt(pSysTotalVarFmt);

                pChildVar->SetTallyFmt(pSysTotalVarFmt);
            }
        }
    }
}
/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSet::Build
//
/////////////////////////////////////////////////////////////////////////////

bool CTabSet::Build(CSpecFile& specFile, std::shared_ptr<ProgressDlg> pDlgProgress, const CString& sVersion, bool bSilent /*=false*/)
{
    CString sCmd, sArg;
    bool bResult = true;

    while (specFile.GetLine(sCmd, sArg) == SF_OK) {
        if (sCmd[0] != '[')  {
            if (sCmd.CompareNoCase(XTS_CMD_LABEL) == 0)  {
                SetLabel(TableLabelSerializer::Parse(sArg, sVersion));
            }
            else if (sCmd.CompareNoCase(XTS_CMD_NAME) == 0)  {
                SetName(sArg);
            }
            else if (sCmd.CompareNoCase(XTS_CMD_SPECTYPE) == 0)  {
                m_eSpecType=FREQ_SPEC;
            }
            else if (sCmd.CompareNoCase(XTS_CMD_PRINTERCHANGED) == 0)  {
                SetSelectedPrinterChanged(sArg.CompareNoCase(XTS_ARG_YES)==0);      // csc 12/3/04
            }
            else if (sCmd.CompareNoCase(XTS_ARG_GENLOGIC) == 0)  {
                sArg.Trim();
                if(sArg.CompareNoCase(_T("No")) ==0){
                    m_bGenLogic = false;
                }
                else{
                    m_bGenLogic = true;
                }
            }
            else if (sCmd.CompareNoCase(XTS_CMD_INPUTDATAFILENAME) == 0)  { // 20090915 GHM
                SetInputDataFilename(sArg);
            }
            else {
                // Unrecognized command at line %d:
                if (!bSilent) {
                    m_sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                    m_sError += _T("\n") + sCmd + _T("=") + sArg;
                    AfxMessageBox(m_sError);
                }
                bResult = false;
            }
        }
        else {
            if (!bSilent) {
                pDlgProgress->SetPos((int)specFile.GetPosition()/100);
                if (pDlgProgress->CheckCancelButton())  {
                    bResult = false;
                    break;
                }
            }
            if (sCmd.CompareNoCase(XTS_SECT_DICTS) == 0) {
                specFile.SkipSection();
            }
            else if (sCmd.CompareNoCase(XTS_SECT_TABLE) == 0) {
                CTable* pTable = new CTable();
                pTable->Build(specFile, GetFmtReg(), sVersion, bSilent);
                m_aPrintTable.Add(pTable);
            }
            else if (sCmd.CompareNoCase(XTS_SECT_SAMPSPEC) == 0) {
                BuildSampSpec(specFile, bSilent);
            }
            else if (sCmd.CompareNoCase(XTS_SECT_AREASTRUCT) == 0) {
                m_pConsolidate->Build(specFile, bSilent);
            }
            else if (sCmd.CompareNoCase(XTS_SECT_LEVEL) == 0) {
                CTabLevel* pLevel = new CTabLevel();
                while (specFile.GetLine(sCmd, sArg) == SF_OK) {
                    if (sCmd[0] != '[')  {
                        if (sCmd.CompareNoCase(XTS_CMD_TABLE) == 0) {
                            bool bFound = false;
                            for (int i = 0 ; i < m_aPrintTable.GetSize() ; i++) {
                                if (sArg.CompareNoCase(m_aPrintTable[i]->GetName()) == 0) {
                                    pLevel->AddTable(m_aPrintTable[i]);
                                    bFound = true;
                                    break;
                                }
                            }
                            if (!bFound) {
                                if (!bSilent) {
                                    m_sError.Format(_T("Unrecognized table at line %d:"), specFile.GetLineNumber());
                                    m_sError += _T("\n") + sCmd + _T("=") + sArg;
                                    AfxMessageBox(m_sError);
                                }
                                bResult = false;
                            }
                        }
                        else {
                            // Unrecognized command at line %d:
                            if (!bSilent) {
                                m_sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                                m_sError += _T("\n") + sCmd + _T("=") + sArg;
                                AfxMessageBox(m_sError);
                            }
                            bResult = false;
                        }
                    }
                    else {
                        specFile.UngetLine();
                        break;
                    }
                }
                m_aTabLevel.Add(pLevel);
            }
            else if (sCmd.CompareNoCase(TFT_SECT_FORMAT_TABSET) == 0){  // csc 11/21/2003
                specFile.UngetLine();
                m_fmtReg.Build(specFile, sVersion);
            }
            else {
                m_sError.Format(_T("Invalid section heading at line %d:"), specFile.GetLineNumber()); // Invalid section heading at line %d:
                m_sError += _T("\n") + sCmd;
                if (!bSilent) {
                    AfxMessageBox(m_sError);
                    specFile.SkipSection();
                }
                bResult = false;
            }
        }
    }

    // csc 11/21/2003
    // add extra table objects (header, footer, etc.) based on formatting info

    // loop through the tables in this set
    for (int iTbl=0 ; iTbl<GetNumTables() ; iTbl++)  {
        CTable* pTbl = GetTable(iTbl);
        pTbl->SetFmtRegPtr(&m_fmtReg);

        // get the table's format, or use the default if a custom one is not provided
        if (NULL==pTbl->GetFmt())  {// borders/leadering/reader brk etc
            CTblFmt* pTblFmt = DYNAMIC_DOWNCAST(CTblFmt, m_fmtReg.GetFmt(FMT_ID_TABLE));
            pTbl->SetFmt(pTblFmt);
        }
        if (NULL==pTbl->GetTblPrintFmt())  {//print stuff
            CTblPrintFmt* pTblPrintFmt = DYNAMIC_DOWNCAST(CTblPrintFmt, m_fmtReg.GetFmt(FMT_ID_TBLPRINT));
            pTbl->SetTblPrintFmt(pTblPrintFmt);
        }


        CTblFmt* pTblFmt = DYNAMIC_DOWNCAST(CTblFmt, pTbl->GetFmt());
        ASSERT_VALID(pTblFmt);

        // get the application's format, or use the default if a custom one is not provided
    /*  if (NULL==GetFmt())  {
            SetFmt(DYNAMIC_DOWNCAST(CAppFmt, m_fmtReg.GetFmt(TFT_FMT_ID_APP)));
        }*/
/*      CAppFmt* pAppFmt = GetFmt();
        ASSERT_VALID(pAppFmt);*/

        // add left header
        CTblOb* pHdrObLeft = pTbl->GetHeader(0);
        if (NULL==pHdrObLeft->GetFmt()) {
            pHdrObLeft->SetFmt((CFmt*)m_fmtReg.GetFmt(FMT_ID_HEADER_LEFT));
        }

        // add center header
        CTblOb* pHdrObCenter = pTbl->GetHeader(1);
        if (NULL==pHdrObCenter->GetFmt()) {
            pHdrObCenter->SetFmt((CFmt*)m_fmtReg.GetFmt(FMT_ID_HEADER_CENTER));
        }

        // add right header
        CTblOb* pHdrObRight = pTbl->GetHeader(2);
        if (NULL==pHdrObRight->GetFmt()) {
            pHdrObRight->SetFmt((CFmt*)m_fmtReg.GetFmt(FMT_ID_HEADER_RIGHT));
        }

        // add left footer
        CTblOb* pFtrObLeft = pTbl->GetFooter(0);
        if (NULL==pFtrObLeft->GetFmt()) {
            pFtrObLeft->SetFmt((CFmt*)m_fmtReg.GetFmt(FMT_ID_FOOTER_LEFT));
        }

        // add center footer
        CTblOb* pFtrObCenter = pTbl->GetFooter(1);
        if (NULL==pFtrObCenter->GetFmt()) {
            pFtrObCenter->SetFmt((CFmt*)m_fmtReg.GetFmt(FMT_ID_FOOTER_CENTER));
        }

        // add right footer
        CTblOb* pFtrObRight = pTbl->GetFooter(2);
        if (NULL==pFtrObRight->GetFmt()) {
            pFtrObRight->SetFmt((CFmt*)m_fmtReg.GetFmt(FMT_ID_FOOTER_RIGHT));
        }

        // add title
        CTblOb* pTitleOb = pTbl->GetTitle();
        pTitleOb->SetText(pTitleOb->GetText());  // different from title template
        if (NULL==pTitleOb->GetFmt()) {
            pTitleOb->SetFmt((CFmt*)m_fmtReg.GetFmt(FMT_ID_TITLE));
        }

        // add subtitle
        CTblOb* pSubTitleOb = pTbl->GetSubTitle();
        pSubTitleOb->SetText(pSubTitleOb->GetText());
        if (NULL==pSubTitleOb->GetFmt()) {
            pSubTitleOb->SetFmt((CFmt*)m_fmtReg.GetFmt(FMT_ID_SUBTITLE));
        }

        // add page note
        CTblOb* pPgNoteOb = pTbl->GetPageNote();
        if (NULL==pPgNoteOb->GetFmt()) {
            pPgNoteOb->SetFmt((CFmt*)m_fmtReg.GetFmt(FMT_ID_PAGENOTE));
        }

        // add end note
        CTblOb* pEndNoteOb = pTbl->GetEndNote();
        if (NULL==pEndNoteOb->GetFmt()) {
            pEndNoteOb->SetFmt((CFmt*)m_fmtReg.GetFmt(FMT_ID_ENDNOTE));
        }

        // add default cell //default from the registry
        /*CDataCell* pDefaultCell = pTbl->GetDefaultCell();
        if (NULL==pDefaultCell->GetFmt()) {
            pDefaultCell->SetFmt((CFmt*)m_fmtReg.GetFmt(TFT_FMT_ID_CELL));
        }
        if (NULL==pDefaultCell->GetDataCellFmt()) {
            pDefaultCell->SetDataCellFmt(pAppFmt->GetDataCellFmtPtr());
        }*/

        // add default stubhead
        CTblOb* pStubhead = pTbl->GetStubhead(0);
        if (NULL==pStubhead->GetFmt()) {
            pStubhead->SetFmt((CFmt*)m_fmtReg.GetFmt(FMT_ID_STUBHEAD));
        }

        // add default secondary stubhead
        CTblOb* pSecStubhead = pTbl->GetStubhead(1);
        if (NULL==pSecStubhead->GetFmt()) {
            pSecStubhead->SetFmt((CFmt*)m_fmtReg.GetFmt(FMT_ID_STUBHEAD_SEC));
        }

        // add default area caption
        CTblOb* pAreaCaption= pTbl->GetAreaCaption();
        if (NULL==pAreaCaption->GetFmt()) {
            pAreaCaption->SetFmt((CFmt*)m_fmtReg.GetFmt(FMT_ID_AREA_CAPTION));
        }

        // add default oneRowCol Total
        CTblOb* pOneRowTotal= pTbl->GetOneRowColTotal();
        if (NULL==pOneRowTotal->GetFmt()) {
            pOneRowTotal->SetFmt((CFmt*)m_fmtReg.GetFmt(FMT_ID_STUB));
        }
    }
    // and finally set the tabsetfmt
    if(!m_pTabSetFmt){
        m_pTabSetFmt = DYNAMIC_DOWNCAST(CTabSetFmt,m_fmtReg.GetFmt(FMT_ID_TABSET));
    }

    return bResult;
}

bool CTabSet::BuildSampSpec(CSpecFile &specFile, bool bSilent /*= false*/)
{
    CString sCmd, sArg, sError;
    csprochar* pszArgs;//Savy (R) sampling app 20090102
    csprochar* pszArg;//Savy (R) sampling app 20090102
    bool bResult = true;

    m_bIsSamplingErrApp = true;
    m_eSampMethodType =METHOD_NONE;
    m_arrDomVar.RemoveAll();

    while (specFile.GetLine(sCmd, sArg) == SF_OK) {
        if (sCmd[0] != '[')  {
            if (sCmd.CompareNoCase(XTS_CMD_SAMP_METHOD) == 0)  {
                if (sArg.CompareNoCase(XTS_ARG_TAYLOR) == 0) {
                    m_eSampMethodType = TAYLOR_SERIES_METHOD;
                }
                else if (sArg.CompareNoCase(XTS_ARG_JACKKNIFE) == 0) {
                    m_eSampMethodType = JACK_KNIFE_METHOD;
                }
                else if (sArg.CompareNoCase(XTS_ARG_BRRP) == 0) {
                    m_eSampMethodType =  BRRP_METHOD;
                }
                else {
                    m_eSampMethodType = TAYLOR_SERIES_METHOD;
                    if (!bSilent) {
                        sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                        sError += _T("\n") + sCmd + _T("=") + sArg;
                        AfxMessageBox(sError);
                    }
                    bResult = false;
                }
            }
            else if (sCmd.CompareNoCase(XTS_CMD_CLUSTER_VAR) == 0) {
                sArg.Trim();
                m_sClusterVar = sArg;
            }
            else if (sCmd.CompareNoCase(XTS_CMD_STRATA_VAR) == 0) {
                sArg.Trim();
                m_sStrataVariable = sArg;
            }
            else if (sCmd.CompareNoCase(XTS_CMD_STRATA_FILE) == 0) {
                sArg.Trim();
                //Savy (R) sampling app 20090102
                pszArgs = sArg.GetBuffer(sArg.GetLength());
                if(sArg.Find(_T(","))>=0){
                    pszArg = strtoken(pszArgs, _T(","), NULL);  // this yields the time/date stuff
                    pszArg = strtoken(NULL, _T(","), NULL);     // now we've got the file name
                }
                else {
                    pszArg=pszArgs;
                }

                CString sPath;
                sPath = specFile.GetFilePath();
                PathRemoveFileSpec(sPath.GetBuffer(MAX_PATH));
                sPath.ReleaseBuffer();

                m_sStrataFileName = WS2CS(MakeFullPath(sPath, pszArg));
            }
            else if (sCmd.CompareNoCase(XTS_CMD_DOMAIN_VAR) == 0) {
                DOMAINVAR domainVariable;
                sArg.Trim();
                domainVariable.m_sDomainVarName = sArg;
                m_arrDomVar.Add(domainVariable);
            }
        }
        else {
            specFile.UngetLine();
            break;
        }
    }

    return bResult;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSet::SetDefaultFmts
//
//  Sets default formats for variables and values, as needed.
//  csc 7/14/04
//
/////////////////////////////////////////////////////////////////////////////

void CTabSet::SetDefaultFmts(CTabVar* pVar, CFmt* pFmtDefaultVar, CFmt* pFmtDefaultVal)
{
    if (pVar->GetFmt()==NULL) {
        pVar->SetFmt(pFmtDefaultVar);
    }
    for (int iVal=0 ; iVal<pVar->GetNumValues() ; iVal++) {
        CTabValue* pVal=pVar->GetValue(iVal);
        if (pVal->GetFmt()==NULL) {
            pVal->SetFmt(pFmtDefaultVal);
        }
    }
    for (int iVar=0 ; iVar<pVar->GetNumChildren() ; iVar++) {
        CTabVar* pChildVar=pVar->GetChild(iVar);
        SetDefaultFmts(pChildVar, pFmtDefaultVar, pFmtDefaultVal);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSet::Save
//
/////////////////////////////////////////////////////////////////////////////

bool CTabSet::Save(const CString& sSpecFilePath, const CString& sDictFilePath, const CString& sInputDataFilename)
{
    CSpecFile specFile;
    if (!specFile.Open(sSpecFilePath, CFile::modeWrite))  {
        m_sError = _T("Error:  Could not open file ") + sSpecFilePath;
        AfxMessageBox(m_sError,MB_ICONEXCLAMATION);
        return false;
    }

    // Save header
    specFile.PutLine(XTS_SECT_TABSPEC);
    specFile.PutLine(CMD_VERSION, CSPRO_VERSION);
    if (m_sLabel.IsEmpty()) {
        m_sLabel = GetFileName(sSpecFilePath);
        int pos = m_sLabel.ReverseFind(DOT);
        if (pos > 0) {
            m_sLabel = m_sLabel.Left(pos);
        }
    }
    if (m_sName.IsEmpty()) {
        m_sName = CIMSAString::MakeName(m_sLabel);
        m_sName += _T("_TF"); //append this to avoid duplicate names.
    }
    specFile.PutLine(XTS_CMD_LABEL, TableLabelSerializer::Create(m_sLabel));
    specFile.PutLine(XTS_CMD_NAME, m_sName);

    if(!m_bGenLogic){
        specFile.PutLine(XTS_ARG_GENLOGIC ,XTS_ARG_NO);
    }
    else {
        specFile.PutLine(XTS_ARG_GENLOGIC ,XTS_ARG_YES);
    }

    if(m_eSpecType == FREQ_SPEC) {
        specFile.PutLine(XTS_CMD_SPECTYPE, _T("Freq"));
    }

    if (HasSelectedPrinterChanged()) {
        specFile.PutLine(XTS_CMD_PRINTERCHANGED, XTS_ARG_YES);  // csc 12/3/04
    }

    if( sInputDataFilename.Compare(_T("")) != 0 ) { // GHM 20090915

        // GHM 20090917 we'll check to make sure that a &I is actually in a header or footer
        // before outputting the InputDataFilename code; this will allow 4.0.003+ tables
        // that don't use this feature to still be read by earlier versions of 4.0

        // this code could safely be removed for 4.1+ releases

        /*bool featureEnabled = false;

        for( int tableNum = 0; tableNum < GetNumTables() && !featureEnabled; tableNum++ )
        {
            CTable * pTbl = GetTable(tableNum);

            for( int i = 0; i < 3; i++ ) // left, center, right headers/footers
            {
                CFmt * pFmtHeader = pTbl->GetHeader(i)->GetDerFmt();
                CFmt * pFmtFooter = pTbl->GetFooter(i)->GetDerFmt();

                if( ( pFmtHeader && pFmtHeader->GetCustom().m_sCustomText.Find(_T("&I")) != -1 ) ||
                    ( pFmtFooter && pFmtFooter->GetCustom().m_sCustomText.Find(_T("&I")) != -1 ) )
                {
                    featureEnabled = true;*/

                    specFile.PutLine(XTS_CMD_INPUTDATAFILENAME,sInputDataFilename); // keep for 4.1
                    m_sInputDataFilename = sInputDataFilename; // keep for 4.1

                    /*break; // end the second for loop (featureEnabled will end the first for loop)
                }
            }
        }*/

        // end code section that can be removed for 4.1+ releases (though keep two lines noted above)
    }


    // Save dictionary name
    if( !sDictFilePath.IsEmpty() ) {
        CString sFile = GetRelativeFName<CString>(sSpecFilePath, sDictFilePath);
        specFile.PutLine (_T(" "));
        specFile.PutLine (XTS_SECT_DICTS);
        specFile.PutLine (XTS_CMD_FILE, sFile);
    }

    //If the .xts belong to sampling app
    if(m_bIsSamplingErrApp){
        SaveSamplingSection(specFile);
    }

    // Save area specifications
    if (m_pConsolidate->GetNumAreas() > 0) {
        m_pConsolidate->Save(specFile);
    }

    //before saving the format reset the fmt used flag and update it to
    //have only fmts which are used.
     // Save each table
    m_fmtReg.ResetFmtUsedFlag();
    if(m_pTabSetFmt){//Frequencies save does not set the default fmt. So this may be null
        m_pTabSetFmt->SetUsedFlag(true);
    }
    for (int i = 0 ; i < m_aPrintTable.GetSize() ; i++) {
        UpdateFmtFlag(m_aPrintTable[i]);
    }

    // save format registry
    m_fmtReg.Save(specFile);

    // Save each table
    for (int i = 0 ; i < m_aPrintTable.GetSize() ; i++) {
        if(m_eSpecType == FREQ_SPEC){
             m_aPrintTable[i]->m_bSaveFreqStats = true;
        }
        else {
             m_aPrintTable[i]->m_bSaveFreqStats = false;
        }
        //Savy (R) sampling app 20090415
        m_aPrintTable[i]->GetTableUnit().SetSamplingErrAppFlag(m_bIsSamplingErrApp);
        m_aPrintTable[i]->Save(specFile);
    }
    // Save level structure
    for (int l = 0 ; l < m_aTabLevel.GetSize() ; l++) {
        m_aTabLevel[l]->Save(specFile);
    }

    specFile.Close();

    return true;
}

void CTabSet::SaveSamplingSection(CSpecFile& specFile)
{
    specFile.PutLine (_T(" "));
    specFile.PutLine (XTS_SECT_SAMPSPEC);

    //Sampling Method
    switch(m_eSampMethodType){
        case TAYLOR_SERIES_METHOD:
            specFile.PutLine (XTS_CMD_SAMP_METHOD, XTS_ARG_TAYLOR);
            break;
        case JACK_KNIFE_METHOD:
            specFile.PutLine (XTS_CMD_SAMP_METHOD, XTS_ARG_JACKKNIFE);
            break;
        case BRRP_METHOD:
            specFile.PutLine (XTS_CMD_SAMP_METHOD, XTS_ARG_BRRP);
            break;
        default:
            break;
    }

    //ClusterVariable
    specFile.PutLine (XTS_CMD_CLUSTER_VAR, m_sClusterVar);

    //Strata Variable
    if(!m_sStrataVariable.IsEmpty()){
        specFile.PutLine (XTS_CMD_STRATA_VAR, m_sStrataVariable);
    }
    else if(!m_sStrataFileName.IsEmpty()){
        //Savy (R) sampling app 20090102
        CString sStrataFileName = GetRelativeFName<CString>(specFile.GetFilePath(), m_sStrataFileName);
        specFile.PutLine (XTS_CMD_STRATA_FILE, sStrataFileName);
    }

    //DOMAIN VARIABLES
    for(int iIndex=0; iIndex<m_arrDomVar.GetCount();iIndex++){
        specFile.PutLine (XTS_CMD_DOMAIN_VAR, m_arrDomVar[iIndex].m_sDomainVarName);
    }

    specFile.PutLine (_T(" "));
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSet::Reconcile
//
/////////////////////////////////////////////////////////////////////////////

bool CTabSet::Reconcile(CString& sError, bool bSilent, bool bAutoFix)
{
    UNREFERENCED_PARAMETER(bAutoFix);
    UNREFERENCED_PARAMETER(sError);
    bool bRet  = false; //If anything changed
    if(m_pDataDict) { //If the data dict is available  then it is reconcile for the designer
        //Do Reconcile Levels
        ReconcileLevels4Tbl(); // Add the messages later
        //Do Reconcile Tables
        CString sTableName;
        CString sTblMsg;
        for (int iTbl=0 ; iTbl<GetNumTables() ; iTbl++)  {
            CTable* pTbl = GetTable(iTbl);
            sTableName = pTbl->GetName() + _T(":\r\n");
            sTblMsg = _T("");
            CString sRowMsg,sColMsg,sTabValMsg;
            //Reconcile VSets for tables  if not in the dictionary delete
            pTbl->ReconcileTabVar(GetDict(), GetWorkDict(), pTbl->GetRowRoot(), sRowMsg, bSilent);
            pTbl->ReconcileTabVar(GetDict(), GetWorkDict(), pTbl->GetColRoot(), sColMsg, bSilent);

            sRowMsg.IsEmpty() ? sTblMsg  = sTblMsg : sTblMsg += sRowMsg;
            sColMsg.IsEmpty() ? sTblMsg  = sTblMsg : sTblMsg += sColMsg;

            CString sMultMsg;
            DoReconcileMultiRecordChk(pTbl,sMultMsg);
            sMultMsg.IsEmpty() ? sTblMsg  = sTblMsg : sTblMsg += sMultMsg;

            CString sMultItemMsg;
            DoReconcileMultiItemChk(pTbl,sMultItemMsg);
            sMultItemMsg.IsEmpty() ? sTblMsg  = sTblMsg : sTblMsg += sMultItemMsg;

            //TBW  viewer will be affected with this 'cos of "Total" and "Percent" stuff.So call it from the frame
            //Now reconcile tabvals and/or special vals  for the  values. If not present delete
            //Remove the specials stuff before the reconcile of the tabvals begin
            if(GetDict()){
                pTbl->SetReconcileErrMsg(_T(""));
                pTbl->ReconcileTabVals(pTbl->GetRowRoot(), GetDict(), GetWorkDict());
                pTbl->ReconcileTabVals(pTbl->GetColRoot(), GetDict(), GetWorkDict());
                sTabValMsg = pTbl->GetReconcileErrMsg();
                pTbl->SetReconcileErrMsg(_T(""));
                sTabValMsg.IsEmpty() ? sTblMsg = sTblMsg : sTblMsg += sTabValMsg + _T("\r\n");
            }

            // reconcile table & subtable units
            CArray<CStringArray,CStringArray&> arrValidSubtableUnits;
            CStringArray validTblUnits;
            CString sUnitMsg;
            UpdateSubtableList(pTbl, arrValidSubtableUnits, validTblUnits, true, &sUnitMsg);
            sUnitMsg.IsEmpty() ? sTblMsg = sTblMsg : sTblMsg += sUnitMsg + _T("\r\n");

            if(pTbl->IsDirty()){
                bRet = true;
            }
            if(!sTblMsg.IsEmpty()){
                sTblMsg  = sTableName + sTblMsg;  // BMD 25 May 2006
                pTbl->SetReconcileErrMsg(sTblMsg);
            }

            //Savy (R) sampling app 20081202
            //m_bIsSamplingErrApp = true;
            if(!GetSamplingErrAppFlag()){
                AddSystemTotalVar(pTbl);
            }
            else {
                AddStatVar(pTbl);
            }

        }

    }
    else { //we do reconcile for the viewer
        //Do here reconcile for the format stuff ??
        //or any thing pertaining to the viewer
    }
    return bRet;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool ReconcileCon(CString& sError, bool bSilent, bool bAutoFix)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabSet::ReconcileCon(CString& sError, bool bSilent /*= false*/)
{
    bool bRet  = false; //If anything changed

    if(m_pDataDict) { //If the data dict is available  then it is reconcile for the designer
        ASSERT(m_pConsolidate);
        bool bAreaNameFound = false;
        int iNumAreas = m_pConsolidate->GetNumAreas();
        int iLevel,iRecord,iItem,iVSet;
        for(int iIndex =iNumAreas-1; iIndex >= 0; iIndex--){
            CString sAreaName = m_pConsolidate->GetArea(iIndex);
            bAreaNameFound = LookupName(sAreaName,&iLevel,&iRecord,&iItem,&iVSet);
            if(!bAreaNameFound){
                bRet = true; // Reconciled .
                sError += sAreaName + _T(" removed from Area Items; not in dictionary\r\n");
                m_pConsolidate->RemoveArea(iIndex);
            }
        }
    }

    if(bRet && !bSilent){
        AfxMessageBox(sError);
    }

    return bRet;
}
/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSet::ReconcileName
//
/////////////////////////////////////////////////////////////////////////////

bool CTabSet::ReconcileName(const CDataDict& dictionary)
{
    ASSERT(GetDict() == &dictionary);

    // dictionary: name change does not matter
    // level: we don't care, as we don't use in the tables
    // record: we care: search and replace unit looping var
    // item: we care
    // value set: we care
    const DictNamedBase* dict_element = dictionary.GetChangedObject();
    bool name_changed = false;

    if( dict_element != nullptr && dict_element->GetElementType() == DictElementType::Record ||
                                   dict_element->GetElementType() == DictElementType::Item || 
                                   dict_element->GetElementType() == DictElementType::ValueSet )
    {
        for( int i = 0; i < GetNumTables(); ++i )
        {
            CTable* pTable = GetTable(i);

            name_changed |= pTable->GetRowRoot()->ReconcileName(dictionary.GetOldName(), dict_element->GetName());
            name_changed |= pTable->GetColRoot()->ReconcileName(dictionary.GetOldName(), dict_element->GetName());

            //reconcile name change in weight, universe, value
            ReconcileName(pTable, dictionary.GetOldName(), dict_element->GetName());
        }        
    }

    return name_changed;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabSet::ReconcileName(CTable* pTbl, CString sOldName, sNewName)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabSet::ReconcileName(CTable* pTbl, CString sOldName, CString sNewName)
{
    CArray<CUnitSpec,CUnitSpec&>&arrUnits = pTbl->GetUnitSpecArr();
    int iNumUnits = arrUnits.GetSize();
    CIMSAString sUniverse,sWeightExpr,sValue,sLoopingVarName;
    CUnitSpec* pUnitSpec = NULL;
    for(int iUnit =-1; iUnit < iNumUnits;iUnit++){

        if (iUnit == -1){
            pUnitSpec = &pTbl->GetTableUnit();
        }
        else {
            pUnitSpec = &arrUnits.GetAt(iUnit);
        }
        CUnitSpec& unitSpec = *pUnitSpec;
        int iNumReplaces = 0;
        sLoopingVarName = unitSpec.GetLoopingVarName();
        iNumReplaces = sLoopingVarName.ReplaceNames(sOldName,sNewName);
        if(iNumReplaces > 0) {
           unitSpec.SetLoopingVarName(sLoopingVarName);
           pTbl->SetDirty();
        }

        sWeightExpr = unitSpec.GetWeightExpr();
        iNumReplaces = sWeightExpr.ReplaceNames(sOldName,sNewName);
        if(iNumReplaces > 0) {
            unitSpec.SetLoopingVarName(sLoopingVarName);
            unitSpec.SetWeightExpr(sWeightExpr);
        }

        sValue = unitSpec.GetValue();
        iNumReplaces = sValue.ReplaceNames(sOldName,sNewName);
        if(iNumReplaces > 0) {
            pTbl->SetDirty();
            unitSpec.SetValue(sValue);
        }

        sUniverse= unitSpec.GetUniverse();
        iNumReplaces =sUniverse.ReplaceNames(sOldName,sNewName);
        if(iNumReplaces > 0){
            pTbl->SetDirty();
            unitSpec.SetValue(sUniverse);
        }

    }
}
/////////////////////////////////////////////////////////////////////////////
//
//                               CTabSet::ReconcileName
//
/////////////////////////////////////////////////////////////////////////////

bool CTabSet::ReconcileLabel(const CDataDict& dictionary)
{
    ASSERT(GetDict() == &dictionary);
    bool modified = false;

    // only label changes in value sets matter
    if( dictionary.GetChangedObject() != nullptr && dictionary.GetChangedObject()->GetElementType() == DictElementType::ValueSet )
    {
        const DictValueSet* dict_value_set = assert_cast<const DictValueSet*>(dictionary.GetChangedObject());

        for( int i = 0; i < GetNumTables(); ++i )
        {
            CTable* pTbl = GetTable(i);
            pTbl->SetReconcileErrMsg(_T(""));
            pTbl->ReconcileTabVals(pTbl->GetRowRoot(), GetDict(), GetWorkDict());
            pTbl->ReconcileTabVals(pTbl->GetColRoot(), GetDict(), GetWorkDict());
            pTbl->SetReconcileErrMsg(_T(""));
        }

        modified = true;
    }

    return modified;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabSet::DoMultiItemChk(CTabVar* pTabVar,CDictItem* pOccDictItem)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabSet::DoMultiItemChk(CTabVar* pTabVar,const CDictItem* pOccDictItem)
{
    bool bRet = true;
    const DictLevel*      pDictLevel;
    const CDictRecord*    pDictRecord;
    const CDictItem*      pDictItem;
    const DictValueSet*   pDictVSet;

    if(pOccDictItem->GetOccurs() <= 1){
        return true;
    }

    if(!pTabVar->IsRoot()){
        const CDataDict* pDict = LookupName(pTabVar->GetName(), &pDictLevel, &pDictRecord, &pDictItem, &pDictVSet);
        if (!pDict) {
            ASSERT(FALSE);
        }
        ASSERT(pDictItem);
        if( pDictItem && pDictItem->GetOccurs() > 1 && pOccDictItem !=pDictItem ){
                return false;
        }
    }
    for(int iIndex =0; iIndex < pTabVar->GetNumChildren(); iIndex++){
        CTabVar* pChildVar = pTabVar->GetChild(iIndex);
        if(!DoMultiItemChk(pChildVar,pOccDictItem)){
            bRet = false;
            break;
        }
    }

    return bRet;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabSet::DoMultiItemChk(CTable* pTable,CDictItem* pOccDictItem,CTabVar* pTargetVar,bool bFromRow,bool bIsPlusOper)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabSet::DoMultiItemChk(CTable* pTable,const CDictItem* pOccDictItem,CTabVar* pTargetVar,bool bFromRow,bool bIsPlusOper)
{
    bool bRet = true;
    ASSERT(pOccDictItem);

    if(pOccDictItem->GetOccurs()<= 1){
        return true;
    }
    if(bFromRow){
        //check col vars for multiple form a different record
        if(!DoMultiItemChk(pTable->GetColRoot(),pOccDictItem)){
            return false;
        }
        if(pTargetVar->IsRoot() ||(pTargetVar->GetParent()->IsRoot() && bIsPlusOper)){
            return true;
        }
        else {
            const DictLevel*      pDictLevel;
            const CDictRecord*    pDictRecord;
            const CDictItem*      pDictItem;
            const DictValueSet*   pDictVSet;
            const CDataDict* pDict = LookupName(pTargetVar->GetName(), &pDictLevel, &pDictRecord, &pDictItem, &pDictVSet);
            if(pDict){
                if(pDictItem && pDictItem->GetOccurs() > 1 && pOccDictItem != pDictItem ){
                    return false;
                }
            }
        }
    }
    else{
        //check row vars for multiple form a different record
        if(!DoMultiItemChk(pTable->GetRowRoot(),pOccDictItem)){
            return false;
        }
        if(pTargetVar->IsRoot() ||(pTargetVar->GetParent()->IsRoot() && bIsPlusOper)){
            return true;
        }
        else {
            const DictLevel*      pDictLevel;
            const CDictRecord*    pDictRecord;
            const CDictItem*      pDictItem;
            const DictValueSet*   pDictVSet;
            const CDataDict* pDict = LookupName(pTargetVar->GetName(), &pDictLevel, &pDictRecord, &pDictItem, &pDictVSet);
            if(pDict){
                if(pDictItem && pDictItem->GetOccurs() > 1 && pOccDictItem != pDictItem ){
                    return false;
                }
            }
        }
    }

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabSet::DoReconcileMultiRecordChk(CTable* pTbl,CString& sMsg)
//
/////////////////////////////////////////////////////////////////////////////////
CDictItem*  pTblMultItem = NULL; //fix the first multiple item you get as the table's multiple record
bool CTabSet::DoReconcileMultiItemChk(CTable* pTbl,CString& sMsg)
{
    bool bRet = false; //indicates if anything was changed
    bool bSavedTableDirty = pTbl->IsDirty();
    pTbl->SetDirty(false);
    //Do Row variables first
    DoRecursiveMultiItemChk(pTbl,pTbl->GetRowRoot(),true,true,sMsg);
    //Do Column variables
    DoRecursiveMultiItemChk(pTbl,pTbl->GetColRoot(),false,true,sMsg);
    bRet = pTbl->IsDirty();
    if(!bRet){
        pTbl->SetDirty(bSavedTableDirty);
    }
    pTblMultItem = NULL;

    return bRet;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabSet::DoRecursiveMultiItemChk(CTable* pTbl,CTabVar* pTabVar,bool bRow,bool bIsPlusOper,CString& sMsg);
//
/////////////////////////////////////////////////////////////////////////////////
void CTabSet::DoRecursiveMultiItemChk(CTable* pTbl,CTabVar* pTabVar,bool bRow,bool bIsPlusOper,CString& sMsg)
{
    CTabVar* pTargetVar = NULL;
    const DictLevel*      pDictLevel;
    const CDictRecord*    pDictRecord;
    const CDictItem*      pDictItem;
    const DictValueSet*   pDictVSet;
    if(pTabVar->IsRoot()){
        pTblMultItem = NULL;
    }
    const CDictItem*      pTblMultItem = NULL; //fix the first multiple item you get as the table's multiple record
    for(int iVar = 0; iVar < pTabVar->GetNumChildren(); iVar++){
        pTargetVar = pTabVar->GetChild(iVar);
        const CDataDict* pDict = LookupName(pTargetVar->GetName(), &pDictLevel, &pDictRecord, &pDictItem, &pDictVSet);
        if(pDict){
            if(!pTblMultItem && pDictItem && pDictItem->GetOccurs() > 1){
                pTblMultItem = pDictItem;
            }
            if(!pTblMultItem) { //no multiple record item in the table yet
                continue;
            }
            else {
                bool bSameMultiItem = true;
                bSameMultiItem = DoMultiItemChk(pTbl,pTblMultItem,pTargetVar,bRow,bIsPlusOper);
                if(!bSameMultiItem){
                    //delete this variable from the row
                    sMsg += pTargetVar->GetName()+ _T(" deleted from Table; Item from invalid multiple occurrence.\r\n");
                    pTbl->m_bDirty = true;
                    pTargetVar->Remove();
                    delete pTargetVar;
                }
                else if(pTargetVar->GetNumChildren() > 0){
                    DoRecursiveMultiItemChk(pTbl,pTargetVar,bRow,false,sMsg);
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabSet::DoReconcileMultiRecordChk(CTable* pTbl,CString& sMsg)
//
/////////////////////////////////////////////////////////////////////////////////
const CDictRecord*    pTblMultRecord = NULL; //fix the first multiple record item you get as the table's multiple record
bool CTabSet::DoReconcileMultiRecordChk(CTable* pTbl,CString& sMsg)
{
    bool bRet = false; //indicates if anything was changed
    bool bSavedTableDirty = pTbl->IsDirty();
    pTbl->SetDirty(false);
    //Do Row variables first
    DoRecursiveMultRecChk(pTbl,pTbl->GetRowRoot(),true,true,sMsg);
    //Do Column variables
    DoRecursiveMultRecChk(pTbl,pTbl->GetColRoot(),false,true,sMsg);
    bRet = pTbl->IsDirty();
    if(!bRet){
        pTbl->SetDirty(bSavedTableDirty);
    }
    pTblMultRecord = NULL;

    return bRet;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabSet::DoRecursiveMultRecChk(CTable* pTbl,CTabVar* pTabVar,bool bRow,bool bIsPlusOper,CString& sMsg)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabSet::DoRecursiveMultRecChk(CTable* pTbl,CTabVar* pTabVar,bool bRow,bool bIsPlusOper,CString& sMsg)
{
    CTabVar* pTargetVar = NULL;
    const DictLevel*      pDictLevel;
    const CDictRecord*    pDictRecord;
    const CDictItem*      pDictItem;
    const DictValueSet*   pDictVSet;
    if(pTabVar->IsRoot()){
        pTblMultRecord = NULL;
    }
    for(int iVar = 0; iVar < pTabVar->GetNumChildren(); iVar++){
        pTargetVar = pTabVar->GetChild(iVar);
        const CDataDict* pDict = LookupName(pTargetVar->GetName(), &pDictLevel, &pDictRecord, &pDictItem, &pDictVSet);
        if(pDict){
            if(!pTblMultRecord && pDictRecord && pDictRecord->GetMaxRecs() > 1){
                pTblMultRecord = pDictRecord;
            }
            if(!pTblMultRecord) { //no multiple record item in the table yet
                continue;
            }
            else {
                bool bFromSameMultiRec = true;
                bFromSameMultiRec = DoMultiRecordChk(pTbl,pTblMultRecord,pTargetVar,bRow,bIsPlusOper);
                if(!bFromSameMultiRec){
                    //delete this variable from the row
                    sMsg += pTargetVar->GetName()+ _T(" deleted from Table; Item from invalid multiple record.\r\n");
                    pTbl->m_bDirty = true;
                    pTargetVar->Remove();
                    delete pTargetVar;
                }
                else if(pTargetVar->GetNumChildren() > 0){
                    DoRecursiveMultRecChk(pTbl,pTargetVar,bRow,false,sMsg);
                }
            }
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabSet::DoMultiRecordChk(CDictRecord* pMultRecord)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabSet::DoMultiRecordChk(CTable* pTable,const CDictRecord* pMultRecord,CTabVar* pTargetVar,bool bFromRow,bool bIsPlusOper)
{
    bool bRet = true;
    ASSERT(pMultRecord);

    if(pMultRecord->GetMaxRecs()<= 1){
        return true;
    }
    if(bFromRow){
        //check col vars for multiple form a different record
        if(!DoMultiRecordChk(pTable->GetColRoot(),pMultRecord)){
            return false;
        }
        if(pTargetVar->IsRoot() ||(pTargetVar->GetParent()->IsRoot() && bIsPlusOper)){
            return true;
        }
        else {
            const DictLevel*      pDictLevel;
            const CDictRecord*    pDictRecord;
            const CDictItem*      pDictItem;
            const DictValueSet*   pDictVSet;
            const CDataDict* pDict = LookupName(pTargetVar->GetName(), &pDictLevel, &pDictRecord, &pDictItem, &pDictVSet);
            if(pDict){
                if(pDictRecord && pDictRecord->GetMaxRecs() > 1 && pMultRecord != pDictRecord ){
                    return false;
                }
            }
        }
    }
    else{
        //check row vars for multiple form a different record
        if(!DoMultiRecordChk(pTable->GetRowRoot(),pMultRecord)){
            return false;
        }
        if(pTargetVar->IsRoot() ||(pTargetVar->GetParent()->IsRoot() && bIsPlusOper)){
            return true;
        }
        else {
            const DictLevel*      pDictLevel;
            const CDictRecord*    pDictRecord;
            const CDictItem*      pDictItem;
            const DictValueSet*   pDictVSet;
            const CDataDict* pDict = LookupName(pTargetVar->GetName(), &pDictLevel, &pDictRecord, &pDictItem, &pDictVSet);
            if(pDict){
                if(pDictRecord && pDictRecord->GetMaxRecs() > 1 && pMultRecord != pDictRecord ){
                    return false;
                }
            }
        }
    }

    return bRet;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabSet::DoMultiRecordChk(CTabVar* pTabVar,CDictRecord* pMultRecord)
// helper function for the other DoMultiRecordChk
/////////////////////////////////////////////////////////////////////////////////
bool CTabSet::DoMultiRecordChk(CTabVar* pTabVar, const CDictRecord* pMultRecord)
{
    bool bRet = true;
    //returns false if pTabVar is from a record other than pMultRecord
    //if pTabVar is root checks all the children to see if they come form a record other than pMultRecord
    //if so returns false
    const DictLevel*      pDictLevel;
    const CDictRecord*    pDictRecord;
    const CDictItem*      pDictItem;
    const DictValueSet*   pDictVSet;

    if(!pTabVar->IsRoot()){
        const CDataDict* pDict = LookupName(pTabVar->GetName(), &pDictLevel, &pDictRecord, &pDictItem, &pDictVSet);
        if (!pDict) {
            ASSERT(FALSE);
        }
        ASSERT(pDictRecord);
        if(pDictRecord && pDictRecord->GetMaxRecs() >1){
            if(pMultRecord != pDictRecord) {
                bRet = false;
                return HasRecordRelation(*pDictRecord, *pMultRecord);
            }
        }
    }
    for(int iIndex =0; iIndex < pTabVar->GetNumChildren(); iIndex++){
        CTabVar* pChildVar = pTabVar->GetChild(iIndex);
        if(!DoMultiRecordChk(pChildVar,pMultRecord)){
            bRet = false;
            break;
        }
    }

    return bRet;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabSet::OpenAreaNameFile(CString sAreaFileName)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabSet::OpenAreaNameFile(const CString& sAreaFileName)
{
    bool bRet = false;
    bool bSilent = false;
    CString sMsg;

    // GHM 20100818 added so that an area names file isn't required
    if ( SO::IsBlank(sAreaFileName) )
        return bRet;

    if (!PortableFunctions::FileExists(sAreaFileName)) {
        sMsg.Format(_T("Area Name file: %s not found!"), (LPCTSTR)sAreaFileName);
        if (!bSilent) {
            AfxMessageBox(sMsg, MB_ICONEXCLAMATION);
        }
        return bRet;
    }
    if(m_pAreaNameFile){
        CloseAreaNameFile(); // GHM 20110727
        /*m_pAreaNameFile->Close();
        delete m_pAreaNameFile;
        m_pAreaNameFile = NULL;*/
    }
    m_pAreaNameFile = new CSpecFile();
    if (!m_pAreaNameFile->Open(sAreaFileName, CFile::modeRead)) {
        if (!bSilent) {
            AfxMessageBox(_T("Failed to open Area Name File."), MB_ICONEXCLAMATION);
        }
        //delete m_pAreaNameFile;
        CloseAreaNameFile(); // GHM 20110727
        return bRet;
    }
    if (!m_pAreaNameFile->IsHeaderOK(_T("[Area Names]"))) {
        if (!bSilent) {
            sMsg.Format(_T("File:  %s  does not begin with\n\n    [Area Names]"), (LPCTSTR)sAreaFileName);
            AfxMessageBox(sMsg, MB_ICONEXCLAMATION);
        }
        //m_pAreaNameFile->Close();
        //delete m_pAreaNameFile;
        CloseAreaNameFile(); // GHM 20110727
        return bRet;
    }
    // BMD 21 Jun 2004
    CString csVersion = CSPRO_VERSION;
    if (!m_pAreaNameFile->IsVersionOK(csVersion)) {
        if (!IsValidCSProVersion(csVersion)) {
                if (!bSilent) {
                    sMsg.Format(_T("File:  %s does not have a valid version number.\nIt may be from a newer version of CSPro or you may have chosen an invalid file."), (LPCTSTR)sAreaFileName);
                    AfxMessageBox(sMsg, MB_ICONEXCLAMATION);
                }
                //m_pAreaNameFile->Close();
                //delete m_pAreaNameFile;
                CloseAreaNameFile(); // GHM 20110727
                return false;
            }
    }
    m_bOldAreaNameFile = false;
    if (GetCSProVersionNumeric(csVersion) < 3.0) {
        m_bOldAreaNameFile = true;
    }
    CString csAttribute, csValue;
    // gsf 6/97:  count number of area levels
   //Not being used anywhere m_uAreaLevels = 0;
    while (m_pAreaNameFile->GetLine(csAttribute, csValue) == SF_OK) {
        if (csAttribute.CompareNoCase(_T("Name")) == 0) {
           // m_uAreaLevels++;
        }
        if (csAttribute.CompareNoCase(_T("[Areas]")) == 0) {
            break;
        }
    }
    m_uAreaFilePos = (UINT)m_pAreaNameFile->GetPosition();

    return true;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabSet::BuildAreaLookupMap()
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabSet::BuildAreaLookupMap()
{
    bool bRet = false;

    if(!m_pAreaNameFile){
        return bRet;
    }

    m_AreaLabelLookup.RemoveAll();
    m_pAreaNameFile->Seek(m_uAreaFilePos, CFile::begin);
    m_pAreaNameFile->SetState(SF_OK);

    CString csAttribute, csValue;

    int iNumAreaLevels = GetConsolidate()->GetNumAreas();
    CArray<int,int> arrAreaLen;
    for(int iArea =0; iArea < iNumAreaLevels ; iArea++){
        const DictLevel* pDictLevel = NULL;
        const CDictRecord* pDictRecord = NULL;
        const CDictItem* pDictItem = NULL;
        const DictValueSet* pDictVSet = NULL;

        CString sAreaName = GetConsolidate()->GetArea(iArea);
        const CDataDict* pDict = LookupName(sAreaName,&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet);
        ASSERT(pDict);
        arrAreaLen.Add(pDictItem->GetLen());
    }

    while (m_pAreaNameFile->GetLine(csAttribute, csValue) == SF_OK) {
        csprochar* pszArgs = csAttribute.GetBuffer(csValue.GetLength());
        csprochar* pszArg;

        UINT uAreaCode;
        CString sBreakKey;
        int iNumIndent =0;
        for (int k = 0 ; k < iNumAreaLevels ; k++) {
            if(k==0){
                pszArg = strtoken(pszArgs, SPACE_COMMA_TAB_STR, NULL);
            }
            else {
                pszArg = strtoken(NULL, SPACE_COMMA_TAB_STR, NULL);
            }
            CIMSAString sPad;
            if (pszArg == NULL) {
                sPad = CString(_T(' '),arrAreaLen[k]);
            }
            else {
                CString cArg = pszArg;
                if (m_bOldAreaNameFile) {
                    uAreaCode = (UINT) _ttoi(pszArg);
                    if (uAreaCode == 0) {
                        sPad = CString(_T(' '),arrAreaLen[k]);
                    }
                    else {
                        iNumIndent++;
                        uAreaCode = (UINT) _ttoi(pszArg);
                        sPad.Str(uAreaCode,arrAreaLen[k],'0');
                    }
                }
                else {
                    if (cArg.CompareNoCase(AREA_NONE_TEXT) == 0) {
                        sPad = CString(_T(' '),arrAreaLen[k]);
                    }
                    else {
                        iNumIndent++;
                        uAreaCode = (UINT) _ttoi(pszArg);
                        sPad.Str(uAreaCode,arrAreaLen[k],'0');
                    }
                }
            }
            sBreakKey += sPad;
        }
        bool bSkip = false;
        pszArg = strtoken(NULL, SPACE_COMMA_TAB_STR, NULL); //See if the rest of the stuff is  zeroes
        while(pszArg){
            if (pszArg == NULL) {
                break;
            }
            else {
                uAreaCode = (UINT) _ttoi(pszArg);
                if(uAreaCode !=0){
                    bSkip = true;
                    break;
                }
            }
            //See if the rest of the stuff is  zeroes
            pszArg = strtoken(NULL, SPACE_COMMA_TAB_STR, NULL); //See if the rest of the stuff is  zeroes
        }
        csAttribute.ReleaseBuffer();
        if(!bSkip && !sBreakKey.IsEmpty()){
            sBreakKey.MakeUpper();
            if(iNumIndent > 0){
                CString sPad;
                sPad = CString(_T(' '),iNumIndent*4);
                csValue = sPad +  csValue;
            }
            m_AreaLabelLookup[sBreakKey] =csValue;
        }
    }

    return bRet;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabSet::CloseAreaNameFile()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabSet::CloseAreaNameFile()
{
    if(m_pAreaNameFile){
        m_pAreaNameFile->Close();
        delete m_pAreaNameFile;
        m_pAreaNameFile = NULL;
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTable::BuildFrqStats(CSpecFile& specFile, bool bSilent /*= false*/)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTable::BuildFrqStats(CSpecFile& specFile, bool bSilent /*= false*/)
{
    CString sCmd;
    CIMSAString sArg;
    bool bResult = true;

    CIMSAString sVal;
    while (specFile.GetLine(sCmd, sArg) == SF_OK) {
        if (sCmd[0] != '[')  {
            sVal = _T("");
            m_bHasFreqStats = true;
            if (sCmd.CompareNoCase(XTS_CMD_FRQ_MEAN) == 0)  {
                  sVal = sArg.GetToken(_T(" "));    // only space separators because of comma decimal
                    if (!sVal.IsNumeric())  {
                        if (!bSilent) {
                            CString sError;
                            sError.Format(_T("Mean value is not numeric:  %s"), (LPCTSTR)sVal);
                            AfxMessageBox(sError);
                        }
                        bResult = false;
                    }
                    m_dFrqMean = (double) sVal.fVal();
            }
            else if (sCmd.CompareNoCase(XTS_CMD_FRQ_MINC) == 0)  {
                sVal = sArg.GetToken(_T(" "));    // only space separators because of comma decimal
                if (!sVal.IsNumeric())  {
                    if (!bSilent) {
                        CString sError;
                        sError.Format(_T("Min value is not numeric:  %s"), (LPCTSTR)sVal);
                        AfxMessageBox(sError);
                    }
                    bResult = false;
                }
                m_dFrqMinCode = (double) sVal.fVal();
            }
            else if (sCmd.CompareNoCase(XTS_CMD_FRQ_MAXC) == 0)  {
                sVal = sArg.GetToken(_T(" "));    // only space separators because of comma decimal
                if (!sVal.IsNumeric())  {
                    if (!bSilent) {
                        CString sError;
                        sError.Format(_T("Max code is not numeric:  %s"), (LPCTSTR)sVal);
                        AfxMessageBox(sError);
                    }
                    bResult = false;
                }
                m_dFrqMaxCode = (double) sVal.fVal();
            }
            else if (sCmd.CompareNoCase(XTS_CMD_FRQ_STDDEV) == 0)  {
                sVal = sArg.GetToken(_T(" "));    // only space separators because of comma decimal
                if (!sVal.IsNumeric())  {
                    if (!bSilent) {
                        CString sError;
                        sError.Format(_T("Standard deviation is not numeric:  %s"), (LPCTSTR)sVal);
                        AfxMessageBox(sError);
                    }
                    bResult = false;
                }
                m_dFrqStdDev = (double) sVal.fVal();
            }
            else if (sCmd.CompareNoCase(XTS_CMD_FRQ_VARIANCE) == 0)  {
                sVal = sArg.GetToken(_T(" "));    // only space separators because of comma decimal
                if (!sVal.IsNumeric())  {
                    if (!bSilent) {
                        CString sError;
                        sError.Format(_T("Variance is not numeric:  %s"), (LPCTSTR)sVal);
                        AfxMessageBox(sError);
                    }
                    bResult = false;
                }
                m_dFrqVariance = (double) sVal.fVal();
            }
            else if (sCmd.CompareNoCase(XTS_CMD_FRQ_MODEC) == 0)  {
                sVal = sArg.GetToken(_T(" "));    // only space separators because of comma decimal
                if (!sVal.IsNumeric())  {
                    if (!bSilent) {
                        CString sError;
                        sError.Format(_T("Mode code is not numeric:  %s"), (LPCTSTR)sVal);
                        AfxMessageBox(sError);
                    }
                    bResult = false;
                }
                m_dFrqModeCode = (double) sVal.fVal();
            }
            else if (sCmd.CompareNoCase(XTS_CMD_FRQ_MEDIANC) == 0)  {
                sVal = sArg.GetToken(_T(" "));    // only space separators because of comma decimal
                if (!sVal.IsNumeric())  {
                    if (!bSilent) {
                        CString sError;
                        sError.Format(_T("Median code is not numeric:  %s"), (LPCTSTR)sVal);
                        AfxMessageBox(sError);
                    }
                    bResult = false;
                }
                m_dFrqMedianCode = (double) sVal.fVal();
            }
             else if (sCmd.CompareNoCase(XTS_CMD_FRQ_MEDIANI) == 0)  {
                sVal = sArg.GetToken(_T(" "));    // only space separators because of comma decimal
                if (!sVal.IsNumeric())  {
                    if (!bSilent) {
                        CString sError;
                        sError.Format(_T("Median is not numeric:  %s"), (LPCTSTR)sVal);
                        AfxMessageBox(sError);
                    }
                    bResult = false;
                }
                m_dFrqMedianInt = (double) sVal.fVal();
            }
            else if (sCmd.CompareNoCase(XTS_CMD_FRQ_TOTCAT) == 0)  {
                sVal = sArg.GetToken(_T(" "));    // only space separators because of comma decimal
                if (!sVal.IsNumeric())  {
                    if (!bSilent) {
                        CString sError;
                        sError.Format(_T("Total categories is not numeric:  %s"), (LPCTSTR)sVal);
                        AfxMessageBox(sError);
                    }
                    bResult = false;
                }
                m_iTotalCategories = (int) sVal.Val();
            }
            else if (sCmd.CompareNoCase(XTS_CMD_FRQ_ALPHA_STATS) == 0)  {
                m_bAlphaFreqStats = ( sArg.CompareNoCase(XTS_ARG_YES) == 0 );
            }
            else {
                // Unrecognized command at line %d:
                if (!bSilent) {
                    m_sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                    m_sError += _T("\n") + sCmd + _T("=") + sArg;
                    AfxMessageBox(m_sError);
                }
                bResult = false;
            }
        }
        else {
            if (sCmd.CompareNoCase(XTS_SECT_TABLE) == 0 || sCmd.CompareNoCase(XTS_SECT_LEVEL) == 0) {
                specFile.UngetLine();
                break;
            }

            else if (sCmd.CompareNoCase(XTS_SECT_FRQNTILES) == 0) {
                BuildNTiles(specFile, bSilent);
            }
            else if (sCmd.CompareNoCase(XTS_SECT_ENDFRQSTATS) == 0) {
                break;
            }

            else {
                m_sError.Format(_T("Invalid section heading at line %d:"), specFile.GetLineNumber()); // Invalid section heading at line %d:
                m_sError += _T("\n") + sCmd;
                if (!bSilent) {
                    AfxMessageBox(m_sError);
                    specFile.SkipSection();
                }
                bResult = false;
            }
        }
    }

    return bResult;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTable::SaveFrqStats(CSpecFile& specFile)
//
/////////////////////////////////////////////////////////////////////////////////
void CTable::SaveFrqStats(CSpecFile& specFile)
{
    if(!m_bHasFreqStats&&!m_bHasFreqNTiles)
        return;

    specFile.PutLine(_T(" "));
    specFile.PutLine(XTS_SECT_FRQSTATS);

    if(m_bHasFreqStats) {
        CString sVal;
        csprochar pszTemp[10];
        GetPrivateProfileString(_T("intl"), _T("sDecimal"), _T("."), pszTemp, 10, _T("WIN.INI"));
        csprochar cDecimal = pszTemp[0];

        if(m_dFrqMean !=0 ) {
          csprochar pszTemp[30];
          sVal = dtoa(m_dFrqMean, pszTemp, 9, cDecimal, false);
          specFile.PutLine(XTS_CMD_FRQ_MEAN, sVal);
        }
        if(m_dFrqMinCode !=0 ) {
          csprochar pszTemp[30];
          sVal = dtoa(m_dFrqMinCode, pszTemp, 9, cDecimal, false);
          specFile.PutLine(XTS_CMD_FRQ_MINC, sVal);
        }
        if(m_dFrqMaxCode !=0 ) {
          csprochar pszTemp[30];
          sVal = dtoa(m_dFrqMaxCode, pszTemp, 9, cDecimal, false);
          specFile.PutLine(XTS_CMD_FRQ_MAXC, sVal);
        }
        if(m_dFrqModeCode !=0 ) {
          csprochar pszTemp[30];
          sVal = dtoa(m_dFrqModeCode, pszTemp, 9, cDecimal, false);
          specFile.PutLine(XTS_CMD_FRQ_MODEC, sVal);
        }
        if(m_dFrqMedianCode !=0 ) {
          csprochar pszTemp[30];
          sVal = dtoa(m_dFrqMedianCode, pszTemp, 9, cDecimal, false);
          specFile.PutLine(XTS_CMD_FRQ_MEDIANC, sVal);
        }
        if(m_dFrqMedianInt !=0 ) {
          csprochar pszTemp[30];
          sVal = dtoa(m_dFrqMedianInt, pszTemp, 9, cDecimal, false);
          specFile.PutLine(XTS_CMD_FRQ_MEDIANI, sVal);
        }
        if(m_dFrqStdDev !=0 ) {
          csprochar pszTemp[30];
          sVal = dtoa(m_dFrqStdDev, pszTemp, 9, cDecimal, false);
          specFile.PutLine(XTS_CMD_FRQ_STDDEV, sVal);
        }
        if(m_dFrqVariance !=0 ) {
          csprochar pszTemp[30];
          sVal = dtoa(m_dFrqVariance, pszTemp, 9, cDecimal, false);
          specFile.PutLine(XTS_CMD_FRQ_VARIANCE, sVal);
        }
        if(m_iTotalCategories !=0 ) {
          specFile.PutLine(XTS_CMD_FRQ_TOTCAT, m_iTotalCategories);
        }
        if(m_bAlphaFreqStats) {
          specFile.PutLine(XTS_CMD_FRQ_ALPHA_STATS, XTS_ARG_YES);
        }
    }

    SaveNTiles(specFile);

    specFile.PutLine(XTS_SECT_ENDFRQSTATS);
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTable::BuildNTiles(CSpecFile& specFile, bool bSilent /*= false*/)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTable::BuildNTiles(CSpecFile& specFile, bool bSilent /*= false*/)
{
    CString sCmd;
    CIMSAString sArg;
    bool bResult = true;

    CString sVal;
    while (specFile.GetLine(sCmd, sArg) == SF_OK) {
        if (sCmd[0] != '[')  {
            sVal = _T("");
            m_bHasFreqNTiles = true;
            if (sCmd.CompareNoCase(XTS_CMD_NTILE) == 0)  {
                while (!sArg.IsEmpty())  {
                    sVal = sArg.GetToken(_T(" "));    // only space separators because of comma decimal
                    m_arrFrqNTiles.Add(sVal);
                }
            }
            else {
                // Unrecognized command at line %d:
                if (!bSilent) {
                    m_sError.Format(_T("Unrecognized command at line %d:"), specFile.GetLineNumber());
                    m_sError += _T("\n") + sCmd + _T("=") + sArg;
                    AfxMessageBox(m_sError);
                }
                bResult = false;
            }
        }
        else {
            if (sCmd.CompareNoCase(XTS_SECT_ENDFRQNTILES) == 0) {
                break;
            }
            else {
                m_sError.Format(_T("Invalid section heading at line %d:"), specFile.GetLineNumber()); // Invalid section heading at line %d:
                m_sError += _T("\n") + sCmd;
                if (!bSilent) {
                    AfxMessageBox(m_sError);
                    specFile.SkipSection();
                }
                bResult = false;
            }
        }
    }

    return bResult;
}



/////////////////////////////////////////////////////////////////////////////////
//
//  void CTable::SaveNTiles(CSpecFile& specFile)
//
/////////////////////////////////////////////////////////////////////////////////
void CTable::SaveNTiles(CSpecFile& specFile)
{
    if(!m_bHasFreqNTiles)
        return;

    const int EntriesPerNTile = 3;
    int iSize = m_arrFrqNTiles.GetSize();
    div_t divt = div(iSize, EntriesPerNTile);

    if(divt.quot == 0 && divt.rem !=0)
        AfxMessageBox(_T("Invalid nTile Structure"));

    if(iSize == 0)
        return;

    specFile.PutLine(_T(" "));
    specFile.PutLine(XTS_SECT_FRQNTILES);

    for( int iIndex = 0; iIndex < iSize; iIndex += EntriesPerNTile )
    {
        specFile.PutLine(XTS_CMD_NTILE, FormatText(_T("%s %s %s"),
            (LPCTSTR)m_arrFrqNTiles[iIndex], (LPCTSTR)m_arrFrqNTiles[iIndex + 1], (LPCTSTR)m_arrFrqNTiles[iIndex + 2]));
    }

    specFile.PutLine(XTS_SECT_ENDFRQNTILES);
}

/*void CTabVar::SetTotalPos(TOTPOS TotalPos)
{
    if (m_TotalPos == TP_BEFORE ) {
        int iSize = m_aTabValue.GetSize();
        if(iSize >=1 ) {
            if(m_aTabValue.GetAt(0)->GetText().CompareNoCase("Total") == 0 ) {
                delete m_aTabValue.GetAt(0);
                m_aTabValue.RemoveAt(0);
            }
        }
    }
    else  if (m_TotalPos == TP_AFTER) {
        int iSize = m_aTabValue.GetSize();
        if(iSize >=1 ) {
            if(m_aTabValue.GetAt(iSize -1 )->GetText().CompareNoCase("Total") == 0 ) {
                delete m_aTabValue.GetAt(iSize -1);
                m_aTabValue.RemoveAt(iSize-1);
            }
        }
    }

    m_TotalPos = TotalPos;  // New Pos

    if (m_TotalPos == TP_BEFORE ) {
        CTabValue* val = new CTabValue();
        val->SetText("Total", false);
        val->SetParentVar(this);
        m_aTabValue.InsertAt(0,val);
    }
    else  if (m_TotalPos == TP_AFTER) {
        CTabValue* val = new CTabValue();
        val->SetText("Total", false);
        val->SetParentVar(this);
        m_aTabValue.Add(val);
    }

}*/


///////////////////////////////////////////////////////////////////////
//
//    CTblOb::Save
//
// Used to save auxiliary table objects that have their own sections:
// - page notes
// - end notes
// - headers (l/c/r)
// - footers (l/c/r)
// - stubheads
// - area captions
//
// Each of these sections has several key only:
// - label (a string)
// - format (a string indicating a CFmtBase ID) -- optional
// - sizing (current, min, max) -- optional
//
// Information is only put in the spec file if a label is present or
// a non-default format is used.
//
///////////////////////////////////////////////////////////////////////
void CTblOb::Save(CSpecFile& specFile, const CString& sSection) const
{
    bool bCustomFmt=false;
    CIMSAString sLabel=GetText();
    CFmt* pFmt=DYNAMIC_DOWNCAST(CFmt,GetFmt());
    if (NULL!=pFmt) {
        bCustomFmt=CFmtReg::IsCustomFmtID(*pFmt);
    }
    //Save always else state info is not being saved
    specFile.PutLine(sSection);

    CIMSAString sEncodedLabel = WS2CS(TableLabelSerializer::Create(sLabel));
    sEncodedLabel.QuoteDelimit();
    specFile.PutLine(XTS_CMD_LABEL, sEncodedLabel);

    if (bCustomFmt) {
        specFile.PutLine(XTS_CMD_FORMAT, pFmt->GetIDInfo());
    }
    SaveStateInfo(specFile);
    specFile.PutLine(_T(" "));
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblBase::SaveStateInfo(CSpecFile& specFile) const
//
/////////////////////////////////////////////////////////////////////////////////
void CTblBase::SaveStateInfo(CSpecFile& specFile) const
{
    if (m_aPrtViewInfo.GetSize()>0) {
        CString sPrint;
//      CFmt* pFmt=DYNAMIC_DOWNCAST(CFmt,GetFmt());
        CSize szIndent;
        CPrtViewInfo prtViewInfo;
        int iSize = m_aPrtViewInfo.GetSize();
        for(int iIndex = 0; iIndex < iSize ; iIndex++) {
            sPrint =_T("");
            prtViewInfo = m_aPrtViewInfo[iIndex];
//          if (pFmt) {
//              szIndent=CSize(pFmt->GetIndent(LEFT) - pFmt->GetIndent(RIGHT), pFmt->GetIndent().top - pFmt->GetIndent().bottom);
//          }
//          else {
//              szIndent=CSize(0,0);
//          }
            sPrint.Format(_T("%d,%d,%d,%d,%d,%d,%d,%d,%s,%s"),
//              prtViewInfo.GetCurrSize().cx - szIndent.cx,
//              prtViewInfo.GetCurrSize().cy - szIndent.cy,  this might need further debugging
                prtViewInfo.GetCurrSize().cx,
                prtViewInfo.GetCurrSize().cy,
                prtViewInfo.GetMinSize().cx,
                prtViewInfo.GetMinSize().cy,
                prtViewInfo.GetMaxSize().cx,
                prtViewInfo.GetMaxSize().cy,
                prtViewInfo.GetExtraSize().cx,
                prtViewInfo.GetExtraSize().cy,
                (prtViewInfo.IsCustom()?XTS_ARG_YES:XTS_ARG_NO),
                (prtViewInfo.IsPageBreak()?XTS_ARG_YES:XTS_ARG_NO));
            specFile.PutLine(XTS_ARG_PRNTVSZ ,sPrint);
        }
//was working here; change col size then save/reload
    }

    if (m_aGrdViewInfo.GetSize()>0) {
        CString sGrid;
        CGrdViewInfo grdViewInfo;
        int iSize = m_aGrdViewInfo.GetSize();
        for(int iIndex = 0; iIndex < iSize ; iIndex++) {
            sGrid =_T("");
            grdViewInfo = m_aGrdViewInfo[iIndex];
            sGrid.Format(_T("%d,%d,%d,%d,%d,%d"),
                grdViewInfo.GetCurrSize().cx,
                grdViewInfo.GetCurrSize().cy,
                grdViewInfo.GetMinSize().cx,
                grdViewInfo.GetMinSize().cy,
                grdViewInfo.GetMaxSize().cx,
                grdViewInfo.GetMaxSize().cy);
            specFile.PutLine(XTS_ARG_GRIDVSZ ,sGrid);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTblBase::BuildStateInfo(CSpecFile& specFile, bool bSilent  = false )
//
/////////////////////////////////////////////////////////////////////////////////
bool CTblBase::BuildStateInfo(CSpecFile& specFile, bool bSilent /* = false */)
{
    RemoveAllGrdViewInfo();
    RemoveAllPrtViewInfo();

    UNREFERENCED_PARAMETER(bSilent);
    const int iMaxSize = 8;
    CString sCmd;
    CIMSAString sArg;
    while (specFile.GetLine(sCmd, sArg) == SF_OK) {
        if (sCmd[0] != '[')  {
            CArray<int,int&> arrSize;
            bool bCustom=false;
            bool bPageBreak=false;
            arrSize.SetSize(iMaxSize);
            if (sCmd.CompareNoCase(XTS_ARG_PRNTVSZ) == 0)  {
                for(int iIndex = 0 ;iIndex < iMaxSize ; iIndex ++ ) {
                    arrSize[iIndex] =0;
                }
                int iPos =0;
                CIMSAString sTemp;
                while (sArg.GetLength() > 0 && iPos <iMaxSize){
                    sTemp = sArg.GetToken();
                    arrSize[iPos]=(int) sTemp.Val();
                    iPos++;
                }
                // get a yes/no for customization
                sTemp=sArg.GetToken();
                if (sTemp.CompareNoCase(XTS_ARG_YES)==0) {
                    bCustom=true;
                }
                // get a yes/no for page break
                sTemp=sArg.GetToken();
                if (sTemp.CompareNoCase(XTS_ARG_YES)==0) {
                    bPageBreak=true;
                }
                CPrtViewInfo prntViewInfo (CSize(arrSize[0],arrSize[1]),
                    CSize(arrSize[2],arrSize[3]),
                    CSize(arrSize[4],arrSize[5]),
                    CSize(arrSize[6],arrSize[7]), bCustom, bPageBreak);
                m_aPrtViewInfo.Add(prntViewInfo);
            }
            else if (sCmd.CompareNoCase(XTS_ARG_GRIDVSZ) == 0)  {
                for(int iIndex = 0 ;iIndex < iMaxSize ; iIndex ++ ) {
                    arrSize[iIndex] =0;
                }
                int iPos =0;
                CIMSAString sTemp;
                while (sArg.GetLength() > 0 && iPos <iMaxSize){
                    sTemp = sArg.GetToken();
                    arrSize[iPos]=(int) sTemp.Val();
                    iPos++;
                }
                CGrdViewInfo gridViewInfo (CSize(arrSize[0],arrSize[1]),
                    CSize(arrSize[2],arrSize[3]),
                    CSize(arrSize[4],arrSize[5]));
                m_aGrdViewInfo.Add(gridViewInfo);
            }
            else {
                specFile.UngetLine();
                break;
            }
        }
        else {
            specFile.UngetLine();
            break;
        }
    }

    return true;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  bool CUnitSpec::Build (CSpecFile& specFile, bool bSilent/*=false*/)
//
/////////////////////////////////////////////////////////////////////////////////
bool CUnitSpec::Build (CSpecFile& specFile, bool bSilent/*=false*/)
{
    UNREFERENCED_PARAMETER(bSilent);
    CString sCmd,sArg;
    m_arrTabLogic.RemoveAll();
    //m_arrPostCalc.RemoveAll();
    while (specFile.GetLine(sCmd, sArg,false) == SF_OK) {
        sCmd.Trim(); //Trim has been set to false in GetLine 'cos tablogic leading spaces are getting trimmed
        if (sCmd[0] != '[')  {
            if (sCmd.CompareNoCase(XTS_CMD_SUBTABLE) == 0)  {
                sArg.Trim();
                SetSubTableString(sArg);
            }
            else if (sCmd.CompareNoCase(XTS_CMD_UNIVERSE) == 0)  {
                SetUniverse(sArg);
            }
            else if (sCmd.CompareNoCase(XTS_CMD_WEIGHT) == 0)  {
                SetWeightExpr(sArg);
            }
            else if (sCmd.CompareNoCase(XTS_CMD_UNITVALUE) == 0)  {
                sArg.Trim();
                SetValue(sArg);
            }
            else if (sCmd.CompareNoCase(XTS_CMD_LOOPINGVAR) == 0)  {
                sArg.Trim();
                SetLoopingVarName(sArg);
                m_bUseDefaultLoopingVar = sArg.IsEmpty();
            }
            else if (sCmd.CompareNoCase(XTS_CMD_TABLOGIC) == 0)  {
                m_arrTabLogic.Add(sArg);
            }
            //Savy (R) sampling app 20090211
            else if (sCmd.CompareNoCase(XTS_CMD_ANALYSIS_TYPE) == 0)  {

                if (sArg.CompareNoCase(XTS_ARG_SAMP_TOTALS) == 0) {
                    m_eSampAnalysisType = TOTALS_ANALYSIS_TYPE;
                }
                else if (sArg.CompareNoCase(XTS_ARG_SAMP_MEANS) == 0) {
                    m_eSampAnalysisType = MEANS_ANALYSIS_TYPE;
                }
                else if (sArg.CompareNoCase(XTS_ARG_SAMP_RATIOS) == 0) {
                    m_eSampAnalysisType = RATIOS_ANALYSIS_TYPE;
                }
                else if (sArg.CompareNoCase(XTS_ARG_SAMP_PROPORTION) == 0) {
                    m_eSampAnalysisType = PROPORTIONS_ANALYSIS_TYPE;
                }
                else
                {
                    m_eSampAnalysisType = ANALYSIS_TYPE_NONE;
                }
            }
            else if (sCmd.CompareNoCase(XTS_CMD_ANALYSIS_VAR) == 0) {
                sArg.Trim();
                m_sAnalysisVariable = sArg;
            }
            else if (sCmd.CompareNoCase(XTS_CMD_DENOMINATOR) == 0) {
                sArg.Trim();
                m_sDenominator = sArg;
            }
            else {
                specFile.UngetLine();
                break;
            }

        }
        else {
            specFile.UngetLine();
            break;
        }
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CUnitSpec::Save (CSpecFile& specFile)
//
/////////////////////////////////////////////////////////////////////////////////
void CUnitSpec::Save (CSpecFile& specFile)
{
    specFile.PutLine(XTS_SECT_UNIT);
    specFile.PutLine(XTS_CMD_SUBTABLE,GetSubTableString());
    if (m_bUseDefaultLoopingVar) {
        specFile.PutLine(XTS_CMD_LOOPINGVAR,_T("")); // blank in file means use default
    }
    else {
        specFile.PutLine(XTS_CMD_LOOPINGVAR,GetLoopingVarName());
    }
    specFile.PutLine(XTS_CMD_WEIGHT,GetWeightExpr());
    specFile.PutLine(XTS_CMD_UNITVALUE,GetValue());
    specFile.PutLine(XTS_CMD_UNIVERSE,GetUniverse());
    for(int iIndex =0;iIndex < m_arrTabLogic.GetSize();iIndex++){
        specFile.PutLine(XTS_CMD_TABLOGIC,m_arrTabLogic[iIndex]);
    }
    //Savy (R) sampling app means save the Analysis Variable, Denominator 20090415
    if(GetSamplingErrAppFlag()){
        //Savy (R) sampling app 20090211
        //Sampling Analysis Type
        switch(m_eSampAnalysisType){
        case TOTALS_ANALYSIS_TYPE:
            specFile.PutLine (XTS_CMD_ANALYSIS_TYPE, XTS_ARG_SAMP_TOTALS);
            break;
        case MEANS_ANALYSIS_TYPE:
            specFile.PutLine (XTS_CMD_ANALYSIS_TYPE, XTS_ARG_SAMP_MEANS);
            break;
        case RATIOS_ANALYSIS_TYPE:
            specFile.PutLine (XTS_CMD_ANALYSIS_TYPE, XTS_ARG_SAMP_RATIOS);
            break;
        case PROPORTIONS_ANALYSIS_TYPE:
            specFile.PutLine (XTS_CMD_ANALYSIS_TYPE, XTS_ARG_SAMP_PROPORTION);
            break;
        default:
            break;
        }
        //Analysis Variable
        specFile.PutLine (XTS_CMD_ANALYSIS_VAR, m_sAnalysisVariable);
        //Denominator
        specFile.PutLine (XTS_CMD_DENOMINATOR, m_sDenominator);
        //
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTable::UpdateSubtableList()
//
/////////////////////////////////////////////////////////////////////////////////
void CTable::UpdateSubtableList()
{
#ifdef _NOT_DEFINED_IN_TABLE
    CArray<CUnitSpec,CUnitSpec&> arrUnitSpec; //local array for processing
    CStringArray sArrRow;
    CStringArray sArrCol;
    //process the row vars first
    CTabVar* pRowRootVar = GetRowRoot();
    int iNumRowVars = pRowRootVar->GetNumChildren();
    if(pRowRootVar->GetNumChildren() ==0){
        //Remove all the Unit specs from the table
        m_aUnitSpec.RemoveAll();
        return;
    }
    //Generate Row Strings
    for(int iIndex =0;iIndex < iNumRowVars ; iIndex++){
        CTabVar* pTabVar = pRowRootVar->GetChild(iIndex);
        CString sParentString;
        if(m_varNameMap.Lookup(pTabVar,sParentString)){
        }
        else {
            sParentString = pTabVar->GetName();
        }
        CUnitSpec unitSpec(sParentString);
        arrUnitSpec.Add(unitSpec);
        sArrRow.Add(sParentString);
        if(pTabVar->GetNumChildren() > 0) {
            int iNumChildren = pTabVar->GetNumChildren();
            for(int iIndex =0;iIndex < iNumChildren ; iIndex++){
                CTabVar* pChildTabVar = pTabVar->GetChild(iIndex);
                CString sChildString;
                if(m_varNameMap.Lookup(pChildTabVar,sChildString)){
                }
                else {
                    sChildString = pChildTabVar->GetName();
                }
                sChildString = sParentString + _T("*") + sChildString;
                sArrRow.Add(sChildString);
            }
        }
    }
    //Generate Col Strings
    CTabVar* pColRootVar = GetColRoot();
    int iNumColVars = pColRootVar->GetNumChildren();
    if(pColRootVar->GetNumChildren() ==0){
        //This may be a over kill get back to this to decide what to do on the
        //existing units
        //Remove all the Unit specs from the table
        m_aUnitSpec.RemoveAll();
        return;
    }
    for(int iIndex =0;iIndex < iNumColVars ; iIndex++){
        CTabVar* pTabVar = pColRootVar->GetChild(iIndex);
        CString sParentString;
        if(m_varNameMap.Lookup(pTabVar,sParentString)){
        }
        else {
            sParentString = pTabVar->GetName();
        }
        sArrCol.Add(sParentString);
        if(pTabVar->GetNumChildren() > 0) {
            int iNumChildren = pTabVar->GetNumChildren();
                for(int iIndex =0;iIndex < iNumChildren ; iIndex++){
                    CTabVar* pChildTabVar = pTabVar->GetChild(iIndex);
                    CString sChildString;
                    if(m_varNameMap.Lookup(pChildTabVar,sChildString)){
                    }
                    else {
                        sChildString = pChildTabVar->GetName();
                    }
                    sChildString = sParentString + _T("*") + sChildString;
                    sArrCol.Add(sChildString);
                }
        }
    }
    //Generate Subtable list of Row By Col
    for (int iRow = 0; iRow < sArrRow.GetSize();iRow++){
        for (int iCol = 0; iCol < sArrCol.GetSize();iCol++){
                CString sSubtable;
                sSubtable = sArrRow[iRow] + _T(" by ") + sArrCol[iCol];
                CUnitSpec unitSpec(sSubtable);
                arrUnitSpec.Add(unitSpec);
        }
    }
    //Reconcile the subtable list in the unit array
    for(iIndex = m_aUnitSpec.GetSize() -1; iIndex >=0 ;iIndex--){
        CString sSubTable = m_aUnitSpec[iIndex].GetSubTableString();
        bool bFound = false;
        for(int iLocal = arrUnitSpec.GetSize()-1 ; iLocal >=0 ;iLocal--){
            CString sLocalSubTable = arrUnitSpec[iLocal].GetSubTableString();
            sSubTable.Trim();
            sLocalSubTable.Trim();
            if(sSubTable.CompareNoCase(sLocalSubTable) ==0){
                bFound =true;
                arrUnitSpec.RemoveAt(iLocal);
                break;
            }

        }
        if(!bFound){//Do we have to let the user know abt this
            m_aUnitSpec.RemoveAt(iIndex);
        }
    }
    //Add the remaining unit specs to the Subtable list
    for(int iLocal = 0; iLocal < arrUnitSpec.GetSize() ;iLocal++){
        m_aUnitSpec.Add(arrUnitSpec[iLocal]);

    }
#endif
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTable::ReconcileTabVar(CTabVar* pTabVar,CDataDict* pWorkDict, CString& sError,bool bSilent =false)
//
/////////////////////////////////////////////////////////////////////////////////
void CTable::ReconcileTabVar(const CDataDict* pDict, const CDataDict* pWorkDict, CTabVar* pTabVar,CString& sError,bool bSilent /*=false*/)
{
    if(!pDict)
        return;

    const CDictItem* pDictItem;
    const DictValueSet* pDictVSet;
    bool bFound = true;
    if(pTabVar == GetRowRoot() || pTabVar == GetColRoot()) { //Do nothing , just process children
    }
    else {
        //Check if the var exists in the dictionary
        bFound = pDict->LookupName(pTabVar->GetName(), nullptr, nullptr, &pDictItem, &pDictVSet);
        if(!bFound && pWorkDict){
            bFound = pWorkDict->LookupName(pTabVar->GetName(), nullptr, nullptr, &pDictItem, &pDictVSet);
        }
        if(!pDictVSet && !pDictItem  ) { //VSet not found so delete this item and the rest of the stuff
            CString sMsg;
            if(pTabVar->GetNumChildren()== 0) {
                m_bDirty = true;
                //sMsg.Format(_T("%s not found in the dictionary. Deleting variable from the table %s \r\n"),pTabVar->GetName(),GetName());
               sMsg.Format(_T("%s deleted from table; not in dictionary.\r\n"), (LPCTSTR)pTabVar->GetName());
            }
            else {
                m_bDirty = true;
                //sMsg.Format(_T("%s not found in the dictionary. Deleting variable and its children from the table %s \r\n"),pTabVar->GetName(),GetName());
                sMsg.Format(_T("%s deleted from table; not in dictionary.\r\n"), (LPCTSTR)pTabVar->GetName());
            }
            sError += sMsg;
        }

        if(bFound){
            if (ReconcileTallyFmt(pTabVar, pDictVSet)) {
                m_bDirty = true;
            }
        }
    }
    //Now do the children
    if(bFound){

        for (int iIndex =0; iIndex < pTabVar->GetNumChildren() ; iIndex++){
            CTabVar* pChild = pTabVar->GetChild(iIndex);
            ReconcileTabVar(pDict ,pWorkDict,pChild,sError,bSilent);
        }
    }
    else{ //Delete the children
        pTabVar->Remove();
        delete pTabVar;
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTable::ReconcileTallyFmt
//
//  Reconcile tally fmt for this tab var.  Return true if it caused a change.
/////////////////////////////////////////////////////////////////////////////////
bool CTable::ReconcileTallyFmt(CTabVar* pTabVar, const DictValueSet* pDictVSet)
{
    ASSERT(pTabVar != GetRowRoot() && pTabVar != GetColRoot());

    // need to to reconcile ranges for stats with ranges (ntiles, median)
    CTallyFmt* pVarTallyFmt = pTabVar->GetTallyFmt();
    bool bChanged = false;
    if (pVarTallyFmt) {
        bChanged = pVarTallyFmt->Reconcile(pDictVSet);
    }
    else {
        // current tally format is default, don't want to reconcile shared def fmt
        // to this vset so make a copy first
        FMT_ID eFmtID = FMT_ID_INVALID;
        eFmtID= IsRowVar(pTabVar) ? FMT_ID_TALLY_ROW: FMT_ID_TALLY_COL;
        ASSERT(m_pFmtReg);
        CTallyFmt* pDefTallyFmt = DYNAMIC_DOWNCAST(CTallyFmt,m_pFmtReg->GetFmt(eFmtID));
        pVarTallyFmt = new CTallyFmt(*pDefTallyFmt);
        pVarTallyFmt->SetIndex(m_pFmtReg->GetNextCustomFmtIndex(*pDefTallyFmt));
        bChanged = pVarTallyFmt->Reconcile(pDictVSet);
        if (bChanged) {
              m_pFmtReg->AddFmt(pVarTallyFmt);
              pTabVar->SetTallyFmt(pVarTallyFmt);
        }
        else {
            SAFE_DELETE(pVarTallyFmt);
        }
    }

    return bChanged;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTable::AddReaderBreakTabVals
//
/////////////////////////////////////////////////////////////////////////////////
void CTable::AddReaderBreakTabVals(CTabVar* pTabVar)
{
    // add is same as reconcile just with no existing values
    CArray<CTabValue*, CTabValue*> arrStatTabVals;
    ReconcileReaderBreakTabVals(pTabVar, arrStatTabVals);
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTable::ReconcileReaderBreakTabVals
//
/////////////////////////////////////////////////////////////////////////////////
void CTable::ReconcileReaderBreakTabVals(CTabVar* pTabVar, CArray<CTabValue*, CTabValue*>& arrStatTabVals)
{
    if (!IsParentRowRoot(pTabVar) ||pTabVar->GetNumChildren() >0) {
        return; // no reader breaks in columns Or  is kind of A*B
    }
    int iReaderBreaks =0;
    CTblFmt* pTblFmt = GetDerFmt();
    if(!pTblFmt && m_pFmtReg){
        pTblFmt = DYNAMIC_DOWNCAST(CTblFmt,m_pFmtReg->GetFmt(FMT_ID_TABLE));
    }
    switch(pTblFmt->GetReaderBreak()){
            case READER_BREAK_ONE:
                iReaderBreaks =1;
                break;
            case READER_BREAK_TWO:
                iReaderBreaks =2;
                break;
            case READER_BREAK_THREE:
                iReaderBreaks =3;
                break;
            case READER_BREAK_FOUR:
                iReaderBreaks =4;
                break;
            case READER_BREAK_FIVE:
                iReaderBreaks =5;
                break;
            case READER_BREAK_SIX:
                iReaderBreaks =6;
                break;
            case READER_BREAK_SEVEN:
                iReaderBreaks =7;
                break;
            case READER_BREAK_EIGHT:
                iReaderBreaks =8;
                break;
            case READER_BREAK_NINE:
                iReaderBreaks =9;
                break;
            case READER_BREAK_TEN:
                iReaderBreaks =10;
                break;
            default:
                break;
    }
//    int iCurPos =iReaderBreaks;
    if(iReaderBreaks){
        CArray<CTabValue*, CTabValue*>& arrTabVals = pTabVar->GetArrTabVals();
        CArray<CTabValue*, CTabValue*> arrTempTabVals;
        int iInsertPos = iReaderBreaks;//TODO pcts
        for(int iTabVal = 0; iTabVal < arrTabVals.GetCount(); iTabVal++){
             CTabValue* pCurTabVal  =arrTabVals[iTabVal];
             TABVAL_TYPE eTabValType = pCurTabVal->GetTabValType();
             if(eTabValType == DICT_TABVAL || eTabValType == SPECIAL_TABVAL
                 || eTabValType == SPECIAL_MISSING_TABVAL || eTabValType == SPECIAL_REFUSED_TABVAL
                 || eTabValType == SPECIAL_NOTAPPL_TABVAL || eTabValType == SPECIAL_DEFAULT_TABVAL
                 || eTabValType == SPECIAL_UNDEFINED_TABVAL || eTabValType ==STAT_PCT_TABVAL){
                 arrTempTabVals.Add(pCurTabVal);
                 iInsertPos--;
                 if(iInsertPos == 0 && iTabVal != arrTabVals.GetCount()-1){//valid insert and not the last of the tabvals
                    iInsertPos = iReaderBreaks;
                    CTabValue* pRdrBrkTabVal = GetStatTabVal(arrStatTabVals,RDRBRK_TABVAL);
                    if(!pRdrBrkTabVal){
                        pRdrBrkTabVal = new CTabValue();
                        pRdrBrkTabVal->SetTabValType(RDRBRK_TABVAL);
                        pRdrBrkTabVal->SetParentVar(pTabVar);
                    }
                    arrTempTabVals.Add(pRdrBrkTabVal);
                 }
                 else {
                     //We are currently on a non stat
                     //if is transition add a reader break;
                     if(iTabVal != arrTabVals.GetCount()-1){//if current tabval is not the last one check for transition
                         CTabValue* pNextTabVal = arrTabVals[iTabVal+1];
                         TABVAL_TYPE eTabValType = pNextTabVal->GetTabValType();
                         if (!(eTabValType == DICT_TABVAL || eTabValType == SPECIAL_TABVAL
                             || eTabValType == SPECIAL_MISSING_TABVAL || eTabValType == SPECIAL_REFUSED_TABVAL
                             || eTabValType == SPECIAL_NOTAPPL_TABVAL || eTabValType == SPECIAL_DEFAULT_TABVAL
                             || eTabValType == SPECIAL_UNDEFINED_TABVAL || eTabValType == STAT_PCT_TABVAL)){
                                 CTabValue* pRdrBrkTabVal = GetStatTabVal(arrStatTabVals,RDRBRK_TABVAL);
                                 if(!pRdrBrkTabVal){
                                     pRdrBrkTabVal = new CTabValue();
                                     pRdrBrkTabVal->SetTabValType(RDRBRK_TABVAL);
                                     pRdrBrkTabVal->SetParentVar(pTabVar);
                                 }
                                 arrTempTabVals.Add(pRdrBrkTabVal);//seperator reader break tabval
                                 iInsertPos = iReaderBreaks; //reset reader break count
                             }
                     }
                 }

            }
            else {
                arrTempTabVals.Add(pCurTabVal);
                //We are currently on a stat
                //if is transition add a reader break;
                if(iTabVal != arrTabVals.GetCount()-1){//if current tabval is not the last one check for transition
                    CTabValue* pNextTabVal = arrTabVals[iTabVal+1];
                    TABVAL_TYPE eTabValType = pNextTabVal->GetTabValType();
                    if(eTabValType == DICT_TABVAL || eTabValType == SPECIAL_TABVAL
                    || eTabValType == SPECIAL_MISSING_TABVAL || eTabValType == SPECIAL_REFUSED_TABVAL
                    || eTabValType == SPECIAL_NOTAPPL_TABVAL || eTabValType == SPECIAL_DEFAULT_TABVAL
                    || eTabValType == SPECIAL_UNDEFINED_TABVAL || eTabValType == STAT_PCT_TABVAL){
                        CTabValue* pRdrBrkTabVal = GetStatTabVal(arrStatTabVals,RDRBRK_TABVAL);
                        if(!pRdrBrkTabVal){
                            pRdrBrkTabVal = new CTabValue();
                            pRdrBrkTabVal->SetTabValType(RDRBRK_TABVAL);
                            pRdrBrkTabVal->SetParentVar(pTabVar);
                        }
                        arrTempTabVals.Add(pRdrBrkTabVal);//seperator reader break tabval
                    }
                }
            }
        }
        arrTabVals.RemoveAll();
        arrTabVals.Append(arrTempTabVals);
        /*
        while(iCurPos < arrTabVals.GetSize()){
            CTabValue* pTabVal = GetStatTabVal(arrStatTabVals,RDRBRK_TABVAL);
            if(!pTabVal){
                pTabVal = new CTabValue();
                pTabVal->SetTabValType(RDRBRK_TABVAL);
                pTabVal->SetParentVar(pTabVar);
            }
            arrTabVals.InsertAt(iCurPos,pTabVal);
            iCurPos += iReaderBreaks+1;
        }*/
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabSet::MakeCrossTabStatement(CTable* pTable , CString& sCrossTabStatement)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabSet::MakeCrossTabStatement(CTable* pRefTable , CString& sCrossTabStatement , XTABSTMENT_TYPE eStatement /*= XTABSTMENT_ALL*/)
{
    MakeNameMap(pRefTable);
    int iNumLevels = GetNumLevels();
    const CDataDict* pDict = GetDict();
    CArray<CStringArray,CStringArray&> arrValidSubtableUnits;
    CStringArray arrValidTableUnits;
    UpdateSubtableList(pRefTable, arrValidSubtableUnits, arrValidTableUnits);
    // the code is this way thru levels  'cos we may need to pass the level info somehow to the srs mgr
    for (int iLevel =0; iLevel < iNumLevels ;iLevel++) {
        CTabLevel* pTabLevel = GetLevel(iLevel);
        int iNumTables = pTabLevel->GetNumTables();
        CString sLevel = pDict->GetLevel(iLevel).GetName();
        CString sRowVarList,sRowStatTotalList;
        CString sTabStatement;
        CString sColVarList,sColStatTotalList;
        //    sCrossTabStatement = "PROC " + sLevel + "\n";
        for (int iTable = 0; iTable < iNumTables; iTable++){
            CTable* pTable = pTabLevel->GetTable(iTable);
            if(pRefTable  != pTable )
                continue;
            sTabStatement += _T("CROSSTAB ") ;
            sTabStatement += pTable->GetName();
            // SAVY N CHRIS && TO DO 11/10
            CTabVar* pRowRoot = pTable->GetRowRoot();
            CTabVar* pColRoot = pTable->GetColRoot();
            if(pRowRoot->GetNumChildren() ==  0 && pColRoot->GetNumChildren() ==0){
                sTabStatement = _T("") ; // No Row or cloumn var . Don't generate proc .compile  will fail
                return ;
            }

            if(pRowRoot->GetNumChildren() >= 0 ) {
                GetVarList(pRowRoot,sRowVarList);
            }
            if(pColRoot->GetNumChildren() >= 0 ) {
                GetVarList(pColRoot,sColVarList);
            }

            if(pRowRoot->GetNumChildren() >= 0 ) {
                GetStatString(pRefTable,sRowStatTotalList);
                sRowStatTotalList.Trim();
            }
            if(pColRoot->GetNumChildren() >=  0 ) {
                GetStatString(pRefTable,sColStatTotalList,false);
                sColStatTotalList.Trim();
            }

        }
        if(!sRowVarList.IsEmpty() || !sColVarList.IsEmpty() ) {
            sCrossTabStatement  += sTabStatement + _T(" ");
            sCrossTabStatement += sRowVarList +  _T(", ");
            sCrossTabStatement += sColVarList ;

            CString sIncludeExclude = GetIncludeExcludeString(pRefTable);
            if(!sIncludeExclude.IsEmpty()){
                sCrossTabStatement = sCrossTabStatement +  _T("\r\n") + sIncludeExclude ; //for now exclude specval
            }
            bool bStat = false;
            if(!sRowStatTotalList.IsEmpty() || !sColStatTotalList.IsEmpty() ) {
                if(!bStat){
                    bStat  = true;
                    sCrossTabStatement += _T("\r\nSTAT ");
                }
                sCrossTabStatement  += sRowStatTotalList + _T(" ") + sColStatTotalList + _T(" ");
            }
            //Do unit
            CString sUnitStatement;
            if(eStatement != XTABSTMENT_BASIC){
            bool bRet = GetUnitStatement(pRefTable,sUnitStatement,eStatement);
            if(bRet){
                sCrossTabStatement += sUnitStatement;
            }
            }
            if(m_pConsolidate &&  m_pConsolidate->GetNumAreas() > 0){
                CString sBreakBy;
                if(pRefTable->GetBreakLevel() == 0){
                    sBreakBy = _T("NOBREAK");
                }
                else {
                    sBreakBy = _T("BREAK BY");
                    int iNumAreas2Process = m_pConsolidate->GetNumAreas();
                    if(pRefTable->GetBreakLevel() != -1){
                        if(pRefTable->GetBreakLevel() <= iNumAreas2Process){
                            iNumAreas2Process =pRefTable->GetBreakLevel();
                        }
                    }
                    for(int iArea = 0;  iArea< iNumAreas2Process; iArea++){
                        sBreakBy += _T(" ") + m_pConsolidate->GetArea(iArea) + _T(",");
                    }
                    sBreakBy.TrimRight(_T(","));
                }
                sCrossTabStatement += sBreakBy;
            }
            sCrossTabStatement += _T(";") ; //end
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabSet::MakeSMeanStatement(CTable* pTable , CString& sCrossTabStatement)
//
/////////////////////////////////////////////////////////////////////////////////
//Savy (R) sampling app 20090213 added method for generate logic
void CTabSet::MakeSMeanStatement(CTable* pRefTable , CString& sCrossSTabStatement , XTABSTMENT_TYPE eStatement /*= XTABSTMENT_ALL*/)
{
    MakeNameMap(pRefTable);
    int iNumLevels = GetNumLevels();
    const CDataDict* pDict = GetDict();
    CArray<CStringArray,CStringArray&> arrValidSubtableUnits;
    CStringArray arrValidTableUnits;
    UpdateSubtableList(pRefTable, arrValidSubtableUnits, arrValidTableUnits);
    // the code is this way thru levels  'cos we may need to pass the level info somehow to the srs mgr
    for (int iLevel =0; iLevel < iNumLevels ;iLevel++) {
        CTabLevel* pTabLevel = GetLevel(iLevel);
        int iNumTables = pTabLevel->GetNumTables();
        CString sLevel = pDict->GetLevel(iLevel).GetName();
        CString sRowVarList,sRowStatTotalList;
        CString sTabStatement;
        CString sColVarList,sColStatTotalList;
        //    sCrossTabStatement = "PROC " + sLevel + "\n";
        for (int iTable = 0; iTable < iNumTables; iTable++){
            CTable* pTable = pTabLevel->GetTable(iTable);
            if(pRefTable  != pTable )
                continue;
            sTabStatement += _T("SMEAN ") ;//Savy (R) sampling app 20090213
            sTabStatement += pTable->GetName();
            // SAVY N CHRIS && TO DO 11/10
            CTabVar* pRowRoot = pTable->GetRowRoot();
            CTabVar* pColRoot = pTable->GetColRoot();
            if(pRowRoot->GetNumChildren() ==  0 && pColRoot->GetNumChildren() ==0){
                sTabStatement = _T("") ; // No Row or cloumn var . Don't generate proc .compile  will fail
                return ;
            }

            if(pRowRoot->GetNumChildren() >= 0 ) {
                GetSVarList(pRowRoot,sRowVarList);
            }

            if(pRowRoot->GetNumChildren() >= 0 ) {
                GetStatString(pRefTable,sRowStatTotalList);
                sRowStatTotalList.Trim();
            }
        }
        //Savy (R) sampling app 20090213
        //Analysis Variable / Denominator
        CString sAnalysisVar,sDenominator;
        sAnalysisVar = pRefTable->GetTableUnit().GetAnalysisVariable();
        sDenominator = pRefTable->GetTableUnit().GetDenominator();
        //end
        if(!sRowVarList.IsEmpty() /*|| !sColVarList.IsEmpty()*/ ) {
            sCrossSTabStatement  += sTabStatement + _T(" ");
            //Savy (R) sampling app 20090213
            if(!sAnalysisVar.IsEmpty()){
                sCrossSTabStatement += sAnalysisVar + _T(" ");
            }
            if(!sDenominator.IsEmpty()){
                sCrossSTabStatement += _T("/");
                sCrossSTabStatement += sDenominator + _T(" ");
            }
            sCrossSTabStatement += sRowVarList +  _T(" ");
           // sCrossSTabStatement += sColVarList ;

            CString sIncludeExclude = GetIncludeExcludeString(pRefTable);
            if(!sIncludeExclude.IsEmpty()){
                sCrossSTabStatement = sCrossSTabStatement +  _T("\r\n") + sIncludeExclude ; //for now exclude specval
            }

            sCrossSTabStatement += _T("BREAK BY");
            sCrossSTabStatement += _T(" ");

            //Cluster variable
            CString sClusterVar = GetClusterVariable();
            if(!sClusterVar.IsEmpty()){
                sCrossSTabStatement = sCrossSTabStatement + sClusterVar;
            }

            //Strata Variable
            CString sStrataVar = GetStrataVariable();
            if(!sStrataVar.IsEmpty()){
                sCrossSTabStatement += _T(",");
                sCrossSTabStatement += sStrataVar;
            }

            //Domainvar
            DOMAINVAR pDomVar;
            if(GetDomainVarArray().GetCount() >0){
                for (int i = 0 ; i < GetDomainVarArray().GetCount() ; i++) {
                    pDomVar = GetDomainVarArray().GetAt(i);
                    sCrossSTabStatement += _T(",");
                    sCrossSTabStatement += pDomVar.m_sDomainVarName;

                }
            }
            //

            sCrossSTabStatement += _T(";") ; //end
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabSet::MakeNameMap(CTable* pTable)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabSet::MakeNameMap(CTable* pTable)
{
    m_varNameMap.RemoveAll();
    CTabVar* pRowRoot = pTable->GetRowRoot();
    CMapStringToString  arrNames;
    MakeNameMap(pRowRoot , arrNames);
    CTabVar* pColRoot = pTable->GetColRoot();
    MakeNameMap(pColRoot , arrNames);
    return;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabSet::MakeNameMap(CTabVar* pTabVar, CMapStringToString& arrNames)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabSet::MakeNameMap(CTabVar* pTabVar, CMapStringToString& arrNames)
{
    CString sName = pTabVar->GetName();
    sName.Trim();
    if(!sName.IsEmpty()){
        sName.MakeUpper();
        CIMSAString sKeyVal;
        int iVal = -1;
        if(arrNames.Lookup(sName,sKeyVal)){
            iVal = (int)sKeyVal.Val();
            CString sVal = IntToString(iVal+1);
            CString sNewName;
            sNewName = sName+_T("(")+sVal+_T(")");
            m_varNameMap[pTabVar]=sNewName;
            arrNames[sName] =sVal;
            //look for the var which has this name and make it name(0)
            POSITION pos = m_varNameMap.GetStartPosition();
            while(pos != NULL) {
                CTabVar* pKeyVar = NULL;
                CString sAssocName;
                m_varNameMap.GetNextAssoc(pos,pKeyVar,sAssocName);
                if(sAssocName.CompareNoCase(sName) ==0){
                    m_varNameMap.SetAt(pKeyVar, sName + _T("(1)"));
                    break;
                }
            }
        }
        else {
            arrNames[sName] = _T("1");
            m_varNameMap[pTabVar]=sName;
        }
    }
    for(int iIndex =0; iIndex < pTabVar->GetNumChildren(); iIndex++){
        CTabVar* pChild  =pTabVar->GetChild(iIndex);
        MakeNameMap(pChild, arrNames);
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabSet::GetVarList(CTabVar* pTabVar , CString& sVarList)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabSet::GetVarList(CTabVar* pTabVar , CString& sVarList)
{
    bool bRet = true;
    int iSize = pTabVar->GetNumChildren();
    for (int iIndex = 0; iIndex < iSize; iIndex++) {
        const DictLevel* pDictLevel = NULL;
        const CDictRecord* pDictRecord = NULL;
        const CDictItem* pDictItem = NULL;
        const DictValueSet* pDictVSet = NULL;

        CString sTotal , sName ;
        CTabVar* pChild = pTabVar->GetChild(iIndex);

        /*  if(m_varNameMap.Lookup(pChild,sName)){
        }
        else {
        sName = pChild->GetName();
        }*/
        sName = pChild->GetName();
        if(pChild->GetOcc() >= 0) {
            CIMSAString sOcc;
            sOcc.Str(pChild->GetOcc()+1) ;//zero based
            sName = sName + _T("(") + sOcc + _T(")");
        }
        if((sName.CompareNoCase(WORKVAR_TOTAL_NAME) ==0)){
            sVarList += WORKVAR_TOTAL_NAME;
            continue;
        }

        const CDataDict* pDict = LookupName(pChild->GetName(),&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet);
        if(!pDictVSet && pDict ) {
            sName = MakeVarList4DummyVSet(pDictItem);
        }
        sVarList +=   sName ;
        if(pChild->GetNumChildren() > 0 ) {
            bool bBoth  = pChild->GetChild(0)->GetName().CompareNoCase(_T("TT_BOTH")) ==0;
            bool bCustom =  pChild->GetChild(0)->GetType() == VT_CUSTOM ;
            if(bBoth && bCustom ){
                //do nothing
            }
            else {
                sVarList += _T(" * ");
                sVarList += _T(" (") ;
                GetVarList(pChild , sVarList);
                sVarList += _T(") ") ;
            }

        }
        if(iIndex < iSize-1) {
            sVarList +=  _T(" + ");
        }
    }

    return bRet;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabSet::GetSVarList(CTabVar* pTabVar , CString& sVarList)
//
/////////////////////////////////////////////////////////////////////////////////
//Savy (R) sampling app 20090213
bool CTabSet::GetSVarList(CTabVar* pTabVar , CString& sVarList)
{
    bool bRet = true;
    int iSize = pTabVar->GetNumChildren();
    for (int iIndex = 0; iIndex < iSize; iIndex++) {
        const DictLevel* pDictLevel = NULL;
        const CDictRecord* pDictRecord = NULL;
        const CDictItem* pDictItem = NULL;
        const DictValueSet* pDictVSet = NULL;

        CString sTotal , sName ;
        CTabVar* pChild = pTabVar->GetChild(iIndex);

        /*  if(m_varNameMap.Lookup(pChild,sName)){
        }
        else {
        sName = pChild->GetName();
        }*/
        sName = pChild->GetName();
        if(pChild->GetOcc() >= 0) {
            CIMSAString sOcc;
            sOcc.Str(pChild->GetOcc()+1) ;//zero based
            sName = sName + _T("(") + sOcc + _T(")");
        }
        if((sName.CompareNoCase(WORKVAR_TOTAL_NAME) ==0)){
            sVarList += WORKVAR_TOTAL_NAME;
            continue;
        }

        const CDataDict* pDict = LookupName(pChild->GetName(),&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet);
        if(!pDictVSet && pDict ) {
            sName = MakeVarList4DummyVSet(pDictItem);
        }
        //Savy (R) sampling app 20090223
        sVarList += _T("SYSTEM_TOTAL");
        sVarList += _T("+");
        sVarList += sName;
        if(pChild->GetNumChildren() > 0 ) {
            bool bBoth  = pChild->GetChild(0)->GetName().CompareNoCase(_T("TT_BOTH")) ==0;
            bool bCustom =  pChild->GetChild(0)->GetType() == VT_CUSTOM ;
            if(bBoth && bCustom ){
                //do nothing
            }
            else {
                sVarList += _T(" * ");
                sVarList += _T(" (") ;
                GetVarList(pChild , sVarList);
                sVarList += _T(") ") ;
            }

        }
        if(iIndex < iSize-1) {
            sVarList +=  _T(" + ");
        }
    }

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabSet::GetStatString(CTable* pTable,CString& sStatString,bool bRow /*=true*/)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabSet::GetStatString(CTable* pTable,CString& sStatString,bool bRow /*=true*/)
{
    bool bRet = true;
    ASSERT(pTable);
    CTabVar* pRootVar = NULL;
    CTallyFmt* pTableTallyFmt = NULL;
    FMT_ID eFmtID = FMT_ID_INVALID;
    bRow ? eFmtID = FMT_ID_TALLY_ROW :eFmtID = FMT_ID_TALLY_COL;
    pTableTallyFmt= DYNAMIC_DOWNCAST(CTallyFmt,m_fmtReg.GetFmt(eFmtID));

    ASSERT(pTableTallyFmt);

    if(bRow) {
        pRootVar = pTable->GetRowRoot();
    }
    else {
        pRootVar = pTable->GetColRoot();
    }

    //Code generation shld entirely depened on the variable's tally attrib and not on the col/row dependencies
    //anyway this is not done right . Here if both have percents ASSERT ???
    /* bRow ? bTblPrecentAlongColVars = false : bTblPrecentAlongColVars = (pTableTallyFmt->GetPercentPos() == PCT_POS_BELOW_OR_RIGHT);
    bool bTablePercents = !(pTableTallyFmt->GetPercentType() == PCT_TYPE_DEFAULT ||  pTableTallyFmt->GetPercentType() ==PCT_TYPE_NONE );
    */
    int iSize = pRootVar->GetNumChildren();
    for (int iIndex = 0; iIndex < iSize; iIndex++) {

        CTabVar* pChild = pRootVar->GetChild(iIndex);
        CTallyFmt* pVarTallyFmt = pChild->GetTallyFmt();
        CString sVarStatString = _T("");
        if(!pVarTallyFmt){
            pVarTallyFmt=pTableTallyFmt;
        }

        sVarStatString = _T("");
        for (int iStat = 0; iStat < pVarTallyFmt->GetStats().GetCount(); ++iStat) {
            sVarStatString += pVarTallyFmt->GetStats().GetAt(iStat)->GetStatVarString(pVarTallyFmt->GetStats());
        }
        sVarStatString.Trim();
        sVarStatString.TrimLeft(_T(","));
        sVarStatString.TrimRight(_T(","));
        if(!sVarStatString.IsEmpty()){
            CString sVarName;
            if(m_varNameMap.Lookup(pChild,sVarName)){
            }
            else {
                sVarName = pChild->GetName();
            }
            sVarStatString = sVarName + _T(" (")+sVarStatString +_T(")\r\n");
            sStatString += sVarStatString;
        }

        //Now do for each child GetFreqString and all other stat strings
        for (int iChild =0;iChild <pChild->GetNumChildren(); iChild++){
            CTabVar* pChildVar = pChild->GetChild(iChild);
            CTallyFmt* pVarTallyFmt = pChildVar->GetTallyFmt();
            if(!pVarTallyFmt){
                pVarTallyFmt = DYNAMIC_DOWNCAST(CTallyFmt,GetFmtRegPtr()->GetFmt(eFmtID));
            }
            ASSERT(pVarTallyFmt->GetID() == eFmtID);
            CString sVarStatString = _T("");

            for (int iStat = 0; iStat < pVarTallyFmt->GetStats().GetCount(); ++iStat) {
                sVarStatString += pVarTallyFmt->GetStats().GetAt(iStat)->GetStatVarString(pVarTallyFmt->GetStats());
            }

            sVarStatString.Trim();
            sVarStatString.TrimLeft(_T(","));
            sVarStatString.TrimRight(_T(","));
            if(sVarStatString.CompareNoCase(_T("FREQ")) == 0){
                sVarStatString = _T("");//No "FREQ" standalone required
            }

            if(!sVarStatString.IsEmpty()){
                CString sVarName;
                if(m_varNameMap.Lookup(pChildVar,sVarName)){
                }
                else {
                    sVarName = pChildVar->GetName();
                }
                sVarStatString = sVarName + _T(" (")+sVarStatString +_T(")\r\n");
                sStatString += sVarStatString;
            }
        }
    }

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CString CTabSet::GetIncludeExcludeString(CTable* pTable)
//
/////////////////////////////////////////////////////////////////////////////////
CString CTabSet::GetIncludeExcludeString(CTable* pTable)
{
    CString sIncludeExclude;
    bool bUseSpecValNotAppl = pTable->GetCustSpecialValSettings().GetUseSpecValNotAppl() ;
    bool bUseSpecValMissing = pTable->GetCustSpecialValSettings().GetUseSpecValMissing();
    bool bUseSpecValRefused = pTable->GetCustSpecialValSettings().GetUseSpecValRefused();
    bool bUseSpecValDefault = pTable->GetCustSpecialValSettings().GetUseSpecValDefault();
    bool bUseSpecValUndefined = pTable->GetCustSpecialValSettings().GetUseSpecValUndefined();

    bool bUseInclude = bUseSpecValNotAppl || bUseSpecValMissing || bUseSpecValRefused || bUseSpecValDefault || bUseSpecValUndefined;
    bool bProcessDictSpecials = pTable->GetCustSpecialValSettings().GetUseCustomSpecVal();
    if(bUseInclude || bProcessDictSpecials) {
        sIncludeExclude = _T("INCLUDE (");
        bUseSpecValNotAppl   ? sIncludeExclude += _T("NOTAPPL, ")   : sIncludeExclude=sIncludeExclude;
        bUseSpecValMissing   ? sIncludeExclude += _T("MISSING, ")   : sIncludeExclude=sIncludeExclude;
        bUseSpecValRefused   ? sIncludeExclude += _T("REFUSED, ")   : sIncludeExclude=sIncludeExclude;
        bUseSpecValDefault   ? sIncludeExclude += _T("DEFAULT, ")   : sIncludeExclude=sIncludeExclude;
        bUseSpecValUndefined ? sIncludeExclude += _T("UNDEFINED, ") : sIncludeExclude=sIncludeExclude;

        sIncludeExclude.TrimRight(_T(", "));
        sIncludeExclude += _T(")");

    }
    else {
        //TO DO no more exclude stuff in the new crosstab . remove this statement later SAVY&&&
       // sIncludeExclude = "Exclude(Specval)";
    }

    return sIncludeExclude;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  CString CTabSet::MakeVarList4DummyVSet (CDictItem* pDictItem)
//
/////////////////////////////////////////////////////////////////////////////////
CString CTabSet::MakeVarList4DummyVSet(const CDictItem* pDictItem)
{
    CString sVarList;
    sVarList = pDictItem->GetName() ;
    // It's an item with no value sets
    int len = pDictItem->GetLen();
    int iVals = (len > 1 ? 11 : 10);
    CString sValue;
    for(int iIndex =0;iIndex < iVals ; iIndex++ ){
        if (len > 1) {
            if (iIndex == 0) {
                sVarList += _T("[");
//              sValue = _T("< 0");
                //serpro's syntax does not take negative values
                continue;
            }
            else {
                csprochar pszTemp[20];
                GetPrivateProfileString(_T("intl"), _T("sDecimal"), _T("."), pszTemp, 10, _T("WIN.INI"));
                csprochar cDec = pszTemp[0];

                CString sFrom(_T('0'), len);
                CString sTo(_T('9'), len);
                sFrom.SetAt(0, (csprochar)(_T('0') + iIndex - 1));
                sTo.SetAt(0, (csprochar)(_T('0') + iIndex - 1));

                UINT uDec = pDictItem->GetDecimal();
                if (uDec > 0 && pDictItem->GetDecChar()) {
                    sFrom.SetAt(len - uDec - 1, pszTemp[0]);
                    sTo.SetAt(len - uDec - 1, pszTemp[0]);
                }
                double dTemp = atod(sFrom, uDec);
                sFrom = dtoa(dTemp, pszTemp, uDec, cDec, false);
                dTemp = atod(sTo, uDec);
                sTo = dtoa(dTemp, pszTemp, uDec, cDec, false);
                sValue += _T("(");
                sValue += sFrom;
                sValue += _T(":");
                sValue += sTo;
                sValue += _T("),");
                sVarList += sValue;
            }
        }
        else {
            csprochar pszTemp[20];
            GetPrivateProfileString(_T("intl"), _T("sDecimal"), _T("."), pszTemp, 10, _T("WIN.INI"));
            csprochar cDec = pszTemp[0];
            if (iIndex == 0) {
                sVarList += _T("[");
//              continue;
            }
            CString sFrom(_T('0'), len);
            sFrom.SetAt(0, (csprochar)(_T('0') + iIndex));

            UINT uDec = pDictItem->GetDecimal();
            if (uDec > 0 && pDictItem->GetDecChar()) {
                sFrom.SetAt(len - uDec - 1, pszTemp[0]);
            }
            double dTemp = atod(sFrom, uDec);
            sFrom = dtoa(dTemp, pszTemp, uDec, cDec, false);
            sValue = sFrom;
            sVarList += sValue +_T(",");
        }
        if(!sValue.IsEmpty()){
            sValue=_T("");
        }
    }
    sVarList.TrimRight(_T(","));
    sVarList+=_T("]");

    return sVarList;
}

void CTable::GetOneUnitSubStatement(const CUnitSpec& unitSpec, CString& sUnitStatement , XTABSTMENT_TYPE eStatement)
{
    CString sSubStatement;
    const CUnitSpec& unitTableSpec = GetTableUnit();
    CString sLoopingVarName = unitSpec.GetLoopingVarName();
    sLoopingVarName.Trim();
    if(this->GetUnitSpecArr().GetSize() ==1){//if there is only one subtable .use table's looping var
        if(!unitTableSpec.GetLoopingVarName().IsEmpty()){
        sLoopingVarName = unitTableSpec.GetLoopingVarName();
    }
    }
    else {
        if(sLoopingVarName.IsEmpty()){
            sLoopingVarName = unitTableSpec.GetLoopingVarName();
            sLoopingVarName.Trim();
        }
    }
    sSubStatement += sLoopingVarName + _T("\r\n");

    //Do Value stuff here ??
    if(eStatement == XTABSTMENT_ALL || eStatement == XTABSTMENT_WGHT_ONLY){
        bool bHasWeight = !unitSpec.GetWeightExpr().IsEmpty() || !unitTableSpec.GetWeightExpr().IsEmpty();
        bool bHasValue = !unitSpec.GetValue().IsEmpty() || !unitTableSpec.GetValue().IsEmpty();
        if(bHasWeight || bHasValue){
            CString sEvalValue = unitSpec.GetValue();
            CString sTableUnitValue = unitTableSpec.GetValue();
            sTableUnitValue.Trim();
            sEvalValue.Trim();
            bool bAddParenV=true;
            if(!sEvalValue.IsEmpty() && !sTableUnitValue.IsEmpty()){
                sEvalValue = _T("(") + sEvalValue + _T(")*") + _T("(") + sTableUnitValue + _T(")");
                bAddParenV =false;
            }
            else if(!sTableUnitValue.IsEmpty()){
                sEvalValue = sTableUnitValue;
            }

            CString sEvalWeight = unitSpec.GetWeightExpr();
            sEvalWeight.Trim();
            CString sTableUnitWeight = unitTableSpec.GetWeightExpr();
            sTableUnitWeight.Trim();
            sEvalWeight.Trim();
            bool bAddParenW=true;
            if(!sEvalWeight.IsEmpty() && !sTableUnitWeight.IsEmpty()){
                sEvalWeight = _T("(") + sEvalWeight + _T(")*") + _T("(") + sTableUnitWeight + _T(")");
                bAddParenW =false;
            }
            else if(!sTableUnitWeight.IsEmpty()){
                sEvalWeight = sTableUnitWeight;
            }


            if(!sEvalValue.IsEmpty() && !sEvalWeight.IsEmpty()){
                if(bAddParenV){
                    sEvalValue = _T("( ") + sEvalValue + _T(" ) *");
                }
                if(bAddParenW){
                    sEvalWeight = _T("( ") + sEvalWeight + _T(" )");
                }
                sSubStatement += _T(" WEIGHTED BY ") + sEvalValue + sEvalWeight + _T("\r\n");
            }
            else {
                sSubStatement += _T(" WEIGHTED BY ") + sEvalValue + sEvalWeight + _T("\r\n") ;
            }
        }
    }
    if(eStatement == XTABSTMENT_ALL || eStatement == XTABSTMENT_UNIV_ONLY){
        CString sTableUniverse = unitTableSpec.GetUniverse();
        sTableUniverse.Trim();
        CString sEvalUniverse = unitSpec.GetUniverse();
        sEvalUniverse.Trim();

        if(!sEvalUniverse.IsEmpty() && !sTableUniverse.IsEmpty()){
            sEvalUniverse = _T("(") + sEvalUniverse + _T(") AND ") + _T("(") + sTableUniverse + _T(")");
        }
        else if(!sTableUniverse.IsEmpty()){
            sEvalUniverse = sTableUniverse;
        }

        if(!sEvalUniverse.IsEmpty()){
            sSubStatement += _T(" SELECT ") + sEvalUniverse + _T("\r\n") ;
        }
    }
    if(eStatement == XTABSTMENT_ALL || eStatement == XTABSTMENT_TABLOGIC_ONLY){
        CUnitSpec* pLocalUnitSpec= const_cast<CUnitSpec*>(&unitSpec);
        CUnitSpec* pTblUnitSpec= const_cast<CUnitSpec*>(&unitTableSpec);
        CStringArray& arrUnitTabLogic = pLocalUnitSpec->GetTabLogicArray();
        CStringArray& arrTblTabLogic = pTblUnitSpec->GetTabLogicArray();
        CString sTabLogic;
        if(arrTblTabLogic.GetSize() > 0 ||arrUnitTabLogic.GetSize()>0 ){
            //Put the table logic
            for(int iIndex =0; iIndex< arrTblTabLogic.GetSize();iIndex++){
                sTabLogic += _T(" ") + arrTblTabLogic[iIndex] + _T("\r\n");
            }
            //put the unit logic
            for(int iIndex =0; iIndex< arrUnitTabLogic.GetSize();iIndex++){
                sTabLogic += _T(" ") + arrUnitTabLogic[iIndex] + _T("\r\n");
            }
        }

        if(!sTabLogic.IsEmpty()){
            sSubStatement += _T("TABLOGIC \r\n")+ sTabLogic + _T("ENDLOGIC \r\n") ;
        }
    }
    if (!sSubStatement.IsEmpty()) {
        sUnitStatement +=  sSubStatement ;
        sUnitStatement += _T("ENDUNIT \r\n");
    }
}

//Savy (R) sampling app 20090218
bool CTable::IsValidSMeanTable(CString &sMsg)
{
    int bRet;
    CString sErrorMsg;

    if(m_tableUnit.GetSampAnalysisType() == SAMPLING_ANALYSIS_TYPE::TOTALS_ANALYSIS_TYPE || m_tableUnit.GetSampAnalysisType() == SAMPLING_ANALYSIS_TYPE::MEANS_ANALYSIS_TYPE || m_tableUnit.GetSampAnalysisType() == SAMPLING_ANALYSIS_TYPE::RATIOS_ANALYSIS_TYPE || m_tableUnit.GetSampAnalysisType() == SAMPLING_ANALYSIS_TYPE::PROPORTIONS_ANALYSIS_TYPE)
    {
        if(m_tableUnit.GetAnalysisVariable().IsEmpty()){
            sErrorMsg = _T("Analysis Variable.");
        }
        if(m_tableUnit.GetSampAnalysisType() == SAMPLING_ANALYSIS_TYPE::RATIOS_ANALYSIS_TYPE)
        {
            if(m_tableUnit.GetDenominator().IsEmpty()){
                if(sErrorMsg.GetLength()>0){
                    sErrorMsg = _T("Denominator, ") + sErrorMsg;
                }
                else{
                    sErrorMsg = _T("Denominator.");
                }

            }

        }
    }
    else{
        sErrorMsg = _T("Analysis Type.\n");
        sErrorMsg += _T("Invalid Analysis Variable / Denominator.");
    }

    if(sErrorMsg.GetLength()>0)
    {
        sErrorMsg = GetName() + _T(" :\nInvalid ") + sErrorMsg;
        sMsg = sErrorMsg;
        bRet = false;
    }
    else{
        bRet = true;
    }

    return bRet;
}
//Savy (R) sampling app 20090219
CString CTable::GetLogic4Samp(XTABSTMENT_TYPE eStatement /* = XTABSTMENT_ALL*/)
{
    CString sLogic4Samp;
    bool bXtabDone = false;
    CStringArray& arrTabLogic = m_tableUnit.GetTabLogicArray();

    if(eStatement == XTABSTMENT_ALL || eStatement == XTABSTMENT_TABLOGIC_ONLY){

        for(int iLine =0; iLine < arrTabLogic.GetSize();iLine++){
            sLogic4Samp +=arrTabLogic[iLine];
            sLogic4Samp += _T("\n");
        }

    }

    if(eStatement ==  XTABSTMENT_ALL || eStatement == XTABSTMENT_UNIV_ONLY){
        if(m_tableUnit.GetUniverse().GetLength() >0){
            bXtabDone = true;
            sLogic4Samp += _T("if (") + m_tableUnit.GetUniverse();
            sLogic4Samp += _T(") then\n");
            if(m_tableUnit.GetWeightExpr().GetLength() > 0){
                sLogic4Samp +=  _T("  xtab(");
                sLogic4Samp +=  GetName() + _T(",");
                sLogic4Samp +=  m_tableUnit.GetWeightExpr() + _T(")");
                sLogic4Samp +=  _T(";\n");
            }
            else{
                sLogic4Samp += _T("  xtab(");
                sLogic4Samp += GetName() + _T(")");
                sLogic4Samp += _T(";\n");
            }
            sLogic4Samp += _T("endif;");
        }
    }
    if(!bXtabDone && (eStatement ==  XTABSTMENT_ALL || eStatement == XTABSTMENT_WGHT_ONLY)){
        //Savy (R) sampling app 20090220
        if(m_tableUnit.GetWeightExpr().GetLength() > 0){
            bXtabDone = true;//Savy (R) sampling app 20090224
            sLogic4Samp +=  _T("xtab(");
            sLogic4Samp +=  GetName() + _T(",");
            sLogic4Samp +=  m_tableUnit.GetWeightExpr() + _T(")");
            sLogic4Samp +=  _T(";\n");
        }
    }
    if(!bXtabDone){
            sLogic4Samp += _T("xtab(");
            sLogic4Samp += GetName() + _T(")");
            sLogic4Samp += _T(";\n");
    }

    return sLogic4Samp;
}

bool CTabSet::GetUnitStatement(CTable* pTable,CString& sUnitStatement , XTABSTMENT_TYPE eStatement /*= XTABSTMENT_ALL*/)
{
    bool bRet = false;
    CArray<CUnitSpec,CUnitSpec&>& arrUnitSpec = pTable->GetUnitSpecArr();
    const CUnitSpec& unitTableSpec = pTable->GetTableUnit();

    //Call update subtable list to get the unit stuff poopulated correctly
    CArray<CStringArray,CStringArray&> arrValidSubtableUnitNames;
    CStringArray arrValidTableUnitNames;

    UpdateSubtableList(pTable, arrValidSubtableUnitNames, arrValidTableUnitNames,
                false,NULL,NULL,NULL);

    for(int iIndex = 0 ; iIndex <arrUnitSpec.GetSize();iIndex++){
        CUnitSpec unitSpec = arrUnitSpec[iIndex];//get a copy of the subtable unit
        if(unitSpec.GetLoopingVarName().IsEmpty() && unitSpec.GetUseDefaultLoopingVar()){
            //set looping var name for forced unit generation
            unitSpec.SetLoopingVarName(arrValidSubtableUnitNames[iIndex].GetAt(0));
        }
        //if(unitSpec.IsUnitPresent() || unitTableSpec.IsUnitPresent() ){
        if(true){//generate all the time item with occs by item with occs problem
            bRet = true;
            sUnitStatement += _T("\r\nUNIT (") + unitSpec.GetSubTableString()+_T(") ");
            pTable->GetOneUnitSubStatement(unitSpec, sUnitStatement, eStatement);
        }
    }
    /*if(!bRet && unitTableSpec.IsUnitPresent()){ //Table unit  never gets generated
        bRet = true;
        sUnitStatement += "\r\nUNIT ";
        GetOneUnitSubStatement(unitSpec, unitTableSpec, eStatement);
    }*/

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabulateDoc::GetPercentString(PCT_TYPE ePerType,TOTALS_POSITION eTotals,CString& sVarStatString)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabSet::GetPercentString(PCT_TYPE ePerType,TOTALS_POSITION eTotals,CString& sVarStatString)
{
    bool bTotalPresent = (eTotals == TOTALS_POSITION_BEFORE ||  eTotals == TOTALS_POSITION_AFTER) ;
    switch(ePerType){
            case PCT_TYPE_TOTAL:
                if(bTotalPresent){
                    if(eTotals == TOTALS_POSITION_BEFORE){
                        sVarStatString +=_T(" TOTAL(PCT(TOTAL)),PCT(TOTAL),");
                    }
                    else {
                        sVarStatString +=_T(" PCT(TOTAL),TOTAL(PCT(TOTAL)),");
                    }
                }
                else {
                    sVarStatString +=_T("PCT(TOTAL),");
                }
                break;
            case PCT_TYPE_ROW:
                if(bTotalPresent){
                    if(eTotals == TOTALS_POSITION_BEFORE){
                        sVarStatString +=_T(" TOTAL(PCT(ROW)),PCT(ROW),");
                    }
                    else {
                        sVarStatString +=_T(" PCT(ROW),TOTAL(PCT(ROW)),");
                    }
                }
                else {
                    sVarStatString +=_T("PCT(ROW),");
                }
                break;
            case PCT_TYPE_COL:
                if(bTotalPresent){
                    if(eTotals == TOTALS_POSITION_BEFORE){
                        sVarStatString +=_T(" TOTAL(PCT(COLUMN)),PCT(COLUMN),");
                    }
                    else {
                        sVarStatString +=_T(" PCT(COLUMN),TOTAL(PCT(COLUMN)),");
                    }
                }
                else {
                    sVarStatString +=_T("PCT(COLUMN),");
                }
                break;
            case PCT_TYPE_CELL:
                if(bTotalPresent){
                    if(eTotals == TOTALS_POSITION_BEFORE){
                        sVarStatString +=_T(" TOTAL(PCT(CELL)),PCT(CELL),");
                    }
                    else {
                        sVarStatString +=_T(" PCT(CELL),TOTAL(PCT(CELL)),");
                    }
                }
                else {
                    sVarStatString +=_T("PCT(CELL),");
                }
                break;
            case PCT_TYPE_NONE:
            case PCT_TYPE_DEFAULT:
            default:
                break;
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabSet::GetPercentString(PCT_TYPE ePerType,TOTALS_POSITION eTotals,CString& sVarStatString)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabSet::GetFreqString(TALLY_STATISTIC eCountType,TOTALS_POSITION eTotals,CString& sVarStatString)
{
    bool bTotalPresent = (eTotals == TOTALS_POSITION_BEFORE ||  eTotals == TOTALS_POSITION_AFTER) ;
    switch(eCountType){
            case TALLY_STATISTIC_YES:
                if(bTotalPresent){
                    if(eTotals == TOTALS_POSITION_BEFORE){
                        sVarStatString +=_T(" TOTAL,FREQ,");
                    }
                    else {
                        sVarStatString +=_T(" FREQ,TOTAL,");
                    }
                }
                else {
                    sVarStatString +=_T("FREQ,");
                }
                break;
            case TALLY_STATISTIC_NO:
            case TALLY_STATISTIC_DEFAULT:
            default:
                break;
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabSet::ReconcileUnitLoopingVar
//
// Reconcile existing looping var for unit with updated list of valid
// looping vars.  Assumes first of valid units is default.
// If old looping var is still valid, keep it.
// If old looping var was set to default, silently set new one to default.
// Otherwise set looping var to default and warn that it has changed.
/////////////////////////////////////////////////////////////////////////////////
void CTabSet::ReconcileUnitLoopingVar(CUnitSpec& unit,
                                      const CStringArray& asValidLoopingVars,
                                      const CString& sSubtableName,
                                      CString& sMsg)
{
    // check for that current unit is a valid one
    int i;
    for (i = 0; i < asValidLoopingVars.GetSize(); ++i) {
        if (unit.GetLoopingVarName().CompareNoCase(asValidLoopingVars[i]) == 0) {
            break;
        }
    }
    if (i == asValidLoopingVars.GetSize()) {
        // old unit no longer valid - use default
        if (!unit.GetUseDefaultLoopingVar()) {
            //only warn user if wasn't previously set to default
            sMsg += _T("UNIT ") + unit.GetLoopingVarName() + _T(" no longer valid for ") + sSubtableName +
                _T("; changing to default unit\n");
        }
        CString sNewLoopingVarName = asValidLoopingVars.GetCount() != 0 ?
                                        asValidLoopingVars[0] : CString();
        unit.SetLoopingVarName(sNewLoopingVarName);
        unit.SetUseDefaultLoopingVar(true);
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  int CTabSet::UpdateSubtableList(CTable* pRefTable)
//
/////////////////////////////////////////////////////////////////////////////////
int CTabSet::UpdateSubtableList(CTable* pRefTable,
                                 CArray<CStringArray,CStringArray&>& arrValidSubtableUnits,
                                 CStringArray& arrValidTableUnits,
                                 bool bReconcile /* = false */,
                                 CString* pReconcileMsg /* = NULL */,CTabVar* pRowVar/*=NULL*/
                                 ,CTabVar* pColVar/*=NULL*/
                                 )
{
    int iRet = -1;
    int iSubTableRow = 0;
    int iSubTableCol = 0;
    MakeNameMap(pRefTable);
    //pRefTable->UpdateSubtableList();
    CArray<CUnitSpec,CUnitSpec&> arrUnitSpec; //local array for processing
    CArray<CUnitSpec,CUnitSpec&>& arrTableUnitSpec = pRefTable->GetUnitSpecArr(); //local array for processing
    CStringArray sArrRow,sAltArrRow;
    CStringArray sArrCol,sAltArrCol;
    CString sCurSelSubTable,sCurSelAltSubTable;
    //process the row vars first
    CTabVar* pRowRootVar = pRefTable->GetRowRoot();
    int iNumRowVars = pRowRootVar->GetNumChildren();\

    if(pRowRootVar->GetNumChildren() ==0){
        //Remove all the Unit specs from the table
        arrTableUnitSpec.RemoveAll();
        return iRet;
    }
    //Generate Row Strings
    CArray<CStringArray,CStringArray> asRowVars;

    for(int iIndex =0;iIndex < iNumRowVars ; iIndex++){
        const DictLevel* pDictLevel = NULL;
        const CDictRecord* pDictRecord = NULL;
        const CDictItem* pDictItem = NULL;
        const DictValueSet* pDictVSet = NULL;

        CTabVar* pTabVar = pRowRootVar->GetChild(iIndex);
        CString sParentString,sAltParentString;
        if(m_varNameMap.Lookup(pTabVar,sParentString)){
        }
        else {
            sParentString = pTabVar->GetName();
        }
        sAltParentString = sParentString;
        //new scheme dont add "single var names" in the sub table list shld be alway VAR1 BY VAR2
        /*CUnitSpec unitSpec(sParentString);
        arrUnitSpec.Add(unitSpec);*/
        if(pTabVar->GetNumChildren() == 0) {
            sArrRow.Add(sParentString);
            //Check the dict . If VSet only one the add Alt String . If more than one vset
            //add the correct string which is the same as sArrRow stuff
            if(LookupName(pTabVar->GetName(),&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet)){
                if(pDictItem->GetNumValueSets() > 1) {
                    sAltArrRow.Add(sAltParentString);
                }
                else {//if it same as vset name then replace . If it is p03_sex_vs1(1) ??
                    if(pDictVSet) {
                        sAltParentString.Replace(pDictVSet->GetName(),pDictItem->GetName());
                        sAltArrRow.Add(sAltParentString);
                    }
                    else {
                        sAltArrRow.Add(sAltParentString);
                    }
                }
            }
            else {
                sAltArrRow.Add(sAltParentString);
            }
            asRowVars.SetSize(sArrRow.GetSize());
            CIMSAString sOcc ;
            if(pTabVar->GetOcc() > -1) {
                sOcc.Str(pTabVar->GetOcc());
                sOcc = _T("(") + sOcc+ _T(")");
            }
            asRowVars[sArrRow.GetSize()-1].Add(pTabVar->GetName()+sOcc);
            if(pTabVar == pRowVar){
                iSubTableRow = sArrRow.GetSize()-1;
            }
        }
        else {
            int iNumChildren = pTabVar->GetNumChildren();
            for(int iIndex =0;iIndex < iNumChildren ; iIndex++){
                CTabVar* pChildTabVar = pTabVar->GetChild(iIndex);
                CString sChildString,sAltChildString;
                if(m_varNameMap.Lookup(pChildTabVar,sChildString)){
                }
                else {
                    sChildString = pChildTabVar->GetName();
                }
                sAltChildString = sChildString;
                if(LookupName(pTabVar->GetName(),&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet)){
                    if(pDictVSet) {
                        sAltChildString.Replace(pDictVSet->GetName(),pDictItem->GetName());
                    }
                }
                asRowVars.SetSize(sArrRow.GetSize()+1);
                asRowVars[sArrRow.GetSize()].Add(pTabVar->GetName());
                asRowVars[sArrRow.GetSize()].Add(pChildTabVar->GetName());
                sChildString = sParentString + _T("*") + sChildString;
                sAltChildString = sAltParentString + _T("*") + sAltChildString;
                sArrRow.Add(sChildString);
                sAltArrRow.Add(sAltChildString);

                if(pChildTabVar == pRowVar){
                    iSubTableRow = sArrRow.GetSize()-1;
                }
            }
        }
    }
    //Generate Col Strings
    CTabVar* pColRootVar = pRefTable->GetColRoot();
    int iNumColVars = pColRootVar->GetNumChildren();
    if(pColRootVar->GetNumChildren() ==0){
        //This may be a over kill get back to this to decide what to do on the
        //existing units
        //Remove all the Unit specs from the table
        arrTableUnitSpec.RemoveAll();
        return iRet;
    }
    CArray<CStringArray,CStringArray> asColVars;
    for(int iIndex =0;iIndex < iNumColVars ; iIndex++){
        const DictLevel* pDictLevel = NULL;
        const CDictRecord* pDictRecord = NULL;
        const CDictItem* pDictItem = NULL;
        const DictValueSet* pDictVSet = NULL;
        CTabVar* pTabVar = pColRootVar->GetChild(iIndex);
        CString sParentString,sAltParentString;
        if(m_varNameMap.Lookup(pTabVar,sParentString)){
        }
        else {
            sParentString = pTabVar->GetName();
        }
        sAltParentString = sParentString;
        if(pTabVar->GetNumChildren() == 0) {
            sArrCol.Add(sParentString);

            //Check the dict . If VSet only one the add Alt String . If more than one vset
            //add the correct string which is the same as sArrRow stuff
            if(LookupName(pTabVar->GetName(),&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet)){
                if(pDictItem->GetNumValueSets() > 1) {
                    sAltArrCol.Add(sAltParentString);
                }
                else {//if it same as vset name then replace . If it is p03_sex_vs1(1) ??
                    if(pDictVSet) {
                        sAltParentString.Replace(pDictVSet->GetName(),pDictItem->GetName());
                        sAltArrCol.Add(sAltParentString);
                    }
                    else {
                        sAltArrCol.Add(sAltParentString);
                    }
                }
            }
            else {
                sAltArrCol.Add(sAltParentString);
            }
            asColVars.SetSize(sArrCol.GetSize());
            CIMSAString sOcc;
            if(pTabVar->GetOcc() > -1) {
                sOcc.Str(pTabVar->GetOcc());
                sOcc = _T("(") + sOcc+ _T(")");
            }
            asColVars[sArrCol.GetSize()-1].Add(pTabVar->GetName()+sOcc);
            if(pTabVar == pColVar){
                iSubTableCol = sArrCol.GetSize()-1;
            }
        }
        else {
            int iNumChildren = pTabVar->GetNumChildren();
                for(int iIndex =0;iIndex < iNumChildren ; iIndex++){
                    CTabVar* pChildTabVar = pTabVar->GetChild(iIndex);
                    CString sChildString,sAltChildString;
                    if(m_varNameMap.Lookup(pChildTabVar,sChildString)){
                    }
                    else {
                        sChildString = pChildTabVar->GetName();
                    }
                    sAltChildString = sChildString;
                    if(LookupName(pTabVar->GetName(),&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet)){
                        if(pDictVSet) {
                            sAltChildString.Replace(pDictVSet->GetName(),pDictItem->GetName());
                        }
                    }
                    asColVars.SetSize(sArrCol.GetSize()+1);
                    asColVars[sArrCol.GetSize()].Add(pTabVar->GetName());
                    asColVars[sArrCol.GetSize()].Add(pChildTabVar->GetName());
                    sChildString = sParentString + _T("*") + sChildString;
                    sAltChildString = sAltParentString + _T("*") + sAltChildString;
                    sArrCol.Add(sChildString);
                    sAltArrCol.Add(sAltChildString);
                    if(pChildTabVar == pColVar){
                        iSubTableCol = sArrCol.GetSize()-1;
                    }
                }
        }
    }


    CArray<CStringArray,CStringArray> asUnitNames;
    asUnitNames.SetSize(sArrRow.GetSize() * sArrCol.GetSize());

    //Generate Subtable list of Row By Col
    CStringArray asAllVarNames;
    for (int iRow = 0; iRow < sArrRow.GetSize();iRow++){
        for (int iCol = 0; iCol < sArrCol.GetSize();iCol++){
            CString sSubtable,sAltSubtable;
            sSubtable = sArrRow[iRow] + _T(" by ") + sArrCol[iCol];
            sAltSubtable = sAltArrRow[iRow] + _T(" by ") + sAltArrCol[iCol];
            CUnitSpec unitSpec(sSubtable);
            if(sSubtable.CompareNoCase(sAltSubtable) !=0){
                unitSpec.SetAltSubTableString(sAltSubtable);
            }
            arrUnitSpec.Add(unitSpec);
            if(iSubTableCol == iCol && iSubTableRow == iRow){
                iRet = arrUnitSpec.GetSize()-1;
                sCurSelAltSubTable = sAltSubtable;
                sCurSelSubTable = sSubtable;
            }
            CStringArray asVarNames;
            asVarNames.Append(asRowVars[iRow]);
            AppendUnique(asVarNames, asColVars[iCol]);
            ComputeSubtableUnitNames(asUnitNames[arrUnitSpec.GetSize()-1], asVarNames);
            AppendUnique(asAllVarNames, asVarNames);
        }
    }

    // compute units for the entire table
    ComputeSubtableUnitNames(arrValidTableUnits, asAllVarNames);

    // reconcile table units
    if (bReconcile) {
        CUnitSpec& tblUnit = pRefTable->GetTableUnit();
        ASSERT(pReconcileMsg);
        ReconcileUnitLoopingVar(tblUnit, arrValidTableUnits,  pRefTable->GetName(), *pReconcileMsg);
    }

    //Reconcile the subtable list in the unit array
    arrValidSubtableUnits.RemoveAll();
    arrValidSubtableUnits.SetSize(arrTableUnitSpec.GetSize());
    for(int iIndex = arrTableUnitSpec.GetSize() -1; iIndex >=0 ;iIndex--){
        CString sSubTable = arrTableUnitSpec[iIndex].GetSubTableString();
        sSubTable.Trim();
        bool bFound = false;
        for(int iLocal = arrUnitSpec.GetSize()-1 ; iLocal >=0 ;iLocal--){
            CString sLocalSubTable = arrUnitSpec[iLocal].GetSubTableString();
            sLocalSubTable.Trim();
            bFound = (sSubTable.CompareNoCase(sLocalSubTable) ==0);
            if(bFound){
                arrTableUnitSpec[iIndex].SetAltSubTableString(arrUnitSpec[iLocal].GetAltSubTableString());
                const CString& sCurrUnit = arrTableUnitSpec[iIndex].GetLoopingVarName();
                if (arrTableUnitSpec[iIndex].GetUseDefaultLoopingVar() || sCurrUnit.IsEmpty()) {
                    // no unit set - use default
                    arrTableUnitSpec[iIndex].SetLoopingVarName(asUnitNames[iLocal][0]);
                    arrTableUnitSpec[iIndex].SetUseDefaultLoopingVar(true);
                }
                else if (bReconcile) {
                    ASSERT(pReconcileMsg);
                    ReconcileUnitLoopingVar(arrTableUnitSpec[iIndex], asUnitNames[iLocal],
                        pRefTable->GetName() + _T(" (") + sLocalSubTable + _T(")"), *pReconcileMsg);
                }
                arrValidSubtableUnits[iIndex].Copy(asUnitNames[iLocal]);
                arrUnitSpec.RemoveAt(iLocal);
                asUnitNames.RemoveAt(iLocal);
                break;
            }

        }
        if(!bFound){//Do we have to let the user know abt this
            arrTableUnitSpec.RemoveAt(iIndex);
        }
    }
    //Add the remaining unit specs to the Subtable list
    const int iNumExistingSubs = arrValidSubtableUnits.GetSize();
    arrValidSubtableUnits.SetSize( iNumExistingSubs + arrUnitSpec.GetSize());
    for(int iLocal = 0; iLocal < arrUnitSpec.GetSize() ;iLocal++){
        arrTableUnitSpec.Add(arrUnitSpec[iLocal]);
        arrValidSubtableUnits[iNumExistingSubs + iLocal].Copy(asUnitNames[iLocal]);
    }
    if(arrTableUnitSpec.GetSize() ==1){
        //move this to the table level unit and set the unit of the table to blank
        CUnitSpec& tableUnit = pRefTable->GetTableUnit();
        CUnitSpec& singleUnit = arrTableUnitSpec[0];
        if(singleUnit.IsUnitPresent()){
            bool bUseDefaultLoopingVar = singleUnit.GetUseDefaultLoopingVar();
            if(!bUseDefaultLoopingVar){
                CString sLoopingVarName = singleUnit.GetLoopingVarName();
                tableUnit.SetLoopingVarName(sLoopingVarName);
                tableUnit.SetUseDefaultLoopingVar(false);
                singleUnit.SetLoopingVarName(_T(""));
                singleUnit.SetUseDefaultLoopingVar(true);
            }

            CString sSingleUnitValue = singleUnit.GetValue();
            sSingleUnitValue.Trim();
            if(!sSingleUnitValue.IsEmpty()){
                tableUnit.SetValue(sSingleUnitValue);
                singleUnit.SetValue(_T(""));
            }
            CString sWeightExpr = singleUnit.GetWeightExpr();
            sWeightExpr.Trim();
            if(!sWeightExpr.IsEmpty()){
                CString sTableWeight = tableUnit.GetWeightExpr();
                sTableWeight.Trim();
                if(!sTableWeight.IsEmpty()){
                    sWeightExpr = _T("(")+sTableWeight+_T(")") + _T("*") +_T("(")+ sWeightExpr+_T(")");
                    tableUnit.SetWeightExpr(sWeightExpr);
                }
                else{
                    tableUnit.SetWeightExpr(sWeightExpr);
                }
                singleUnit.SetWeightExpr(_T(""));
            }
            CString sSelectExpr = singleUnit.GetUniverse();
            sSelectExpr.Trim();
            if(!sSelectExpr.IsEmpty()){
                CString sTableUniverse = tableUnit.GetUniverse();
                sTableUniverse.Trim();
                if(!sTableUniverse.IsEmpty()){
                    sSelectExpr = sTableUniverse + _T(" AND ") + sSelectExpr;
                    tableUnit.SetUniverse(sSelectExpr);
                }
                else {
                    tableUnit.SetUniverse(sSelectExpr);
                }
                singleUnit.SetUniverse(_T(""));
            }
            //Finally there should be no subtable unit spec associated with this single unit
            ASSERT(!singleUnit.IsUnitPresent());
        }

    }
    else {//Update the iRet after reconcile
        iRet = -1;
        for(int iIndex =0; iIndex < arrTableUnitSpec.GetSize(); iIndex++){
            CUnitSpec& curUnit = arrTableUnitSpec[iIndex];
            CString sCurUnitSubTableString = curUnit.GetSubTableString();
            CString sCurUnitAltSubTableString = curUnit.GetAltSubTableString();
            if(sCurUnitAltSubTableString.IsEmpty()){
                if( (sCurUnitSubTableString.CompareNoCase(sCurSelSubTable) ==0)){
                    iRet = iIndex;
                    break;
                }
            }
            else {
                if( (sCurUnitSubTableString.CompareNoCase(sCurSelSubTable) ==0)&& (sCurUnitAltSubTableString.CompareNoCase(sCurSelAltSubTable) ==0)){
                    iRet = iIndex;
                    break;
                }
            }
        }
    }

    return iRet;
}

/////////////////////////////////////////////////////////////////////////////////
//                      CTabSet::ResetTableUnits
// Reset all units to default, clear all universe and weights.
/////////////////////////////////////////////////////////////////////////////////
void CTabSet::ResetTableUnits(CTable* pRefTable)
{
    UNREFERENCED_PARAMETER(pRefTable);
/*  CArray<CUnitSpec,CUnitSpec&>& arrTableUnitSpec = pRefTable->GetUnitSpecArr();
    for (int i = 0; i < arrTableUnitSpec.GetCount(); ++i) {
        CUnitSpec& unit = arrTableUnitSpec[i];
        unit.SetUseDefaultLoopingVar(true);
        unit.SetUniverse("");
        unit.SetWeightExpr("");
        unit.SetValue("");
    } */
    /*
    CUnitSpec& unit = pRefTable->GetTableUnit();
    unit.SetUseDefaultLoopingVar(true);
    unit.SetUniverse("");
    unit.SetWeightExpr("");
    unit.SetValue("");*/
}


/////////////////////////////////////////////////////////////////////////////////
//                      CTabSet::GetAllRecordRelations
// Find relations for a given record.
/////////////////////////////////////////////////////////////////////////////////
void CTabSet::GetAllRecordRelations(const CDictRecord& dict_record, CArray<const DictRelation*>& aRelations)
{
    //check if these are linked by a relation
    for( const auto& dict_relation : GetDict()->GetRelations() ) {
        if (SO::EqualsNoCase(dict_record.GetName(), dict_relation.GetPrimaryName())) {
            aRelations.Add(&dict_relation);
        }
        else {
            for( const auto& dict_relation_part : dict_relation.GetRelationParts() ) {
                if (SO::EqualsNoCase(dict_record.GetName(), dict_relation_part.GetSecondaryName())) {
                    aRelations.Add(&dict_relation);
                    break;
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//                      CTabSet::GetAllRecordRelations
// Find relations between two records if any exist.
/////////////////////////////////////////////////////////////////////////////////
void CTabSet::GetAllRecordRelations(const CDictRecord& dict_record1, const CDictRecord& dict_record2, CArray<const DictRelation*>& aRelations)
{
    //check if these are linked by a relation
    for( const auto& dict_relation : GetDict()->GetRelations() ) {
        bool bInRelMulti1 = SO::EqualsNoCase(dict_record1.GetName(), dict_relation.GetPrimaryName());
        if(bInRelMulti1 || SO::EqualsNoCase(dict_record2.GetName(), dict_relation.GetPrimaryName())) {
            const CDictRecord& compare_record = bInRelMulti1 ? dict_record2 : dict_record1;
            for( const auto& dict_relation_part : dict_relation.GetRelationParts() ) {
                if(SO::EqualsNoCase(compare_record.GetName(), dict_relation_part.GetSecondaryName())) {
                    aRelations.Add(&dict_relation);
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//                      CTabSet::HasRecordRelation
// Returns true if relation between two records exists.
/////////////////////////////////////////////////////////////////////////////////
bool CTabSet::HasRecordRelation(const CDictRecord& dict_record1, const CDictRecord& dict_record2)
{
    CArray<const DictRelation*> aRelations;
    GetAllRecordRelations(dict_record1, dict_record2, aRelations);
    return !aRelations.IsEmpty();
}

/////////////////////////////////////////////////////////////////////////////////
//                      CTabSet::ComputeSubtableUnitNames
// Compute the list of all possible units for the subtable and store asUnitNames.
// First entry in asUnitNames will be default unit.
/////////////////////////////////////////////////////////////////////////////////
void CTabSet::ComputeSubtableUnitNames(CStringArray& asUnitNames, const CStringArray& asAllVarNames)
{
    const CDataDict* pDict = GetDict();
    const CDataDict* pResDict = GetDict();
    ASSERT(pDict);

    const DictLevel* pDictLevel = NULL;
    const CDictRecord* pDictRecord = NULL;
    const CDictItem* pDictItem = NULL;
    const DictValueSet* pDictVSet = NULL;

    // remove remove names for working dictionary variables
    CStringArray asVarNames;
    int i;
    for (i = 0; i < asAllVarNames.GetCount(); ++i) {
        CString sName  = asAllVarNames[i];
        int iFound = sName.ReverseFind('(');
        if (iFound > 0){//Name has occ included in it
            sName = sName.Left(iFound);
        }
        pResDict = LookupName(sName,&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet);
        ASSERT(pResDict);
        if (pDict == pResDict) {
            asVarNames.Add(asAllVarNames[i]);
        }
    }


    // check for multiply occuring items
    const CDictItem* pFirstMultipleItem = NULL;
    const CDictItem* pFirstMultipleItem4ParticularOcc = NULL;
    CString sTRName4ParticularOcc;
    for (i = 0; i < asVarNames.GetSize(); ++i) {
        CString sName  = asVarNames[i];
        int iFound = sName.ReverseFind('(');
        bool bIsOcc = false;
        if (iFound > 0){//Name has occ included in it
            sName = sName.Left(iFound);
            bIsOcc = true;
        }
        pResDict = LookupName(sName,&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet);
        // GSF 5/30/2008: commented out ASSERT
        // it happens when you paste a table that uses a working storage dict item
        // and there's no WS dict in the target app.  seems harmless in this case
//        ASSERT(pResDict);
        if (pDictItem->GetItemType() == ItemType::Item) {
            // item check for multiple occurences
            if (pDictItem->GetOccurs() > 1) {
                if(bIsOcc){//Particular Occ. Use Record as the default
                    pFirstMultipleItem4ParticularOcc = pDictItem;
                    sTRName4ParticularOcc = pDictRecord->GetName();
                }
                else {
                pFirstMultipleItem = pDictItem;
                }
                //break; //dont break we need to check other item
                continue;
            }
        }
        else {
            // subitem - could be multiply occuring or could have multiply occuring parent
            ASSERT(pDictItem->GetItemType() == ItemType::Subitem);
            ASSERT(pDictItem->GetParentItem() != NULL);
            if (pDictItem->GetOccurs() > 1) {
                // subitem multiply occurs
                ASSERT(pDictItem->GetParentItem()->GetOccurs() == 1);
                if(bIsOcc){//Particular Occ. Use Record as the default
                    pFirstMultipleItem4ParticularOcc = pDictItem;
                    sTRName4ParticularOcc = pDictRecord->GetName();
                }
                else {
                pFirstMultipleItem = pDictItem;
                }
                //break; //dont break we need to check other item
                continue;
            }
            else {
                if (pDictItem->GetParentItem()->GetOccurs() > 1) {
                    // subitem parent multiply occurs
                    if(bIsOcc){//Particular Occ. Use Record as the default
                        pFirstMultipleItem4ParticularOcc = pDictItem->GetParentItem();
                        sTRName4ParticularOcc = pDictRecord->GetName();
                    }
                    else {
                    pFirstMultipleItem = pDictItem->GetParentItem();
                    }
                    //break; //dont break we need to check other item
                    continue;
                }
            }
       }

    }

    if (pFirstMultipleItem || !sTRName4ParticularOcc.IsEmpty()) {
        // found multiply occuring item - check to make sure this is only one
        if(pFirstMultipleItem) {//Exclude case of particular Occ
        for (i = 0; i < asVarNames.GetSize(); ++i) {
                CString sName  = asVarNames[i];
                int iFound = sName.ReverseFind('(');
                if (iFound > 0){//Name has occ included in it
                    sName = sName.Left(iFound);
                }
                pResDict = LookupName(sName,&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet);
            ASSERT(pResDict);
            if (pDictItem != pFirstMultipleItem && pDictItem->GetOccurs() > 1 ||
                (pDictItem->GetItemType() == ItemType::Subitem && pDictItem->GetParentItem()->GetOccurs() > 1 && pDictItem->GetParentItem() != pFirstMultipleItem)) {
                // not possible - return empty list
                asUnitNames.RemoveAll();
                return;
            }
        }
        }
        // only one item - it must be the unit
        if(pFirstMultipleItem){
        asUnitNames.Add(pFirstMultipleItem->GetName());
        }
        else {
            asUnitNames.Add(sTRName4ParticularOcc);
        }
        return;
    }

    // check for multiply occuring records
    const CDictRecord* pFirstMultipleRecord = NULL;
    for (i = 0; i < asVarNames.GetSize(); ++i) {
        CString sName  = asVarNames[i];
        int iFound = sName.ReverseFind('(');
        if (iFound > 0){//Name has occ included in it
            sName = sName.Left(iFound);
        }
        pResDict = LookupName(sName,&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet);
        ASSERT(pResDict);
        if (pDictRecord->GetMaxRecs() > 1) {
            pFirstMultipleRecord = pDictRecord;
            break;
        }
    }

    if (pFirstMultipleRecord) {
        CArray<const DictRelation*> aAllRelations;

        // found multiply occuring record - check to make sure this is only one
        for (i = 0; i < asVarNames.GetSize(); ++i) {
            CString sName  = asVarNames[i];
            int iFound = sName.ReverseFind('(');
            if (iFound > 0){//Name has occ included in it
                sName = sName.Left(iFound);
            }
            pResDict = LookupName(sName,&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet);
            ASSERT(pResDict);
            if (pDictRecord != pFirstMultipleRecord && pDictRecord->GetMaxRecs() > 1) {
                CArray<const DictRelation*> aRelationsThisPair;
                GetAllRecordRelations(*pDictRecord, *pFirstMultipleRecord, aRelationsThisPair);
                if (aAllRelations.IsEmpty()) {
                    aAllRelations.Copy(aRelationsThisPair);
                }
                else {
                    CArray<const DictRelation*> aTemp;
                    IntersectArrays(aTemp, aAllRelations, aRelationsThisPair);
                    aAllRelations.RemoveAll();
                    aAllRelations.Copy(aTemp);
                }
                if (aAllRelations.IsEmpty()) {
                    // not possible - return empty list
                    asUnitNames.RemoveAll();
                    return;
                }
            }
        }

        if (!aAllRelations.IsEmpty()) {

            // can only have relations as units - first one will be default
            for (i = 0; i < aAllRelations.GetSize(); ++i) {
                asUnitNames.Add(aAllRelations[i]->GetName());
            }
        }
        else {
            // default unit is the multiple record
            asUnitNames.Add(pFirstMultipleRecord->GetName());

            // add any multiple items within that record
            for (int iItem = 0; iItem < pFirstMultipleRecord->GetNumItems(); ++iItem) {
                const CDictItem* pItem = pFirstMultipleRecord->GetItem(iItem);
                ASSERT(pItem);
                if (pItem->GetOccurs() > 1) {
                    asUnitNames.Add(pItem->GetName());
                }
            }

            // add in any relations for this record
            CArray<const DictRelation*> aRelations;
            GetAllRecordRelations(*pFirstMultipleRecord, aRelations);
            for (i = 0; i < aRelations.GetSize(); ++i) {
                asUnitNames.Add(aRelations[i]->GetName());
            }
        }

        return;
    }

    // find lowest level of all items
    int iLowestLevel = -1;
    for (i = 0; i < asVarNames.GetSize(); ++i) {
        CString sName  = asVarNames[i];
        int iFound = sName.ReverseFind('(');
        if (iFound > 0){//Name has occ included in it
            sName = sName.Left(iFound);
        }
        pResDict = LookupName(sName,&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet);
        ASSERT(pResDict && pDictLevel);
        iLowestLevel = std::max(iLowestLevel, (int)pDictLevel->GetLevelNumber());
    }
    if(asVarNames.GetSize() ==0){
        //All the variable are from working storage
        iLowestLevel = 0;
    }
    ASSERT(iLowestLevel >= 0 && iLowestLevel <= (int)pDict->GetNumLevels());

    // if there is an item from a non-required record at this level then use that
    // record
    const DictLevel& lowest_dict_level = pDict->GetLevel(iLowestLevel);

    CArray<const CDictRecord*, const CDictRecord*> aNotReqRecrdsAtLowestLvl;
    for (i = 0; i < asVarNames.GetSize(); ++i) {
        CString sName = asVarNames[i];
        int iFound = sName.ReverseFind('(');
        if (iFound > 0){//Name has occ included in it
            sName = sName.Left(iFound);
        }
        pResDict = LookupName(sName,&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet);
        ASSERT(pResDict);
        if (pDictLevel == &lowest_dict_level && !pDictRecord->GetRequired()) {
            if (FindInArray(aNotReqRecrdsAtLowestLvl, pDictRecord) == NONE) {
                aNotReqRecrdsAtLowestLvl.Add(pDictRecord);
            }
        }
    }
    if (aNotReqRecrdsAtLowestLvl.GetSize() > 0) {

        // add all non required records at this level - first one we find is default
        for (int iRcrd = 0; iRcrd < aNotReqRecrdsAtLowestLvl.GetSize(); ++iRcrd) {

            const CDictRecord* pRecord = aNotReqRecrdsAtLowestLvl.GetAt(iRcrd);

            asUnitNames.Add(pRecord->GetName());

            // add any multiple items within that record
            for (int iItem = 0; iItem < pRecord->GetNumItems(); ++iItem) {
                const CDictItem* pItem = pRecord->GetItem(iItem);
                ASSERT(pItem);
                if (pItem->GetOccurs() > 1) {
                    asUnitNames.Add(pItem->GetName());
                }
            }
        }

        // add levels, non required single records, multiple records,  or multiple items
        // starting from lowest level and in for any lower levels
        for (int iLvl = iLowestLevel; iLvl < (int)pDict->GetNumLevels(); ++iLvl) {
            const DictLevel& dict_level = pDict->GetLevel(iLvl);

            // iterate over all records in the level
            for (int iRcrd = 0; iRcrd < dict_level.GetNumRecords(); ++iRcrd) {

                const CDictRecord* pRecord = dict_level.GetRecord(iRcrd);

                // skip the ones we added above
                if (FindInArray(aNotReqRecrdsAtLowestLvl, pRecord) != NONE) {
                    continue;
                }

                // add non-requried single records and multiple records
                if (pRecord->GetMaxRecs() > 1 || !pRecord->GetRequired()) {
                    asUnitNames.Add(pRecord->GetName());
                }

                // add any multiple items from records
                for (int iItem = 0; iItem < pRecord->GetNumItems(); ++iItem) {
                    const CDictItem *pItem = pRecord->GetItem(iItem);

                    if (pItem->GetOccurs() > 1) {
                        asUnitNames.Add(pItem->GetName());
                    }
                } // for iItem
            } // for iRcrd
        } // for ILvl

        return;
    }

    // add levels, non required single records, multiple records,  or multiple items
    // starting from lowest level and in for any lower levels
    for (int iLvl = iLowestLevel; iLvl < (int)pDict->GetNumLevels(); ++iLvl) {

        const DictLevel& dict_level = pDict->GetLevel(iLvl);

        // add lowest level to list
        asUnitNames.Add(dict_level.GetName());

        // iterate over all records in the level
        for (int iRcrd = 0; iRcrd < dict_level.GetNumRecords(); ++iRcrd) {

            const CDictRecord* pRecord = dict_level.GetRecord(iRcrd);

            // add non-requried single records and multiple records
            if (pRecord->GetMaxRecs() > 1 || !pRecord->GetRequired()) {
                asUnitNames.Add(pRecord->GetName());
            }

            // add any multiple items from records
            for (int iItem = 0; iItem < pRecord->GetNumItems(); ++iItem) {
                const CDictItem *pItem = pRecord->GetItem(iItem);

                if (pItem->GetOccurs() > 1) {
                    asUnitNames.Add(pItem->GetName());
                }
            } // for iItem
        } // for iRcrd
    } // for ILvl
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabSet::ReconcileLevels4Tbl(CTable* pTable = NULL)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabSet::ReconcileLevels4Tbl(CTable* pTable /*= NULL*/)
{
    if(pTable){
        //Get the table level
        //Get the level to which this table belongs
        int iCurLevel = -1;
        int iCurTblIndex =  -1;
        CTabLevel* pLevel = NULL;
        for(int iLevel = 0; iLevel < this->GetNumLevels(); iLevel++){
            pLevel = m_aTabLevel[iLevel];
            for(int iTable = 0; iTable < pLevel->GetNumTables(); iTable++){
                if(pLevel->GetTable(iTable)== pTable){
                    iCurLevel  = iLevel;
                    iCurTblIndex = iTable;
                    break;
                }
            }
            if(iCurLevel != -1)
                break;
        }

        int iTblLevel = GetTableLevelFromUnits(pTable);
        //if same done . if not move the table from this level
        if(iTblLevel != iCurLevel){
            pLevel->DeleteTable(iCurTblIndex);
            m_aTabLevel[iTblLevel]->AddTable(pTable);
        }
    }
    else { //For all tables
        for(int iPrintTable =0; iPrintTable < m_aPrintTable.GetSize(); iPrintTable++){
            pTable = m_aPrintTable[iPrintTable];
            if(pTable){
                //Get the table level
                //Get the level to which this table belongs
                int iCurLevel = -1;
                int iCurTblIndex =  -1;
                CTabLevel* pLevel = NULL;
                for(int iLevel = 0; iLevel < this->GetNumLevels(); iLevel++){
                    pLevel = m_aTabLevel[iLevel];
                    for(int iTable = 0; iTable < pLevel->GetNumTables(); iTable++){
                        if(pLevel->GetTable(iTable)== pTable){
                            iCurLevel  = iLevel;
                            iCurTblIndex = iTable;
                            break;
                        }
                    }
                    if(iCurLevel != -1)
                        break;
                }

                int iTblLevel = GetTableLevelFromUnits(pTable);
                //if same done . if not move the table from this level
                if(iTblLevel != iCurLevel){
                    pLevel->DeleteTable(iCurTblIndex);
                    m_aTabLevel[iTblLevel]->AddTable(pTable);
                }
            }
        }
        //Get the table level
        //Get the level to which this table belongs
        //if same done . if not move the table from this level to the other one
    }
}

bool CTabSet::ConsistencyCheckSubTblNTblLevel(CString& sMsg,bool bSilent /*=false*/)
{
    bool bRet  = true;
    //For each table
    for(int iTbl =0; iTbl < GetNumTables(); iTbl++){
        CString sTblMsg;
        CTable* pTbl = GetTable(iTbl);
        CArray<CStringArray,CStringArray&> arrValidSubtableUnits;
        CStringArray arrValidTableUnits;
        UpdateSubtableList(pTbl, arrValidSubtableUnits, arrValidTableUnits);
        int iTblLevel = GetTableLevelFromUnits(pTbl);
        {
            //Get array of units for  the table
            CArray<CUnitSpec,CUnitSpec&>& arrUnitSpec = pTbl->GetUnitSpecArr();
            if(arrUnitSpec.GetCount() <= 1){//if only one subtable
                CString sUnitName ;
                sUnitName = pTbl->GetTableUnit().GetLoopingVarName();
                if(sUnitName.IsEmpty() && pTbl->GetTableUnit().GetUseDefaultLoopingVar() && arrUnitSpec.GetCount() ==1 && arrValidTableUnits.GetCount() > 0){
                    sUnitName = arrValidTableUnits[0];
                }
                if(!sUnitName.IsEmpty()){
                    int iLevel =-1;
                    int iRecord = -1;
                    int iItem = -1;
                    int iVSet =-1;
                    BOOL bFound = m_pDataDict->LookupName(sUnitName,&iLevel,&iRecord,&iItem,&iVSet);
                    int iSubTblLevelFromVars = GetSubTblLevelFromVars(pTbl,arrUnitSpec[0]);
                    if(iSubTblLevelFromVars > iLevel){
                        iLevel = iSubTblLevelFromVars;
                    }
                    if(bFound && iTblLevel != iLevel){//TableLevel and SubTable unit level do not match
                        sTblMsg += _T("Subtable ") + arrUnitSpec[0].GetAltSubTableString() + _T(" \n");
                        bRet = false;
                    }

                }
            }
            else {//if more than one subtable
                //See which level they come from
                for(int iIndex =0; iIndex < arrUnitSpec.GetSize(); iIndex++){
                    CString sUnitName ;
                    if(arrUnitSpec.GetCount() > 1){ //if more than one subtable
                        sUnitName = arrUnitSpec[iIndex].GetLoopingVarName();
                        if(sUnitName.IsEmpty() &&  arrUnitSpec[iIndex].GetUseDefaultLoopingVar() && arrValidSubtableUnits.GetCount() >= iIndex){
                            CStringArray& arrUnitNames = arrValidSubtableUnits[iIndex];
                            if(arrUnitNames.GetCount() > 1){
                                sUnitName =arrUnitNames[0];
                            }
                        }
                    }
                    if(!sUnitName.IsEmpty()){
                        int iLevel =-1;
                        int iRecord = -1;
                        int iItem = -1;
                        int iVSet =-1;
                        BOOL bFound = m_pDataDict->LookupName(sUnitName,&iLevel,&iRecord,&iItem,&iVSet);
                        int iSubTblLevelFromVars = GetSubTblLevelFromVars(pTbl,arrUnitSpec[iIndex]);
                        if(iSubTblLevelFromVars > iLevel){
                            iLevel = iSubTblLevelFromVars;
                        }
                        if(bFound && iTblLevel != iLevel){//TableLevel and SubTable unit level do not match
                            sTblMsg += _T("Subtable ") + arrUnitSpec[iIndex].GetAltSubTableString() + _T("\n");
                            bRet = false;
                        }

                    }
                }
            }
        }
        if(!sTblMsg.IsEmpty()){
            sTblMsg = _T("\n") + pTbl->GetName() + _T(":\n") + sTblMsg;
            sMsg += sTblMsg;
        }
    }
    if(!bSilent && !sMsg.IsEmpty()){
        CString sWarning = _T("The following table(s) have subtable(s) with units coming from different levels in the dictionary.\n");
        sWarning += _T("Results will not be the same as if the subtable were run as a separate table.\n");
        sMsg = sWarning + sMsg;
        AfxMessageBox(sMsg);
    }

    return bRet;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  int CTabSet::GetTableLevelFromUnits(CTable* pTable)
//
/////////////////////////////////////////////////////////////////////////////////
int CTabSet::GetTableLevelFromUnits(CTable* pTable)
{
    int iCurLevel = 0;
    //Get array of units for  the table
    CArray<CStringArray,CStringArray&> arrValidSubtableUnits;
    CStringArray arrValidTableUnits;
    UpdateSubtableList(pTable, arrValidSubtableUnits, arrValidTableUnits);

    CArray<CUnitSpec,CUnitSpec&>& arrUnitSpec = pTable->GetUnitSpecArr();

    if(arrUnitSpec.GetCount() <= 1){//if only one subtable
        CString sUnitName ;
        sUnitName = pTable->GetTableUnit().GetLoopingVarName();
        if(sUnitName.IsEmpty() && pTable->GetTableUnit().GetUseDefaultLoopingVar() && arrUnitSpec.GetCount() ==1 && arrValidTableUnits.GetCount() > 0){
            sUnitName = arrValidTableUnits[0];
        }
        if(!sUnitName.IsEmpty()){
            int iLevel =-1;
            int iRecord = -1;
            int iItem = -1;
            int iVSet =-1;
            BOOL bFound = m_pDataDict->LookupName(sUnitName,&iLevel,&iRecord,&iItem,&iVSet);
            if(bFound && iCurLevel < iLevel){
                iCurLevel = iLevel;
            }

        }
    }
    else {//if more than one subtable
        //See which level they come from
        for(int iIndex =0; iIndex < arrUnitSpec.GetSize(); iIndex++){
            CString sUnitName ;
             if(arrUnitSpec.GetCount() > 1){ //if more than one subtable
                sUnitName = arrUnitSpec[iIndex].GetLoopingVarName();
                if(sUnitName.IsEmpty() &&  arrUnitSpec[iIndex].GetUseDefaultLoopingVar() && arrValidSubtableUnits.GetCount() >= iIndex){
                    CStringArray& arrUnitNames = arrValidSubtableUnits[iIndex];
                    if(arrUnitNames.GetCount() > 1){
                        sUnitName =arrUnitNames[0];
                    }
                }
            }
            if(!sUnitName.IsEmpty()){
                int iLevel =-1;
                int iRecord = -1;
                int iItem = -1;
                int iVSet =-1;
                BOOL bFound = m_pDataDict->LookupName(sUnitName,&iLevel,&iRecord,&iItem,&iVSet);
                if(bFound && iCurLevel < iLevel){
                    iCurLevel = iLevel;
                }

            }
        }
    }
    int iTblLevelFromVariables = GetTableLevel(pTable);
    if(iTblLevelFromVariables > iCurLevel){
        iCurLevel = iTblLevelFromVariables;
    }
    //get the lowest level

    return iCurLevel;
}
/////////////////////////////////////////////////////////////////////////////////
// Get Table Level from variables
//  int CTabSet::GetTableLevel(CTable* pTable)
//
/////////////////////////////////////////////////////////////////////////////////
int CTabSet::GetTableLevel(CTable* pTable)
{
    int iCurLevel = 0;
    CTabVar* pTabVar = pTable->GetRowRoot();
    for(int iRowVar =0; iRowVar < pTabVar->GetNumChildren(); iRowVar++){
        CTabVar* pChildVar = pTabVar->GetChild(iRowVar);
        GetTableLevel(pChildVar,iCurLevel);
        for(int iChild=0; iChild < pChildVar->GetNumChildren();iChild++){
            GetTableLevel(pChildVar->GetChild(iChild),iCurLevel);
        }
    }
    pTabVar = pTable->GetColRoot();
    for(int iColVar =0; iColVar < pTabVar->GetNumChildren(); iColVar++){
        CTabVar* pChildVar = pTabVar->GetChild(iColVar);
        GetTableLevel(pChildVar,iCurLevel);
        for(int iChild=0; iChild < pChildVar->GetNumChildren();iChild++){
            GetTableLevel(pChildVar->GetChild(iChild),iCurLevel);
        }
    }

    return iCurLevel;
}
/////////////////////////////////////////////////////////////////////////////////
// Get Table Level from variables
//  void CTabSet::GetTableLevel(CTabVar*pTabVar, int& iCurLevel)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabSet::GetTableLevel(CTabVar*pTabVar, int& iCurLevel)
{
    //Find the level from which this var comes from
    //if the level is greater than iCurLevel then adjust iCurLevel
    int iLevel = -1;
    int iRecord = -1;
    int iItem = -1;
    int iVSet = -1;
    LookupName(pTabVar->GetName(),&iLevel,&iRecord,&iItem,&iVSet);
    if(iLevel > iCurLevel) {
        iCurLevel = iLevel;
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabSet::SaveTBWBegin(CSpecFile& specFile, const CString& sDictFilePath)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabSet::SaveTBWBegin(CSpecFile& specFile, const CString& sDictFilePath)
{
    // Save header
    specFile.PutLine(XTS_SECT_TABSPEC);
    specFile.PutLine(CMD_VERSION, CSPRO_VERSION);
    CString sSpecFilePath = specFile.GetFileName();
    if (m_sLabel.IsEmpty()) {
        m_sLabel = GetFileName(sSpecFilePath);
        int pos = m_sLabel.ReverseFind(DOT);
        if (pos > 0) {
            m_sLabel = m_sLabel.Left(pos);
        }
    }
    if (m_sName.IsEmpty()) {
        m_sName = CIMSAString::MakeName(m_sLabel);
    }
    specFile.PutLine(XTS_CMD_LABEL, TableLabelSerializer::Create(m_sLabel));
    specFile.PutLine(XTS_CMD_NAME, m_sName);

    if(!m_bGenLogic){
        specFile.PutLine(XTS_ARG_GENLOGIC ,XTS_ARG_NO);
    }
    else {
        specFile.PutLine(XTS_ARG_GENLOGIC ,XTS_ARG_YES);
    }

    if(m_eSpecType == FREQ_SPEC) {
        specFile.PutLine(XTS_CMD_SPECTYPE, _T("Freq"));
    }

    // Save dictionary name
    if( !sDictFilePath.IsEmpty() ) {
        CString sFile = GetRelativeFName<CString>(sSpecFilePath, sDictFilePath);
        specFile.PutLine (_T(" "));
        specFile.PutLine (XTS_SECT_DICTS);
        specFile.PutLine (XTS_CMD_FILE, sFile);
    }

    // Save area specifications
    if (m_pConsolidate->GetNumAreas() > 0) {
        m_pConsolidate->Save(specFile);
    }

    // save format registry
    m_fmtReg.Save(specFile);

    return true;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabSet::SaveTBWEnd(CSpecFile& specFile)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabSet::SaveTBWEnd(CSpecFile& specFile)
{
    // Save level structure
    for (int l = 0 ; l < m_aTabLevel.GetSize() ; l++) {
        specFile.PutLine (_T(" "));
        specFile.PutLine (XTS_SECT_LEVEL);
        CTabLevel* pTabLevel = m_aTabLevel[l];
        for (int i = 0 ; i < pTabLevel->GetNumTables() ; i ++) {
            CTable* pTable  = pTabLevel->GetTable(i);
            if(pTable->m_bDoSaveinTBW){
                specFile.PutLine(XTS_CMD_TABLE, pTable->GetName());
            }
        }
    }

    return true;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabSet::SaveTBWTables(CSpecFile& specFile)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabSet::SaveTBWTables(CSpecFile& specFile)
{
// Save each table
    for (int i = 0 ; i < m_aPrintTable.GetSize() ; i++) {
        if(m_eSpecType == FREQ_SPEC){
            m_aPrintTable[i]->m_bSaveFreqStats = true;
        }
        else {
            m_aPrintTable[i]->m_bSaveFreqStats = false;
        }
        if(m_aPrintTable[i]->m_bDoSaveinTBW){
            m_aPrintTable[i]->Save(specFile);
        }
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CString CTable::GetTitleText()
//
/////////////////////////////////////////////////////////////////////////////////
const CString& CTable::GetTitleText()
{
    CFmt* pFmt = GetTitle()->GetDerFmt();
    if (pFmt) {
        ASSERT_VALID(pFmt);
        if (pFmt->IsTextCustom()) {
            return pFmt->GetCustomText();
        }
    }

    return GetTitle()->GetText();
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CTable::OnTallyAttributesChange
//
/////////////////////////////////////////////////////////////////////////////////
void CTable::OnTallyAttributesChange(CTabVar* pTabVar,
                                     const CDWordArray& aMapStatNewToOldPos,
                                     const CDataDict* pDict,
                                     const CDataDict* pWorkDict)
{
    // for each stat
    // if it was in the old list - renumber the tab vals and reconcile
    // otherwise add new tab vals with new numbering

    if(pDict){
        //do a lookup
        //get the tabvals for this Vset
        CTabSetFmt* pTabSetFmt = DYNAMIC_DOWNCAST(CTabSetFmt,m_pFmtReg->GetFmt(FMT_ID_TABSET));
        ASSERT(pTabSetFmt);
        const FOREIGN_KEYS& foreignKeys = pTabSetFmt->GetAltForeignKeys();

        const CDictItem* pDictItem;
        const DictValueSet* pDictVSet;
        BOOL bFound = TRUE;
        bFound = pDict->LookupName(pTabVar->GetName(), nullptr, nullptr, &pDictItem, &pDictVSet);
        if(!bFound && pWorkDict){
            bFound = pWorkDict->LookupName(pTabVar->GetName(), nullptr, nullptr, &pDictItem, &pDictVSet);
        }
        CArray<CTabValue*,CTabValue*>& aTabVals = pTabVar->GetArrTabVals();

        // copy off old tab vals
        CArray<CTabValue*,CTabValue*> aOldTabVals;
        aOldTabVals.Copy(aTabVals);
        aTabVals.RemoveAll();


        CTallyFmt* pVarTallyFmt = pTabVar->GetTallyFmt();
        ASSERT(m_pFmtReg);
        FMT_ID eFmtID = FMT_ID_INVALID;
        pVarTallyFmt ? eFmtID= pVarTallyFmt->GetID() : eFmtID = (IsRowVar(pTabVar) ? FMT_ID_TALLY_ROW: FMT_ID_TALLY_COL);
        ASSERT(eFmtID != FMT_ID_INVALID && eFmtID != FMT_ID_TALLY);
        CTallyFmt* pTableTallyFmt = DYNAMIC_DOWNCAST(CTallyFmt,m_pFmtReg->GetFmt(eFmtID));
        if(!pVarTallyFmt){
            pVarTallyFmt = pTableTallyFmt;
        }

        // reconcile each stat with tab vals for that stat
        const CArray<CTallyVarStatFmt*>& aStats = pVarTallyFmt->GetStats();
        for (int iStat = 0; iStat < aStats.GetSize(); ++iStat) {

            CTallyVarStatFmt* pStat = aStats.GetAt(iStat);

            if (iStat < aMapStatNewToOldPos.GetSize() && aMapStatNewToOldPos[iStat] != -1) {
                // this stat existed previously

                // Get the tab vals for this stat
                CArray<CTabValue*,CTabValue*> aStatOldTabVals;
                for (int iVal = 0; iVal < aOldTabVals.GetSize(); ++iVal) {
                    if (aOldTabVals.GetAt(iVal)->GetStatId() == (int) aMapStatNewToOldPos[iStat]) {
                        aStatOldTabVals.Add(aOldTabVals.GetAt(iVal));
                        aOldTabVals.RemoveAt(iVal);
                        --iVal;
                    }
                }

                ASSERT(aStatOldTabVals.GetSize() > 0);
                pStat->ReconcileTabVals(aTabVals, iStat, aStatOldTabVals, pDictVSet, &GetCustSpecialValSettings(), aStats, &foreignKeys);

                // clean up any unused
                for (int iVal = 0; iVal < aStatOldTabVals.GetSize(); ++iVal) {
                    SAFE_DELETE(aStatOldTabVals[iVal]);
                }
                aStatOldTabVals.RemoveAll();
            }
            else {
                // this is a new stat - create new tab vals for it
                pStat->GetTabVals(aTabVals, iStat, pDictVSet, &GetCustSpecialValSettings(), aStats, &foreignKeys);
            }
        }

        // interleave percents w. counts if there are any
        InterleavePercents(pTabVar);

        // set pct row/col # num places to one
        SetCellDecimalPlaces(pTabVar);

        // add in reader breaks if any
        AddReaderBreakTabVals(pTabVar);

        // cleanup leftovers
        for (int iVal = 0; iVal < aOldTabVals.GetSize(); ++iVal) {
            SAFE_DELETE(aOldTabVals[iVal]);
        }

    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CTable::InterleavePercents
//
// Interleave counts and percents in the tab vals for this variable if needed.
/////////////////////////////////////////////////////////////////////////////////
void CTable::InterleavePercents(CTabVar* pTabVar)
{
    CArray<CTabValue*,CTabValue*>& aTabVals = pTabVar->GetArrTabVals();

    CTallyFmt* pVarTallyFmt = pTabVar->GetTallyFmt();
    ASSERT(m_pFmtReg);
    FMT_ID eFmtID = FMT_ID_INVALID;
    pVarTallyFmt ? eFmtID= pVarTallyFmt->GetID() : eFmtID = (IsRowVar(pTabVar) ? FMT_ID_TALLY_ROW: FMT_ID_TALLY_COL);
    ASSERT(eFmtID != FMT_ID_INVALID && eFmtID != FMT_ID_TALLY);
    CTallyFmt* pTableTallyFmt = DYNAMIC_DOWNCAST(CTallyFmt,m_pFmtReg->GetFmt(eFmtID));
    if(!pVarTallyFmt){
        pVarTallyFmt = pTableTallyFmt;
    }

    CArray<CTallyFmt::InterleavedStatPair> aInterleavedStats;
    pVarTallyFmt->GetInterleavedStats(aInterleavedStats);
    for (int iPair = 0; iPair < aInterleavedStats.GetCount(); ++ iPair) {
        const CTallyFmt::InterleavedStatPair& p = aInterleavedStats.GetAt(iPair);
        InterleaveStatTabVals(aTabVals, p.m_first, p.m_second);
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CTable::InterleaveStatTabVals
//
// Interleave tab vals for one stat with tab vals for another stat (e.g. counts & pcts)
// Stat1 and Stat2 are the stat id's of the tab vals to interleave
// Puts first val of stat1 followed by first val of stat2 the second of stat1,
// second of stat2, etc....
/////////////////////////////////////////////////////////////////////////////////
void CTable::InterleaveStatTabVals(CArray<CTabValue*,CTabValue*>& aTabVals, int iStat1, int iStat2)
{
    // collect all the stat2 vals
    CArray<CTabValue*,CTabValue*> aStat2s;
    for (int iVal = 0; iVal < aTabVals.GetSize(); ++iVal) {
        if (aTabVals.GetAt(iVal)->GetStatId() == iStat2) {
            aStat2s.Add(aTabVals.GetAt(iVal));
            aTabVals.RemoveAt(iVal);
            --iVal;
        }
    }

    // stick them one by one in after the stat1s
    int iCurrStat2 = 0;
    for (int iVal = 0; iVal < aTabVals.GetSize(); ++iVal) {
        if (aTabVals.GetAt(iVal)->GetStatId() == iStat1) {
            ASSERT(iCurrStat2 < aStat2s.GetCount());
            aTabVals.InsertAt(++iVal, aStat2s[iCurrStat2++]);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTable::ReconcileTabVals(CTabVar* pTabVar, const CDataDict* pDict, const CDataDict* pWorkDict)
//
/////////////////////////////////////////////////////////////////////////////////
void CTable::ReconcileTabVals(CTabVar* pTabVar, const CDataDict* pDict, const CDataDict* pWorkDict)
{
    CTabSetFmt* pTabSetFmt = DYNAMIC_DOWNCAST(CTabSetFmt,m_pFmtReg->GetFmt(FMT_ID_TABSET));
    ASSERT(pTabSetFmt);
    const FOREIGN_KEYS& foreignKeys = pTabSetFmt->GetAltForeignKeys();

    int iNumChildren = pTabVar->GetNumChildren();
    for(int iIndex =0; iIndex < iNumChildren;iIndex++){
        CTabVar* pChildVar = pTabVar->GetChild(iIndex);
        //Now do a lookup and fix the "actual" tabvals
        if(pDict){
            //do a lookup
            //get the tabvals for this Vset
            //for each tabval in the vset . sync the label in the tabvals
            //add if tabvals are missing
            const CDictItem* pDictItem;
            const DictValueSet* pDictVSet;
            BOOL bFound = TRUE;
            bFound = pDict->LookupName(pChildVar->GetName(), nullptr, nullptr, &pDictItem, &pDictVSet);
            if(!bFound && pWorkDict){
                bFound = pWorkDict->LookupName(pChildVar->GetName(), nullptr, nullptr, &pDictItem, &pDictVSet);
            }
            CArray<CTabValue*,CTabValue*>& aTabVals = pChildVar->GetArrTabVals();

            // copy off old tab vals
            CArray<CTabValue*,CTabValue*> aOldTabVals;
            aOldTabVals.Copy(aTabVals);
            aTabVals.RemoveAll();


            CTallyFmt* pVarTallyFmt = pChildVar->GetTallyFmt();
            ASSERT(m_pFmtReg);
            FMT_ID eFmtID = FMT_ID_INVALID;
            pVarTallyFmt ? eFmtID= pVarTallyFmt->GetID() : eFmtID = (IsRowVar(pChildVar) ? FMT_ID_TALLY_ROW: FMT_ID_TALLY_COL);
            ASSERT(eFmtID != FMT_ID_INVALID && eFmtID != FMT_ID_TALLY);
            CTallyFmt* pTableTallyFmt = DYNAMIC_DOWNCAST(CTallyFmt,m_pFmtReg->GetFmt(eFmtID));
            if(!pVarTallyFmt){
                pVarTallyFmt = pTableTallyFmt;
            }

            // reconcile each stat with tab vals for that stat
            bool bChanged = false;
            bool bAddMsg = false;
            const CArray<CTallyVarStatFmt*>& aStats = pVarTallyFmt->GetStats();
            for (int iStat = 0; iStat < aStats.GetSize(); ++iStat) {

                CTallyVarStatFmt* pStat = aStats.GetAt(iStat);

                // Get the tab vals for this stat
                CArray<CTabValue*,CTabValue*> aStatOldTabVals;
                int iVal;
                for (iVal = 0; iVal < aOldTabVals.GetSize(); ++iVal) {
                    //ASSERT(aOldTabVals.GetAt(iVal)->GetStatId() != -1);
                    if (aOldTabVals.GetAt(iVal)->GetStatId() == iStat) {
                        aStatOldTabVals.Add(aOldTabVals.GetAt(iVal));
                        aOldTabVals.RemoveAt(iVal);
                        --iVal;
                    }
                }

                if (aStatOldTabVals.GetSize() > 0) {
                    // this stat existed before - do a reconcile
                    if (pStat->ReconcileTabVals(aTabVals, iStat, aStatOldTabVals, pDictVSet, &GetCustSpecialValSettings(), aStats, &foreignKeys)) {
                        bChanged = true;
                        if (!_tcscmp(pStat->GetType(), _T("Counts"))) {
                            bAddMsg = true;
                        }
                    }

                    // clean up any unused
                    for (iVal = 0; iVal < aStatOldTabVals.GetSize(); ++iVal) {
                        SAFE_DELETE(aStatOldTabVals[iVal]);
                    }
                    aStatOldTabVals.RemoveAll();
                }
                else {
                    // this is a new stat - create new tab vals for it
                    pStat->GetTabVals(aTabVals, iStat, pDictVSet, &GetCustSpecialValSettings(), aStats, &foreignKeys);
                }
            }

            // interleave percents w. counts if there are any
            InterleavePercents(pChildVar);

            // set pct row/col # num places to one
            SetCellDecimalPlaces(pChildVar);

            // add reader breaks
            ReconcileReaderBreakTabVals(pChildVar, aOldTabVals);

            // cleanup leftovers
            for (int iVal = 0; iVal < aOldTabVals.GetSize(); ++iVal) {
                SAFE_DELETE(aOldTabVals[iVal]);
            }

            if (bChanged) {
                SetDirty();
            }

            if (bAddMsg) {
                CString sMsg = pChildVar->GetName() + _T(" values changed; vset changed in dictionary");
                m_sErrMsg += _T("  ") + sMsg + _T("\r\n");  // BMD 25 May 2006
            }
        }

        if(pChildVar->GetNumChildren() > 0){
            ReconcileTabVals(pChildVar,pDict,pWorkDict);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CTable::IsParentRowRoot
//
// Determines if given tab var is on row or column.
/////////////////////////////////////////////////////////////////////////////////
bool CTable::IsParentRowRoot(CTabVar* pTabVar)
{
    CTabVar* pTemp = pTabVar;
    while(pTemp){
        if(pTemp->GetParent() == GetRowRoot()){
            return true;
        }
        pTemp = pTemp->GetParent();
    }

    return false;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CTable::SetCellDecimalPlaces
//
// Sets correct default dec places for rows and columns based on the statistic
// they are tied to i.e. pcts have one dec place by default.
/////////////////////////////////////////////////////////////////////////////////
void CTable::SetCellDecimalPlaces(CTabVar* pTabVar)
{
    if (pTabVar->GetNumChildren() != 0) {
        // if tab var has children, must be spanner or caption, don't want to mess w.
        // decimal places in that case
        return;
    }

    bool bIsParentRowRoot = IsParentRowRoot(pTabVar);

    CArray<CTabValue*,CTabValue*>& aTabVals = pTabVar->GetArrTabVals();

    CTallyFmt* pVarTallyFmt = pTabVar->GetTallyFmt();
    ASSERT(m_pFmtReg);
    FMT_ID eFmtID = FMT_ID_INVALID;
    pVarTallyFmt ? eFmtID= pVarTallyFmt->GetID() : eFmtID = (IsRowVar(pTabVar) ? FMT_ID_TALLY_ROW: FMT_ID_TALLY_COL);
    ASSERT(eFmtID != FMT_ID_INVALID && eFmtID != FMT_ID_TALLY);
    CTallyFmt* pTableTallyFmt = DYNAMIC_DOWNCAST(CTallyFmt,m_pFmtReg->GetFmt(eFmtID));
    if(!pVarTallyFmt){
        pVarTallyFmt = pTableTallyFmt;
    }

    eFmtID = FMT_ID_INVALID;
    bIsParentRowRoot ? eFmtID = FMT_ID_STUB : eFmtID = FMT_ID_COLHEAD;
    CDataCellFmt *pDefDataCellFmt = DYNAMIC_DOWNCAST(CDataCellFmt,m_pFmtReg->GetFmt(eFmtID));

    for (int iTabVal = 0; iTabVal < aTabVals.GetCount(); ++iTabVal) {
        CTabValue* pTabVal = aTabVals.GetAt(iTabVal);

        if (pTabVal->GetStatId() != NONE) {
            CTallyVarStatFmt* pStat = pVarTallyFmt->GetStats().GetAt(pTabVal->GetStatId());
            NUM_DECIMALS eStatNumDec = pStat->GetDefaultNumDecimals(pTabVal);
            CDataCellFmt* pDataCellFmt = pTabVal->GetDerFmt();
            if (pDataCellFmt) {
                // fmt exists, set num dec if not already in agreement
                if (pDataCellFmt->GetNumDecimals() == NUM_DECIMALS_DEFAULT && eStatNumDec != NUM_DECIMALS_DEFAULT) {
                    pDataCellFmt->SetNumDecimals(eStatNumDec);
                }
            }
            else {
                // no fmt, see if we need to create one (only if stat want's diff number of decimals from default)
                if (pDefDataCellFmt->GetNumDecimals() != eStatNumDec && eStatNumDec != NUM_DECIMALS_DEFAULT) {
                    CDataCellFmt* pDataCellFmt = new CDataCellFmt;
                    pDataCellFmt->Init();
                    pDataCellFmt->SetID(pDefDataCellFmt->GetID());
                    pDataCellFmt->SetIndex(m_pFmtReg->GetNextCustomFmtIndex(*pDefDataCellFmt));
                    pDataCellFmt->SetUnits(pDefDataCellFmt->GetUnits());
                    pDataCellFmt->SetNumDecimals(eStatNumDec);
                    pTabVal->SetFmt(pDataCellFmt);
                    m_pFmtReg->AddFmt(pDataCellFmt);
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CString CTable::FormatDataCell(const double& dData, CTabSetFmt* pTabSetFmt,CDataCellFmt* pDataCellFmt)
//
/////////////////////////////////////////////////////////////////////////////////
csprochar pszTemp[128];
CString CTable::FormatDataCell(const double& dData, CTabSetFmt* pTabSetFmt,CDataCellFmt* pDataCellFmt)
{
    if(pDataCellFmt->IsTextCustom()){
        return pDataCellFmt->GetCustom().m_sCustomText;
    }
    // Get decimal part
    int iNumDecimals = 0;
    switch(pDataCellFmt->GetNumDecimals()){
    case NUM_DECIMALS_NOTAPPL:
    case NUM_DECIMALS_DEFAULT:
        ASSERT(FALSE);
        break; //Can't be default
    case NUM_DECIMALS_ZERO:
        iNumDecimals=0;
        break;
    case NUM_DECIMALS_ONE:
        iNumDecimals=1;
        break;
    case NUM_DECIMALS_TWO:
        iNumDecimals=2;
        break;
    case NUM_DECIMALS_THREE:
        iNumDecimals=3;
        break;
    case NUM_DECIMALS_FOUR:
        iNumDecimals=4;
        break;
    case NUM_DECIMALS_FIVE:
        iNumDecimals=5;
        break;
    default:
        ASSERT(FALSE);
    }
    ASSERT(pDataCellFmt);
    ASSERT(pTabSetFmt);

    CString sRet;

    // Handle zero value
    if (dData == 0 && pTabSetFmt->GetZeroMask().GetLength() > 0) {
        sRet = pTabSetFmt->GetZeroMask();
        return sRet;
    }

    // Break up value into integer part and decimal part
    double dDec, dInt;
    __int64 kDec, kInt;

    dDec = modf(dData, &dInt);
    int k = 1;
    for (UINT i = 0 ; i < (UINT)iNumDecimals ; i++) {
        k *= 10;
    }
    if (dData < 0) {
        kInt = (__int64) -dInt;
        kDec = (__int64) (-dDec * k + 0.5);
    }
    else {
        kInt = (__int64) dInt;
        kDec = (__int64) (dDec * k + 0.5);
    }
    if (kDec >= k) {
        kInt++;
    }

    // Handle rounded to zero value
    if (dData != 0 && kInt == 0 && kDec == 0 && pTabSetFmt->GetZRoundMask().GetLength() > 0) {
        sRet = pTabSetFmt->GetZRoundMask();
        return sRet;
    }

    // If negative output hyphen
    //memset(pszTemp,_T('\0'),128);
    memset(pszTemp,_T('\0'),128 * sizeof(csprochar)); // GHM 20111213 for unicode
    csprochar* pszStart = sRet.GetBuffer(128);
    if (dData < 0) {
        *pszStart = HYPHEN;
        pszStart++;
    }

    // Get integer part
    if (kInt > 0 || iNumDecimals == 0 || pTabSetFmt->GetZeroBeforeDecimal()) {
        i64toa(kInt, pszTemp);
        // Format integer part
        int l = _tcslen(pszTemp);
        if (pTabSetFmt->GetDigitGrouping() == DIGIT_GROUPING_NONE || l <= 3) {
            //memmove(pszStart, pszTemp, l);
            memmove(pszStart, pszTemp, l * sizeof(csprochar)); // GHM 20111213 for unicode
            pszStart += l;
        }
        else {
            CString sThSep = pTabSetFmt->GetThousandsSep();
            csprochar* pszGroup = pszTemp;
            if (pTabSetFmt->GetDigitGrouping() == DIGIT_GROUPING_THOUSANDS) {
                int m = l % 3;
                if (m > 0) {
                    //memmove(pszStart, pszGroup, m);
                    memmove(pszStart, pszGroup, m * sizeof(csprochar)); // GHM 20111213 for unicode
                    pszStart += m;
                    pszGroup += m;
                    l -= m;
                    /**pszStart = cThSep;
                    pszStart++;*/
                    //memcpy(pszStart,sThSep,sThSep.GetLength());
                    memcpy(pszStart,sThSep,sThSep.GetLength() * sizeof(csprochar)); // GHM 20111213 for unicode
                    pszStart +=sThSep.GetLength();
                }
                while (l > 0) {
                    //memmove(pszStart, pszGroup, 3);
                    memmove(pszStart, pszGroup, 3 * sizeof(csprochar)); // GHM 20111213 for unicode
                    pszStart += 3;
                    pszGroup += 3;
                    l -= 3;
                    if (l > 0) {
                        /**pszStart = cThSep;
                        pszStart++;*/
                        //memcpy(pszStart,sThSep,sThSep.GetLength());
                        memcpy(pszStart,sThSep,sThSep.GetLength() * sizeof(csprochar)); // GHM 20111213 for unicode
                        pszStart +=sThSep.GetLength();
                    }
                }
            }
            else if (pTabSetFmt->GetDigitGrouping() == DIGIT_GROUPING_INDIC){
                int m = (l - 3) % 2;
                if (m > 0) {
                    //memmove(pszStart, pszGroup, m);
                    memmove(pszStart, pszGroup, m * sizeof(csprochar)); // GHM 20111213 for unicode
                    pszStart += m;
                    pszGroup += m;
                    l -= m;
                    /**pszStart = cThSep;
                    pszStart++;*/
                    //memcpy(pszStart,sThSep,sThSep.GetLength());
                    memcpy(pszStart,sThSep,sThSep.GetLength() * sizeof(csprochar)); // GHM 20111213 for unicode
                    pszStart +=sThSep.GetLength();
                }
                while (l > 3) {
                    //memmove(pszStart, pszGroup, 2);
                    memmove(pszStart, pszGroup, 2 * sizeof(csprochar)); // GHM 20111213 for unicode
                    pszStart += 2;
                    pszGroup += 2;
                    l -= 2;
                    /**pszStart = cThSep;
                    pszStart++;*/
                    //memcpy(pszStart,sThSep,sThSep.GetLength());
                    memcpy(pszStart,sThSep,sThSep.GetLength() * sizeof(csprochar)); // GHM 20111213 for unicode
                    pszStart +=sThSep.GetLength();
                }
                //memmove(pszStart, pszGroup, 3);
                memmove(pszStart, pszGroup, 3 * sizeof(csprochar)); // GHM 20111213 for unicode
                pszStart += 3;
            }
        }
    }

    if (iNumDecimals > 0) {
        i64toc(kDec, pszTemp, iNumDecimals, TRUE);

        // Format decimal part
        /**pszStart = pTabSetFmt->GetDecimalSep();*/
        CString sDecSep = pTabSetFmt->GetDecimalSep();
        //memcpy(pszStart,sDecSep,sDecSep.GetLength());
        memcpy(pszStart,sDecSep,sDecSep.GetLength() * sizeof(csprochar)); // GHM 20111213 for unicode
        /*pszStart++;*/
        pszStart += sDecSep.GetLength();
        //memmove(pszStart, pszTemp, iNumDecimals);
        memmove(pszStart, pszTemp, iNumDecimals * sizeof(csprochar)); // GHM 20111213 for unicode
        pszStart += iNumDecimals;
    }

    *pszStart = EOS;
    sRet.ReleaseBuffer(-1);

    return sRet;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CTabValue* CTable::GetStatTabVal(CArray<CTabValue*,CTabValue*>&arrTabVals , TABVAL_TYPE eValType)
//
/////////////////////////////////////////////////////////////////////////////////
//Use this only for getting the stat /special
CTabValue* CTable::GetStatTabVal(CArray<CTabValue*,CTabValue*>&arrTabVals , TABVAL_TYPE eValType)
{
    ASSERT(eValType != DICT_TABVAL);
    CTabValue* pRet = NULL;

    for (int iIndex =arrTabVals.GetSize()-1; iIndex>=0 ; iIndex--){
        CTabValue* pTabVal = arrTabVals.GetAt(iIndex);
        if(pTabVal->GetTabValType() == eValType){
            pRet = pTabVal;
            arrTabVals.RemoveAt(iIndex);
            break;
        }
    }

    return pRet;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CTabSet::LookupName
//
/////////////////////////////////////////////////////////////////////////////////
const CDataDict* CTabSet::LookupName(const CString& name, const DictLevel** dict_level, const CDictRecord** dict_record, const CDictItem** dict_item, const DictValueSet** dict_value_set) const
{
    if( m_pDataDict != nullptr && m_pDataDict->LookupName(name, dict_level, dict_record, dict_item, dict_value_set) )
    {
        return m_pDataDict.get();
    }

    else if( m_pWorkDict != nullptr && m_pWorkDict->LookupName(name, dict_level, dict_record, dict_item, dict_value_set) )
    {
        return m_pWorkDict.get();
    }

    return nullptr;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabSet::LookupName(const CString& csName,int* iLevel,int* iRecord,
//            int* iItem,int* iVSet) const
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabSet::LookupName(const CString& csName,int* iLevel,int* iRecord,int* iItem,int* iVSet) const
{
    bool bFound =false;

    if(m_pDataDict){
        bFound = m_pDataDict->LookupName(csName,iLevel,iRecord,iItem,iVSet);
    }
    if(m_pWorkDict && !bFound){
        bFound = m_pWorkDict->LookupName(csName,iLevel,iRecord,iItem,iVSet);
    }

    return bFound;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabSet::AddSystemTotalVar()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabSet::AddSystemTotalVar(CTable* pTable)
{
    bool bRow =true;
    const DictValueSet* pDictVSet =NULL;
    const CDictItem* pDictItem = NULL;
    const CDictRecord* pDictRecord = NULL;
    const DictLevel* pDictLevel = NULL;

    CTabVar* pTabVar = NULL;
     if(pTable->GetRowRoot()->GetNumChildren() ==0 && pTable->GetColRoot()->GetNumChildren() ==0){
         return;
     }

    if(pTable->GetRowRoot()->GetNumChildren() ==0){
        bRow = true;
    }
    else if(pTable->GetColRoot()->GetNumChildren() ==0){
        bRow = false;
    }
    else {//Remove If it already exists ??
        return ;
    }

    const CDataDict* pDict = LookupName(WORKVAR_TOTAL_NAME,&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet);

    if(!pDict){//what if cant find work var???
        //ASSERT(FALSE);
        CString sMsg;
        sMsg.Format(_T("Cannot find variable %s"), (LPCTSTR)WORKVAR_TOTAL_NAME);
       // AfxMessageBox(sMsg);
        return;
    }
    CTabVar* pAddToTabVar = NULL;
    bRow ? pAddToTabVar = pTable->GetRowRoot() : pAddToTabVar = pTable->GetColRoot();
    ASSERT(pAddToTabVar->GetNumChildren() <= 1);
    if(pAddToTabVar->GetNumChildren() == 1) {
        /*bool bSystemTotalExists = pAddToTabVar->GetChild(0)->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) ==0;
        ASSERT(bSystemTotalExists);
        pTable->SetDirty(true);*/
        ASSERT(FALSE);
        return;
    }
    else {
        //Do lookup and make sure the item exists .
        CTallyFmt* pDefTableTallyFmt = NULL;
        FMT_ID eFmtID = FMT_ID_INVALID;
        bRow ? eFmtID = FMT_ID_TALLY_ROW :eFmtID = FMT_ID_TALLY_COL;
        pDefTableTallyFmt= DYNAMIC_DOWNCAST(CTallyFmt,m_fmtReg.GetFmt(eFmtID));

        ASSERT(pDefTableTallyFmt);

        // make special fmt for sys total that contains only total stat (no freqs)
        CTallyFmt* pSysTotalVarFmt = new CTallyFmt;
        pSysTotalVarFmt->Init();
        pSysTotalVarFmt->SetID(pDefTableTallyFmt->GetID());
        pSysTotalVarFmt->SetIndex(m_fmtReg.GetNextCustomFmtIndex(*pDefTableTallyFmt));
        pSysTotalVarFmt->ClearStats();
        pSysTotalVarFmt->AddStat(CTallyVarStatFmtFactory::GetInstance()->Create(_T("Total")));
        m_fmtReg.AddFmt(pSysTotalVarFmt);

        CStringArray arrVals;
        pTabVar= new CTabVar(pDictItem,arrVals,*pSysTotalVarFmt);
        pTabVar->SetOcc(-1);
        pTabVar->SetTallyFmt(pSysTotalVarFmt);
        pAddToTabVar->AddChildVar(pTabVar);
    }
}

//Savy (R) sampling app 20081201
void CTabSet::AddStatVar(CTable* pTable)
{
    bool bRow =true;
    const DictValueSet* pDictVSet =NULL;
    const CDictItem* pDictItem = NULL;
    const CDictRecord* pDictRecord = NULL;
    const DictLevel* pDictLevel = NULL;

    CTabVar* pTabVar = NULL;
     if(pTable->GetRowRoot()->GetNumChildren() ==0 && pTable->GetColRoot()->GetNumChildren() ==0){
         return;
     }

    if(pTable->GetRowRoot()->GetNumChildren() ==0){
        bRow = true;
    }
    else if(pTable->GetColRoot()->GetNumChildren() ==0){
        bRow = false;
    }
    else {//Remove If it already exists ??
        return ;
    }

    const CDataDict* pDict = LookupName(WORKVAR_STAT_NAME,&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet);

    if(!pDict){//what if cant find work var???
        //ASSERT(FALSE);
        CString sMsg;
        sMsg.Format(_T("Cannot find variable %s"), (LPCTSTR)WORKVAR_STAT_NAME);
       // AfxMessageBox(sMsg);
        return;
    }
    CTabVar* pAddToTabVar = NULL;
    bRow ? pAddToTabVar = pTable->GetRowRoot() : pAddToTabVar = pTable->GetColRoot();
    ASSERT(pAddToTabVar->GetNumChildren() <= 1);
    if(pAddToTabVar->GetNumChildren() == 1) {
        /*bool bSystemTotalExists = pAddToTabVar->GetChild(0)->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) ==0;
        ASSERT(bSystemTotalExists);
        pTable->SetDirty(true);*/
        ASSERT(FALSE);
        return;
    }
    else {
        //Do lookup and make sure the item exists .
        CTallyFmt* pDefTableTallyFmt = NULL;
        FMT_ID eFmtID = FMT_ID_INVALID;
        bRow ? eFmtID = FMT_ID_TALLY_ROW :eFmtID = FMT_ID_TALLY_COL;
        pDefTableTallyFmt= DYNAMIC_DOWNCAST(CTallyFmt,m_fmtReg.GetFmt(eFmtID));

        ASSERT(pDefTableTallyFmt);

        // make special fmt for sys total that contains only total stat (no freqs)
        CTallyFmt* pSysTotalVarFmt = new CTallyFmt;
        pSysTotalVarFmt->Init();
        pSysTotalVarFmt->SetID(pDefTableTallyFmt->GetID());
        pSysTotalVarFmt->SetIndex(m_fmtReg.GetNextCustomFmtIndex(*pDefTableTallyFmt));
        pSysTotalVarFmt->ClearStats();
        pSysTotalVarFmt->AddStat(CTallyVarStatFmtFactory::GetInstance()->Create(_T("R")));
        pSysTotalVarFmt->AddStat(CTallyVarStatFmtFactory::GetInstance()->Create(_T("SE")));
        pSysTotalVarFmt->AddStat(CTallyVarStatFmtFactory::GetInstance()->Create(_T("N-UNWE")));
        pSysTotalVarFmt->AddStat(CTallyVarStatFmtFactory::GetInstance()->Create(_T("N-WEIG")));
        //Savy (R) sampling app 20081209
        pSysTotalVarFmt->AddStat(CTallyVarStatFmtFactory::GetInstance()->Create(_T("SER")));
        pSysTotalVarFmt->AddStat(CTallyVarStatFmtFactory::GetInstance()->Create(_T("SD")));
        pSysTotalVarFmt->AddStat(CTallyVarStatFmtFactory::GetInstance()->Create(_T("DEFT")));
        pSysTotalVarFmt->AddStat(CTallyVarStatFmtFactory::GetInstance()->Create(_T("ROH")));
        pSysTotalVarFmt->AddStat(CTallyVarStatFmtFactory::GetInstance()->Create(_T("SE/R")));
        pSysTotalVarFmt->AddStat(CTallyVarStatFmtFactory::GetInstance()->Create(_T("R-2SE")));
        pSysTotalVarFmt->AddStat(CTallyVarStatFmtFactory::GetInstance()->Create(_T("R+2SE")));
        pSysTotalVarFmt->AddStat(CTallyVarStatFmtFactory::GetInstance()->Create(_T("SAMP_BASE")));
        pSysTotalVarFmt->AddStat(CTallyVarStatFmtFactory::GetInstance()->Create(_T("B")));

        m_fmtReg.AddFmt(pSysTotalVarFmt);

        CStringArray arrVals;
        //Savy (R) sampling app 20081208
        //pTabVar= new CTabVar(pDictItem,arrVals,*pSysTotalVarFmt);
        pTabVar= new CTabVar();
        pTabVar->Init(pDictItem,arrVals,*pSysTotalVarFmt);

        pTabVar->SetOcc(-1);
        pTabVar->SetTallyFmt(pSysTotalVarFmt);
        pAddToTabVar->AddChildVar(pTabVar);
    }
}

//Savy (R) sampling app 20090218
bool CTabSet::IsValidSamplingStatSettings(CString &sMsg)
{
    int bRet = false;
    CString sErrorMsg;

    if(GetSampMethodType() == SAMPLING_METHOD_TYPE::TAYLOR_SERIES_METHOD|| GetSampMethodType() == SAMPLING_METHOD_TYPE::JACK_KNIFE_METHOD || GetSampMethodType()== SAMPLING_METHOD_TYPE::BRRP_METHOD)
    {
        if(GetClusterVariable().IsEmpty())
        {
            sErrorMsg = _T("Cluster Variable");
        }
        if(GetSampMethodType() == SAMPLING_METHOD_TYPE::TAYLOR_SERIES_METHOD)
        {
            if(GetStrataVariable().IsEmpty() && GetStrataFileName().IsEmpty())
            {
                if(sErrorMsg.GetLength()>0){
                    sErrorMsg = _T("Strata Variable / Strata File Name, ") + sErrorMsg;
                }
                else{
                    sErrorMsg = _T("Strata Variable / Strata File Name");
                }

            }
        }
    }
    else{
        sErrorMsg = _T("Sampling Method");
    }
    if(sErrorMsg.GetLength() > 0){
        sErrorMsg = _T("Invalid ") + sErrorMsg;
        sMsg = sErrorMsg;
        bRet = false;
    }
    else{
        bRet = true;
    }

    return bRet;
}
//Savy (R) sampling app 20090219
CString CTabSet::GetBreakBy4SampApp()
{
    CString sBreakby;

    if(GetSampMethodType() == SAMPLING_METHOD_TYPE::TAYLOR_SERIES_METHOD|| GetSampMethodType() == SAMPLING_METHOD_TYPE::JACK_KNIFE_METHOD || GetSampMethodType()== SAMPLING_METHOD_TYPE::BRRP_METHOD){

        if(GetClusterVariable().GetLength()>0){
            if(GetStrataVariable().GetLength()>0 || GetDomainVarArray().GetCount() >0){
                sBreakby = GetClusterVariable() + _T(",");
            }
            else{
                sBreakby = GetClusterVariable();
            }
        }
        if(GetStrataVariable().GetLength()>0){

            if(GetDomainVarArray().GetCount() >0){
                sBreakby += GetStrataVariable() + _T(",");
            }
            else{
                sBreakby += GetStrataVariable();
            }
        }
        //Domainvar
        DOMAINVAR pDomVar;
        if(GetDomainVarArray().GetCount() >0){
            for (int iarrLen = 0 ; iarrLen < GetDomainVarArray().GetCount() ; iarrLen++) {
                pDomVar = GetDomainVarArray().GetAt(iarrLen);
                sBreakby += pDomVar.m_sDomainVarName;
                if( iarrLen!= GetDomainVarArray().GetCount()-1){
                    sBreakby += _T(",");
                }

            }
        }
        sBreakby = _T("BREAK BY ") + sBreakby;
    }

    return sBreakby;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabSet::DoAllSubTablesHaveSameUnit(CString& sUnitName)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabSet::DoAllSubTablesHaveSameUnit(CTable* pTable, CString& sUnitName)
{
    //Make sure you call the update subtable list to update  subtable list before calling this
    CString sCurUnitName;
    //Get array of units for  the table
    CArray<CStringArray,CStringArray&> arrValidSubtableUnits;
    CStringArray arrValidTableUnits;
    UpdateSubtableList(pTable, arrValidSubtableUnits, arrValidTableUnits);

    CArray<CUnitSpec,CUnitSpec&>& arrUnitSpec = pTable->GetUnitSpecArr();

    //if only one subtable then we are done
    if(arrUnitSpec.GetCount() ==1){
        sUnitName = pTable->GetTableUnit().GetLoopingVarName();
        if(sUnitName.IsEmpty() && pTable->GetTableUnit().GetUseDefaultLoopingVar() && arrValidTableUnits.GetCount() > 0){
            sUnitName = arrValidTableUnits[0];
        }
        return true;
    }
    //We have more than one subtable
    for(int iIndex= 0; iIndex < arrUnitSpec.GetSize(); iIndex++){
        CString sCurUnitName ;
        sCurUnitName = arrUnitSpec[iIndex].GetLoopingVarName();
        //Get the cur UnitName for Subtable
        if(sCurUnitName.IsEmpty() &&  arrUnitSpec[iIndex].GetUseDefaultLoopingVar() && arrValidSubtableUnits.GetCount() >= iIndex){
            CStringArray& arrUnitNames = arrValidSubtableUnits[iIndex];
            if(arrUnitNames.GetCount() > 1){
                sCurUnitName =arrUnitNames[0];
            }
        }
        if(!sUnitName.IsEmpty()){
            if(sUnitName.CompareNoCase(sCurUnitName) ==0){
                continue;
            }
            else {
                sUnitName =_T("");
                return false;
            }
        }
        else {
            sUnitName = sCurUnitName;
        }
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  int CTabSet::UpdateSubtableList(CTable* pRefTable)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabSet::UpdateSubtableListNMarkNewSubTables(CTable* pRefTable,CTabVar* pRowVar /*=NULL*/,CTabVar* pColVar /*=NULL*/)
{
    CArray<CStringArray,CStringArray&>arrValidSubtableUnits;
    CStringArray arrValidTableUnits;

    CString sUnitMsg;
    bool    bReconcile = false; //reconcile  not used
    //Firt call the update subtable list
    UpdateSubtableList(pRefTable, arrValidSubtableUnits, arrValidTableUnits);

    int iRet = -1;
    int iSubTableRow = -1;
    int iSubTableCol = -1;
    MakeNameMap(pRefTable);
    //pRefTable->UpdateSubtableList();
    CArray<CUnitSpec,CUnitSpec&> arrUnitSpec; //local array for processing
    CArray<CUnitSpec,CUnitSpec&>& arrTableUnitSpec = pRefTable->GetUnitSpecArr(); //local array for processing
    CStringArray sArrRow,sAltArrRow;
    CStringArray sArrCol,sAltArrCol;
    CString sCurSelSubTable,sCurSelAltSubTable;
    //process the row vars first
    CTabVar* pRowRootVar = pRefTable->GetRowRoot();
    int iNumRowVars = pRowRootVar->GetNumChildren();\



    if(pRowRootVar->GetNumChildren() ==0){
        //Remove all the Unit specs from the table
        arrTableUnitSpec.RemoveAll();
        return;
    }
    //Generate Row Strings
    CArray<CStringArray,CStringArray> asRowVars;

    for(int iIndex =0;iIndex < iNumRowVars ; iIndex++){
        const DictLevel* pDictLevel = NULL;
        const CDictRecord* pDictRecord = NULL;
        const CDictItem* pDictItem = NULL;
        const DictValueSet* pDictVSet = NULL;

        CTabVar* pTabVar = pRowRootVar->GetChild(iIndex);
        CString sParentString,sAltParentString;
        if(m_varNameMap.Lookup(pTabVar,sParentString)){
        }
        else {
            sParentString = pTabVar->GetName();
        }
        sAltParentString = sParentString;
        //new scheme dont add "single var names" in the sub table list shld be alway VAR1 BY VAR2
        /*CUnitSpec unitSpec(sParentString);
        arrUnitSpec.Add(unitSpec);*/
        if(pTabVar->GetNumChildren() == 0) {
            sArrRow.Add(sParentString);
            //Check the dict . If VSet only one the add Alt String . If more than one vset
            //add the correct string which is the same as sArrRow stuff
            if(LookupName(pTabVar->GetName(),&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet)){
                if(pDictItem->GetNumValueSets() > 1) {
                    sAltArrRow.Add(sAltParentString);
                }
                else {//if it same as vset name then replace . If it is p03_sex_vs1(1) ??
                    if(pDictVSet) {
                        sAltParentString.Replace(pDictVSet->GetName(),pDictItem->GetName());
                        sAltArrRow.Add(sAltParentString);
                    }
                    else {
                        sAltArrRow.Add(sAltParentString);
                    }
                }
            }
            else {
                sAltArrRow.Add(sAltParentString);
            }
            asRowVars.SetSize(sArrRow.GetSize());
            asRowVars[sArrRow.GetSize()-1].Add(pTabVar->GetName());
            if(pTabVar == pRowVar){
                iSubTableRow = sArrRow.GetSize()-1;
            }
        }
        else {
            int iNumChildren = pTabVar->GetNumChildren();
            for(int iIndex =0;iIndex < iNumChildren ; iIndex++){
                CTabVar* pChildTabVar = pTabVar->GetChild(iIndex);
                CString sChildString,sAltChildString;
                if(m_varNameMap.Lookup(pChildTabVar,sChildString)){
                }
                else {
                    sChildString = pChildTabVar->GetName();
                }
                sAltChildString = sChildString;
                if(LookupName(pTabVar->GetName(),&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet)){
                    if(pDictVSet) {
                        sAltChildString.Replace(pDictVSet->GetName(),pDictItem->GetName());
                    }
                }
                asRowVars.SetSize(sArrRow.GetSize()+1);
                asRowVars[sArrRow.GetSize()].Add(pTabVar->GetName());
                asRowVars[sArrRow.GetSize()].Add(pChildTabVar->GetName());
                sChildString = sParentString + _T("*") + sChildString;
                sAltChildString = sAltParentString + _T("*") + sAltChildString;
                sArrRow.Add(sChildString);
                sAltArrRow.Add(sAltChildString);

                if(pChildTabVar == pRowVar){
                    iSubTableRow = sArrRow.GetSize()-1;
                }
            }
        }
    }
    //Generate Col Strings
    CTabVar* pColRootVar = pRefTable->GetColRoot();
    int iNumColVars = pColRootVar->GetNumChildren();
    if(pColRootVar->GetNumChildren() ==0){
        //This may be a over kill get back to this to decide what to do on the
        //existing units
        //Remove all the Unit specs from the table
        arrTableUnitSpec.RemoveAll();
        return;
    }
    CArray<CStringArray,CStringArray> asColVars;
    for(int iIndex =0;iIndex < iNumColVars ; iIndex++){
        const DictLevel* pDictLevel = NULL;
        const CDictRecord* pDictRecord = NULL;
        const CDictItem* pDictItem = NULL;
        const DictValueSet* pDictVSet = NULL;
        CTabVar* pTabVar = pColRootVar->GetChild(iIndex);
        CString sParentString,sAltParentString;
        if(m_varNameMap.Lookup(pTabVar,sParentString)){
        }
        else {
            sParentString = pTabVar->GetName();
        }
        sAltParentString = sParentString;
        if(pTabVar->GetNumChildren() == 0) {
            sArrCol.Add(sParentString);

            //Check the dict . If VSet only one the add Alt String . If more than one vset
            //add the correct string which is the same as sArrRow stuff
            if(LookupName(pTabVar->GetName(),&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet)){
                if(pDictItem->GetNumValueSets() > 1) {
                    sAltArrCol.Add(sAltParentString);
                }
                else {//if it same as vset name then replace . If it is p03_sex_vs1(1) ??
                    if(pDictVSet) {
                        sAltParentString.Replace(pDictVSet->GetName(),pDictItem->GetName());
                        sAltArrCol.Add(sAltParentString);
                    }
                    else {
                        sAltArrCol.Add(sAltParentString);
                    }
                }
            }
            else {
                sAltArrCol.Add(sAltParentString);
            }
            asColVars.SetSize(sArrCol.GetSize());
            asColVars[sArrCol.GetSize()-1].Add(pTabVar->GetName());
            if(pTabVar == pColVar){
                iSubTableCol = sArrCol.GetSize()-1;
            }
        }
        else {
            int iNumChildren = pTabVar->GetNumChildren();
                for(int iIndex =0;iIndex < iNumChildren ; iIndex++){
                    CTabVar* pChildTabVar = pTabVar->GetChild(iIndex);
                    CString sChildString,sAltChildString;
                    if(m_varNameMap.Lookup(pChildTabVar,sChildString)){
                    }
                    else {
                        sChildString = pChildTabVar->GetName();
                    }
                    sAltChildString = sChildString;
                    if(LookupName(pTabVar->GetName(),&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet)){
                        if(pDictVSet) {
                            sAltChildString.Replace(pDictVSet->GetName(),pDictItem->GetName());
                        }
                    }
                    asColVars.SetSize(sArrCol.GetSize()+1);
                    asColVars[sArrCol.GetSize()].Add(pTabVar->GetName());
                    asColVars[sArrCol.GetSize()].Add(pChildTabVar->GetName());
                    sChildString = sParentString + _T("*") + sChildString;
                    sAltChildString = sAltParentString + _T("*") + sAltChildString;
                    sArrCol.Add(sChildString);
                    sAltArrCol.Add(sAltChildString);
                    if(pChildTabVar == pColVar){
                        iSubTableCol = sArrCol.GetSize()-1;
                    }
                }
        }
    }


    CArray<CStringArray,CStringArray> asUnitNames;
    asUnitNames.SetSize(sArrRow.GetSize() * sArrCol.GetSize());

    //Generate Subtable list of Row By Col
    CStringArray asAllVarNames;
    for (int iRow = 0; iRow < sArrRow.GetSize();iRow++){
        for (int iCol = 0; iCol < sArrCol.GetSize();iCol++){
            CString sSubtable,sAltSubtable;
            sSubtable = sArrRow[iRow] + _T(" by ") + sArrCol[iCol];
            sAltSubtable = sAltArrRow[iRow] + _T(" by ") + sAltArrCol[iCol];
            CUnitSpec unitSpec(sSubtable);
            if(sSubtable.CompareNoCase(sAltSubtable) !=0){
                unitSpec.SetAltSubTableString(sAltSubtable);
            }
            if(iSubTableCol == iCol || iSubTableRow == iRow){
                unitSpec.SetNewMark(true);
            }
            arrUnitSpec.Add(unitSpec);
            if(iSubTableCol == iCol && iSubTableRow == iRow){
                iRet = arrUnitSpec.GetSize()-1;
                sCurSelAltSubTable = sAltSubtable;
                sCurSelSubTable = sSubtable;
            }
            CStringArray asVarNames;
            asVarNames.Append(asRowVars[iRow]);
            AppendUnique(asVarNames, asColVars[iCol]);
            ComputeSubtableUnitNames(asUnitNames[arrUnitSpec.GetSize()-1], asVarNames);
            AppendUnique(asAllVarNames, asVarNames);
        }
    }

    // compute units for the entire table
    ComputeSubtableUnitNames(arrValidTableUnits, asAllVarNames);

    // reconcile table units
    if (bReconcile) {//no reconcile stuff in here ..DELME later
        ASSERT(FALSE);
        /*CUnitSpec& tblUnit = pRefTable->GetTableUnit();
        ASSERT(pReconcileMsg);
        ReconcileUnitLoopingVar(tblUnit, arrValidTableUnits,  pRefTable->GetName(), *pReconcileMsg);*/
    }

    //Reconcile the subtable list in the unit array
    arrValidSubtableUnits.RemoveAll();
    arrValidSubtableUnits.SetSize(arrTableUnitSpec.GetSize());
    for(int iIndex = arrTableUnitSpec.GetSize() -1; iIndex >=0 ;iIndex--){
        CString sSubTable = arrTableUnitSpec[iIndex].GetSubTableString();
        sSubTable.Trim();
        bool bFound = false;
        for(int iLocal = arrUnitSpec.GetSize()-1 ; iLocal >=0 ;iLocal--){
            CString sLocalSubTable = arrUnitSpec[iLocal].GetSubTableString();
            sLocalSubTable.Trim();
            bFound = (sSubTable.CompareNoCase(sLocalSubTable) ==0);
            if(bFound){
                arrTableUnitSpec[iIndex].SetNewMark(arrUnitSpec[iLocal].IsMarkedNew());
                arrTableUnitSpec[iIndex].SetAltSubTableString(arrUnitSpec[iLocal].GetAltSubTableString());
                const CString& sCurrUnit = arrTableUnitSpec[iIndex].GetLoopingVarName();
                if (arrTableUnitSpec[iIndex].GetUseDefaultLoopingVar() || sCurrUnit.IsEmpty()) {
                    // no unit set - use default
                    arrTableUnitSpec[iIndex].SetLoopingVarName(asUnitNames[iLocal][0]);
                    arrTableUnitSpec[iIndex].SetUseDefaultLoopingVar(true);
                }
                else if (bReconcile) {
                    ASSERT(FALSE);
                    /*ReconcileUnitLoopingVar(arrTableUnitSpec[iIndex], asUnitNames[iLocal],
                        pRefTable->GetName() + " (" + sLocalSubTable + ")", *pReconcileMsg);*/
                }
                arrValidSubtableUnits[iIndex].Copy(asUnitNames[iLocal]);
                arrUnitSpec.RemoveAt(iLocal);
                asUnitNames.RemoveAt(iLocal);
                break;
            }

        }
        if(!bFound){//Do we have to let the user know abt this
            arrTableUnitSpec.RemoveAt(iIndex);
        }
    }
    //Add the remaining unit specs to the Subtable list
    const int iNumExistingSubs = arrValidSubtableUnits.GetSize();
    arrValidSubtableUnits.SetSize( iNumExistingSubs + arrUnitSpec.GetSize());
    for(int iLocal = 0; iLocal < arrUnitSpec.GetSize() ;iLocal++){
        ASSERT(arrUnitSpec[iLocal].IsMarkedNew());
        arrTableUnitSpec.Add(arrUnitSpec[iLocal]);
        arrValidSubtableUnits[iNumExistingSubs + iLocal].Copy(asUnitNames[iLocal]);
    }
    if(arrTableUnitSpec.GetSize() ==1){
        //move this to the table level unit and set the unit of the table to blank
        CUnitSpec& tableUnit = pRefTable->GetTableUnit();
        CUnitSpec& singleUnit = arrTableUnitSpec[0];
        if(singleUnit.IsUnitPresent()){
            bool bUseDefaultLoopingVar = singleUnit.GetUseDefaultLoopingVar();
            if(!bUseDefaultLoopingVar){
                CString sLoopingVarName = singleUnit.GetLoopingVarName();
                tableUnit.SetLoopingVarName(sLoopingVarName);
                tableUnit.SetUseDefaultLoopingVar(false);
                singleUnit.SetLoopingVarName(_T(""));
                singleUnit.SetUseDefaultLoopingVar(true);
            }

            CString sSingleUnitValue = singleUnit.GetValue();
            sSingleUnitValue.Trim();
            if(!sSingleUnitValue.IsEmpty()){
                tableUnit.SetValue(sSingleUnitValue);
                singleUnit.SetValue(_T(""));
            }
            CString sWeightExpr = singleUnit.GetWeightExpr();
            sWeightExpr.Trim();
            if(!sWeightExpr.IsEmpty()){
                CString sTableWeight = tableUnit.GetWeightExpr();
                sTableWeight.Trim();
                if(!sTableWeight.IsEmpty()){
                    sWeightExpr = _T("(")+sTableWeight+_T(")") + _T("*") +_T("(")+ sWeightExpr+_T(")");
                    tableUnit.SetWeightExpr(sWeightExpr);
                }
                else{
                    tableUnit.SetWeightExpr(sWeightExpr);
                }
                singleUnit.SetWeightExpr(_T(""));
            }
            CString sSelectExpr = singleUnit.GetUniverse();
            sSelectExpr.Trim();
            if(!sSelectExpr.IsEmpty()){
                CString sTableUniverse = tableUnit.GetUniverse();
                sTableUniverse.Trim();
                if(!sTableUniverse.IsEmpty()){
                    sSelectExpr = sTableUniverse + _T(" AND ") + sSelectExpr;
                    tableUnit.SetUniverse(sSelectExpr);
                }
                else {
                    tableUnit.SetUniverse(sSelectExpr);
                }
                singleUnit.SetUniverse(_T(""));
            }
            //Finally there should be no subtable unit spec associated with this single unit
            ASSERT(!singleUnit.IsUnitPresent());
        }

    }
    else {//Update the iRet after reconcile
        iRet = -1;
        for(int iIndex =0; iIndex < arrTableUnitSpec.GetSize(); iIndex++){
            CUnitSpec& curUnit = arrTableUnitSpec[iIndex];
            CString sCurUnitSubTableString = curUnit.GetSubTableString();
            CString sCurUnitAltSubTableString = curUnit.GetAltSubTableString();
            if(sCurUnitAltSubTableString.IsEmpty()){
                if( (sCurUnitSubTableString.CompareNoCase(sCurSelSubTable) ==0)){
                    iRet = iIndex;
                    break;
                }
            }
            else {
                if( (sCurUnitSubTableString.CompareNoCase(sCurSelSubTable) ==0)&& (sCurUnitAltSubTableString.CompareNoCase(sCurSelAltSubTable) ==0)){
                    iRet = iIndex;
                    break;
                }
            }
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabSet::IsSubTableFromWrkStg(CTable* pTable,const CUnitSpec& unitSpec)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabSet::IsSubTableFromWrkStg(CTable* pTable,const CUnitSpec& unitSpec)
{
    UNREFERENCED_PARAMETER(pTable);

    bool bRet = false;
    CIMSAString sSubTableString;
    if(unitSpec.GetAltSubTableString().IsEmpty()){
        sSubTableString = unitSpec.GetSubTableString();
    }
    else {
        sSubTableString = unitSpec.GetAltSubTableString();
    }
    //Parse the subtable string
    CStringArray arrVarNames;
    CIMSAString sVarName =sSubTableString;

    while (sSubTableString.GetLength() > 0) {
        sVarName=sSubTableString.GetToken();
        if (sVarName.CompareNoCase(_T("BY"))==0) {
            continue;
        }
        CString sFirstVar;
        sFirstVar = sVarName.GetToken(_T("*"));
        sFirstVar.Trim();
        sVarName.Trim();
        if(!sFirstVar.IsEmpty()){
            if(sFirstVar.Find(_T("(")) > 0){
                arrVarNames.Add(sFirstVar.Left(sFirstVar.Find(_T("("))));
            }
            else {
                arrVarNames.Add(sFirstVar);
            }
        }
        if(!sVarName.IsEmpty()){
            if(sVarName.Find(_T("(")) > 0){
                arrVarNames.Add(sVarName.Left(sVarName.Find(_T("("))));
            }
            else {
                arrVarNames.Add(sVarName);
            }
        }
    }
    const CDataDict* pWorkDict = this->GetWorkDict();
    for(int iIndex =0; iIndex < arrVarNames.GetCount();iIndex++){
        int iLevel =-1;
        int iRecord = -1;
        int iItem = -1;
        int iVSet =-1;
        BOOL bFound = pWorkDict->LookupName(arrVarNames[iIndex],&iLevel,&iRecord,&iItem,&iVSet);
        if(!bFound){
            return false;
        }
    }
    bRet = arrVarNames.GetCount() > 0;//if we are here then if arrofnames is gt zero we found all vars in wkstg
    return bRet;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  int CTabSet::GetSubTblLevelFromVars(CTable* pTable,const CUnitSpec& unitSpec)
//
/////////////////////////////////////////////////////////////////////////////////
int CTabSet::GetSubTblLevelFromVars(CTable* pTable,const CUnitSpec& unitSpec)
{
    UNREFERENCED_PARAMETER(pTable);

    int iCurLevel =0;
    CIMSAString sSubTableString;
    if(unitSpec.GetAltSubTableString().IsEmpty()){
        sSubTableString = unitSpec.GetSubTableString();
    }
    else {
        sSubTableString = unitSpec.GetAltSubTableString();
    }
    //Parse the subtable string
    CStringArray arrVarNames;
    CIMSAString sVarName =sSubTableString;

    while (sSubTableString.GetLength() > 0) {
        sVarName=sSubTableString.GetToken();
        if (sVarName.CompareNoCase(_T("BY"))==0) {
            continue;
        }
        CString sFirstVar;
        sFirstVar = sVarName.GetToken(_T("*"));
        sFirstVar.Trim();
        sVarName.Trim();
        if(!sFirstVar.IsEmpty()){
            if(sFirstVar.Find(_T("(")) > 0){
                arrVarNames.Add(sFirstVar.Left(sFirstVar.Find(_T("("))));
            }
            else {
                arrVarNames.Add(sFirstVar);
            }
        }
        if(!sVarName.IsEmpty()){
            if(sVarName.Find(_T("(")) > 0){
                arrVarNames.Add(sVarName.Left(sVarName.Find(_T("("))));
            }
            else {
                arrVarNames.Add(sVarName);
            }
        }
    }
    for(int iIndex =0; iIndex < arrVarNames.GetCount();iIndex++){
        int iLevel =-1;
        int iRecord = -1;
        int iItem = -1;
        int iVSet =-1;
        BOOL bFound = m_pDataDict->LookupName(arrVarNames[iIndex],&iLevel,&iRecord,&iItem,&iVSet);
        if(!bFound){
            continue;
        }
        else{
            if(iLevel > iCurLevel){
                iCurLevel = iLevel;
            }
        }
    }
    return iCurLevel;
}
CString CTabSet::GetLoopingVar4Samp(CTable* pTable)
{
    //bool bRet = false;
    CArray<CUnitSpec,CUnitSpec&>& arrUnitSpec = pTable->GetUnitSpecArr();
    const CUnitSpec& unitTableSpec = pTable->GetTableUnit();
    CString sLoopVarName = unitTableSpec.GetLoopingVarName();
    ASSERT(!sLoopVarName.IsEmpty());
    //SAvy to do we may need to expand this for the correct looping var

    return sLoopVarName;
    /*//Call update subtable list to get the unit stuff populated correctly
    CArray<CStringArray,CStringArray&> arrValidSubtableUnitNames;
    CStringArray arrValidTableUnitNames;

    UpdateSubtableList(pTable, arrValidSubtableUnitNames, arrValidTableUnitNames,
                false,NULL,NULL,NULL);

    for(int iIndex = 0 ; iIndex <arrUnitSpec.GetSize();iIndex++){
        CUnitSpec unitSpec = arrUnitSpec[iIndex];//get a copy of the subtable unit
        if(unitSpec.GetLoopingVarName().IsEmpty() && unitSpec.GetUseDefaultLoopingVar()){
            //set looping var name for forced unit generation
            unitSpec.SetLoopingVarName(arrValidSubtableUnitNames[iIndex].GetAt(0));
        }

    }

    return bRet;*/
}
