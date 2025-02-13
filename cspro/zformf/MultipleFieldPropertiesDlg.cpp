#include "StdAfx.h"
#include "MultipleFieldPropertiesDlg.h"
#include "FieldPropertiesDlg.h"
#include "KeyboardInputDlg.h"


IMPLEMENT_DYNAMIC(CMultipleFieldPropertiesDlg, CDialog)

BEGIN_MESSAGE_MAP(CMultipleFieldPropertiesDlg, CDialog)
    ON_BN_CLICKED(IDC_PROTECTED, OnBnClickedProtected)
    ON_BN_CLICKED(IDC_UPPERCASE, OnBnClickedUppercase)
    ON_BN_CLICKED(IDC_ENTERKEY, OnBnClickedEnterkey)
    ON_BN_CLICKED(IDC_VERIFY, OnBnClickedVerify)
    ON_BN_CLICKED(IDC_MULT_HIDE_IN_CASETREE, OnBnClickedHideInCaseTree)
    ON_BN_CLICKED(IDC_ALWAYS_VISUAL_VALUE, OnBnClickedAlwaysVisualValue)
    ON_BN_CLICKED(IDC_APPLY_TO_SELECTED_FIELDS, OnBnClickedFieldsChooser)
    ON_BN_CLICKED(IDC_APPLY_TO_ALL_FIELDS, OnBnClickedFieldsChooser)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


CMultipleFieldPropertiesDlg::CMultipleFieldPropertiesDlg(
    std::vector<CDEField*> fields,
    const std::function<std::vector<CDEField*>()>* all_fields_getter,
    CWnd* pParent /*=NULL*/)
    :   CDialog(CMultipleFieldPropertiesDlg::IDD, pParent),
        m_fieldSetIndex(0),
        m_allFieldsGetter(all_fields_getter),
        m_iCaptureType(CAPTURETYPE_NO_CHANGE),
        m_iValidationMethod(0),
        m_iScreenText(0)
{
    m_fieldSets.emplace_back(fields);
}


BOOL CMultipleFieldPropertiesDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    if( m_allFieldsGetter != nullptr )
    {
        ((CButton*)GetDlgItem(IDC_APPLY_TO_SELECTED_FIELDS))->SetCheck(BST_CHECKED);
    }

    else
    {
        ((CButton*)GetDlgItem(IDC_APPLY_TO_SELECTED_FIELDS))->EnableWindow(FALSE);
        ((CButton*)GetDlgItem(IDC_APPLY_TO_ALL_FIELDS))->SetCheck(BST_CHECKED);
    }

    SetPropertiesBasedOnFieldValues();

    return TRUE;
}


void CMultipleFieldPropertiesDlg::SetTriStateCheckbox(UINT rID, int* value, int numSelected, int numTotal)
{
    *value = ( numSelected == 0 )        ? BST_UNCHECKED :
             ( numSelected == numTotal ) ? BST_CHECKED :
                                           BST_INDETERMINATE;

    ((CButton*)GetDlgItem(rID))->SetCheck(*value);
}

