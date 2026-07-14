#include <engine/StandardSystemIncludes.h>
#include "AndroidLocalFileServer.h"
#include <zHtml/LocalhostUrl.h>


struct VirtualFileMappingResponseObject
{
    JNIEnv& jni_env;
    AndroidLocalFileServer::Response response;
};


// --------------------------------------------------------------------------
// AndroidLocalFileServer
// --------------------------------------------------------------------------

AndroidLocalFileServer::AndroidLocalFileServer()
    :   PortableLocalFileServer(LocalhostUrl::AndroidBaseUrl)
{
}


AndroidLocalFileServer& AndroidLocalFileServer::GetInstance()
{
    static AndroidLocalFileServer local_file_server;
    return local_file_server;
}


AndroidLocalFileServer::Response AndroidLocalFileServer::GetVirtualFile(JNIEnv& jni_env, const std::string& path)
{
    VirtualFileMappingResponseObject response_object { jni_env };
    ASSERT(response_object.response.content == nullptr);

    VirtualFileMappingResponse vfm_response(&response_object);

    PortableLocalFileServer::GetVirtualFile(path, vfm_response);

    return response_object.response;
}



// --------------------------------------------------------------------------
// VirtualFileMappingHandler::ServeContent
// --------------------------------------------------------------------------

void VirtualFileMappingResponse::SetContent(const void* const content_data, const size_t content_size, const std::string& content_type)
{
    ASSERT(content_data != nullptr);

    VirtualFileMappingResponseObject& response_object = *static_cast<VirtualFileMappingResponseObject*>(m_responseObject);

    response_object.response.content = response_object.jni_env.NewByteArray(content_size);
    response_object.jni_env.SetByteArrayRegion(response_object.response.content, 0, content_size, static_cast<const jbyte*>(content_data));

    response_object.response.content_type = content_type;
}
