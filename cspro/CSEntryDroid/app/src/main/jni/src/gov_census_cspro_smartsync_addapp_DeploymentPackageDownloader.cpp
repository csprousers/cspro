#include <engine/StandardSystemIncludes.h>
#include <Zentryo/DeploymentPackageDownloader.h>
#include <zSyncO/BarcodeCredentials.h>
#include <zSyncO/SyncException.h>
#include <zToolsO/Utf8Convert.h>
#include "gov_census_cspro_smartsync_addapp_DeploymentPackageDownloader.h"
#include "JNIHelpers.h"

JNIEXPORT jlong JNICALL Java_gov_census_cspro_smartsync_addapp_DeploymentPackageDownloader_CreateNativeInstance
  (JNIEnv*, jobject)
  {
    return (jlong) new DeploymentPackageDownloader();
  }

JNIEXPORT void JNICALL Java_gov_census_cspro_smartsync_addapp_DeploymentPackageDownloader_CleanupNativeInstance
  (JNIEnv *, jobject, jlong reference)
{
    auto pDownloader = (DeploymentPackageDownloader*) reference;
    if (pDownloader) {
        pDownloader->Disconnect();
        delete pDownloader;
    }
}

JNIEXPORT jint JNICALL
Java_gov_census_cspro_smartsync_addapp_DeploymentPackageDownloader_ConnectToServer(
        JNIEnv *env, jobject, jlong reference, jstring jserver_name, jstring japplication_name, jstring jcredentials)
{
    auto* pDownloader = (DeploymentPackageDownloader*) reference;
    auto serverName = JavaToWSZ(env, jserver_name);

    // decrypt and check if the credentials are valid
    std::optional<std::tuple<CString, CString>> username_password;

    if (japplication_name != nullptr && jcredentials != nullptr)
        username_password = BarcodeCredentials::Decode(JavaToWSZ(env, japplication_name), JavaToWSZ(env, jcredentials));

    return username_password ?
        (jint)pDownloader->ConnectToServer(serverName, std::get<0>(*username_password), std::get<1>(*username_password)) :
        (jint)pDownloader->ConnectToServer(serverName);
}


static void packageListToJava(JNIEnv *pEnv, const std::vector<ApplicationPackage> &packages, jobject jpackages)
{
    for (size_t i = 0; i < packages.size(); ++i) {
        JNIReferences::scoped_local_ref<jstring> jPackageName(pEnv, WideToJava(pEnv, packages.at(i).getName()));
        JNIReferences::scoped_local_ref<jstring> jPackageDescription(pEnv, WideToJava(pEnv, packages.at(i).getDescription()));
        const int SECONDS_TO_MILLISECONDS = 1000;
        JNIReferences::scoped_local_ref<jobject> jBuildTime(pEnv, pEnv->NewObject(JNIReferences::classDate, JNIReferences::methodDateConstructorLong, ((jlong) packages.at(i).getBuildTime()) * SECONDS_TO_MILLISECONDS));
        JNIReferences::scoped_local_ref<jobject> jInstalledBuildTime(pEnv,
                                                                     packages.at(i).getInstalledVersionBuildTime() > 0
                                                                     ? pEnv->NewObject(JNIReferences::classDate, JNIReferences::methodDateConstructorLong, ((jlong) packages.at(i).getInstalledVersionBuildTime()) * SECONDS_TO_MILLISECONDS)
                                                                     : nullptr);
        JNIReferences::scoped_local_ref<jstring> jServerUrl(pEnv, WideToJava(pEnv, packages.at(i).getServerUrl()));


        JNIReferences::scoped_local_ref<jobject> jPackage(pEnv, pEnv->NewObject(JNIReferences::classDeploymentPackage, JNIReferences::methodDeploymentPackageConstructor,
                                                                                jPackageName.get(),
                                                                                jPackageDescription.get(),
                                                                                jBuildTime.get(),
                                                                                jInstalledBuildTime.get(),
                                                                                jServerUrl.get(),
                                                                                (jint) packages.at(i).getDeploymentType()));

        pEnv->CallBooleanMethod(jpackages,JNIReferences::methodListAdd, jPackage.get());
    }
}

JNIEXPORT jint JNICALL Java_gov_census_cspro_smartsync_addapp_DeploymentPackageDownloader_ListPackages
    (JNIEnv *pEnv, jobject, jlong reference, jobject jpackages)
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
  (JNIEnv *pEnv, jobject, jlong reference, jstring jPackageName, jboolean forceFullInstall)
  {
    auto* pDownloader = (DeploymentPackageDownloader*) reference;
    CString sPackageName = JavaToWSZ(pEnv, jPackageName);
    return (jint) pDownloader->Install(sPackageName, forceFullInstall);
  }

JNIEXPORT jint JNICALL
Java_gov_census_cspro_smartsync_addapp_DeploymentPackageDownloader_ListUpdatablePackages(
    JNIEnv *env, jobject , jlong native_instance, jobject jpackages)
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
Java_gov_census_cspro_smartsync_addapp_DeploymentPackageDownloader_UpdatePackage(JNIEnv *env,
                                                                                 jobject,
                                                                                 jstring jpackage_name,
                                                                                 jstring jserver_url,
                                                                                 jlong native_instance)
{
    auto downloader = (DeploymentPackageDownloader*) native_instance;
    CString package_name = JavaToWSZ(env, jpackage_name);
    CString server_url = JavaToWSZ(env, jserver_url);
    return (jint) downloader->Update(package_name, server_url);
}