void CMultipleFieldPropertiesDlg::SetPropertiesBasedOnFieldValues()
{
    // calculate all of the states
    int numFields = 0;
    int numProtected = 0;
    int numUpperCase = 0;
    int numEnterKey = 0;
    int numVerify = 0;
    int numHideInCaseTree = 0;
    int numAlwaysVisualValue = 0;
    int numAlphaFields = 0;
    std::set<int> valid_capture_types;
    std::set<int> capture_types_used;
    std::set<ValidationMethod> validation_methods_used;
    std::set<FieldLabelType> field_labels_types_used;
    std::set<UINT> klids_used;

    valid_capture_types.insert((int)CaptureType::Unspecified);

    for( const CDEField* pField : GetFields() )
    {
        if( pField->IsMirror() )
            continue;

        const CDictItem* pDictItem = pField->GetDictItem();

        ++numFields;

        numProtected += pField->IsProtected();
        numUpperCase += pField->IsUpperCase();
        numEnterKey += pField->IsEnterKeyRequired();
        numVerify += pField->GetVerifyFlag();
        numHideInCaseTree += pField->IsHiddenInCaseTree();
        numAlwaysVisualValue += pField->IsAlwaysVisualValue();

        if( pDictItem->GetContentType() == ContentType::Alpha )
        {
            ++numAlphaFields;
            valid_capture_types.insert(CAPTURETYPE_TEXTBOX_NO_TICKMARKS);
            valid_capture_types.insert(CAPTURETYPE_TEXTBOX_MULTILINE);
        }

        for( int capture_type = (int)CaptureType::FirstDefined; capture_type <= (int)CaptureType::LastDefined; ++capture_type )
        {
            if( CaptureInfo::IsCaptureTypePossible(*pDictItem, (CaptureType)capture_type) )
                valid_capture_types.insert(capture_type);
        }

        int this_capture_type = (int)pField->GetCaptureInfo().GetCaptureType();

        if( this_capture_type == (int)CaptureType::TextBox )
        {
            if( pField->AllowMultiLine() )
                this_capture_type = CAPTURETYPE_TEXTBOX_MULTILINE;

            else if( pField->UseUnicodeTextBox() )
                this_capture_type = CAPTURETYPE_TEXTBOX_NO_TICKMARKS;
        }

        capture_types_used.insert(this_capture_type);

        validation_methods_used.insert(pField->GetValidationMethod());
        field_labels_types_used.insert(pField->GetFieldLabelType());
        klids_used.insert(pField->GetKLID());
    }


    // set the checkboxes
    SetTriStateCheckbox(IDC_PROTECTED, &m_iProtected, numProtected, numFields),
    SetTriStateCheckbox(IDC_UPPERCASE, &m_iUpperCase, numUpperCase, numAlphaFields),
    SetTriStateCheckbox(IDC_ENTERKEY, &m_iEnterKey, numEnterKey, numFields),
    SetTriStateCheckbox(IDC_VERIFY, &m_iVerify, numVerify, numFields),
    SetTriStateCheckbox(IDC_MULT_HIDE_IN_CASETREE, &m_iHideInCaseTree, numHideInCaseTree, numFields);
    SetTriStateCheckbox(IDC_ALWAYS_VISUAL_VALUE, &m_iAlwaysVisualValue, numAlwaysVisualValue, numFields);

    GetDlgItem(IDC_UPPERCASE)->EnableWindow(( numAlphaFields > 0 ));


    // set the capture types
    CComboBox* pComboBox = (CComboBox*)GetDlgItem(IDC_CAPTURETYPE);
    pComboBox->ResetContent();

    pComboBox->SetItemDataPtr(pComboBox->AddString(_T("<no change>")), (void*)CAPTURETYPE_NO_CHANGE);
    pComboBox->SetItemDataPtr(pComboBox->AddString(_T("<default for field>")), (void*)CAPTURETYPE_DEFAULT);
    pComboBox->SetItemDataPtr(pComboBox->AddString(_T("<linked to dictionary item when possible>")), (void*)CAPTURETYPE_LINK_TO_DICT_IF_DEFINED);

    for( int int_capture_type : valid_capture_types )
    {
        const TCHAR* text = ( int_capture_type == CAPTURETYPE_TEXTBOX_NO_TICKMARKS ) ? CAPTURETYPE_TEXTBOX_NO_TICKMARKS_DESCRIPTION :
                            ( int_capture_type == CAPTURETYPE_TEXTBOX_MULTILINE )    ? CAPTURETYPE_TEXTBOX_MULTILINE_DESCRIPTION :
                            ( int_capture_type == (int)CaptureType::Unspecified )    ? CAPTURETYPE_UNASSIGNED_DESCRIPTION :
                                                                                       CaptureInfo::GetCaptureTypeName((CaptureType)int_capture_type, true);

        int index = pComboBox->AddString(text);
        pComboBox->SetItemDataPtr(index, (void*)int_capture_type);
    }

    int capture_type_to_select = ( capture_types_used.size() == 1 ) ? *capture_types_used.begin() :
                                                                      CAPTURETYPE_NO_CHANGE;

    for( int i = 0; i < pComboBox->GetCount(); ++i )
    {
        if( capture_type_to_select == (int)pComboBox->GetItemDataPtr(i) )
        {
            pComboBox->SetCurSel(i);
            break;
        }
    }


    // set the other combo boxes
    m_iValidationMethod = ( validation_methods_used.size() == 1 ) ? ( (int)*(validation_methods_used.begin()) + 1 ) : 0;
    ((CComboBox*)GetDlgItem(IDC_VALIDATION_METHOD))->SetCurSel(m_iValidationMethod);

    m_iScreenText = ( field_labels_types_used.size() == 1 ) ? ( (int)*(field_labels_types_used.begin()) + 1 ) : 0;
    ((CComboBox*)GetDlgItem(IDC_SCREEN_TEXT))->SetCurSel(m_iScreenText);


    // set the keyboard input
    pComboBox = (CComboBox*)GetDlgItem(IDC_KEYBOARDINPUT);
    pComboBox->ResetContent();
    int keyboard_selection_index = 0;

    pComboBox->SetItemDataPtr(pComboBox->AddString(_T("<no change>")), (void*)HKL_NEXT); // HKL_NEXT will signify no change

    int iNumEnumeratedHKLs = GetKeyboardLayoutList(0, NULL);
    HKL* pEnumeratedHKLs = new HKL[iNumEnumeratedHKLs];
    GetKeyboardLayoutList(iNumEnumeratedHKLs, pEnumeratedHKLs);

    for( int i = -1; i < iNumEnumeratedHKLs; i++ )
    {
        HKL hKL = ( i == -1 ) ? nullptr : pEnumeratedHKLs[i];
        int posNum = pComboBox->AddString(CKeyboardInputDlg::GetDisplayNameHKL(hKL));
        pComboBox->SetItemDataPtr(posNum, hKL);

        // for speed reasons, we'll just work with HKLs and then convert the selected value to a KLID after the operation
        if( keyboard_selection_index == 0 && klids_used.size() == 1 && CKeyboardInputDlg::HKL2KLID(hKL) == *klids_used.begin() )
            keyboard_selection_index = posNum;
    }

    delete [] pEnumeratedHKLs;

    pComboBox->SetCurSel(keyboard_selection_index);
}


