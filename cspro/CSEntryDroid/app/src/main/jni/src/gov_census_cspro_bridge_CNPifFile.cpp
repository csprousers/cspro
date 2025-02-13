#include <jni.h>

#include <engine/StandardSystemIncludes.h>
#include <zPlatformO/PortableMFC.h>

#include "JNIHelpers.h"
#include "gov_census_cspro_bridge_CNPifFile.h"

#include <zToolsO/Tools.h>
#include <zUtilO/ConnectionString.h>
#include <zUtilO/Interapp.h>
#include <ZBRIDGEO/npff.h>
#include <zEngineO/PffExecutor.h>


JNIEXPORT jlong JNICALL Java_gov_census_cspro_bridge_CNPifFile_LoadPif
  (JNIEnv * pEnv, jobject, jstring filename)
{
    std::wstring sFilename = JavaToWSZ(pEnv,filename);

    CNPifFile* pPifFile = new CNPifFile(sFilename);

    if( pPifFile->LoadPifFile() && ( pPifFile->GetAppType() == APPTYPE::ENTRY_TYPE || PffExecutor::CanExecute(pPifFile->GetAppType()) ) )
		return (long)pPifFile;

	delete pPifFile;
	return 0;
}

JNIEXPORT jstring JNICALL Java_gov_census_cspro_bridge_CNPifFile_GetDescription
  (JNIEnv * pEnv, jobject, jlong reference)
{
    CNPifFile* pPifFile = (CNPifFile*)reference;
    return WideToJava(pEnv, pPifFile->GetEvaluatedAppDescription());
}

JNIEXPORT jboolean JNICALL Java_gov_census_cspro_bridge_CNPifFile_IsAppTypeEntry
  (JNIEnv * pEnv, jobject, jlong reference)
{
    CNPifFile* pPifFile = (CNPifFile*)reference;
    return ( pPifFile->GetAppType() == APPTYPE::ENTRY_TYPE );
}


JNIEXPORT jint JNICALL Java_gov_census_cspro_bridge_CNPifFile_GetShowInApplicationListing
  (JNIEnv * pEnv, jobject, jlong reference)
{
    CNPifFile* pPifFile = (CNPifFile*)reference;
    return (int)pPifFile->GetShowInApplicationListing();
}

JNIEXPORT jstring JNICALL Java_gov_census_cspro_bridge_CNPifFile_GetInputFilename
  (JNIEnv * pEnv, jobject, jlong reference)
{
    const CNPifFile* pPifFile = (const CNPifFile*)reference;

    if( pPifFile->GetSingleInputDataConnectionString().IsFilenamePresent() ) {
        return WideToJava(pEnv, pPifFile->GetSingleInputDataConnectionString().GetFilename());
    }
    else {
        return WideToJava(pEnv, wstring_view());
    }
}

JNIEXPORT jstring JNICALL Java_gov_census_cspro_bridge_CNPifFile_GetAppFilename
  (JNIEnv * pEnv, jobject, jlong reference)
{
    CNPifFile* pPifFile = (CNPifFile*)reference;

    return WideToJava(pEnv, pPifFile->GetAppFName());
}

JNIEXPORT void JNICALL Java_gov_census_cspro_bridge_CNPifFile_ClosePif
  (JNIEnv *, jobject, jlong reference)
{
    CNPifFile* pPifFile = (CNPifFile*)reference;
    delete pPifFile;
}

JNIEXPORT jobjectArray JNICALL Java_gov_census_cspro_bridge_CNPifFile_GetExternalFilenames
  (JNIEnv * pEnv, jobject, jlong reference)
{
    CNPifFile* pPifFile = (CNPifFile*)reference;
    const auto& external_data_connection_strings = pPifFile->GetExternalDataConnectionStrings();
    jobjectArray stringArray = pEnv->NewObjectArray(external_data_connection_strings.size(),
        JNIReferences::classString, pEnv->NewStringUTF(""));

    int i = 0;
    for( auto itr = external_data_connection_strings.cbegin(); itr != external_data_connection_strings.cend(); itr++, i++ )
    {
        const auto& connection_string = itr->second;
        jstring jFilename = WideToJava(pEnv, connection_string.GetFilename());
        pEnv->SetObjectArrayElement(stringArray, i, jFilename);
        pEnv->DeleteLocalRef(jFilename);
    }

    return stringArray;
}

JNIEXPORT jobjectArray JNICALL Java_gov_census_cspro_bridge_CNPifFile_GetUserFilenames
  (JNIEnv * pEnv, jobject, jlong reference)
{
    const CNPifFile* pPifFile = (CNPifFile*)reference;
    const auto user_files = pPifFile->GetUserFiles();
    jobjectArray stringArray = pEnv->NewObjectArray(user_files.size(),JNIReferences::classString,pEnv->NewStringUTF(""));

    for( size_t i = 0; i < user_files.size(); i++ )
    {
        CString sFilename = user_files[i];
        NormalizePathSlash(sFilename);
        jstring jFilename = WideToJava(pEnv, sFilename);
        pEnv->SetObjectArrayElement(stringArray, i, jFilename);
        pEnv->DeleteLocalRef(jFilename);
    }

    return stringArray;
}

JNIEXPORT jstring JNICALL Java_gov_census_cspro_bridge_CNPifFile_GetWriteFilename
  (JNIEnv* pEnv, jobject, jlong reference)
{
    CNPifFile* pPifFile = (CNPifFile*)reference;
    CString csFilename = pPifFile->GetWriteFName(false);
    NormalizePathSlash(csFilename);
    return WideToJava(pEnv,csFilename);
}

JNIEXPORT jstring JNICALL Java_gov_census_cspro_bridge_CNPifFile_GetOnExitFilename
  (JNIEnv* pEnv, jobject, jlong reference)
{
    CNPifFile* pPifFile = (CNPifFile*)reference;
    CString csFilename = pPifFile->GetOnExitFilename();
    return WideToJava(pEnv,csFilename);
}

JNIEXPORT jstring JNICALL Java_gov_census_cspro_bridge_CNPifFile_CreatePffFromIntentExtras
  (JNIEnv* pEnv, jobject, jstring jpff_file_name, jobject jbundle_extras)
{
    CString pff_filename = JavaToWSZ(pEnv, jpff_file_name);

    CNPifFile pff(pff_filename);

    if( !pff.LoadPifFile() )
        return jpff_file_name;

    std::map<std::wstring, std::wstring> bundle_extras = JavaBundleToMap(pEnv, jbundle_extras);
    for (const auto& [command, argument]: bundle_extras) {
        pff.SetProperties(command, { argument });
    }

    // set the description from the PFF before saving it as a new PFF (which would lead to a different
    // evaluated description if the description was not defined)
    pff.SetAppDescription(pff.GetEvaluatedAppDescription());

    // save the file as a temporary PFF and set that as the new PFF filename
    std::wstring temp_pff_filename = GetUniqueTempFilename(PortableFunctions::PathGetFilename(pff_filename), true);
    pff.SetPifFileName(WS2CS(temp_pff_filename));
    pff.Save(true);

    return WideToJava(pEnv, temp_pff_filename);
}
