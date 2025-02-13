#include "StdAfx.h"
#include "CommonStoreDlg.h"


BEGIN_MESSAGE_MAP(CommonStoreDlg, CDialog)
    ON_BN_CLICKED(IDC_RADIO_COMMON_STORE_ADD, OnBnClickedRadioCommonStoreAdd)
    ON_BN_CLICKED(IDC_BUTTON_COMMON_STORE_MODIFY, OnBnClickedButtonCommonStoreModify)
    ON_BN_CLICKED(IDC_BUTTON_COMMON_STORE_DELETE, OnBnClickedButtonCommonStoreDelete)
    ON_NOTIFY(LVN_ITEMCHANGED,IDC_LIST_COMMON_STORE, OnLvnItemchangedListCommonStore)
    ON_CBN_SELCHANGE(IDC_COMBO_COMMON_STORE_TYPE, OnCbnSelchangeComboCommonStoreType)
END_MESSAGE_MAP()


CommonStoreDlg::CommonStoreDlg(CWnd* pParent/* = nullptr*/)
    :   CDialog(IDD_COMMON_STORE, pParent),
        m_selectedItem(-1),
        m_pListCtrl(nullptr),
        m_pComboBoxType(nullptr)
{
}


void CommonStoreDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_EDIT_COMMON_STORE_ATTRIBUTE, m_attribute);
    DDX_Text(pDX, IDC_EDIT_COMMON_STORE_VALUE, m_value);
}


BOOL CommonStoreDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    if( !m_commonStore.Open({ CommonStore::TableType::UserSettings,
                              CommonStore::TableType::ConfigVariables }) )
    {
        AfxMessageBox(_T("There was a problem opening the Common Store settings file."));
        PostMessage(WM_CLOSE);
    }

    else
    {
        // set up the list control columns and allow full row selection
        m_pListCtrl = (CListCtrl*)GetDlgItem(IDC_LIST_COMMON_STORE);
        m_pListCtrl->InsertColumn(0, _T("Attribute"));
        m_pListCtrl->SetColumnWidth(0, 160);
        m_pListCtrl->InsertColumn(1, _T("Value"));
        m_pListCtrl->SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);

        m_pListCtrl->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

        m_pComboBoxType = (CComboBox*)GetDlgItem(IDC_COMBO_COMMON_STORE_TYPE);
        m_pComboBoxType->SetCurSel(0);
        OnCbnSelchangeComboCommonStoreType();
    }

    return TRUE;
}


void CommonStoreDlg::OnCbnSelchangeComboCommonStoreType()
{
    // populate the list control with the settings
    m_pListCtrl->DeleteAllItems();

    m_commonStore.SwitchTable(( m_pComboBoxType->GetCurSel() == 0 ) ? CommonStore::TableType::UserSettings :
                                                                      CommonStore::TableType::ConfigVariables);

    std::wstring attribute;
    std::wstring value;

    while( m_commonStore.NextString(&attribute, &value) )
        AddSetting(attribute.c_str(), value.c_str());

    // start in Add mode
    OnBnClickedRadioCommonStoreAdd();
}


int CommonStoreDlg::AddSetting(const TCHAR* attribute, const TCHAR* value)
{
    int item = m_pListCtrl->InsertItem(0, attribute);
    m_pListCtrl->SetItemText(item, 1, value);
    return item;
}


