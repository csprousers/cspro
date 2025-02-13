#pragma once

#include <CSPro/NewFileCreator.h>


class NewFileDlg : public CDialog
{
    DECLARE_DYNAMIC(NewFileDlg)

public:
    NewFileDlg(CWnd* pParent = nullptr);

    enum { IDD = IDD_NEW_FILE };

    AppFileType GetAppFileType() const                     { return m_appFileType; }
    EntryApplicationStyle GetEntryApplicationStyle() const { return m_entryApplicationStyle; }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnLvnItemchangedListCategory(NMHDR* pNMHDR, LRESULT* pResult);
    void OnLvnItemchangedListType(NMHDR* pNMHDR, LRESULT* pResult);
    void OnClickList(NMHDR* pNMHDR, LRESULT* pResult);
    void OnDoubleClickList(NMHDR* pNMHDR, LRESULT* pResult);

    void OnOK() override;

private:
    CListCtrl m_newFileCategories;
    CListCtrl m_newFileTypes;

    CImageList m_imageList;
    std::map<unsigned, int> m_imageMapping;

    int m_selectedCategory;
    std::map<int, int> m_selectedType;

    AppFileType m_appFileType;
    EntryApplicationStyle m_entryApplicationStyle;
};
