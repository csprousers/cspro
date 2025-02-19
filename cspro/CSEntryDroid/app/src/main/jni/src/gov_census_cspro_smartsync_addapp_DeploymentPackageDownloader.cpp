#include <engine/StandardSystemIncludes.h>
#include <zPlatformO/PlatformInterface.h>
#include <Zentryo/DeploymentPackageDownloader.h>
#include <zSyncO/BarcodeCredentials.h>
#include "gov_census_cspro_smartsync_addapp_DeploymentPackageDownloader.h"
#include "JNIHelpers.h"


JNIEXPORT jlong JNICALL Java_gov_census_cspro_smartsync_addapp_DeploymentPackageDownloader_CreateNativeInstance
  (JNIEnv*, jobject)
  {
    return (jlong) new DeploymentPackageDownloader(ApplicationPackageManager(PlatformInterface::GetInstance()->GetCSEntryDirectory()));
  }

JNIEXPORT void JNICALL Java_gov_census_cspro_smartsync_addapp_DeploymentPackageDownloader_CleanupNativeInstance
  (JNIEnv*, jobject, jlong reference)
{
    auto pDownloader = (DeploymentPackageDownloader*) reference;
    if (pDownloader) {
        pDownloader->Disconnect();
        delete pDownloader;
    }
}

JNIEXPORT jint JNICALL
Java_gov_census_cspro_smartsync_addapp_DeploymentPackageDownloader_ConnectToServer(
        JNIEnv* env, jobject, jlong reference, jstring jserver_name, jstring japplication_name, jstring jcredentials)
{
    auto* pDownloader = (DeploymentPackageDownloader*) reference;
    SyncConnectionString sync_connection_string = JavaString::ToUtf8(*env, jserver_name);

    // decrypt and check if the credentials are valid
    if( japplication_name != nullptr && jcredentials != nullptr )
    {
        std::optional<std::tuple<std::string, std::string>> username_password =
            BarcodeCredentials::Decode(JavaString::ToUtf8(*env, japplication_name),
                                       JavaString::ToUtf8(*env, jcredentials));

        if( username_password.has_value() )
        {
            sync_connection_string.SetUsernamePasswordProperties(std::move(std::get<0>(*username_password)),
                                                                 std::move(std::get<1>(*username_password)));
        }
    }

    return (jint)pDownloader->ConnectToServer(sync_connection_string);
}


static void packageListToJava(JNIEnv* pEnv, const std::vector<ApplicationPackage> &packages, jobject jpackages)
{
    for (size_t i = 0; i < packages.size(); ++i) {
        JNIReferences::scoped_local_ref<jstring> jPackageName(pEnv, JavaString::ToJava(*pEnv, packages.at(i).GetName()));
        JNIReferences::scoped_local_ref<jstring> jPackageDescription(pEnv, JavaString::ToJava(*pEnv, packages.at(i).GetDescription()));
        const int SECONDS_TO_MILLISECONDS = 1000;
        JNIReferences::scoped_local_ref<jobject> jBuildTime(pEnv, pEnv->NewObject(JNIReferences::classDate, JNIReferences::methodDateConstructorLong, ((jlong) packages.at(i).GetBuildTime()) * SECONDS_TO_MILLISECONDS));
        JNIReferences::scoped_local_ref<jobject> jInstalledBuildTime(pEnv,
                                                                     packages.at(i).GetInstalledVersionBuildTime() > 0
                                                                     ? pEnv->NewObject(JNIReferences::classDate, JNIReferences::methodDateConstructorLong, ((jlong) packages.at(i).GetInstalledVersionBuildTime()) * SECONDS_TO_MILLISECONDS)
                                                                     : nullptr);
        JNIReferences::scoped_local_ref<jstring> jServerUrl(pEnv, JavaString::ToJava(*pEnv, packages.at(i).GetServerUrl()));


        JNIReferences::scoped_local_ref<jobject> jPackage(pEnv, pEnv->NewObject(JNIReferences::classDeploymentPackage, JNIReferences::methodDeploymentPackageConstructor,
                                                                                jPackageName.get(),
                                                                                jPackageDescription.get(),
                                                                                jBuildTime.get(),
                                                                                jInstalledBuildTime.get(),
                                                                                jServerUrl.get(),
                                                                                (jint) packages.at(i).GetDeploymentType()));

        pEnv->CallBooleanMethod(jpackages,JNIReferences::methodListAdd, jPackage.get());
    }
}

JNIEXPORT jint JNICALL Java_gov_census_cspro_smartsync_addapp_DeploymentPackageDownloader_ListPackages
    (JNIEnv* pEnv, jobject, jlong reference, jobject jpackages)
{
    auto* pDownloader = (DeploymentPackageDownloader*) reference;
    std::vector<ApplicationPackage> packages;
    auto result = pDownloader->List(packages);
    if (result != SyncClient::SyncResult::SYNC_OK)
        return (int) result;
    packageListToJava(pEnv, packages, jpackages);
    return (int) SyncClient::SyncResult ::SYNC_OK;
}


JNIEXPORT jint JNICALL Java_gov_census_cspro_smartsync_addapp_DeploymentPackageDownloader_InstallPackage
  (JNIEnv* pEnv, jobject, jlong reference, jstring jPackageName, jboolean forceFullInstall)
  {
    auto* pDownloader = (DeploymentPackageDownloader*) reference;
    return (jint) pDownloader->Install(JavaString::ToUtf8(*pEnv, jPackageName), forceFullInstall);
  }

JNIEXPORT jint JNICALL
Java_gov_census_cspro_smartsync_addapp_DeploymentPackageDownloader_ListUpdatablePackages(
    JNIEnv* env, jobject , jlong native_instance, jobject jpackages)
{
    auto downloader = (DeploymentPackageDownloader*) native_instance;
    std::vector<ApplicationPackage> packages;
    auto result = downloader->ListUpdatable(packages);
    if (result != SyncClient::SyncResult::SYNC_OK)
        return (int) result;
    packageListToJava(env, packages, jpackages);
    return (int) SyncClient::SyncResult ::SYNC_OK;
}

JNIEXPORT jint JNICALL
Java_gov_census_cspro_smartsync_addapp_DeploymentPackageDownloader_UpdatePackage(JNIEnv* env,
                                                                                 jobject,
                                                                                 jstring jpackage_name,
                                                                                 jstring jserver_url,
                                                                                 jlong native_instance)
{
    auto downloader = (DeploymentPackageDownloader*) native_instance;
    return (jint) downloader->Update(JavaString::ToUtf8(*env, jpackage_name),
                                     JavaString::ToUtf8(*env, jserver_url));
}
