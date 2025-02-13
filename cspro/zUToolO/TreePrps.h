#pragma once
//***************************************************************************
//  File name: TreePrps.h
//
//  Description:
//       Tree properties dialog.  Property sheet like dialog with a tree control on
//  the left for choosing between the pages.  Pages are actually just CDialogs to
//  allow for use of same dialog class as page or as seperate modal dialog.
//
//***************************************************************************

#include <zUToolO/zUtoolO.h>
#include <zUToolO/GradLbl.h>

/////////////////////////////////////////////////////////////////////////////
// CTreePropertiesDlg dialog

class OX_CLASS_DECL CTreePropertiesDlg : public CDialog
{
// Construction
public:
    CTreePropertiesDlg(const CString& sTitle,
                       std::optional<unsigned> dialog_id_override = std::nullopt,
                       CWnd* pParent = NULL);   // standard constructor

    // add new page to dialog
    void AddPage(CDialog* pPage,        // ptr to dialog to add as page, must have created dlg as modeless
                                        // client should delete dlg when done
                 LPCTSTR sCaption,      // caption to use for this page in tree
                 CDialog* pParent = NULL, // optional parent in tree for this page
                 CDialog* pInsertAfter = NULL); // optional node in tree to insert after

    // set the current page displayed
    void SetPage(CDialog* pPage);

    // get current page being displayed
    CDialog* GetPage() const
    {
        return m_pCurrDlg;
    }

    // override to do updates when user changes page
    virtual void OnPageChange(CDialog* pOldPage, CDialog* pNewPage);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    CDialog* m_pCurrDlg;

    HTREEITEM FindItemByPage(CDialog* pPage);
    HTREEITEM ForEachTreeItem(bool fn(CTreeCtrl&, HTREEITEM, void*), void* pUserData);

    virtual void ResizeDlg(const CRect& newPageRect);

    bool IsInitialized();

    struct DeferAddStruct {
        CDialog* pPage;
        CDialog* pParent;
        CDialog* pInsertAfter;
        CString sCaption;
    };
    CArray<DeferAddStruct, DeferAddStruct&> m_deferAddPages;
    CString m_sDlgTitle;

    CTreeCtrl   m_treeCtrl;
    CGradientLabel m_captionCtrl; // for gradient caption
    CRect m_pageRect;
    virtual BOOL OnInitDialog();
    afx_msg void OnSelchangedTree(NMHDR* pNMHDR, LRESULT* pResult);
    virtual void OnOK();
    DECLARE_MESSAGE_MAP()
};
