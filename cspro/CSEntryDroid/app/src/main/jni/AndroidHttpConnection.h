#pragma once

#include <zNetwork/HttpConnection.h>
#include <jni.h>


//  HTTP connection implementation for Android that calls through to Java HttpConnection class via JNI.

class AndroidHttpConnection : public HttpConnection
{
public:
    AndroidHttpConnection();
    ~AndroidHttpConnection();

    HttpResponse Request(const HttpRequest& request) override;

    std::shared_ptr<SyncListener> GetSharedSyncListener() override { return m_syncListener; }
    void SetSyncListener(std::shared_ptr<SyncListener> sync_listener) override;

private:
    int doRequest(CString method, CString url, std::istream* postData, int64_t postDataSizeBytes,
        std::ostream& response, int64_t expectedResponseSizeBytes, const HeaderList& requestHeaders, HeaderList& responseHeaders);
    static bool isRetryableError(JNIEnv* pEnv, jthrowable exception);
    static std::string ByteArrayToString(JNIEnv *pEnv, jbyteArray bytes, int length);

private:
    jobject m_javaImpl;
    std::shared_ptr<SyncListener> m_syncListener;
};
