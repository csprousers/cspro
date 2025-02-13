#pragma once

#include <TextView/Tvmisc.h>

//***************************************************************************
//  File name: TVDoc.h
//
//  Description:
//       Header for TextView document
//
//  History:    Date       Author   Comment
//              ---------------------------
//              18 Nov 99   bmd     Created for CSPro 2.0
//
//***************************************************************************

class CTVDoc : public CDocument
{
    DECLARE_DYNCREATE(CTVDoc)

protected: // create from serialization only
    CTVDoc();

// Attributes
private:
    CBufferMgr m_buffMgr;
    bool m_bIsReloadingOrClosing;   // = true if we're in the middle of a reload/close deleted file operation (this prevents recursion)

// Operations
public:
    CBufferMgr* GetBufferMgr() { return &m_buffMgr; }

    bool IsReloadingOrClosing() const { return m_bIsReloadingOrClosing; }

    void UpdateStatusBar();
    void ReloadFile();
    void CloseDeletedFile();

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnOpenDocument(const TCHAR* pszFileName) override;

    afx_msg void OnFileSaveAs();
};
