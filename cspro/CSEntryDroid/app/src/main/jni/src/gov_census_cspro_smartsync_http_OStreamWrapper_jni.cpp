#include "gov_census_cspro_smartsync_http_OStreamWrapper_jni.h"
#include <engine/StandardSystemIncludes.h>
#include "JNIHelpers.h"
#include <ostream>
#include <errno.h>

/*
 * Class:     gov_census_cspro_smartsync_http_OStreamWrapper
 * Method:    write
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_smartsync_http_OStreamWrapper_write__I
  (JNIEnv *pEnv, jobject thisObj, jint byte)
{
    std::ostream* pNativeStream = reinterpret_cast<std::ostream*>(pEnv->GetLongField(thisObj, JNIReferences::fieldOStreamWrapperNativeOStream));
    pNativeStream->put((char) byte);
    if (pNativeStream->bad()) {
        jclass IOException = pEnv->FindClass("java/io/IOException");
        pEnv->ThrowNew(IOException, strerror(errno));
    }
}

/*
 * Class:     gov_census_cspro_smartsync_http_OStreamWrapper
 * Method:    write
 * Signature: ([BII)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_smartsync_http_OStreamWrapper_write___3BII
  (JNIEnv *pEnv, jobject thisObj, jbyteArray b, jint off, jint len)
{
    std::ostream* pNativeStream = reinterpret_cast<std::ostream*>(pEnv->GetLongField(thisObj, JNIReferences::fieldOStreamWrapperNativeOStream));
    jbyte* buffer = pEnv->GetByteArrayElements(b, NULL);
    std::string bs(reinterpret_cast<char*>(buffer), len);
    pNativeStream->write(reinterpret_cast<char*>(buffer) + off, len);
    pEnv->ReleaseByteArrayElements(b, buffer, JNI_ABORT);
    if (pNativeStream->bad()) {
        jclass IOException = pEnv->FindClass("java/io/IOException");
        pEnv->ThrowNew(IOException, strerror(errno));
    }
}

/*
 * Class:     gov_census_cspro_smartsync_http_OStreamWrapper
 * Method:    flush
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_smartsync_http_OStreamWrapper_flush
  (JNIEnv* pEnv, jobject thisObj)
{
    std::ostream* pNativeStream = reinterpret_cast<std::ostream*>(pEnv->GetLongField(thisObj, JNIReferences::fieldOStreamWrapperNativeOStream));
    pNativeStream->flush();
    if (pNativeStream->bad()) {
        jclass IOException = pEnv->FindClass("java/io/IOException");
        pEnv->ThrowNew(IOException, strerror(errno));
    }
}
