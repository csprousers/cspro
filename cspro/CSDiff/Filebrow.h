#pragma once

class CCSDiffDoc;


class CFilesBrow : public CDialog
{
public:
    CFilesBrow(CCSDiffDoc* pDoc, PFF& pff, CWnd* pParent = nullptr);   // standard constructor

    enum { IDD = IDD_FILEBROW };

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog();

    afx_msg void OnOK();
    afx_msg void OnListbrow();
    afx_msg void OnInpbrow();
    afx_msg void OnRefbrow();
    afx_msg void OnChangeInputfile();
    afx_msg void OnChangeListfile();
    afx_msg void OnChangeReferencefile();

private:
    void EnableDisable();

private:
    CCSDiffDoc* m_pDoc;
    DiffSpec& m_diffSpec;
    PFF& m_pff;

    std::wstring m_inputFilename;
    std::wstring m_referenceFilename;
    std::wstring m_listingFilename;
    int m_roneway;
    int m_indexseq;
};
