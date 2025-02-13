#include <engine/StandardSystemIncludes.h>
#include "AndroidHttpConnection.h"
#include "JNIHelpers.h"
#include <zSyncO/SyncException.h>
#include <zSyncO/ISyncListener.h>
#include <vector>
#include <android/log.h>
#include <zToolsO/Tools.h>
#include <zToolsO/Utf8Convert.h>

#define JNI_VERSION JNI_VERSION_1_6

#include <rxcpp/rx-lite.hpp>

namespace {

    class HttpOperation
    {
    public:
        HttpOperation(JNIEnv *env, jobject jResponse):
            m_jResponse(env->NewGlobalRef(jResponse))
        {
            m_jInputStream = env->CallObjectMethod(jResponse, JNIReferences::methodHttpResponseGetBody);
            if (m_jInputStream)
                m_jInputStream = env->NewGlobalRef(m_jInputStream);
        }

        ~HttpOperation()
        {
            JNIEnv *env = GetJNIEnvForCurrentThread();
            if (m_jInputStream) {
                env->CallVoidMethod(m_jInputStream, JNIReferences::methodInputStreamClose);
                env->DeleteGlobalRef(m_jInputStream);
            }
            env->CallVoidMethod(m_jResponse, JNIReferences::methodHttpResponseClose);
            env->DeleteGlobalRef(m_jResponse);
        }

        int Read(JNIEnv *env, jbyteArray jBuffer) const
        {
            if (!m_jInputStream)
                return -1;

            return env->CallIntMethod(m_jInputStream,
                                      JNIReferences::methodInputStreamRead,
                                      jBuffer);
        }

    private:
        jobject m_jResponse;
        jobject m_jInputStream;
    };
}


AndroidHttpConnection::AndroidHttpConnection()
: m_pListener(nullptr)
{
    JNIEnv *pEnv = GetJNIEnvForCurrentThread();
    jobject impl = pEnv->NewObject(JNIReferences::classAndroidHttpConnection,
                                   JNIReferences::methodAndroidHttpConnectionConstructor);
    m_javaImpl = pEnv->NewGlobalRef(impl);
}

AndroidHttpConnection::~AndroidHttpConnection()
{
    JNIEnv *pEnv = GetJNIEnvForCurrentThread();
    // Release ref to java implementation
    pEnv->DeleteGlobalRef(m_javaImpl);
}

