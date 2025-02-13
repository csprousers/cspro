#pragma once


// CMultipleFieldPropertiesDlg dialog added 20120613

class CMultipleFieldPropertiesDlg : public CDialog
{
    DECLARE_DYNAMIC(CMultipleFieldPropertiesDlg)

    std::vector<std::vector<CDEField*>> m_fieldSets;
    size_t m_fieldSetIndex;
    const std::function<std::vector<CDEField*>()>* m_allFieldsGetter;

    int m_iProtected;
    int m_iUpperCase;
    int m_iEnterKey;
    int m_iVerify;
    int m_iHideInCaseTree;
    int m_iAlwaysVisualValue;

    int m_iCaptureType;
    int m_iValidationMethod;
    int m_iScreenText;

    UINT m_KLID;

    void SetTriStateCheckbox(UINT rID, int* value, int numSelected, int numTotal);
    void SetPropertiesBasedOnFieldValues();

    void ProcessClick(UINT rID);

public:
    CMultipleFieldPropertiesDlg(std::vector<CDEField*> fields,
        const std::function<std::vector<CDEField*>()>* all_fields_getter,
        CWnd* pParent = NULL);

    const std::vector<CDEField*>& GetFields() const { return m_fieldSets[m_fieldSetIndex]; }

    bool ApplyProtected() const         { return m_iProtected != BST_INDETERMINATE; }
    bool ApplyUpperCase() const         { return m_iUpperCase != BST_INDETERMINATE; }
    bool ApplyEnterKey() const          { return m_iEnterKey != BST_INDETERMINATE; }
    bool ApplyVerify() const            { return m_iVerify != BST_INDETERMINATE; }
    bool ApplyHideInCaseTree() const    { return m_iHideInCaseTree != BST_INDETERMINATE; }
    bool ApplyAlwaysVisualValue() const { return m_iAlwaysVisualValue != BST_INDETERMINATE; }

    bool GetProtected() const         { return m_iProtected != BST_UNCHECKED; }
    bool GetUpperCase() const         { return m_iUpperCase != BST_UNCHECKED; }
    bool GetEnterKey() const          { return m_iEnterKey != BST_UNCHECKED; }
    bool GetVerify() const            { return m_iVerify != BST_UNCHECKED; }
    bool GetHideInCaseTree() const    { return m_iHideInCaseTree != BST_UNCHECKED; }
    bool GetAlwaysVisualValue() const { return m_iAlwaysVisualValue != BST_UNCHECKED; }

    bool ApplyCaptureType() const;
    bool UseDefaultCaptureType() const;
    bool LinkToDictionaryCaptureTypeWhenPossible() const;
    bool GetUseUnicodeTextBox() const;
    bool GetMultiLineOption() const;
    CaptureType GetCaptureType() const;

    bool ApplyValidationMethod() const           { return ( m_iValidationMethod != 0 ); }
    ValidationMethod GetValidationMethod() const { return (ValidationMethod)( m_iValidationMethod - 1 ); }

    bool ApplyFieldLabelType() const            { return ( m_iScreenText != 0 ); }
    FieldLabelType GetFieldLabelType() const    { return (FieldLabelType)( m_iScreenText - 1 ); }

    bool ApplyKLID() const                      { return ( m_KLID != HKL_NEXT ); }
    UINT GetKLID() const                        { return m_KLID; }

// Dialog Data
    enum { IDD = IDD_MULTIPLE_FIELD_PROP };

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;

    afx_msg void OnBnClickedProtected()         { ProcessClick(IDC_PROTECTED); }
    afx_msg void OnBnClickedUppercase()         { ProcessClick(IDC_UPPERCASE); }
    afx_msg void OnBnClickedEnterkey()          { ProcessClick(IDC_ENTERKEY); }
    afx_msg void OnBnClickedVerify()            { ProcessClick(IDC_VERIFY); }
    afx_msg void OnBnClickedHideInCaseTree()    { ProcessClick(IDC_MULT_HIDE_IN_CASETREE); }
    afx_msg void OnBnClickedAlwaysVisualValue() { ProcessClick(IDC_ALWAYS_VISUAL_VALUE); }
    afx_msg void OnBnClickedFieldsChooser();

    afx_msg void OnBnClickedOk();
};
