#pragma once

#include <zNetwork/zNetwork.h>
#include <zNetwork/IFtpConnection.h>


/// <summary>
/// FTP client implementation using libcurl
/// </summary>
class ZNETWORK_API CurlFtpConnection : public IFtpConnection
{
public:
    CurlFtpConnection();

    ~CurlFtpConnection();

    void connect(CString serverUrl, CString username, CString password) override;

    void disconnect();

    void download(CString remoteFilePath, CString localFilePath) override;

    void download(CString remoteFilePath, std::ostream& localFileStream) override;

    void upload(CString localFilePath, CString remoteFilePath) override;

    void upload(std::istream& localFileData, int64_t fileSizeBytes, CString remoteFilePath) override;

    std::vector<FileInfo>* getDirectoryListing(CString remotePath) override;

    time_t getLastModifiedTime(CString remotePath) override;

    void setListener(ISyncListener* pListener) override;

private:

    void renameFile(CString directory, CString originalName, CString newName);
    void deleteFile (CString filePath);
    bool fileExists(CString filePath);
    std::vector<FileInfo>* getDirectoryListingMLSD(CString remotePath);
    std::vector<FileInfo>* getDirectoryListingLIST(CString remotePath);

    bool m_bSupportsMLSD;
    void* m_pCurl;
    std::string m_serverUrl;
    ISyncListener* m_pListener;
};
