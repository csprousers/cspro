#include "StdAfx.h"
#include "FieldPropertiesDlg.h"
#include "KeyboardInputDlg.h"


BEGIN_MESSAGE_MAP(CFieldPropDlg, CDialog)
    ON_BN_CLICKED(IDC_PROTECTED, OnProtected)
    ON_BN_CLICKED(IDC_PERSISTENT, OnPersistent)
    ON_BN_CLICKED(IDC_AUTOINCREMENT, OnAutoIncrement)
    ON_BN_CLICKED(IDC_CHANGE_KEYBOARD, OnBnClickedChangeKeyboard)
    ON_BN_CLICKED(IDC_LINKED_TO_DICT, OnLinkedToDict)
    ON_EN_CHANGE(IDC_FLDTXT, OnChangeLabel)
    ON_CBN_SELCHANGE(IDC_CAPTURE_TYPE, OnCbnSelchangeCaptureInfo)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////

CFieldPropDlg::CFieldPropDlg(CDEField* pField, CFormScrollView* pParent)
    :   CDialog(CFieldPropDlg::IDD, pParent)
{
    m_pField = pField;
    m_sFieldName = pField->GetName();
    m_sFldTxt = pField->GetCDEText().GetText();

    m_eTxtUse = pParent->GetDragOptions().GetTextUse();
    m_bTextLinkedToDictionary = ( pField->GetFieldLabelType() != FieldLabelType::Custom );

    m_bEnterKey          = pField->IsEnterKeyRequired();
    m_bSequential        = pField->IsSequential();
    m_bMirror            = pField->IsMirror();
    m_bProtected         = pField->IsProtected();
    m_bPersist           = pField->IsPersistent();
    m_bAutoIncrement     = pField->IsAutoIncrement();
    m_bUpperCase         = pField->IsUpperCase();
    m_bHideInCaseTree    = pField->IsHiddenInCaseTree();
    m_bAlwaysVisualValue = pField->IsAlwaysVisualValue();

    m_pMyParent = pParent;

    const DictLevel* dict_level;
    const CDictRecord* dict_record;

    const CDataDict* pDD = m_pMyParent->GetDocument()->GetSharedDictionary().get();

    bool bFound = pDD->LookupName<CDictItem>(pField->GetItemName(), &dict_level, &dict_record, &m_pDictItem);
    ASSERT(bFound);

    m_bIDItem = ( dict_record->GetSonNumber() == COMMON );
    m_bItemOnFirstLevel = ( dict_level->GetLevelNumber() == 0 );
    m_bRepeatingItem = ( m_pDictItem->GetOccurs() > 1 || dict_record->GetMaxRecs() > 1 ); // 20120504
    m_bVerify = m_pField->GetVerifyFlag();
    m_eValidationMethod = m_pField->GetValidationMethod();
    m_KLID = m_pField->GetKLID();
}

/////////////////////////////////////////////////////////////////////////////

void CFieldPropDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_FLDNAME, m_sFieldName);

    DDV_MaxChars(pDX, m_sFldTxt, 256);

    if( pDX->m_bSaveAndValidate ) // 20130506
    {
        DDX_Text(pDX, IDC_FLDTXT, m_sFldTxt);
        m_sFldTxt = DelimitCRLF(m_sFldTxt);
    }

    else
    {
        CString csUndelimitedText = UndelimitCRLF(m_sFldTxt);
        DDX_Text(pDX, IDC_FLDTXT, csUndelimitedText);
    }

    DDX_Check(pDX, IDC_LINKED_TO_DICT, m_bTextLinkedToDictionary);

    DDX_Control(pDX, IDC_SKIPTO, m_cmbFldSel);

    DDX_Control(pDX, IDC_CAPTURE_TYPE, m_cmbCaptureType);
    DDX_Control(pDX, IDC_DATE_FORMAT, m_cmbCaptureTypeDateFormat);

    DDX_Control(pDX, IDC_VALIDATION_METHOD, m_cmbValidationMethod);

    DDX_Check(pDX, IDC_ENTERKEY, m_bEnterKey);
    DDX_Check(pDX, IDC_PROTECTED, m_bProtected);
    DDX_Check(pDX, IDC_HIDE_IN_CASETREE, m_bHideInCaseTree);
    DDX_Check(pDX, IDC_ALWAYS_VISUAL_VALUE, m_bAlwaysVisualValue);
    DDX_Check(pDX, IDC_SEQUENTIAL, m_bSequential);
    DDX_Check(pDX, IDC_MIRROR, m_bMirror);
    DDX_Check(pDX, IDC_PERSISTENT, m_bPersist);
    DDX_Check(pDX, IDC_AUTOINCREMENT, m_bAutoIncrement);
    DDX_Check(pDX, IDC_UPPERCASE, m_bUpperCase);
    DDX_Check(pDX, IDC_VERIFY, m_bVerify);
    DDX_Text(pDX, IDC_KEYBOARD_DESC, m_sKeyboardDescription);
}

