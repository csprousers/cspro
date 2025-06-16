#include <engine/StandardSystemIncludes.h>
#include "gov_census_cspro_bridge_CNPifFile.h"
#include "JNIHelpers.h"
#include <zPlatformO/PlatformInterface.h>
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/Encoders.h>
#include <zToolsO/Tools.h>
#include <zUtilO/ConnectionString.h>
#include <zUtilO/Interapp.h>
#include <zNetwork/ParsedUri.h>
#include <zBridgeO/npff.h>
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

    if( pPifFile->GetSingleInputDataConnectionString().HasFilePath() ) {
        return JavaString::ToJava(*pEnv, pPifFile->GetSingleInputDataConnectionString().GetFilePath());
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
        const ConnectionString& connection_string = itr->second;

        if( connection_string.HasFilePath() )
        {
            jstring jFilename = JavaString::ToJava(*pEnv, connection_string.GetFilePath());
            pEnv->SetObjectArrayElement(stringArray, i, jFilename);
            pEnv->DeleteLocalRef(jFilename);
        }
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
        const CString sFilename = user_files[i];
        jstring jFilename = WideToJava(pEnv, PortableFunctions::PathToNativeSlash(sFilename));
        pEnv->SetObjectArrayElement(stringArray, i, jFilename);
        pEnv->DeleteLocalRef(jFilename);
    }

    return stringArray;
}


JNIEXPORT jstring JNICALL Java_gov_census_cspro_bridge_CNPifFile_GetWriteFilename
  (JNIEnv* pEnv, jobject, jlong reference)
{
    CNPifFile* pPifFile = (CNPifFile*)reference;
    const CString csFilename = pPifFile->GetWriteFName(false);
    return WideToJava(pEnv, PortableFunctions::PathToNativeSlash(csFilename));
}


JNIEXPORT jstring JNICALL Java_gov_census_cspro_bridge_CNPifFile_GetOnExitFilename
  (JNIEnv* pEnv, jobject, jlong reference)
{
    CNPifFile* pPifFile = (CNPifFile*)reference;
    CString csFilename = pPifFile->GetOnExitFilename();
    return WideToJava(pEnv,csFilename);
}


template<typename T>
jstring CreatePffWorker(JNIEnv* const pEnv, const std::string& pff_file_path, const T& pff_properties)
{
    // if there are no properties, we can use the PFF that exists
    if( pff_properties.empty() )
        return JavaString::ToJava(*pEnv, pff_file_path);

    // otherwise add the properties to the PFF
    PFF pff(UTF8_TODO::GetCString(pff_file_path));

    if( !pff.LoadPifFile() )
        throw CSProException("Error loading PFF: %s", pff_file_path.c_str());

    for( const auto& [command, argument] : pff_properties )
        pff.SetProperties(UTF8_TODO::GetWide(command), { UTF8_TODO::GetWide(argument) });

    // set the description from the PFF before saving it as a new PFF (which would lead to a different
    // evaluated description if the description was not defined)
    pff.SetAppDescription(pff.GetEvaluatedAppDescription());

    // save the file as a temporary PFF and set that as the new PFF filename
    const std::string temp_pff_file_path = GetUniqueTempFilePath(Path::GetFilename(pff_file_path), true);
    pff.SetPifFileName(UTF8_TODO::GetCString(temp_pff_file_path));
    pff.Save(true);

    return JavaString::ToJava(*pEnv, temp_pff_file_path);
}


JNIEXPORT jstring JNICALL Java_gov_census_cspro_bridge_CNPifFile_CreatePffFromIntentExtras
  (JNIEnv* pEnv, jobject, jstring jPffFilePath, jobject jBundleExtras)
{
    const std::string pff_file_path = JavaString::ToUtf8(*pEnv, jPffFilePath);
    const std::map<std::string, std::string> bundle_extras = JavaBundleToMap(pEnv, jBundleExtras);

    try
    {
        return CreatePffWorker(pEnv, pff_file_path, bundle_extras);
    }

    catch( const CSProException& exception )
    {
        ThrowJavaException(pEnv, exception);
        return nullptr;
    }
}


JNIEXPORT jstring JNICALL Java_gov_census_cspro_bridge_CNPifFile_CreatePffFromDeepLinkUrl
  (JNIEnv* pEnv, jobject, jstring jUrl)
{
    const std::string url = JavaString::ToUtf8(*pEnv, jUrl);
    const ParsedUri parsed_uri(url);

    ASSERT(parsed_uri.scheme == "https" && parsed_uri.domain == "csprousers.org");
    ASSERT(parsed_uri.path == Path::ToForwardSlash(parsed_uri.path));

    try
    {
        constexpr std::string_view PathPrefix_sv = "/pff/";

        if( !SO::StartsWith(parsed_uri.path, PathPrefix_sv) )
            throw ProgrammingErrorException();

        std::vector<std::string> path_segments = SO::SplitString(std::string_view(parsed_uri.path).substr(PathPrefix_sv.length()), '/', true, true);

        if( path_segments.empty() )
            throw CSProException("You must specify the path to a PFF.");

        // process the query string as properties
        std::vector<std::tuple<std::string, std::string>> pff_properties;
        std::string& last_path_segment = path_segments.back();
        const size_t query_string_start_pos = last_path_segment.find('?');

        if( query_string_start_pos != std::string::npos )
        {
            SO::ForeachSection(last_path_segment.substr(query_string_start_pos + 1), '&',
                [&](const std::string_view attribute_and_value_sv)
                {
                    const auto [attribute_sv, value_sv] = SO::GetTextOnEitherSideOfCharacter(attribute_and_value_sv, '=');

                    pff_properties.emplace_back(Encoders::FromUrlQueryString(attribute_sv),
                                                Encoders::FromUrlQueryString(value_sv));
                });

            last_path_segment.erase(query_string_start_pos);
        }

        const std::string& root_directory = PlatformInterface::GetInstance()->GetCSEntryDirectory();
        std::string pff_file_path = root_directory;

        // construct a full file path when more than a filename is provided
        if( path_segments.size() > 1 )
        {
            for( const std::string& path_segment : path_segments )
                Path::MakeCombine(pff_file_path, Encoders::FromPercentEncoding(path_segment));

            if( !PortableFunctions::FileIsRegular(pff_file_path) )
                throw CSProException("No PFF found: %s", pff_file_path.c_str());
        }

        // when only the filename is provided, search for it first in the root directory,
        // and if not found, recursively search for it
        else
        {
            const std::string filename = Encoders::FromPercentEncoding(path_segments.front());
            Path::MakeCombine(pff_file_path, filename);

            if( !PortableFunctions::FileIsRegular(pff_file_path) )
            {
                DirectoryLister directory_lister(true, true, false, true, false);
                directory_lister.SetNameFilter(filename);

                std::vector<std::string> pff_file_paths = directory_lister.GetPaths(root_directory);

                if( pff_file_paths.empty() )
                    throw CSProException("No PFF found with the filename: %s", filename.c_str());

                if( pff_file_paths.size() > 1 )
                    throw CSProException("Multiple PFFs found with the filename: %s", filename.c_str());

                pff_file_path = std::move(pff_file_paths.front());
            }
        }

        return CreatePffWorker(pEnv, pff_file_path, pff_properties);
    }

    catch( const CSProException& exception )
    {
        ThrowJavaException(pEnv, exception);
        return nullptr;
    }
}
