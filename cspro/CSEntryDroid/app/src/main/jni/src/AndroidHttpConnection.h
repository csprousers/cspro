#pragma once

#include <zNetwork/IHttpConnection.h>
#include <jni.h>

/**
* Http connection implementation for Android
* that calls through to Java HttpConnection class via JNI.
*/
class AndroidHttpConnection : public IHttpConnection {

public:

    AndroidHttpConnection();

    ~AndroidHttpConnection();

    HttpResponse Request(const HttpRequest& request) override;

    void setListener(ISyncListener* listener) override;

private:

    int doRequest(CString method, CString url, std::istream* postData, int64_t postDataSizeBytes,
        std::ostream& response, int64_t expectedResponseSizeBytes, const HeaderList& requestHeaders, HeaderList& responseHeaders);
    static bool isRetryableError(JNIEnv* pEnv, jthrowable exception);
    static std::string ByteArrayToString(JNIEnv *pEnv, jbyteArray bytes, int length);

    jobject m_javaImpl;
    ISyncListener* m_pListener;
};
