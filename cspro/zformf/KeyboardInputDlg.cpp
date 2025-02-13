// KeyboardInputDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "KeyboardInputDlg.h"


const int LanguageBufferSize = 512;
TCHAR * DefaultKeyboard      = _T("Default Keyboard");
TCHAR * KeyboardNotInstalled = _T("Keyboard Not Installed");

// CKeyboardInputDlg dialog

IMPLEMENT_DYNAMIC(CKeyboardInputDlg, CDialogEx)

BEGIN_MESSAGE_MAP(CKeyboardInputDlg, CDialogEx)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_KEYBOARD_LIST, &CKeyboardInputDlg::OnLvnItemchangedKeyboardList)
END_MESSAGE_MAP()

CKeyboardInputDlg::CKeyboardInputDlg(CWnd* pParent /*=NULL*/)
    : CDialogEx(CKeyboardInputDlg::IDD, pParent)
{
}

CKeyboardInputDlg::~CKeyboardInputDlg()
{
}

void CKeyboardInputDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}


BOOL CKeyboardInputDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    m_pList = (CListCtrl *)GetDlgItem(IDC_KEYBOARD_LIST);
    m_pList->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT);

    int iNumEnumeratedHKLs = GetKeyboardLayoutList(0,NULL);
    HKL * pEnumeratedHKLs = new HKL[iNumEnumeratedHKLs];
    GetKeyboardLayoutList(iNumEnumeratedHKLs,pEnumeratedHKLs);

    m_pList->InsertColumn(0,_T("Language / Input Method"),LVCFMT_LEFT,185);
    m_pList->InsertColumn(1,_T("Country"),LVCFMT_LEFT,175);
    m_pList->InsertColumn(2,_T("Keyboard ID"),LVCFMT_RIGHT,85);

    LVITEM lvItem;

    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = 0;
    lvItem.iSubItem = 0;
    lvItem.pszText = DefaultKeyboard;
    int nItem = m_pList->InsertItem(&lvItem);
    m_pList->SetItemText(nItem,2,_T("0"));
    m_pList->SetItemData(nItem,0);

    bool bSelectedKLID = false;

    if( !m_KLID )
    {
        m_pList->SetItemState(0,LVIS_FOCUSED | LVIS_SELECTED,LVIS_FOCUSED | LVIS_SELECTED);
        bSelectedKLID = true;
    }

    HKL hCurrentKL = GetKeyboardLayout(0);
    bool bProcessedCurrentAlready = false; // a way to tell if the keyboard loading failed

    for( int i = 0; i < iNumEnumeratedHKLs + 1; i++ )
    {
        TCHAR layoutName[KL_NAMELENGTH];
        CString langInfo;

        HKL thisKL;

        if( i == iNumEnumeratedHKLs )
        {
            if( bSelectedKLID )
                break;

            else
            {
                KLID2LayoutName(m_KLID,layoutName); // load the keyboard
                thisKL = LoadKeyboardLayout(layoutName,KLF_ACTIVATE);
            }
        }

        else
        {
            thisKL = pEnumeratedHKLs[i];

            ActivateKeyboardLayout(thisKL,0);
            GetKeyboardLayoutName(layoutName);
        }

        if( ( thisKL == hCurrentKL && bProcessedCurrentAlready ) || !GetLocaleInfo(MAKELCID((UINT)thisKL,SORT_DEFAULT),LOCALE_SLANGUAGE,langInfo.GetBuffer(LanguageBufferSize),LanguageBufferSize) )
            langInfo = KeyboardNotInstalled;

        else if( thisKL == hCurrentKL )
            bProcessedCurrentAlready = true;

        langInfo.ReleaseBuffer();

        CString keyboard,country,id;
        int parenthesisPos;

        if( ( parenthesisPos = langInfo.Find(_T('(')) ) > 0  )
        {
            int rParenthesisPos = langInfo.Find(_T(')'));

            keyboard = langInfo.Left(parenthesisPos - 1);
            country = langInfo.Mid(parenthesisPos + 1,rParenthesisPos - parenthesisPos - 1);
        }

        else
            keyboard = langInfo;

        UINT klid = LayoutName2KLID(layoutName);

        id.Format(_T("%u"),(UINT)klid);

        lvItem.iItem = i + 1;
        lvItem.pszText = keyboard.GetBuffer();
        nItem = m_pList->InsertItem(&lvItem);
        m_pList->SetItemText(nItem,1,country);
        m_pList->SetItemText(nItem,2,id);

        m_pList->SetItemData(nItem,klid);

        if( m_KLID == klid )
        {
            m_pList->SetItemState(nItem,LVIS_FOCUSED | LVIS_SELECTED,LVIS_FOCUSED | LVIS_SELECTED);
            bSelectedKLID = true;
        }

        if( i == iNumEnumeratedHKLs ) // the current keyboard wasn't in the user's current list, so unload it
            UnloadKeyboardLayout(thisKL);
    }

    ActivateKeyboardLayout(hCurrentKL,0);

    delete [] pEnumeratedHKLs;

    m_pList->SetFocus();

    return FALSE;
}


void CKeyboardInputDlg::OnLvnItemchangedKeyboardList(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

    if( pNMLV->uChanged & LVIF_STATE )
    {
        bool bSelectionMade = pNMLV->uNewState & LVNI_SELECTED;

        if( bSelectionMade )
            m_SelectedKLID = m_pList->GetItemData(pNMLV->iItem);

        GetDlgItem(IDOK)->EnableWindow(bSelectionMade); // enable the OK button only if something is selected
    }

    *pResult = 0;
}


