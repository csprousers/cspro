#pragma once

#include "afxdialogex.h"
#include <zUtilO/imsaDlg.H>

class CTroubleshootingDialog : public CDialog // GHM 20120522
{
public:
    CHtmlStatic m_staticWWW;
    CHtmlStatic m_staticEmail;
    CHtmlStatic m_staticCSProUsers;
    CHtmlStatic m_staticPack;

public:
    enum { IDD = IDD_TROUBLESHOOTING };

    CTroubleshootingDialog() : CDialog(CTroubleshootingDialog::IDD) {}

protected:
    virtual BOOL OnInitDialog();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    DECLARE_MESSAGE_MAP()
};


class CTransparentStaticImage : public CStatic // GHM 20120523
{
private:
    WORD m_resourceID;
    COLORREF m_transparentBit;

public:
    void SetBitmap(WORD rID,COLORREF transparentColor) { m_resourceID = rID; m_transparentBit = transparentColor; }

protected:
    void OnPaint();
    DECLARE_MESSAGE_MAP()
};


class CAboutDialog : public CDialogEx // GHM 20120523
{
    DECLARE_DYNAMIC(CAboutDialog)

public:
    CAboutDialog(CWnd* pParent = NULL);

    enum { IDD = IDD_NEW_ABOUT };

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;
    HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);

private:
    std::unique_ptr<CFont> CreateCustomFont(int size, bool bold);

private:
    CStatic m_title;
    CStatic m_version;
    CStatic m_developers;
    CHtmlStatic m_licenses;

    std::unique_ptr<CFont> m_pLargeFont;
    std::unique_ptr<CFont> m_pMediumFont;
    std::unique_ptr<CFont> m_pSmallFont;

    CStatic m_whiteBackground;
    CTransparentStaticImage m_logo;
};
