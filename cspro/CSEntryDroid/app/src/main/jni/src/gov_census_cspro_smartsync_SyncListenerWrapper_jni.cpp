#include "gov_census_cspro_smartsync_SyncListenerWrapper_jni.h"
#include <engine/StandardSystemIncludes.h>
#include "JNIHelpers.h"
#include <zSyncO/ISyncListener.h>

/*
 * Class:     gov_census_cspro_smartsync_SyncListenerWrapper
 * Method:    onProgress
 * Signature: (F)V
 */
extern "C" JNIEXPORT void JNICALL Java_gov_census_cspro_smartsync_SyncListenerWrapper_onProgress
  (JNIEnv* pEnv, jobject thisObj, jlong progress)
{
    auto pNativeListener = reinterpret_cast<ISyncListener*>(pEnv->GetLongField(thisObj, JNIReferences::fieldSyncListenerWrapperNativeListener));
    if (pNativeListener)
        pNativeListener->onProgress(progress);
}

/*
 * Class:     gov_census_cspro_smartsync_SyncListenerWrapper
 * Method:    isCancelled
 * Signature: ()Z
 */
extern "C" JNIEXPORT jboolean JNICALL Java_gov_census_cspro_smartsync_SyncListenerWrapper_isCancelled
  (JNIEnv* pEnv, jobject thisObj)
{
    auto pNativeListener = reinterpret_cast<ISyncListener*>(pEnv->GetLongField(thisObj, JNIReferences::fieldSyncListenerWrapperNativeListener));
    if (pNativeListener)
        return static_cast<jboolean>(pNativeListener->isCancelled());
    return JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_gov_census_cspro_smartsync_SyncListenerWrapper_setTotal(JNIEnv *env, jobject thiz, jlong total)
{
    auto native_listener = reinterpret_cast<ISyncListener*>(env->GetLongField(thiz, JNIReferences::fieldSyncListenerWrapperNativeListener));
    if (native_listener)
        native_listener->setProgressTotal(total);
}

extern "C" JNIEXPORT jlong JNICALL
Java_gov_census_cspro_smartsync_SyncListenerWrapper_getTotal(JNIEnv *env, jobject thiz)
{
    auto native_listener = reinterpret_cast<ISyncListener*>(env->GetLongField(thiz, JNIReferences::fieldSyncListenerWrapperNativeListener));
    return native_listener->getProgressTotal();
}
