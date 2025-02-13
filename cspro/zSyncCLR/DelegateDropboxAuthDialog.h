#pragma once

#include <zSyncO/IDropboxAuthDialog.h>
#include <vcclr.h>
using namespace System;

namespace CSPro {
    namespace Sync {

        public delegate System::String^ OnShowDropboxAuthDialog(System::String^ clientId);
    }
}

class DelegateDropboxAuthDialog : public IDropboxAuthDialog
{
public:
    CString Show(CString clientId)
    {
        return CString(OnShowDelegate->Invoke(gcnew System::String(clientId)));
    }

    gcroot<CSPro::Sync::OnShowDropboxAuthDialog^> OnShowDelegate;
};