/////////////////////////////////////////////////////////////////////////////

BOOL CFieldPropDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_pCaptureErrorIcon = (CStatic*)GetDlgItem(IDC_CAPTURE_ERROR_ICON);
    m_pCaptureErrorText = (CStatic*)GetDlgItem(IDC_CAPTURE_ERROR_TEXT);

    HICON hIcon = (HICON)LoadImage(zFormF_hInstance, MAKEINTRESOURCE(IDI_CAPTURE_ERROR), IMAGE_ICON, 16, 16, LR_SHARED);
    m_pCaptureErrorIcon->SetIcon(hIcon);

    // populate the capture type options (and then simulate changing the capture info
    // so that the proper informational windows are shown/hidden)
    m_captureInfo = m_pField->GetCaptureInfo();
    m_bUseUnicodeTextBox = m_pField->UseUnicodeTextBox();
    m_bMultiLineOption = m_pField->AllowMultiLine();

    PopulateCaptureInfo();
    OnCbnSelchangeCaptureInfo();

    m_sKeyboardDescription = CKeyboardInputDlg::GetDisplayName(m_pField->GetKLID());

    UpdateData(false);
    CDEFormFile* pFF = &m_pMyParent->GetDocument()->GetFormFile();
    bool bSkip = !pFF->IsPathOn();

    if( bSkip )
        BuildSkipToSel();


    auto disable_windows = [&](std::initializer_list<int> dialog_ids)
    {
        for( int id : dialog_ids )
            GetDlgItem(id)->EnableWindow(FALSE);
    };

    if( m_bProtected )
        disable_windows({ IDC_ENTERKEY });

    if( m_bMirror )
    {
        disable_windows({ IDC_ENTERKEY, IDC_PROTECTED, IDC_SEQUENTIAL,
                          IDC_UPPERCASE, IDC_VERIFY, IDC_HIDE_IN_CASETREE, IDC_ALWAYS_VISUAL_VALUE,
                          IDC_CAPTURE_TYPE, IDC_VALIDATION_METHOD, IDC_CHANGE_KEYBOARD });
    }

    //for ID Items switch off sequential fields
    if( m_bIDItem || !m_bRepeatingItem || m_pDictItem->GetContentType() != ContentType::Numeric )
        disable_windows({ IDC_SEQUENTIAL });

    if( m_pDictItem->GetContentType() != ContentType::Alpha )
        disable_windows({ IDC_UPPERCASE });

    EnablePersistentAutoIncrementCheckboxes();

    // the "Mirror" checkbox is always disabled, user can not change this
    disable_windows({ IDC_MIRROR });

/*
keep this around as an example of how to default enum type strings;

    if (true)
        SetDlgItemText (pCB->GetDlgCtrlID(), "Mirror");

    else    // it's Keyed or Protected
    {
        pCB = (CComboBox*)GetDlgItem(IDC_INPUTTYPE);

        pCB->AddString ("Keyed");
        pCB->AddString ("Protected");

        if (m_eIT == Keyed)

            pCB->SetCurSel (0);
        else
            pCB->SetCurSel (1);
    }
*/
    CFormDoc* pDoc = m_pMyParent->GetDocument();

    if(pDoc->GetFormFile().GetDefaultTextFont().IsArabic()) {
        GetDlgItem(IDC_FLDTXT)->SetFont(&pDoc->GetFormFile().GetDefaultTextFont().GetCFont());
    }


