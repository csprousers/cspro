#pragma once

#include <zDictF/Dddoc.h>
#include <zDictF/RelGrid.h>


class CRelDlg : public CDialog
{
public:
    CRelDlg(CWnd* pParent = NULL);   // standard constructor

    enum { IDD = IDD_RELATION };

protected:
    DECLARE_MESSAGE_MAP()

    BOOL PreTranslateMessage(MSG* pMsg) override;

    BOOL OnInitDialog() override;
    void OnOK() override;
    void OnCancel() override;

    afx_msg void OnClose();
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnEditAdd();
    afx_msg void OnAdd();
    afx_msg void OnDelete();
    afx_msg void OnInsert();

public:
    CDDDoc* m_pDoc;

private:
    CRelGrid m_Relgrid;
    LOGFONT m_lf;
};
