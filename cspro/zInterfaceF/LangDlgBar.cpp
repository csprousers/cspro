#include "StdAfx.h"
#include "LangDlgBar.h"


BEGIN_MESSAGE_MAP(LangDlgBar, CDialogBar)
    ON_CBN_SELENDOK(IDC_LANGCOMBO, OnLanguageChange)
END_MESSAGE_MAP()


void LangDlgBar::UpdateLanguageList(const CDataDict& dictionary)
{
    CComboBox* pCombo = reinterpret_cast<CComboBox*>(GetDlgItem(IDC_LANGCOMBO));
    ASSERT(pCombo != nullptr);

    pCombo->ResetContent();

    for( const Language& language : dictionary.GetLanguages() )
        pCombo->AddString(FormatText(_T("%s: %s"), language.GetName().c_str(), language.GetLabel().c_str()));

    // set the select to the dictionary's current language
    pCombo->SetCurSel(dictionary.GetCurrentLanguageIndex());
}


void LangDlgBar::SelectNextLanguage()
{
    CComboBox* pCombo = reinterpret_cast<CComboBox*>(GetDlgItem(IDC_LANGCOMBO));
    ASSERT(pCombo != nullptr && pCombo->GetCount() > 0);

    const int next_selection = pCombo->GetCurSel() + 1;

    pCombo->SetCurSel(( next_selection < pCombo->GetCount() ) ? next_selection :
                                                                0);

    OnLanguageChange();
}


void LangDlgBar::OnLanguageChange()
{
    CComboBox* pCombo = reinterpret_cast<CComboBox*>(GetDlgItem(IDC_LANGCOMBO));
    ASSERT(pCombo != nullptr);

    // post current selection to parent window
    AfxGetMainWnd()->PostMessage(UWM::Interface::SelectLanguage, pCombo->GetCurSel());
}
