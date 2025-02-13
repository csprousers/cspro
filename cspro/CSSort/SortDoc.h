#pragma once

//***************************************************************************
//  File name: SortDoc.h
//
//  Description:
//       Header for CSSort document
//
//  History:    Date       Author   Comment
//              ---------------------------
//              11 Dec 00   bmd     Created for CSPro 2.1
//
//***************************************************************************


struct SORTITEM
{
    const CDictItem* dict_item;
    SortSpec::SortOrder order;
};


class CSortDoc : public CDocument
{
public:
    CSortDoc();
    DECLARE_DYNCREATE(CSortDoc)

    CString GetSpecFileName() const;
    CString GetDictFileName() const;

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
    BOOL SaveModified() override;

    afx_msg void OnFileRun();
    afx_msg void OnFileSave();
    afx_msg void OnFileSaveAs();
    afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
    afx_msg void OnUpdateFileSaveAs(CCmdUI* pCmdUI);
    afx_msg void OnOptionsSortType();
    afx_msg void OnUpdateOptionsSortType(CCmdUI* pCmdUI);

private:
    bool OpenSpecFile(const TCHAR* filename);
    bool OpenDictFile(const TCHAR* filename);

    void ConvertSortItemsSpecToSortDoc();
    void ConvertSortItemsSortDocToSpec();

    void SaveSpecFile();

    void RunBatchSort();

public:
    CArray<SORTITEM, SORTITEM> m_aItem;
    CArray<int, int> m_aAvail;
    CArray<int, int> m_aKey;
    bool m_bRetSave;

private:
    std::shared_ptr<SortSpec> m_sortSpec;
    PFF m_pff;
};
