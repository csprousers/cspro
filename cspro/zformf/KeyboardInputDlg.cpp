#include "StdAfx.h"
#include "KeyboardInputDlg.h"
#include <zUtilF/KeyboardLoader.h>


BEGIN_MESSAGE_MAP(KeyboardInputDlg, CDialog)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_KEYBOARD_LIST, OnItemChanged)
END_MESSAGE_MAP()


KeyboardInputDlg::KeyboardInputDlg(const UINT klid, CWnd* const pParent/*= nullptr*/)
    :   CDialog(IDD_KEYBOARD_LAYOUTS, pParent),
        m_keyboardLayouts(KeyboardLoader::GetKeyboardLayouts(true, false)),
        m_klid(klid),
        m_selectedKlid(0)
{
}


void KeyboardInputDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_KEYBOARD_LIST, m_klidList);
}


BOOL KeyboardInputDlg::OnInitDialog()
{
    __super::OnInitDialog();

    m_klidList.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0 ,LVS_EX_FULLROWSELECT);

    m_klidList.InsertColumn(0, L"Language / Input Method", LVCFMT_LEFT, 185);
    m_klidList.InsertColumn(1, L"Country", LVCFMT_LEFT, 175);
    m_klidList.InsertColumn(2, L"Keyboard ID", LVCFMT_RIGHT, 85);

    int row = 0;
    std::optional<int> row_to_select;

    for( auto [hKL, display_name] : m_keyboardLayouts )
    {
        std::wstring country;

        const size_t left_parenthesis = display_name.find('(');
        size_t right_parenthesis;

        if( ( left_parenthesis != std::wstring::npos ) &&
            ( ( right_parenthesis = display_name.find(')', left_parenthesis + 1) ) != std::wstring::npos ) )
        {
            country = display_name.substr(left_parenthesis + 1, right_parenthesis - left_parenthesis - 1);
            display_name.erase(left_parenthesis);
        }

        const unsigned int klid = KeyboardLoader::GetKlidFromHKL(hKL);
        const std::wstring klid_text = FormatText(L"%u", klid);

        m_klidList.InsertItem(row, display_name.c_str());
        m_klidList.SetItemText(row, 1, country.c_str());
        m_klidList.SetItemText(row, 2, klid_text.c_str());
        m_klidList.SetItemData(row, klid);

        if( klid == m_klid )
            row_to_select = row;

        ++row;
    }

    if( row_to_select.has_value() )
        m_klidList.SetItemState(*row_to_select, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

    m_klidList.SetFocus();

    return FALSE;
}


void KeyboardInputDlg::OnItemChanged(NMHDR* const pNMHDR, LRESULT* const pResult)
{
    NMLISTVIEW* const pNMLV = reinterpret_cast<NMLISTVIEW*>(pNMHDR);

    if( ( pNMLV->uChanged & LVIF_STATE ) != 0 )
    {
        const bool selected = ( ( pNMLV->uNewState & LVNI_SELECTED ) != 0 );

        if( selected )
            m_selectedKlid = m_klidList.GetItemData(pNMLV->iItem);

        // enable the OK button only if something is selected
        GetDlgItem(IDOK)->EnableWindow(selected);
    }

    *pResult = 0;
}
