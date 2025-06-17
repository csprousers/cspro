#pragma once

class CFormScrollView;


// single and multiple field properties
constexpr int CAPTURETYPE_TEXTBOX_NO_TICKMARKS                     = static_cast<int>(CaptureType::Unspecified) - 1;
constexpr const char* CAPTURETYPE_TEXTBOX_NO_TICKMARKS_DESCRIPTION = "Text Box (No Tickmarks)";

constexpr int CAPTURETYPE_TEXTBOX_MULTILINE                        = static_cast<int>(CaptureType::Unspecified) - 2;
constexpr const char* CAPTURETYPE_TEXTBOX_MULTILINE_DESCRIPTION    = "Text Box (Multiline)";

constexpr const char* CAPTURETYPE_UNASSIGNED_DESCRIPTION           = "<linked to dictionary item>";

// multiple field properties
constexpr int CAPTURETYPE_NO_CHANGE                                = static_cast<int>(CaptureType::Unspecified) - 3;
constexpr int CAPTURETYPE_DEFAULT                                  = static_cast<int>(CaptureType::Unspecified) - 4;
constexpr int CAPTURETYPE_LINK_TO_DICT_IF_DEFINED                  = static_cast<int>(CaptureType::Unspecified) - 5;


class CFieldPropDlg : public CDialog
{
private:
    CArray<CDEItemBase*,CDEItemBase*> m_arrFieldSel;
    CDEField*                   m_pField;
    int                         m_iCurSkipSel;
    CDEFormBase::TextUse        m_eTxtUse;

public:

    bool                m_bIDItem;
    bool                m_bItemOnFirstLevel;

    CFormScrollView*    m_pMyParent;

    const CDictItem*    m_pDictItem;

    bool                m_bRepeatingItem;

    CaptureInfo         m_captureInfo;
    bool                m_bUseUnicodeTextBox;
    bool                m_bMultiLineOption;

    ValidationMethod    m_eValidationMethod;

    std::wstring        m_keyboardDescription;
    UINT                m_klid;

    CStatic*            m_pCaptureErrorIcon;
    CStatic*            m_pCaptureErrorText;

// Construction
public:
    CFieldPropDlg (CDEField* pField, CFormScrollView* pParent);

    void BuildSkipToSel();

// Dialog Data
    //{{AFX_DATA(CFieldPropDlg)
    enum { IDD = IDD_FIELDPROP };

    CString         m_sFieldName;

    CString         m_sFldTxt;
    BOOL            m_bTextLinkedToDictionary;

    CComboBox       m_cmbFldSel;
    CComboBox       m_cmbCaptureType;
    CComboBox       m_cmbCaptureTypeDateFormat;
    CComboBox       m_cmbValidationMethod;

    BOOL            m_bEnterKey;
    BOOL            m_bProtected;
    BOOL            m_bHideInCaseTree;
    BOOL            m_bAlwaysVisualValue;
    BOOL            m_bSequential;
    BOOL            m_bMirror;
    BOOL            m_bPersist;
    BOOL            m_bAutoIncrement;
    BOOL            m_bUpperCase;
    BOOL            m_bVerify;
    //}}AFX_DATA


protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
    BOOL OnInitDialog() override;
    void OnOK() override;

    afx_msg void OnProtected();
    afx_msg void OnPersistent();
    afx_msg void OnAutoIncrement();
    afx_msg void OnLinkedToDict();
    afx_msg void OnChangeLabel();

    afx_msg void OnBnClickedChangeKeyboard();
    afx_msg void OnCbnSelchangeCaptureInfo();

private:
    void EnablePersistentAutoIncrementCheckboxes();

    void PopulateCaptureInfo();
};
