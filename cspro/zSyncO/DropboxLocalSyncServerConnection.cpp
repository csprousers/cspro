#include "stdafx.h"
#include "DropboxLocalSyncServerConnection.h"
#include <easyloggingwrapper.h>
#include "SyncException.h"
#include <zSyncO/ConnectResponse.h>
#include <external/jsoncons/json.hpp>

namespace {
#ifdef WIN_DESKTOP

    CString getDropboxDirectoryFromInfoFile(CString infoFilePath)
    {
        std::ifstream is(infoFilePath);
        if (!is)
            return CString();

        jsoncons::json info_json = jsoncons::json::parse(is);
        if (info_json.contains("personal") && info_json["personal"].contains("path")) {
            return UTF8Convert::UTF8ToWide<CString>(info_json["personal"]["path"].as<std::string_view>());
        }
        if (info_json.contains("business") && info_json["business"].contains("path")) {
            return UTF8Convert::UTF8ToWide<CString>(info_json["business"]["path"].as<std::string_view>());
        }

        return CString();
    }

    CString getDropboxInfoFilePath()
    {
        CString dbInfoPath;
        PWSTR appDataPath = NULL;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appDataPath))) {
            dbInfoPath = CString(appDataPath) + _T("\\Dropbox\\info.json");
        }
        CoTaskMemFree(appDataPath);

        if (PortableFunctions::FileExists(dbInfoPath)) {
            return dbInfoPath;
        }

        appDataPath = NULL;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &appDataPath))) {
            dbInfoPath = CString(appDataPath) + _T("\\Dropbox\\info.json");
        }
        CoTaskMemFree(appDataPath);

        if (PortableFunctions::FileExists(dbInfoPath)) {
            return dbInfoPath;
        }
        else {
            return CString();
        }
    }

    CString getDropboxDirectory()
    {
        // Get directory from Dropbox info.json file see https://help.dropbox.com/installs-integrations/desktop/locate-dropbox-folder
        CString dbInfoPath = getDropboxInfoFilePath();
        if (!dbInfoPath.IsEmpty()) {
            CString dropboxDirectory = getDropboxDirectoryFromInfoFile(dbInfoPath);
            if (!dropboxDirectory.IsEmpty()) {
                return dropboxDirectory;
            }
        }

        // Couldn't find it using the input file, try the default location
        PWSTR userProfilePath = NULL;
        CString dropboxDirectory;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Profile, 0, NULL, &userProfilePath))) {
            dropboxDirectory = CString(userProfilePath) + _T("\\Dropbox");
        }
        CoTaskMemFree(userProfilePath);

        return dropboxDirectory;
    }
#endif
}

DropboxLocalSyncServerConnection::DropboxLocalSyncServerConnection() :
    LocalFileSyncServerConnection(getDropboxDirectory(), "Dropbox")
{
}

ConnectResponse* DropboxLocalSyncServerConnection::connect()
{
    if (!PortableFunctions::FileIsDirectory(getRootDirectory())) {
        CLOG(ERROR, "sync") << "Dropbox directory " << UTF8Convert::WideToUTF8(getRootDirectory()) << " not found on this computer";
        throw SyncError(100148);
    }
    CLOG(INFO, "sync") << "Dropbox directory " << UTF8Convert::WideToUTF8(getRootDirectory());
    return new ConnectResponse(CString(L"Dropbox"), getRootDirectory());
}