//Fill the combo stuff with the values
//Find the Current sel

    if(bSkip && !m_bMirror){
        m_iCurSkipSel = -1;

        auto add_target = [&](const CString& name, CDEItemBase* pBase)
        {
            int iPos = m_cmbFldSel.AddString(name);
            m_cmbFldSel.SetItemDataPtr(iPos, pBase);

            if( m_pField->GetPlusTarget().CompareNoCase(name) == 0 )
                m_iCurSkipSel = iPos;
        };

        for( int iIndex = 0; iIndex < m_arrFieldSel.GetSize(); iIndex++ )
        {
            CDEItemBase* pBase = m_arrFieldSel[iIndex];
            CString name = ( pBase != nullptr ) ? pBase->GetName() : CString();
            add_target(name, pBase);
        }

        if( m_arrFieldSel.GetSize() > 0 )
            add_target(_T("<END>"), nullptr);

        m_cmbFldSel.SetCurSel(m_iCurSkipSel);
    }

    else {
        m_cmbFldSel.EnableWindow(FALSE);
    }

    m_cmbValidationMethod.SetCurSel((int)m_eValidationMethod);

    return true;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// CFormScrollView::OnEditFieldProp() handles adding (if nec) the new field to the form
// as well as modifying any vals changed herein

// still have to figure out the adding it to a dictionary

void CFieldPropDlg::OnOK()
{
    CDialog::OnOK();    // mfc-gen; it will invoke my DoDataExchange func

    int iCurSel = m_cmbFldSel.GetCurSel();
    if(m_iCurSkipSel != iCurSel) {
       m_pMyParent->GetDocument()->SetModifiedFlag(true);
    }

    if(iCurSel >= 0) {
        CDEItemBase* pBase = (CDEItemBase*)m_cmbFldSel.GetItemDataPtr(iCurSel);
        if(pBase) {
            m_pField->SetPlusTarget(pBase->GetName());
        }
        else {
            CString sString;
            m_cmbFldSel.GetLBText(iCurSel, sString);
            m_pField->SetPlusTarget(sString);
        }
    }

    if( m_captureInfo.GetCaptureType() == CaptureType::Date )
    {
        CString date_format;
        m_cmbCaptureTypeDateFormat.GetLBText(m_cmbCaptureTypeDateFormat.GetCurSel(), date_format);

        m_captureInfo.GetExtended<DateCaptureInfo>().SetFormat(date_format);
        ASSERT(m_captureInfo == m_captureInfo.MakeValid(*m_pDictItem, nullptr, false));
    }

    m_eValidationMethod = (ValidationMethod)m_cmbValidationMethod.GetCurSel();
}


/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////
//
//  void CFieldPropDlg::OnProtected()
//
/////////////////////////////////////////////////////////////////////////////////
void CFieldPropDlg::OnProtected()
{
    m_bProtected = !m_bProtected;   // toggle the local member
    CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_ENTERKEY);
    pCB->EnableWindow(!m_bProtected);
}


void CFieldPropDlg::OnPersistent()
{
    m_bPersist = !m_bPersist;
    EnablePersistentAutoIncrementCheckboxes();
}

void CFieldPropDlg::OnAutoIncrement()
{
    m_bAutoIncrement = !m_bAutoIncrement;
    EnablePersistentAutoIncrementCheckboxes();
}

