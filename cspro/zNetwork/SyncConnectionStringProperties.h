#pragma once

#include <zDataO/ConnectionStringProperties.h>


namespace SCSProperty // sync connection string property
{
    constexpr const char* account           = "account";            // DropboxConnection
    constexpr const char* createDirectory   = "createDirectory";    // LocalFileConnection
    constexpr const char* email             = JK::email;            // DropboxConnection
    constexpr const char* password          = CSProperty::password; // CSWebConnection + FtpConnection
    constexpr const char* useLocal          = "useLocal";           // DropboxConnection
    constexpr const char* username          = CSProperty::username; // CSWebConnection + FtpConnection
}


namespace SCSValue // sync connection string value
{
    constexpr const char* true_             = CSValue::true_;
    constexpr const char* false_            = CSValue::false_;

    constexpr const char* try_              = "try";
}
