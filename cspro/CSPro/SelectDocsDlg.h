#pragma once


class SelectDocsDlg : public CDialog
{
public:
    SelectDocsDlg(std::vector<CDocument*> documents, const TCHAR* dialog_title = nullptr, CWnd* pParent = nullptr);

    void SetRelativePathAdjuster(const CString& filename);

    const std::vector<CDocument*>& GetSelectedDocuments() const { return m_documents; }

protected:
    enum { IDD = IDD_SELECT_DOCS };

    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;
    void OnOK() override;

    void OnSelectAll();

private:
    std::vector<CDocument*> m_documents;
    CString m_dialogTitle;
    std::optional<CString> m_relativePathFilename;
};
