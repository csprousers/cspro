#pragma once

#include <zSyncO/ILoginDialog.h>
#include <vcclr.h>
using namespace System;

namespace CSPro {
    namespace Sync {

        public ref struct LoginInfo
        {
            System::String^ username;
            System::String^ password;
        };

        public delegate LoginInfo^ OnShowLoginDialog(bool bShowInvalidError);
    }
}

class DelegateLoginDialog : public ILoginDialog
{
public:
    std::optional<std::tuple<CString, CString>> Show(const CString& /*server*/, bool show_invalid_error) override
    {
        auto info = OnLoginDelegate->Invoke(show_invalid_error);
        if (info == nullptr)
            return {};

        return std::make_tuple((CString)info->username, (CString)info->password);
    }

    gcroot<CSPro::Sync::OnShowLoginDialog^> OnLoginDelegate;
};