void CFieldPropDlg::EnablePersistentAutoIncrementCheckboxes()
{
    bool bEnablePersistent = false;
    bool bEnableAutoIncrement = false;

    if( m_bIDItem && !m_bMirror )
    {
        bEnablePersistent = ( m_bPersist || !m_bAutoIncrement );
        bEnableAutoIncrement = ( m_bItemOnFirstLevel &&
                                 m_pDictItem->GetContentType() == ContentType::Numeric &&
                                 ( m_bAutoIncrement || !m_bPersist ) );
    }

    ((CComboBox*)GetDlgItem(IDC_PERSISTENT))->EnableWindow(bEnablePersistent);
    ((CComboBox*)GetDlgItem(IDC_AUTOINCREMENT))->EnableWindow(bEnableAutoIncrement);
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CFieldPropDlg::BuildSkipToSel()
//
/////////////////////////////////////////////////////////////////////////////////
void CFieldPropDlg::BuildSkipToSel()
{
    m_arrFieldSel.RemoveAll();

    //Now for each group go through all the fields and get the selection set;
    const CDEGroup* pGroup = m_pField->GetParent();
    bool reached_current_field = false;

    for( int iIndex = 0; iIndex < pGroup->GetNumItems(); iIndex++ )
    {
        CDEItemBase* pBase = pGroup->GetItem(iIndex);

        if( reached_current_field )
        {
            bool add_entity =
                pBase->IsKindOf(RUNTIME_CLASS(CDEGroup)) ||
                pBase->IsKindOf(RUNTIME_CLASS(CDEBlock)) ||
                ( pBase->IsKindOf(RUNTIME_CLASS(CDEField)) && !((CDEField*)pBase)->IsMirror() );

            if( add_entity )
            {
                if( m_arrFieldSel.GetSize() == 0 )
                    m_arrFieldSel.InsertAt(0, (CDEItemBase*)NULL);

                m_arrFieldSel.Add(pBase);
            }
        }

        else
        {
            reached_current_field = ( pBase->GetName().CompareNoCase(m_pField->GetName()) == 0 );
        }
    }
}


void CFieldPropDlg::OnBnClickedChangeKeyboard() // 20120817
{
    CKeyboardInputDlg dlg;

    dlg.SetKLID(m_KLID);

    if( dlg.DoModal() == IDOK )
    {
        m_KLID = dlg.GetKLID();
        m_sKeyboardDescription = CKeyboardInputDlg::GetDisplayName(m_KLID);
        UpdateData(FALSE);
    }

    GotoDlgCtrl((CButton *)GetDlgItem(IDOK));
}


void CFieldPropDlg::OnLinkedToDict()
{
    m_bTextLinkedToDictionary = !m_bTextLinkedToDictionary;

    if( m_bTextLinkedToDictionary )
    {
        m_sFldTxt = ( m_eTxtUse == CDEFormBase::TextUse::Label ) ? m_pField->GetLabel() : m_pField->GetName();
        GetDlgItem(IDC_FLDTXT)->SetWindowText(m_sFldTxt);
    }
}

void CFieldPropDlg::OnChangeLabel()
{
    // the label is no longer linked if the user modified it
    m_bTextLinkedToDictionary = false;
    ((CButton*)GetDlgItem(IDC_LINKED_TO_DICT))->SetCheck(BST_UNCHECKED);
}


void CFieldPropDlg::PopulateCaptureInfo()
{
    auto add_entry = [&](auto capture_type, const TCHAR* text, std::optional<bool> selected = std::nullopt)
    {
        int capture_type_pos = m_cmbCaptureType.AddString(text);
        m_cmbCaptureType.SetItemDataPtr(capture_type_pos, (void*)capture_type);

        if( !selected.has_value() )
            selected = ( (int)capture_type == (int)m_captureInfo.GetCaptureType() );

        if( *selected )
            m_cmbCaptureType.SetCurSel(capture_type_pos);
    };

    // add the Unspecified option (to use the dictionary item's setting)
    add_entry(CaptureType::Unspecified, CAPTURETYPE_UNASSIGNED_DESCRIPTION);

    // add the TextBox options
    if( CaptureInfo::IsCaptureTypePossible(*m_pDictItem, CaptureType::TextBox) )
    {
        bool textbox_selected = ( m_captureInfo.GetCaptureType() == CaptureType::TextBox );

        add_entry(CaptureType::TextBox, CaptureInfo::GetCaptureTypeName(CaptureType::TextBox, true),
            ( textbox_selected && !m_bUseUnicodeTextBox ));

        if( m_pDictItem->GetContentType() == ContentType::Alpha )
        {
            add_entry(CAPTURETYPE_TEXTBOX_NO_TICKMARKS, CAPTURETYPE_TEXTBOX_NO_TICKMARKS_DESCRIPTION,
                ( textbox_selected && m_bUseUnicodeTextBox && !m_bMultiLineOption));

            add_entry(CAPTURETYPE_TEXTBOX_MULTILINE, CAPTURETYPE_TEXTBOX_MULTILINE_DESCRIPTION,
                ( textbox_selected && m_bUseUnicodeTextBox && m_bMultiLineOption));
        }
    }

    // add the other capture types in order of use/commonality
#ifdef _DEBUG
    int specified_capture_types_added = 1;
#endif

    for( CaptureType capture_type : { CaptureType::RadioButton,
                                      CaptureType::ToggleButton,
                                      CaptureType::CheckBox,
                                      CaptureType::DropDown,
                                      CaptureType::ComboBox,
                                      CaptureType::Date,
                                      CaptureType::Slider,
                                      CaptureType::Barcode,
                                      CaptureType::Photo,
                                      CaptureType::Signature,
                                      CaptureType::Audio,
                                      CaptureType::NumberPad } )
    {
        if( CaptureInfo::IsCaptureTypePossible(*m_pDictItem, capture_type) )
            add_entry(capture_type, CaptureInfo::GetCaptureTypeName(capture_type, true));

#ifdef _DEBUG
        ++specified_capture_types_added;
#endif
    }

    ASSERT(specified_capture_types_added == ( (int)CaptureType::LastDefined + 1 ));


    // add the date formats
    for( const TCHAR* date_format : DateCaptureInfo::GetPossibleFormats(*m_pDictItem) )
        m_cmbCaptureTypeDateFormat.AddString(date_format);
}



void CFieldPropDlg::OnCbnSelchangeCaptureInfo()
{
    int int_capture_type = (int)m_cmbCaptureType.GetItemDataPtr(m_cmbCaptureType.GetCurSel());

    if( int_capture_type == CAPTURETYPE_TEXTBOX_NO_TICKMARKS || int_capture_type == CAPTURETYPE_TEXTBOX_MULTILINE )
    {
        m_captureInfo.SetCaptureType(CaptureType::TextBox);
        m_bUseUnicodeTextBox = true;
        m_bMultiLineOption = ( int_capture_type == CAPTURETYPE_TEXTBOX_MULTILINE );
    }

    else
    {
        m_captureInfo.SetCaptureType((CaptureType)int_capture_type);
        m_bUseUnicodeTextBox = false;
        m_bMultiLineOption = false;
    }

    // conditionally show some extra information
    bool is_unspecified = ( m_captureInfo.GetCaptureType() == CaptureType::Unspecified );
    bool is_date = ( m_captureInfo.GetCaptureType() == CaptureType::Date );

    CWnd* pCaptureDescriptionWnd = GetDlgItem(IDC_CAPTURE_DESCRIPTION);
    pCaptureDescriptionWnd->ShowWindow(is_unspecified);
    GetDlgItem(IDC_STATIC_DATE_FORMAT)->ShowWindow(is_date);
    GetDlgItem(IDC_DATE_FORMAT)->ShowWindow(is_date);

    if( is_date )
    {
        DateCaptureInfo& date_capture_info = m_captureInfo.GetExtended<DateCaptureInfo>();

        while( true )
        {
            bool date_format_index_selected = false;

            for( int i = 0; i < m_cmbCaptureTypeDateFormat.GetCount(); ++i )
            {
                CString date_format;
                m_cmbCaptureTypeDateFormat.GetLBText(i, date_format);

                if( date_capture_info.GetFormat() == date_format )
                {
                    date_format_index_selected = true;
                    m_cmbCaptureTypeDateFormat.SetCurSel(i);
                    break;
                }
            }

            // this may not happen; it would if an invalid date format was somehow set
            if( !date_format_index_selected )
                date_capture_info.SetFormat(date_capture_info.GetDefaultFormat(*m_pDictItem));

            else
                break;
        }
    }

    const CaptureInfo& source_capture_info = is_unspecified ? m_pDictItem->GetCaptureInfo() : m_captureInfo;

    CaptureInfo valid_capture_info = source_capture_info.MakeValid(*m_pDictItem, m_pDictItem->GetFirstValueSetOrNull(), false);

    bool capture_info_is_not_fully_valid = ( source_capture_info != valid_capture_info );
    bool show_invalid_capture_type_warning = false;

    if( is_unspecified )
    {
        pCaptureDescriptionWnd->SetWindowText(FormatText(_T("%s%s"),
            capture_info_is_not_fully_valid ? _T("Evaluates to: ") : _T(""),
            valid_capture_info.GetDescription().GetString()));
    }

    else if( capture_info_is_not_fully_valid )
    {
        ASSERT(!is_date);
        show_invalid_capture_type_warning = true;
    }

    m_pCaptureErrorIcon->ShowWindow(show_invalid_capture_type_warning);
    m_pCaptureErrorText->ShowWindow(show_invalid_capture_type_warning);
}
