#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/imsaStr.H>


//***************************************************************************
//  File name: IMSADLG.H
//
//  Description:
//       Header for dialog box classes.
//
//  History:    Date       Author   Comment
//              ---------------------------
//              21 Jan 98   BMD     Created from IMPS 4.1
//              17 Jan 05   csc     added CHtmlStatic and implemented in about box
//
//***************************************************************************
//***************************************************************************
//***************************************************************************
//
//  class CNoteDlg : public CDialog
//
//  Description:
//      Dialog box for notes of lists of error messages.
//
//  Construction
//      CNoteDlg(CWnd* pParent = NULL);
//
//  Implementation
//      OnInitDialog            Initilzie the dialog box.
//      SetNote                 Set the note string.
//      GetNote                 Get the note string.
//      SetTitle                Set the dialog box title from a string.
//
//***************************************************************************
//***************************************************************************
//***************************************************************************


/////////////////////////////////////////////////////////////////////////////
//
//                               CNoteDlg
//
/////////////////////////////////////////////////////////////////////////////

class CLASS_DECL_ZUTILO CNoteDlg : public CDialog
{
public:
    CNoteDlg(bool treat_help_button_as_clear = false, CWnd* pParent = NULL);

    void SetNote(const CString& csNoteEdit) { m_csNoteEdit = csNoteEdit; }
    const CString& GetNote() const          { return m_csNoteEdit; }

    void SetTitle(const CString& csTitle)   { m_csTitle = csTitle; }

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;
    void DoDataExchange(CDataExchange* pDX) override;

    void OnOK() override;
    void OnCancel() override;

    afx_msg void OnHelpbutton();
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

private:
    bool m_treatHelpButtonAsClear;
    CString m_csTitle;
    CString m_csNoteEdit;
};




/////////////////////////////////////////////////////////////////////////////
//
//                           CHtmlStatic
//
// This is a custom static control that mimics an HTML hyperlink.  To use:
//    - add an instance to your dialog
//    - subclass it in your dialog's OnInitDialog
//    - set the control's text and HTML command action (SetText and SetAction)
//
/////////////////////////////////////////////////////////////////////////////

class CLASS_DECL_ZUTILO CHtmlStatic : public CStatic
{
// Operations:
public:
    CHtmlStatic();

    void SetText(const CString& text) { m_text = text; }

    void SetShellAction(const CString& url)      { m_action = url; }
    void SetMessageAction(unsigned message)      { m_action = message; }
    void SetAction(std::function<void()> action) { m_action = action; }

protected:
    DECLARE_MESSAGE_MAP()

    void OnPaint();
    BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    void OnLButtonUp(UINT nFlags, CPoint point);

private:
    CString m_text;
    std::variant<CString, UINT, std::function<void()>> m_action;
};




/////////////////////////////////////////////////////////////////////////////
//
//                           CIMSAAboutDlg (App About)
//
/////////////////////////////////////////////////////////////////////////////

class CLASS_DECL_ZUTILO CIMSAAboutDlg : public CDialog
{
public:
    // client exe must pass these members in before DoModal()
    CIMSAString         m_csModuleName;
    HICON               m_hIcon;
    CHtmlStatic         m_staticWWW;
    CHtmlStatic         m_staticEmail;

public:
    CIMSAAboutDlg(wstring_view module_name_sv = wstring_view(), HICON hIcon = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;

    afx_msg void OnPaint();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
};
