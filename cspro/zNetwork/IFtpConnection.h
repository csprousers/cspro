#pragma once

#include <zNetwork/FileInfo.h>

struct ISyncListener;


/// <summary>
/// FTP client interface
/// </summary>
/// <remarks>
/// Supports standard FTP operations: upload, download, directory listing
/// </remarks>
struct IFtpConnection
{
    virtual ~IFtpConnection() {}

    virtual void connect(CString serverUrl, CString username, CString password) = 0;

    virtual void disconnect() = 0;

    virtual void download(CString remoteFilePath, CString localFilePath) = 0;

    virtual void download(CString remoteFilePath, std::ostream& localFileStream) = 0;

    virtual void upload(CString localFilePath, CString remoteFilePath) = 0;

    virtual void upload(std::istream& localFileStream, int64_t fileSizeBytes, CString remoteFilePath) = 0;

    virtual std::vector<FileInfo>* getDirectoryListing(CString remotePath) = 0;

    virtual time_t getLastModifiedTime(CString remotePath) = 0;

    virtual void setListener(ISyncListener* pListener) = 0;
};
