#include "gov_census_cspro_smartsync_SyncListenerWrapper_jni.h"
#include <engine/StandardSystemIncludes.h>
#include <zNetwork/SyncListener.h>
#include "JNIHelpers.h"

extern "C" JNIEXPORT void JNICALL
Java_gov_census_cspro_smartsync_SyncListenerWrapper_onProgress(JNIEnv* pEnv, jobject thisObj, jlong progress)
{
    SyncListener* const native_sync_listener = reinterpret_cast<SyncListener*>(pEnv->GetLongField(thisObj, JNIReferences::fieldSyncListenerWrapperNativeListener));

    if( native_sync_listener != nullptr )
        native_sync_listener->Progress(progress);
}


extern "C" JNIEXPORT jboolean JNICALL
Java_gov_census_cspro_smartsync_SyncListenerWrapper_isCanceled(JNIEnv* pEnv, jobject thisObj)
{
    SyncListener* const native_sync_listener = reinterpret_cast<SyncListener*>(pEnv->GetLongField(thisObj, JNIReferences::fieldSyncListenerWrapperNativeListener));

    if( native_sync_listener != nullptr )
        return static_cast<jboolean>(native_sync_listener->IsCanceled());

    return JNI_FALSE;
}


extern "C" JNIEXPORT void JNICALL
Java_gov_census_cspro_smartsync_SyncListenerWrapper_setTotal(JNIEnv *env, jobject thiz, jlong total)
{
    SyncListener* const native_sync_listener = reinterpret_cast<SyncListener*>(env->GetLongField(thiz, JNIReferences::fieldSyncListenerWrapperNativeListener));

    if( native_sync_listener != nullptr )
        native_sync_listener->SetProgressTotal(total);
}


extern "C" JNIEXPORT jlong JNICALL
Java_gov_census_cspro_smartsync_SyncListenerWrapper_getTotal(JNIEnv *env, jobject thiz)
{
    SyncListener* const native_sync_listener = reinterpret_cast<SyncListener*>(env->GetLongField(thiz, JNIReferences::fieldSyncListenerWrapperNativeListener));

    if( native_sync_listener != nullptr )
        return native_sync_listener->GetProgressTotal();

    return ReturnProgrammingError(0);
}
