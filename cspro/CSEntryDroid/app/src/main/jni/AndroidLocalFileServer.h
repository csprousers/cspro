#pragma once

#include <zHtml/PortableLocalFileServer.h>
#include <jni.h>


class AndroidLocalFileServer : public PortableLocalFileServer
{
private:
    AndroidLocalFileServer();

public:
    static AndroidLocalFileServer& GetInstance();

    struct Response
    {
        jbyteArray content;
        std::string content_type;
    };

    // Response::content will be null on error
    Response GetVirtualFile(JNIEnv& jni_env, const std::string& path);
};
