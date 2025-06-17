#pragma once

//***************************************************************************
//  File name: DefaultStylesDlg.h
//
//  Description:
//  Dialog for picking default styles for tables and table elements.
//  Edits values in format registry
//
//***************************************************************************

#include <zTableF/AppFmtDlg.h>
#include <zTableF/CompFmtDlg.h>
#include <zTableF/TallyVarDlg.h>
#include <zTableF/TblFmtDlg.h>
#include <zTableF/TblPrintFmtDlg.h>
#include <zTableO/Style.h>
#include <zUToolO/TreePrps.h>


class CDefaultStylesDlg : public CTreePropertiesDlg
{
public:

    // constructor
    CDefaultStylesDlg(CFmtReg& fmtReg,CTabView* pTabView);

    // destructor
    ~CDefaultStylesDlg();

    // called when page changes for extra handling
    virtual bool OnPageChange(CDialog* pOldPage, CDialog* pNewPage);

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
