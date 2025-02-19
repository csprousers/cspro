#pragma once

#include <zSyncF/SyncLoginAccessor.h>
#include <vcclr.h>


namespace CSPro
{
    namespace Sync
    {
        public ref struct UsernamePassword
        {
            System::String^ username;
            System::String^ password;
        };

        public delegate UsernamePassword^ OnQueryUsernamePassword(bool show_invalid_error);

        public delegate System::String^ OnAuthorizeDropbox();
    }
}



class SyncCLRLoginAccessor : public SyncLoginAccessor
{
public:
    std::optional<UsernamePassword> QueryUsernamePassword(const std::string& server, bool show_invalid_error) override;

    gcroot<CSPro::Sync::OnQueryUsernamePassword^> on_query_username_password;
};
