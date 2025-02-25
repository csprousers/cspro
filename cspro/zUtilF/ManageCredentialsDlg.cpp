#include "StdAfx.h"
#include "ManageCredentialsDlg.h"
#include <zUtilO/CredentialStore.h>
#include <zMessageO/Messages.h>


IMPLEMENT_DYNAMIC(SettingsDlg, CDialog)

BEGIN_MESSAGE_MAP(SettingsDlg, CDialog)
    ON_BN_CLICKED(IDC_BUTTON_CLEAR_CREDENTIALS, OnClearCredentials)
END_MESSAGE_MAP()


SettingsDlg::SettingsDlg(CWnd* pParent /*=NULL*/)
    :   CDialog(IDD_CSPRO_SETTINGS, pParent)
{
}


void SettingsDlg::OnClearCredentials()
{
    std::function<bool(size_t)> confirmation_callback = [](size_t number_credentials) -> bool
    {
        if( number_credentials == 0 )
        {
            AfxMessageBox(MGF::GetMessageText(94331, "There are no saved credentials").GetString());
            return true;
        }

        else
        {
            const SharableString formatter = MGF::GetMessageText(94332, "Are you sure that you want to delete %d credential(s)?");
            const std::string message = FormatText(formatter->c_str(), static_cast<int>(number_credentials));
            return ( AfxMessageBox(message, MB_OKCANCEL) == IDOK );
        }
    };

    CredentialStore::ClearAll(&confirmation_callback);
}