HttpResponse AndroidHttpConnection::Request(const HttpRequest &request)
{
    JNIEnv *env = GetJNIEnvForCurrentThread();

    // Convert args to java
    JNIReferences::scoped_local_ref<jstring> jMethod(env, env->NewStringUTF(
        HttpRequestMethodToString(request.method)));
    JNIReferences::scoped_local_ref<jstring> jUrl(env, WideToJava(env, request.url));
    JNIReferences::scoped_local_ref<jobjectArray> jRequestHeaders(env);
    if (!request.headers.empty()) {
        jRequestHeaders.reset((jobjectArray) env->NewObjectArray(request.headers.size(),
                                                                 env->FindClass(
                                                                      "java/lang/String"), NULL));
        for (int i = 0; i < request.headers.size(); ++i) {
            JNIReferences::scoped_local_ref<jstring> jHeader(env, WideToJava(env,
                                                                              request.headers.at(
                                                                                  i)));
            env->SetObjectArrayElement(jRequestHeaders.get(), i, jHeader.get());
        }
    }

    JNIReferences::scoped_local_ref<jobject> jPostData(env);
    if (request.upload_data) {
        // This cast is important or you end up storing the full 64 bit value which messes us up later
        // when we extract the ptr from the jlong.
        jlong jnativePostDataStream = (intptr_t) request.upload_data;
        jPostData.reset(env->NewObject(JNIReferences::classIStreamWrapper,
                                        JNIReferences::methodIStreamWrapperConstructor,
                                        jnativePostDataStream));
    }

    JNIReferences::scoped_local_ref<jobject> jResponse(env, env->CallObjectMethod(m_javaImpl,
                                                                                    JNIReferences::methodAndroidHttpConnectionRequest,
                                                                                    jMethod.get(),
                                                                                    jUrl.get(),
                                                                                    jPostData.get(),
                                                                                    (jint) request.upload_data_size_bytes,
                                                                                    jRequestHeaders.get()));

    // Java could throw an IOException
    JNIReferences::scoped_local_ref<jthrowable> exception(env, env->ExceptionOccurred());
    if (exception.get()) {
        env->ExceptionClear();
    } else {

        int response_code = env->CallIntMethod(jResponse.get(),
                                                JNIReferences::methodHttpResponseGetHttpStatus);
        HttpResponse response(response_code);

        // Copy response headers from java
        JNIReferences::scoped_local_ref<jobject> jResponseHeaders(env, env->CallObjectMethod(
            jResponse.get(), JNIReferences::methodHttpResponseGetHeaders));
        int numResponseHeaders = env->CallIntMethod(jResponseHeaders.get(),
                                                     JNIReferences::methodListSize);
        for (int i = 0; i < numResponseHeaders; ++i) {
            JNIReferences::scoped_local_ref<jstring> jHeader(env, (jstring) env->CallObjectMethod(
                jResponseHeaders.get(), JNIReferences::methodListGet, i));
            CString h = JavaToWSZ(env, jHeader.get());
            response.headers.push_back(h);
        }

        std::shared_ptr<HttpOperation> operation = std::make_shared<HttpOperation>(env, jResponse.get()); // observable takes ownership, cleanup happens when observable goes out of scope

        if (m_pListener && m_pListener->getProgressTotal() <= 0) {
            CString content_length_string = response.headers.value("Content-Length");
            if (!content_length_string.IsEmpty()) {
                int64_t content_length = std::strtoll(UTF8Convert::WideToUTF8(content_length_string).c_str(), nullptr, 0);
                if (content_length > 0)
                    m_pListener->setProgressTotal(content_length);
            }
        }

        response.body.observable = rxcpp::observable<>::create<std::string>(
            [operation, this](rxcpp::subscriber <std::string> s) {
                try {

                    JNIEnv *env = GetJNIEnvForCurrentThread(); // in case observable is subscribed in other thread
                    JNIReferences::scoped_local_ref<jbyteArray> jReadBuffer(env, env->NewByteArray(4096));
                    int64_t total_read = 0;
                    while (true) {
                        int bytes_read = operation->Read(env, jReadBuffer.get());
                        JNIReferences::scoped_local_ref<jthrowable> exception(env, env->ExceptionOccurred());
                        if (exception.get()) {
                            env->ExceptionClear();
                            logException(env, ANDROID_LOG_ERROR, "AndroidHttpClient", exception.get());

                            std::wstring exceptionMessage = exceptionToString(env, exception.get());
                            if (isRetryableError(env, exception.get()))
                                throw SyncRetryableNetworkError(100119, exceptionMessage);
                            else
                                throw SyncError(100119, exceptionMessage);
                        }
                        if (bytes_read == -1)
                            break;
                        if (bytes_read > 0) {
                           s.on_next(ByteArrayToString(env, jReadBuffer.get(), bytes_read));
                        }
                        if (m_pListener) {
                            total_read += bytes_read;
                            m_pListener->onProgress(total_read);
                        }
                    }
                    s.on_completed();
                }
                catch (...) {
                    s.on_error(std::current_exception());
                }
            });

        return response;
    }

    if (exception.get()) {

        if (env->IsInstanceOf(exception.get(),
                               env->FindClass("gov/census/cspro/smartsync/SyncCancelException"))) {
            throw SyncCancelException();
        }

        logException(env, ANDROID_LOG_ERROR, "AndroidHttpClient", exception.get());

        std::wstring exceptionMessage = exceptionToString(env, exception.get());
        if (isRetryableError(env, exception.get()))
            throw SyncRetryableNetworkError(100119, exceptionMessage);
        else
            throw SyncError(100119, exceptionMessage);
    }

    // Should never get here
    return HttpResponse(0);
}

std::string AndroidHttpConnection::ByteArrayToString(JNIEnv *pEnv, jbyteArray jbytes, int length)
{
    jbyte *buffer = pEnv->GetByteArrayElements(jbytes, NULL);
    std::string bytes_string(reinterpret_cast<char *>(buffer),
                                     length);
    pEnv->ReleaseByteArrayElements(jbytes, buffer, JNI_ABORT);
    return bytes_string;
}

bool AndroidHttpConnection::isRetryableError(JNIEnv *pEnv, jthrowable exception)
{
    // This list comes from Apache http DefaultHttpRequestRetryHandler
    // but removed SocketTimeoutException since that one occurs when server goes down in my testing
    if (pEnv->IsInstanceOf(exception, pEnv->FindClass("java/net/UnknownHostException")) ||
        pEnv->IsInstanceOf(exception, pEnv->FindClass("java/net/ConnectException")) ||
        pEnv->IsInstanceOf(exception, pEnv->FindClass("javax/net/ssl/SSLException")))
        return false;
    else
        return true;
}

void AndroidHttpConnection::setListener(ISyncListener* pListener)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    jlong jNativeListener = (intptr_t) pListener;
    jobject jListener = pEnv->NewObject(JNIReferences::classSyncListenerWrapper, JNIReferences::methodSyncListenerWrapperConstructor, jNativeListener);
    pEnv->CallVoidMethod(m_javaImpl, JNIReferences::methodAndroidHttpConnectionSetListener, jListener);
    m_pListener = pListener;
}
