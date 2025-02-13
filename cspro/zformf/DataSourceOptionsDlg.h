#pragma once

class Application;


class CDataSourceOptionsDlg : public CDialog
{
    DECLARE_DYNAMIC(CDataSourceOptionsDlg)

public:
    CDataSourceOptionsDlg(Application* pApplication, CWnd* pParent = NULL);   // standard constructor

    int GetAutoPartialSaveMinutes() const { return m_iAutomaticPartialSaveMinutes; }
    bool GetCreateListingFileFlag() const { return ( m_iCreateListingFile == BST_CHECKED ); }
    bool GetCreateLogFileFlag() const { return ( m_iCreateLogFile == BST_CHECKED ); }
    bool GetNoteDeleteOtherOperatorsFlag() const { return m_iOtherOperatorNotesDelete == BST_CHECKED; }
    bool GetNoteEditOtherOperatorsFlag() const { return m_iOtherOperatorNotesEdit == BST_CHECKED; }

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DATA_SOURCE_OPTIONS_DLG };
#endif

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;
    void DoDataExchange(CDataExchange* pDX) override;
    void OnOK() override;

public:
    afx_msg void OnCheckboxClicked();

private:
    void EnableDisableSelections();

    int m_iAutomaticPartialSaveMinutes; // the actual value
    int m_iAutomaticPartialSave; // the checkbox
    CString m_csAutomaticPartialSaveMinutes; // the textbox

    int m_iCreateListingFile;
    int m_iCreateLogFile;

    int m_iOtherOperatorNotesDelete;
    int m_iOtherOperatorNotesEdit;
};
