#pragma once
//***************************************************************************
//  File name: DfStylDg.h
//
//  Description:
//  Dialog for picking default styles for tables and table elements.
//  Edits values in format registry
//
//***************************************************************************

class CFmtReg;

#include <zUToolO/TreePrps.h>
#include <zTableO/Style.h>
#include <zTableF/AppFmtD.h>
#include <zTableF/CmpFmtD.h>
#include <zTableF/TblFmtD.h>
#include <zTableF/TblPFmtD.h>
#include <zTableF/TlyVrDlg.h>

class CTabView;

class CDefaultStylesDlg : public CTreePropertiesDlg
{
public:

    // constructor
    CDefaultStylesDlg(CFmtReg& fmtReg,CTabView* pTabView);

    // destructor
    ~CDefaultStylesDlg();

    // called when page changes for extra handling
    virtual void OnPageChange(CDialog* pOldPage, CDialog* pNewPage);

    // called when dialog dismissed with OK
    virtual void OnOK();

    // check if any style options were changed (update needed)
    bool GetStylesChanged() const
    {
        return m_bChanged;
    }

protected:

    void AddObjectFormatPage(FMT_ID fmtId,
         CCompFmtDlg& objFmtdlg,
         CDialog* pParentPage);
    CCompFmtDlg* FindObjectFormatPage(FMT_ID fmtId);

    CFmtReg& m_fmtReg;
    CTabView*   m_pTabView;
    CArray<CCompFmtDlg,CCompFmtDlg> m_aObjFmtDlgs;
    CDialog m_objFormatPage;
    CTallyVarDlg m_varTallyFmtDlgR;
    CTallyVarDlg m_varTallyFmtDlgC;
    CAppFmtDlg m_appFmtPage;
    CTblFmtDlg m_tblFmtDlg;
    CTblPrintFmtDlg m_tblPrintFmtDlg;
    bool m_bChanged;
    LOGFONT m_lfLastPrintHdr;
    LOGFONT m_lfLastPrintFtr;
};
