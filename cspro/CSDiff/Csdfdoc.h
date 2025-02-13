#pragma once


class CCSDiffDoc : public CDocument
{
    DECLARE_DYNCREATE(CCSDiffDoc)

public:
    CCSDiffDoc();

    const DiffSpec& GetDiffSpec() const { return *m_diffSpec; }
    DiffSpec& GetDiffSpec()             { return *m_diffSpec; }

    const CDataDict& GetDictionary() const { return m_diffSpec->GetDictionary(); }

    const PFF& GetPff() const { return m_pff; }

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
    BOOL SaveModified() override;

    afx_msg void OnFileSave();
    afx_msg void OnFileSaveAs();
    afx_msg void OnFileRun();
    afx_msg void OnUpdateIsDictionaryDefined(CCmdUI* pCmdUI);
    afx_msg void OnOptionsExcluded();
    afx_msg void OnUpdateOptionsExcluded(CCmdUI* pCmdUI);

private:
    bool OpenDictFile(const std::wstring& filename);
    bool OpenSpecFile(const std::wstring& filename);

    void SaveSpecFile();

    void RunBatchDiff();

private:
    std::shared_ptr<DiffSpec> m_diffSpec;
    PFF m_pff;

    bool m_bRetSave;
};
