#include "stdafx.h"
#include "SyncCLRLoginAccessor.h"
#include <zNetwork/UsernamePassword.h>


std::optional<UsernamePassword> SyncCLRLoginAccessor::QueryUsernamePassword(const std::string& /*server*/, const bool show_invalid_error)
{
    CSPro::Sync::UsernamePassword^ username_password = on_query_username_password->Invoke(show_invalid_error);

    if( username_password == nullptr )
        return std::nullopt;

    return UsernamePassword { clr_helpers::to_string(username_password->username), clr_helpers::to_string(username_password->password) };
}
