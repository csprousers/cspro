#include "stdafx.h"
#include "LoginAccessor.h"
#include "HttpConnection.h"
#include "SyncCredentialStore.h"
#include <zSyncO/BluetoothDeviceInfo.h>

#ifdef WIN_DESKTOP
#include "CurlFtpConnection.h"
#include "LoginDlg.h"
#include <zUtilO/UIThreadRunner.h>
#else
#include "FtpConnection.h"
#include "UsernamePassword.h"
#include <zPlatformO/PlatformInterface.h>
#endif


// --------------------------------------------------------------------------
// LoginAccessor
// --------------------------------------------------------------------------

std::shared_ptr<SyncCredentialStore> LoginAccessor::GetSyncCredentialStore()
{
    return std::make_unique<SyncCredentialStore>();
}


std::unique_ptr<FtpConnection> LoginAccessor::CreateFtpConnection()
{
#ifdef WIN_DESKTOP
    return std::make_unique<CurlFtpConnection>();
#else
    return PlatformInterface::GetInstance()->GetApplicationInterface()->CreateFtpConnection();
#endif
}


std::unique_ptr<HttpConnection> LoginAccessor::CreateHttpConnection()
{
    return HttpConnection::Create();
}


#ifdef WIN_DESKTOP

namespace
{
    LoginAccessor::WinFormsQueryUsernamePassword* win_forms_query_username_password = nullptr;
}


void LoginAccessor::SetWinFormsQueryUsernamePasswordCallback(WinFormsQueryUsernamePassword* const callback)
{
    win_forms_query_username_password = callback;
}

#endif


std::optional<UsernamePassword> LoginAccessor::QueryUsernamePassword(const std::string& server, const bool show_invalid_error)
{
#ifdef WIN_DESKTOP
    UNREFERENCED_PARAMETER(server);

    ASSERT(( AfxGetApp() == nullptr ) == ( win_forms_query_username_password != nullptr ));

    // if called from WinForms, use the callback
    if( win_forms_query_username_password != nullptr )
    {
        return win_forms_query_username_password->QueryUsernamePassword(show_invalid_error);
    }

    // otherwise show the dialog (on the UI thread when possible)
    else
    {
        LoginDlg login_dlg(show_invalid_error);
        DialogUIThreadRunner ui_thread_runner(&login_dlg);

        if( ui_thread_runner.DoModalOnUIThreadOrOnMainThread() == IDOK )
            return login_dlg.ReleaseUsernamePassword();

        return std::nullopt;
    }

#else
    return PlatformInterface::GetInstance()->GetApplicationInterface()->ShowLoginDialog(server, show_invalid_error);
#endif
}



// --------------------------------------------------------------------------
// LoginAccessorWithoutBluetoothSupport
// --------------------------------------------------------------------------

std::shared_ptr<IBluetoothAdapter> LoginAccessorWithoutBluetoothSupport::GetBluetoothAdapter()
{
    return ReturnProgrammingError(nullptr);
}


std::optional<BluetoothDeviceInfo> LoginAccessorWithoutBluetoothSupport::ChooseBluetoothDevice()
{
    return ReturnProgrammingError(std::nullopt);
}
