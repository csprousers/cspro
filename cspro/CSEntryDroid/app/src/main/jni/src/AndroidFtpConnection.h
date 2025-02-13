#pragma once

#include <engine/StandardSystemIncludes.h>
#include <zNetwork/IFtpConnection.h>
#include <jni.h>

/**
* Ftp connection implementation for Android
* that calls through to ftp4j library
*/
class AndroidFtpConnection : public IFtpConnection {

public:

    AndroidFtpConnection();

    ~AndroidFtpConnection();

    virtual void connect(CString serverUrl, CString username, CString password);

    virtual void disconnect();

    void download(CString remoteFilePath, CString localFilePath);

    void download(CString remoteFilePath, std::ostream& localFileStream);

    void upload(CString localFilePath, CString remoteFilePath);

    void upload(std::istream& localFileData, int64_t fileSizeBytes, CString remoteFilePath);

    std::vector<FileInfo>* getDirectoryListing(CString remotePath);

    time_t getLastModifiedTime(CString remotePath);

    virtual void setListener(ISyncListener* pListener);

private:

    void handleJavaException(jthrowable exception);

    JNIEnv* m_pEnv;
    jobject m_javaImpl;
};
