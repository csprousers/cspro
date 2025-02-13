#pragma once
#include <zSyncO/zSyncO.h>

///<summary>Interface for retreiving credentials via platform specific oauth dialog for Dropbox</summary>
struct IDropboxAuthDialog
{
    /// <summary>
    /// Prompt user for Dropbox credentials/linkage
    /// </summary>
    /// <returns>Access token for Dropbox or empty string if failed</returns>
    virtual CString Show(CString clientId) = 0;

    virtual ~IDropboxAuthDialog() {}
};

