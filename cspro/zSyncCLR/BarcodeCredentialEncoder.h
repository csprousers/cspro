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
                return clr_helpers::to_SystemString(BarcodeCredentials::Encode(clr_helpers::to_string(application_name),
                                                                               clr_helpers::to_string(username),
                                                                               clr_helpers::to_string(password)));
            }
        };
    }
}
