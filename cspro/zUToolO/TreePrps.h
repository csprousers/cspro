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
#include <zUToolO/TreePropertiesPageValidator.h>


/////////////////////////////////////////////////////////////////////////////
// CTreePropertiesDlg dialog

class OX_CLASS_DECL CTreePropertiesDlg : public CDialog
{
// Construction
public:
    CTreePropertiesDlg(std::wstring title, std::optional<unsigned> dialog_id_override = std::nullopt,
                       bool use_caption = true, CWnd* pParent = nullptr);

    // add new page to dialog
    void AddPage(CDialog* pPage,                                            // ptr to dialog to add as page, must have created dlg as modeless
                                                                            // client should delete dlg when done
                 LPCTSTR sCaption,                                          // caption to use for this page in tree
                 CDialog* pParent = NULL,                                   // optional parent in tree for this page
                 CDialog* pInsertAfter = NULL,                              // optional node in tree to insert after
                 TreePropertiesPageValidator* page_validator = nullptr);    // optional validator for the page to be called instead of OnOK

    // set the current page displayed
    void SetPage(CDialog* pPage);

    // get current page being displayed
    CDialog* GetPage() const
    {
        return m_pCurrDlg;
    }

protected:
    // override to do updates when the user changes the page;
    // the function should return true if the focus should be set to the page (rather than saying on the control)
    virtual bool OnPageChange(CDialog* pOldPage, CDialog* pNewPage);

    // override to modify tree view item values (such as the icon index) before insertion
    virtual void OnModifyTreeItemBeforeInsert(CDialog* dlg, TVITEMW& item);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL OnInitDialog() override;

    afx_msg void OnSelchangedTree(NMHDR* pNMHDR, LRESULT* pResult);

    // returns true if all pages were validated successfully
    bool ValidatePages();

    void OnOK() override;

    HTREEITEM FindItemByPage(const CDialog* pPage);
    HTREEITEM ForEachTreeItem(bool fn(CTreeCtrl&, HTREEITEM, void*), void* pUserData);

    virtual void ResizeDlg(const CRect& newPageRect);

    bool IsInitialized();

protected:
    CTreeCtrl m_treeCtrl;
    CDialog* m_pCurrDlg;

private:
    struct DeferAddStruct
    {
        CDialog* pPage;
        CDialog* pParent;
        CDialog* pInsertAfter;
        CString sCaption;
        TreePropertiesPageValidator* page_validator;
    };

    CArray<DeferAddStruct, DeferAddStruct&> m_deferAddPages;
    std::wstring m_sDlgTitle;

    std::unique_ptr<CGradientLabel> m_captionCtrl; // for gradient caption
    CRect m_pageRect;

    std::map<CDialog*, TreePropertiesPageValidator*> m_pageValidators;
};