void CommonStoreDlg::UpdateSelections()
{
    bool adding_setting = ( m_selectedItem < 0 );

    CheckRadioButton(IDC_RADIO_COMMON_STORE_ADD,IDC_RADIO_COMMON_STORE_MODIFY,
        adding_setting ? IDC_RADIO_COMMON_STORE_ADD : IDC_RADIO_COMMON_STORE_MODIFY);

    GetDlgItem(IDC_BUTTON_COMMON_STORE_MODIFY)->SetWindowText(adding_setting ? _T("Add") : _T("Modify"));
    GetDlgItem(IDC_RADIO_COMMON_STORE_MODIFY)->EnableWindow(!adding_setting);
    GetDlgItem(IDC_BUTTON_COMMON_STORE_DELETE)->EnableWindow(!adding_setting);

    if( adding_setting )
    {
        POSITION pos = m_pListCtrl->GetFirstSelectedItemPosition();

        while( pos != nullptr )
        {
            int item = m_pListCtrl->GetNextSelectedItem(pos);
            m_pListCtrl->SetItemState(item, 0, LVIS_SELECTED);
        }
    }

    m_attribute = adding_setting ? _T("") : m_pListCtrl->GetItemText(m_selectedItem, 0);
    m_value = adding_setting ? _T("") : m_pListCtrl->GetItemText(m_selectedItem, 1);

    UpdateData(FALSE);

    GetDlgItem(adding_setting ? IDC_EDIT_COMMON_STORE_ATTRIBUTE : IDC_EDIT_COMMON_STORE_VALUE)->SetFocus();
}


void CommonStoreDlg::OnBnClickedRadioCommonStoreAdd()
{
    m_selectedItem = -1;
    UpdateSelections();
}


void CommonStoreDlg::OnLvnItemchangedListCommonStore(NMHDR *pNMHDR,LRESULT *pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

    if( ( pNMLV->uChanged & LVIF_STATE ) == LVIF_STATE &&
        ( pNMLV->uNewState & LVIS_SELECTED ) == LVIS_SELECTED && ( pNMLV->uOldState & LVIS_SELECTED ) == 0 )
    {
        m_selectedItem = pNMLV->iItem;
        UpdateSelections();
    }

    *pResult = 0;
}


void CommonStoreDlg::OnBnClickedButtonCommonStoreModify()
{
    UpdateData(TRUE);

    try
    {
        if( m_attribute.IsEmpty() || m_value.IsEmpty() )
            throw CSProException("You must specify both an attribute and a value.");

        bool adding_setting = ( m_selectedItem < 0 );
        bool modify_setting_changing_attribute = !adding_setting && ( m_attribute.Compare(m_pListCtrl->GetItemText(m_selectedItem, 0)) != 0 );

        if( adding_setting || modify_setting_changing_attribute )
        {
            if( m_commonStore.Exists(m_attribute) )
                throw CSProException("A setting with the specified attribute already exists.");
        }

        bool success = true;

        // delete the existing setting
        if( !adding_setting ) 
            success = m_commonStore.Delete(m_pListCtrl->GetItemText(m_selectedItem, 0));

        // add the setting
        if( success )
            success = m_commonStore.PutString(m_attribute, m_value);

        if( !success )
            throw CSProException(_T("There was an error %s the setting."), adding_setting ? _T("adding") : _T("modifying"));

        if( adding_setting )
        {
            AddSetting(m_attribute, m_value);
            UpdateSelections();
        }

        else if( modify_setting_changing_attribute )
        {
            m_pListCtrl->DeleteItem(m_selectedItem);
            m_selectedItem = AddSetting(m_attribute, m_value);
            m_pListCtrl->SetItemState(m_selectedItem, LVIS_SELECTED, LVIS_SELECTED);
        }

        else
        {
            m_pListCtrl->SetItemText(m_selectedItem, 1, m_value);
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void CommonStoreDlg::OnBnClickedButtonCommonStoreDelete()
{
    CString attribute = m_pListCtrl->GetItemText(m_selectedItem, 0);

    if( m_commonStore.Delete(attribute) )
    {
        m_pListCtrl->DeleteItem(m_selectedItem);
        --m_selectedItem;

        if( m_selectedItem >= 0 )
            m_pListCtrl->SetItemState(m_selectedItem, LVIS_SELECTED, LVIS_SELECTED);

        else
            UpdateSelections();
    }

    else
    {
        AfxMessageBox(_T("There was an error deleting the setting."));
    }
}
