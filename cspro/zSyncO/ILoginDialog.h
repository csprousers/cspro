#pragma once
#include <zSyncO/zSyncO.h>

///<summary>Interface for retrieving credentials via platform specific login dialog</summary>
struct ILoginDialog
{
    /// <summary>
    /// Prompt user for credentials
    /// </summary>
    /// <param name="server">Name of server to log into</param>
    /// <param name="show_invalid_error">If true display an error message that username/pwd is invalid</param>
    /// <returns>Tuple of username/password if credentials entered, none if canceled</returns>
    virtual std::optional<std::tuple<CString, CString>> Show(const CString& server, bool show_invalid_error) = 0;

    virtual ~ILoginDialog() {}
};