void CMultipleFieldPropertiesDlg::OnBnClickedFieldsChooser()
{
    if( m_allFieldsGetter != nullptr )
    {
        if( ((CButton*)GetDlgItem(IDC_APPLY_TO_ALL_FIELDS))->GetCheck() == BST_CHECKED )
        {
            // get all the fields if they haven't already been gotten
            if( m_fieldSets.size() != 2 )
                m_fieldSets.emplace_back((*m_allFieldsGetter)());

            m_fieldSetIndex = 1;
        }

        else
            m_fieldSetIndex = 0;
    }

    SetPropertiesBasedOnFieldValues();
}


void CMultipleFieldPropertiesDlg::ProcessClick(UINT rID)
{
    CButton* pButton = (CButton*)GetDlgItem(rID);

    if( pButton->GetCheck() != BST_CHECKED )
        pButton->SetCheck(BST_UNCHECKED);

    else
        pButton->SetCheck(BST_CHECKED);
}


void CMultipleFieldPropertiesDlg::OnBnClickedOk()
{
    m_iProtected = ((CButton*)GetDlgItem(IDC_PROTECTED))->GetCheck();
    m_iUpperCase = ((CButton*)GetDlgItem(IDC_UPPERCASE))->GetCheck();
    m_iEnterKey = ((CButton*)GetDlgItem(IDC_ENTERKEY))->GetCheck();
    m_iVerify = ((CButton*)GetDlgItem(IDC_VERIFY))->GetCheck();
    m_iHideInCaseTree = ((CButton*)GetDlgItem(IDC_MULT_HIDE_IN_CASETREE))->GetCheck();
    m_iAlwaysVisualValue = ((CButton*)GetDlgItem(IDC_ALWAYS_VISUAL_VALUE))->GetCheck();


    CComboBox* pComboBox = (CComboBox*)GetDlgItem(IDC_CAPTURETYPE);
    m_iCaptureType = (int)pComboBox->GetItemDataPtr(pComboBox->GetCurSel());

    m_iValidationMethod = ((CComboBox*)GetDlgItem(IDC_VALIDATION_METHOD))->GetCurSel();

    m_iScreenText = ((CComboBox*)GetDlgItem(IDC_SCREEN_TEXT))->GetCurSel();


    // HKL_NEXT means <no change> ... if not, we need to convert the HKL to a KLID
    pComboBox = (CComboBox*)GetDlgItem(IDC_KEYBOARDINPUT);
    HKL hKL = (HKL)pComboBox->GetItemDataPtr(pComboBox->GetCurSel());
    m_KLID = ( hKL == (void*)HKL_NEXT ) ? (UINT)HKL_NEXT : CKeyboardInputDlg::HKL2KLID(hKL);

    CDialog::OnOK();
}


bool CMultipleFieldPropertiesDlg::ApplyCaptureType() const
{
    return ( m_iCaptureType != CAPTURETYPE_NO_CHANGE );
}

bool CMultipleFieldPropertiesDlg::UseDefaultCaptureType() const
{
    return ( m_iCaptureType == CAPTURETYPE_DEFAULT );
}

bool CMultipleFieldPropertiesDlg::LinkToDictionaryCaptureTypeWhenPossible() const
{
    return ( m_iCaptureType == CAPTURETYPE_LINK_TO_DICT_IF_DEFINED );
}

bool CMultipleFieldPropertiesDlg::GetUseUnicodeTextBox() const
{
    return ( m_iCaptureType == CAPTURETYPE_TEXTBOX_NO_TICKMARKS );
}

bool CMultipleFieldPropertiesDlg::GetMultiLineOption() const
{
    return ( m_iCaptureType == CAPTURETYPE_TEXTBOX_MULTILINE );
}

CaptureType CMultipleFieldPropertiesDlg::GetCaptureType() const
{
    if( m_iCaptureType == CAPTURETYPE_TEXTBOX_NO_TICKMARKS || m_iCaptureType == CAPTURETYPE_TEXTBOX_MULTILINE )
    {
        return CaptureType::TextBox;
    }

    else
    {
        ASSERT(m_iCaptureType >= (int)CaptureType::Unspecified);
        return (CaptureType)m_iCaptureType;
    }
}
