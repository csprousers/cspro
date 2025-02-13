#pragma once

#include <zInterfaceF/zInterfaceF.h>


class CLASS_DECL_ZINTERFACEF LangDlgBar : public CDialogBar
{
public:
    void UpdateLanguageList(const CDataDict& dictionary);
    void SelectNextLanguage();

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnLanguageChange();
};
