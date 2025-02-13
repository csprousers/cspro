#pragma once

#include <zUtilF/zUtilF.h>


class CLASS_DECL_ZUTILF SettingsDlg : public CDialog
{
    DECLARE_DYNAMIC(SettingsDlg)

public:
    SettingsDlg(CWnd* pParent = NULL); // standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_CSPRO_SETTINGS };
#endif

protected:
    afx_msg void OnClearCredentials();

    DECLARE_MESSAGE_MAP()
};
