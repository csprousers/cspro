#include <engine/StandardSystemIncludes.h>
#include "gov_census_cspro_smartsync_p2p_AndroidBluetoothAdapter_jni.h"
#include <zPlatformO/PlatformInterface.h>
#include "AndroidApplicationInterface.h"

JNIEXPORT jboolean JNICALL Java_gov_census_cspro_smartsync_p2p_AndroidBluetoothAdapter_isCancelled
 (JNIEnv *, jobject)
{
    auto pAppIFace = (AndroidApplicationInterface*)PlatformInterface::GetInstance()->GetApplicationInterface();
    return (jboolean) pAppIFace->IsProgressDialogCancelled();
}
