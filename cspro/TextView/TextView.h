#pragma once

//***************************************************************************
//  File name: TextView.h
//
//  Description:
//       Header for TextView application
//
//  History:    Date       Author   Comment
//              ---------------------------
//              18 Nov 99   bmd     Created for CSPro 2.0
//              4  Jan 03   csc     Support for font size changes
//
//***************************************************************************

#include <TextView/TVDoc.h>
#include <mutex>


/////////////////////////////////////////////////////////////////////////////
//
//                             CTextViewApp
//
/////////////////////////////////////////////////////////////////////////////

extern struct OFOSM
{
    bool open_file_directly = true;
    std::vector<CString> filenames;
    std::mutex filenames_mutex;

} OpenFilesOnStartupManager;


class CTextViewApp : public CWinApp
{
public:
    CTextViewApp();

    void UpdateAllAppViews(LPARAM lHint);

    CTVDoc* FindFile(const CString& csFileName) const;

    CDocument* OpenDocumentFile(LPCTSTR lpszFileName) override;

protected:
    DECLARE_MESSAGE_MAP()

    BOOL InitInstance() override;

    afx_msg void OnFileOpen();
    afx_msg void OnUpdateViewRuler(CCmdUI* pCmdUI);
    afx_msg void OnAppAbout();
    afx_msg void OnFilePageSetup();

private:
    bool OpenInDataViewerQuery(const CString& filename);

public:
    int     m_iNumColors;
    BOOL    m_bCalledAsChild;  // TRUE when we are invoked with the /c command line switch (as a dependent) from another IMPS application

    CString         m_csModuleName;
    CString         m_csWndClassName;
    CString         m_csFileOpenDefExt;
    CString         m_csFileOpenFilter;
    HICON           m_hIcon;
};
