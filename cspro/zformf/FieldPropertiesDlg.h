#pragma once

class CFormScrollView;


// single and multiple field properties
#define CAPTURETYPE_TEXTBOX_NO_TICKMARKS             ( (int)CaptureType::Unspecified - 1 )
#define CAPTURETYPE_TEXTBOX_NO_TICKMARKS_DESCRIPTION _T("Text Box (No Tickmarks)")

#define CAPTURETYPE_TEXTBOX_MULTILINE                ( (int)CaptureType::Unspecified - 2 )
#define CAPTURETYPE_TEXTBOX_MULTILINE_DESCRIPTION    _T("Text Box (Multiline)")
                                              
#define CAPTURETYPE_UNASSIGNED_DESCRIPTION           _T("<linked to dictionary item>")
                                              
// multiple field properties                  
#define CAPTURETYPE_NO_CHANGE                        ( (int)CaptureType::Unspecified - 3 )
#define CAPTURETYPE_DEFAULT                          ( (int)CaptureType::Unspecified - 4 )
#define CAPTURETYPE_LINK_TO_DICT_IF_DEFINED          ( (int)CaptureType::Unspecified - 5 )


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

    CString             m_sKeyboardDescription;
    UINT                m_KLID;

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
