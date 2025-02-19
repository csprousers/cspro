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
    DECLARE_DYNCREATE(CSortDoc)

public:
    CSortDoc();

    const std::string& GetSpecFilePath() const;
    const std::string& GetDictionaryFilePath() const;

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
    BOOL SaveModified() override;

    void OnFileRun();
    void OnFileSave();
    void OnFileSaveAs();
    void OnUpdateFileSave(CCmdUI* pCmdUI);
    void OnUpdateFileSaveAs(CCmdUI* pCmdUI);
    void OnOptionsSortType();
    void OnUpdateOptionsSortType(CCmdUI* pCmdUI);

private:
    bool OpenSpecFile(const std::string& file_path);
    bool OpenDictionary(const std::string& file_path);

    void ConvertSortItemsSpecToSortDoc();
    void ConvertSortItemsSortDocToSpec();

    void SaveSpecFile();

    void RunBatchSort();

public:
    CArray<SORTITEM, SORTITEM> m_aItem;
    CArray<int, int> m_aAvail;
    CArray<int, int> m_aKey;

private:
    bool m_bRetSave;
    std::shared_ptr<SortSpec> m_sortSpec;
    PFF m_pff;
};
