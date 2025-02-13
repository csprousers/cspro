#pragma once

#include <zSyncO/BarcodeCredentials.h>

namespace CSPro
{
    namespace Sync
    {
        public ref class BarcodeCredentialEncoder sealed
        {
        public:
            static System::String^ GetCredentials(System::String^ application_name, System::String^ username, System::String^ password)
            {
                return gcnew System::String(BarcodeCredentials::Encode(application_name, username, password));
            }
        };
    }
}