CString CKeyboardInputDlg::GetDisplayName(UINT klid)
{
    CString displayName;

    if( !klid )
        displayName = DefaultKeyboard;

    else
    {
        int iNumEnumeratedHKLs = GetKeyboardLayoutList(0,NULL);
        HKL * pEnumeratedHKLs = new HKL[iNumEnumeratedHKLs];
        GetKeyboardLayoutList(iNumEnumeratedHKLs,pEnumeratedHKLs);

        HKL hCurrentKL = GetKeyboardLayout(0);

        /*TCHAR layoutName[KL_NAMELENGTH],layoutNameCheck[KL_NAMELENGTH];
        KLID2LayoutName(klid,layoutName);

        HKL hKL = LoadKeyboardLayout(layoutName,KLF_ACTIVATE);
        GetKeyboardLayoutName(layoutNameCheck);

        if( wcsicmp(layoutName,layoutNameCheck) )
            displayName = KeyboardNotInstalled;

        else if( !GetLocaleInfo(MAKELCID((UINT)hKL,SORT_DEFAULT),LOCALE_SLANGUAGE,displayName.GetBuffer(LanguageBufferSize),LanguageBufferSize) )
            displayName = KeyboardNotInstalled;

        else
        {
            displayName.ReleaseBuffer();

            int parenthesisPos = displayName.Find(_T('('));

            if( parenthesisPos > 0 )
                displayName.Truncate(parenthesisPos - 1);
        }

        ActivateKeyboardLayout(hCurrentKL,0);

        bool bWasKeyboardLoaded = false;

        for( int i = 0; !bWasKeyboardLoaded && i < iNumEnumeratedHKLs; i++ )
            bWasKeyboardLoaded = ( pEnumeratedHKLs[i] == hKL );

        if( !bWasKeyboardLoaded )
            UnloadKeyboardLayout(hKL);
        */

        // the above code (loading each keyboard was ridiculously slow on testing, so we'll avoid that as much as possible)
        HKL hFoundKL = NULL;
        TCHAR layoutName[KL_NAMELENGTH];

        for( int i = 0; !hFoundKL && i < iNumEnumeratedHKLs; i++ )
        {
            ActivateKeyboardLayout(pEnumeratedHKLs[i],0);

            GetKeyboardLayoutName(layoutName);

            if( LayoutName2KLID(layoutName) == klid )
                hFoundKL = pEnumeratedHKLs[i];
        }

        if( !hFoundKL ) // only if we can't find it in the current list of keyboards, load the keyboard
        {
            TCHAR layoutNameCheck[KL_NAMELENGTH];
            KLID2LayoutName(klid,layoutName);

            HKL hKL = LoadKeyboardLayout(layoutName,KLF_ACTIVATE);
            GetKeyboardLayoutName(layoutNameCheck);

            if( _tcsicmp(layoutName,layoutNameCheck) )
                displayName = KeyboardNotInstalled;

            else
            {
                hFoundKL = hKL;
                UnloadKeyboardLayout(hKL);
            }
        }


        if( hFoundKL )
        {
            if( !GetLocaleInfo(MAKELCID((UINT)hFoundKL,SORT_DEFAULT),LOCALE_SLANGUAGE,displayName.GetBuffer(LanguageBufferSize),LanguageBufferSize) )
                displayName = KeyboardNotInstalled;

            else
            {
                displayName.ReleaseBuffer();

                int parenthesisPos = displayName.Find(_T('('));

                if( parenthesisPos > 0 )
                    displayName.Truncate(parenthesisPos - 1);
            }
        }

        ActivateKeyboardLayout(hCurrentKL,0);

        delete [] pEnumeratedHKLs;
    }

    return displayName;
}


CString CKeyboardInputDlg::GetDisplayNameHKL(HKL hKL)
{
    CString displayName;

    if( !hKL )
        displayName = DefaultKeyboard;

    else if( !GetLocaleInfo(MAKELCID((UINT)hKL,SORT_DEFAULT),LOCALE_SLANGUAGE,displayName.GetBuffer(LanguageBufferSize),LanguageBufferSize) )
        displayName = KeyboardNotInstalled;

    else
    {
        displayName.ReleaseBuffer();

        int parenthesisPos = displayName.Find(_T('('));

        if( parenthesisPos > 0 )
            displayName.Truncate(parenthesisPos - 1);
    }

    return displayName;
}


UINT CKeyboardInputDlg::LayoutName2KLID(TCHAR * pLayoutName) // GHM 20120822 this function treats the string as a hex value, converting it to a number
{
    UINT klid = 0;

    for( int i = 0; i < KL_NAMELENGTH - 1; i++ ) // -1 for the 0 byte at the end of the string
    {
        klid *= 16;

        if( isdigit(pLayoutName[i]) )
            klid += pLayoutName[i] - '0';

        else
            klid += tolower(pLayoutName[i]) - 'a' + 10;
    }

    return klid;
}


void CKeyboardInputDlg::KLID2LayoutName(UINT klid,TCHAR * pLayoutName) // GHM 20120822
{
    CString temp;
    temp.Format(_T("%08x"),klid);
    wcscpy(pLayoutName,temp);
}


UINT CKeyboardInputDlg::HKL2KLID(HKL hKL) // GHM 20120822
{
    if( !hKL )
        return 0;

    HKL hCurrentKL = GetKeyboardLayout(0);

    ActivateKeyboardLayout(hKL,0);

    TCHAR layoutName[KL_NAMELENGTH];
    GetKeyboardLayoutName(layoutName);

    ActivateKeyboardLayout(hCurrentKL,0);

    return CKeyboardInputDlg::LayoutName2KLID(layoutName);
}
