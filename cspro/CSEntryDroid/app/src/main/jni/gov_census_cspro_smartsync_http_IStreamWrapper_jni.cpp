#include "gov_census_cspro_smartsync_http_IStreamWrapper_jni.h"
#include <engine/StandardSystemIncludes.h>
#include "JNIHelpers.h"
#include <istream>
#include <errno.h>

/*
 * Class:     gov_census_cspro_smartsync_http_IStreamWrapper
 * Method:    read
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_gov_census_cspro_smartsync_http_IStreamWrapper_read__
  (JNIEnv* pEnv, jobject thisObj)
{
    std::istream* pNativeStream = reinterpret_cast<std::istream*>(pEnv->GetLongField(thisObj, JNIReferences::fieldIStreamWrapperNativeIStream));
    int result = pNativeStream->get();
    if (pNativeStream->bad()) {
        jclass IOException = pEnv->FindClass("java/io/IOException");
        pEnv->ThrowNew(IOException, strerror(errno));
    }
    if (result == std::istream::traits_type::eof())
        return -1;
    else
        return result;
}

/*
 * Class:     gov_census_cspro_smartsync_http_IStreamWrapper
 * Method:    read
 * Signature: ([BII)I
 */
JNIEXPORT jint JNICALL Java_gov_census_cspro_smartsync_http_IStreamWrapper_read___3BII
  (JNIEnv* pEnv, jobject thisObj, jbyteArray b, jint off, jint len)
{
    if (len == 0)
        return 0;

    std::istream* pNativeStream = reinterpret_cast<std::istream*>(pEnv->GetLongField(thisObj, JNIReferences::fieldIStreamWrapperNativeIStream));

    if (pNativeStream->eof())
        return -1;

    jbyte* buffer = pEnv->GetByteArrayElements(b, NULL);
    pNativeStream->read(reinterpret_cast<char*>(buffer) + off, len);
    pEnv->ReleaseByteArrayElements(b, buffer, JNI_COMMIT);
    if (pNativeStream->bad()) {
        jclass IOException = pEnv->FindClass("java/io/IOException");
        pEnv->ThrowNew(IOException, strerror(errno));
    }
    return pNativeStream->gcount();
}
