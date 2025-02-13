#include "stdafx.h"
#include "JsonConverter.h"
#include "CaseJsonStreamParser.h"
#include "CaseJsonWriter.h"
#include "ConnectResponse.h"
#include "OauthTokenRequest.h"
#include "OauthTokenResponse.h"
#include "SyncErrorResponse.h"
#include "SyncMessage.h"
#include "SyncRequest.h"
#include <zNetwork/FileInfo.h>
#include <zToolsO/VectorHelpers.h>
#include <zAppO/SyncTypes.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseConstructionHelpers.h>
#include <zCaseO/CaseItemReference.h>
#include <zDataO/SyncJsonBinaryDataReader.h>
#include <external/jsoncons/json.hpp>
#include <easyloggingwrapper.h>

namespace {
    const std::string_view BINARY_CASE_HEADER_sv = "cssync";
}

namespace jsoncons {

    // Extend jsoncons to handle CString
    template<class Json>
    struct json_type_traits<Json, CString>
    {
        using allocator_type = typename Json::allocator_type;

        static bool is(const Json& j) noexcept
        {
            return j.is_string();
        }

        static CString as(const Json& j)
        {
            return UTF8Convert::UTF8ToWide<CString>(j.template as<std::string_view>());
        }
        static Json to_json(const CString& val,
            allocator_type alloc = allocator_type())
        {
            return Json(UTF8Convert::WideToUTF8(val));
        }
    };
}


namespace {

    jsoncons::json_options GetJsonOptions()
    {
        jsoncons::json_options options;
        options.spaces_around_colon(jsoncons::spaces_option::no_spaces);
        options.spaces_around_comma(jsoncons::spaces_option::no_spaces);
        return options;
    }

    jsoncons::compact_json_string_encoder MakeEncoder(std::string& s)
    {
        return jsoncons::compact_json_string_encoder(s, GetJsonOptions());
    }

    FileInfo readDropboxFileInfo(const jsoncons::json& file_info)
    {
        // Response from a file upload is missing the .tag but we know it is
        // a file in that case.
        FileInfo::FileType type;
        if (!file_info.contains(".tag")) {
            type = FileInfo::FileType::File;
        }
        else {
            std::string_view type_string = file_info[".tag"].as<std::string_view>();
            if (type_string == "file") {
                type = FileInfo::FileType::File;
            }
            else if (type_string == "folder") {
                type = FileInfo::FileType::Directory;
            }
            else {
                throw InvalidJsonException(L"file info invalid. Must be directory or file.");
            }
        }

        CString name = file_info["name"].as<CString>();
        CString directory = PortableFunctions::PathGetDirectory<CString>(file_info["path_lower"].as<CString>());

        if (type == FileInfo::FileType::File) {
            int64_t size = file_info["size"].as<int64_t>();
            time_t lastModified = PortableFunctions::ParseRFC3339DateTime(file_info["server_modified"].as<std::wstring>());
            return FileInfo(type, name, directory, size, lastModified, std::wstring());
        }
        else {
            return FileInfo(type, name, directory);
        }
    }

    DictionaryInfo readDictionaryInfo(const jsoncons::json& dictionary_info)
    {
        auto name = dictionary_info["name"].as<CString>();
        auto label = dictionary_info["label"].as<CString>();
        int case_count = dictionary_info.get_value_or<int>("caseCount", -1);
        return DictionaryInfo(
            name,
            label,
            case_count);
    }

    std::vector<ApplicationPackage::FileSpec> readFileSpecArray(const jsoncons::json& json_array)
    {
        std::vector<ApplicationPackage::FileSpec> files;
        for (const auto& json_file_spec : json_array.array_range()) {
            ApplicationPackage::FileSpec file_spec;
            file_spec.Path = json_file_spec["path"].as<CString>();
            file_spec.Signature = json_file_spec.get_value_or<CString>("signature", CString());
            file_spec.OnlyOnFirstInstall = json_file_spec.get_value_or<bool>("onlyOnFirstInstall", false);

            files.emplace_back(file_spec);
        }
        return files;
    }

    template<class Writer>
    void writeFileSpecArray(Writer& writer, const std::vector<ApplicationPackage::FileSpec>& files)
    {
        writer.begin_array();
        for (const auto& f : files) {
            writer.begin_object();
            writer.key("path");
            writer.string_value(UTF8Convert::WideToUTF8(f.Path));
            if (!f.Signature.IsEmpty()) {
                writer.key("signature");
                writer.string_value(UTF8Convert::WideToUTF8(f.Signature));
            }
            if (f.OnlyOnFirstInstall) {
                writer.key("onlyOnFirstInstall");
                writer.bool_value(f.OnlyOnFirstInstall);
            }
            writer.end_object();
        }
        writer.end_array();
    }

    ApplicationPackage readApplicationPackage(const jsoncons::json& json_object)
    {
        const CString name = json_object["name"].as<CString>();
        const CString description = json_object["description"].as<CString>();
        time_t build_time = 0;
        if (json_object.contains("buildTime"))
            build_time = PortableFunctions::ParseRFC3339DateTime(json_object["buildTime"].as<std::wstring>());

        ApplicationPackage::DeploymentType deploy_type = ApplicationPackage::DeploymentType::None;
        CString url;
        if (json_object.contains("deployment")) {
            const auto& deployment = json_object["deployment"];
            if (deployment.contains("type")) {
                auto deploy_type_string = deployment["type"].as<std::string_view>();
                if (deploy_type_string == "CSWeb") {
                    deploy_type = ApplicationPackage::DeploymentType::CSWeb;
                    url = deployment.get_value_or<CString>("cswebUrl", CString());
                }
                else if (deploy_type_string == "FTP") {
                    deploy_type = ApplicationPackage::DeploymentType::FTP;
                    url = deployment.get_value_or<CString>("ftpUrl", CString());
                }
                else if (deploy_type_string == "Dropbox") {
                    deploy_type = ApplicationPackage::DeploymentType::Dropbox;
                }
                else if (deploy_type_string == "LocalFile") {
                    deploy_type = ApplicationPackage::DeploymentType::LocalFile;
                }
                else if (deploy_type_string == "LocalFolder") {
                    deploy_type = ApplicationPackage::DeploymentType::LocalFolder;
                }
            }
        }

        std::vector<ApplicationPackage::FileSpec> files;
        if (json_object.contains("files")) {
            const auto& files_json = json_object["files"];
            files = readFileSpecArray(files_json);
        }

        return ApplicationPackage(name, description, build_time, deploy_type, url, files);
    }
}


std::string JsonConverter::toJson(const Case& data_case, bool CSPro74BackwardsCompatible)
{
    CaseJsonWriter writer;
    writer.SetCSPro74BackwardsCompatible(CSPro74BackwardsCompatible);
    return writer.ToJson(data_case);
}

std::vector<std::shared_ptr<Case>> JsonConverter::caseListFromJson(
    std::string_view json,
    const CaseAccess& case_access,
    std::shared_ptr<CaseConstructionReporter> case_construction_reporter)
{
    try {
        CaseJsonStreamParser parser(case_access, case_construction_reporter);
        std::vector<std::shared_ptr<Case>> cases = parser.Parse(json);
        VectorHelpers::Append(cases, parser.EndParse());
        return cases;
    }
    catch (const jsoncons::json_exception& ex) {
        throw InvalidJsonException(ex.what());
    }
}

std::string JsonConverter::toJson(const std::vector<std::shared_ptr<Case>>& cases, bool CSPro74BackwardsCompatible)
{
    CaseJsonWriter writer;
    writer.SetCSPro74BackwardsCompatible(CSPro74BackwardsCompatible);
    return writer.ToJson(cases);
}

std::string JsonConverter::toJson(const BinaryCaseItem& binary_case_item, const CaseItemIndex& index, bool CSPro74BackwardsCompatible)
{
    CaseJsonWriter writer;
    writer.SetCSPro74BackwardsCompatible(CSPro74BackwardsCompatible);
    return writer.ToJson(binary_case_item, index);
}

std::string JsonConverter::readCasesJsonFromBinary(std::istringstream& iss)
{
    CLOG(INFO, "sync") << "Reading cases json from binary format";

    int version;
    iss.clear();
    iss.seekg(0);

    //read binary json header
    readBinaryJsonHeader(iss, version);
    ASSERT(version == 1);

    //read size of client json.
    int questionaireSize;
    iss.read(reinterpret_cast<char*>(&questionaireSize), sizeof(questionaireSize));

    //read json case content
    std::string casesJson(questionaireSize, '\0');

    iss.read(casesJson.data(), questionaireSize);

    return casesJson;
}

void JsonConverter::readBinaryJsonHeader(std::istringstream& iss, int& version)
{
    std::string buffer;
    buffer.resize(BINARY_CASE_HEADER_sv.length());
    iss.read(&buffer[0], BINARY_CASE_HEADER_sv.length());

    ASSERT(IsBinaryJson(buffer));

    //  version number.
    iss.read(reinterpret_cast<char*>(&version), sizeof(version));
}

void JsonConverter::writeBinaryJsonHeader(std::ostringstream& oss)
{
    //  signature bytes.
    oss << BINARY_CASE_HEADER_sv;

    //  version number.
    const int version = 1;
    oss.write(reinterpret_cast<const char*>(&version), sizeof(version));
}

void JsonConverter::writeBinaryJson(std::ostringstream& oss, const std::string& casesJson, const std::vector<std::pair<const BinaryCaseItem*, CaseItemIndex>>& binaryCases, bool cspro74_compatible /*= false*/)
{
    //write header
    writeBinaryJsonHeader(oss);
    //write cases (size of json and json string)
    const int case_size = casesJson.size();
    oss.write(reinterpret_cast<const char*>(&case_size), sizeof(case_size));
    oss << casesJson;
    //write binary case items
    writeBinaryCaseItems(oss, binaryCases, cspro74_compatible);
}

void JsonConverter::writeBinaryCaseItems(std::ostringstream& oss, const std::vector<std::pair<const BinaryCaseItem*, CaseItemIndex>>& binaryCaseItems, bool cspro74_compatible /*=false*/)
{
    //For each binary case item that needs to be sent in the package write to the output stream
    /*length of binary item json
    * binary item json
    {
			"length": 32000,
			"caseid": "797e52bd-17ad-477f-8aed-6babe4db7c6c",
			"type": "m4a",
			"signature": "1789AFBE3742C6755708BD02798B98F1"
	}
    * byte data of the binary item
    */
    for (const auto& [binary_case_item, index] : binaryCaseItems) {
        //  binary json.
        const std::string binaryCaseItemJSON = toJson(*binary_case_item, index, cspro74_compatible);
        const unsigned int binaryCaseItemJSONLength = binaryCaseItemJSON.length();

        //  write binary json length.
        oss.write(reinterpret_cast<const char*>(&binaryCaseItemJSONLength), sizeof(binaryCaseItemJSONLength));

        //  write binary json.
        oss << binaryCaseItemJSON;

        //  write binary data.
        const BinaryData* binaryData = binary_case_item->GetBinaryData_noexcept(index);
        unsigned int length = 0;
        if (binaryData != nullptr) {
            const std::vector<std::byte>& bytes = binaryData->GetContent();
            //  write binary data length.
            length = bytes.size();
            //write byte size
            oss.write(reinterpret_cast<const char*>(&length), sizeof(length));
            //  write binary data bytes.
            oss.write(reinterpret_cast<const char*>(bytes.data()), length);
            CLOG(INFO, "sync") << "Writing to stream binary item md5: " << binaryData->GetMetadata().GetBinaryDataKey() << " Binary item size: " << length;
        }
        else {
            //write byte size
            ASSERT(false);
            oss.write(reinterpret_cast<const char*>(&length), sizeof(length));
        }
    }
}


void JsonConverter::readBinaryItemsForCase(std::istringstream& content, Case& binaryCase)
{
    if (!binaryCase.GetCaseMetadata().UsesBinaryData())
        return;

    std::map<std::wstring, std::tuple<const BinaryCaseItem*, CaseItemIndex>> md5Map = filterUniqueBinaryItemsForCase(binaryCase);

    if (md5Map.empty()) //no binary items
        return;

    while (!content.eof()) {
        std::string caseID = UTF8Convert::WideToUTF8(binaryCase.GetUuid());
        //  size of binary json.
        unsigned int binaryItemJSONSize = 0;
        std::streampos startPos = content.tellg();
        content.read(reinterpret_cast<char*>(&binaryItemJSONSize), sizeof(binaryItemJSONSize));
        if (binaryItemJSONSize > 0) {
            //read the binary JSON
            /*
            {
                "length": 32000,
                "caseid": "797e52bd-17ad-477f-8aed-6babe4db7c6c",
                "signature": "1789AFBE3742C6755708BD02798B98F1",
                 "metadata":{
                  "mime":"image/jpg",
                  “filename”: test.jpg”
                }
            }
            */

            std::unique_ptr<char[]> binaryItemJson = std::make_unique_for_overwrite<char[]>(binaryItemJSONSize + 1);
            binaryItemJson[binaryItemJSONSize] = 0;
            content.read(binaryItemJson.get(), binaryItemJSONSize);

            std::wstring signature;

            try
            {
                const jsoncons::json json_object = jsoncons::json::parse(binaryItemJson.get());

                signature = UTF8Convert::UTF8ToWide(json_object["signature"].as_string_view());

                //if binary json item does not correspond to the current case return. set the stringstream position back to its state and return
                if (json_object["caseid"].as_string_view().compare(caseID) != 0) {
                    content.seekg(startPos);
                    return;
                }
            }

            catch( const jsoncons::json_exception& ex )
            {
                throw InvalidJsonException(ex.what());
            }

            //read the binary data content size
            unsigned int binaryContentLength = 0;
            content.read(reinterpret_cast<char*>(&binaryContentLength), sizeof(binaryContentLength));

            if (signature.empty() ) {
                ASSERT(binaryContentLength == 0);
                continue;
            }
            else if (binaryContentLength < 0) {
                ASSERT(false);
                continue;
            }
            
            ASSERT(md5Map.count(signature) == 1); //the map should contain the md5 match for the given signature

            //  read the binary data.
            std::unique_ptr<std::vector<std::byte>> bytes = std::make_unique<std::vector<std::byte>>(binaryContentLength);
            content.read(reinterpret_cast<char*>(bytes.get()->data()), binaryContentLength);

            //  look up the md5 in the map and do SetContent on the bytes.
            const auto& find = md5Map.find(signature);
            if (find != md5Map.end()) {
                //  write the binary data and metadata to the corresponding binary data item.
                const BinaryCaseItem& binary_case_item = *std::get<0>(find->second);
                CaseItemIndex& index = std::get<1>(find->second);

                BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(index);
                ASSERT(binary_data_accessor.IsDefined());

                SyncJsonBinaryDataReader* json_binary_data_reader = const_cast<SyncJsonBinaryDataReader*>(
                                                                    assert_cast<const SyncJsonBinaryDataReader*>(binary_data_accessor.GetBinaryDataReader()));
                ASSERT(json_binary_data_reader->GetMetadata().GetBinaryDataKey() == signature);

                CLOG(INFO, "sync") << "Setting the binary content from stream read. Binary item md5: " << signature << " Binary item size: " << bytes->size();

                json_binary_data_reader->SetSyncedContent(std::move(bytes));
            }
            else {
                //This should not happen
                ASSERT(false);
                CLOG(ERROR, "sync") << "Read the binary content from stream but not setting item content for md5: " << signature << " Binary item size: " << bytes->size();
            }
        }
    }
}


//Used for processing binary data items during read. This function sets the reader to SyncJSONReader for binary items
std::map<std::wstring, std::tuple<const BinaryCaseItem*, CaseItemIndex>>JsonConverter::filterUniqueBinaryItemsForCase(const Case& binaryCase)
{
    std::map<std::wstring, std::tuple<const BinaryCaseItem*, CaseItemIndex>> md5Map;

    //Filter binary case items by ignoring items which have same md5 signature to set the byte data from the input stream if it
    //has been sent in the package. Avoid setting the binary content multiple times for the same md5 and serialzing to the database
    binaryCase.ForeachDefinedBinaryCaseItem(
        [&](const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
        {
            const BinaryDataMetadata* binary_data_metadata = binary_case_item.GetBinaryDataMetadata_noexcept(index);

            if( binary_data_metadata != nullptr )
            {
                ASSERT(!binary_data_metadata->GetBinaryDataKey().empty());

                if( md5Map.find(binary_data_metadata->GetBinaryDataKey()) == md5Map.end() )
                    md5Map.try_emplace(binary_data_metadata->GetBinaryDataKey(), &binary_case_item, index);
            }
        });

    return md5Map;
}


void JsonConverter::readBinaryCaseItems(std::istringstream& iss, std::vector<std::shared_ptr<Case>>& cases)
{
    //SYNC_TODO: need to address if binary data size hits  memory limit size. For the first cut assume the binary content sent and received is loaded and read by client
       //for all cases capture binary data items one per md5
    if( cases.empty() || !cases.front()->GetCaseMetadata().UsesBinaryData() ) {
        return;
    }

    try {
        for( const std::shared_ptr<Case>& data_case : cases ) {
            readBinaryItemsForCase(iss, *data_case);
        }
        ASSERT(iss.eof());
    }
    catch (const jsoncons::json_exception& ex) {
        throw InvalidJsonException(ex.what());
    }
}

bool JsonConverter::IsBinaryJson(std::string_view json)
{
    return (json.substr(0, BINARY_CASE_HEADER_sv.length()) == BINARY_CASE_HEADER_sv);
}

ConnectResponse* JsonConverter::connectResponseFromJson(std::string_view json)
{
    try {
        jsoncons::json response = jsoncons::json::parse(json);
        CString deviceId = response["deviceId"].as<CString>();
        float apiVersion = response.get_value_or<float>("apiVersion", 0);
        return new ConnectResponse(deviceId, apiVersion);
    }
    catch (const jsoncons::json_exception& ex) {
        throw InvalidJsonException(ex.what());
    }
}

std::string JsonConverter::toJson(const ConnectResponse& response)
{
    std::string s;
    auto encoder = MakeEncoder(s);
    encoder.begin_object();
    encoder.key("deviceId");
    encoder.string_value(UTF8Convert::WideToUTF8(response.getServerDeviceId()));
    if (!response.getServerName().IsEmpty()) {
        encoder.key("serverName");
        encoder.string_value(UTF8Convert::WideToUTF8(response.getServerName()));
    }
    if (!response.getUserName().IsEmpty()) {
        encoder.key("userName");
        encoder.string_value(UTF8Convert::WideToUTF8(response.getUserName()));
    }
    if (response.getApiVersion() != 0.0) {
        encoder.key("apiVersion");
        encoder.double_value(response.getApiVersion());
    }
    encoder.end_object();
    return s;
}

std::string JsonConverter::toJson(const OauthTokenRequest& request)
{
    std::string s;
    auto encoder = MakeEncoder(s);
    encoder.begin_object();
    encoder.key("client_id");
    encoder.string_value(UTF8Convert::WideToUTF8(request.getClientId()));
    encoder.key("client_secret");
    encoder.string_value(UTF8Convert::WideToUTF8(request.getClientSecret()));
    if (request.getGrantType() == OauthTokenRequest::GrantType::Password) {
        encoder.key("grant_type");
        encoder.string_value("password");
        encoder.key("username");
        encoder.string_value(UTF8Convert::WideToUTF8(request.getUsername()));
        encoder.key("password");
        encoder.string_value(UTF8Convert::WideToUTF8(request.getPassword()));
    } else {
        encoder.key("grant_type");
        encoder.string_value("refresh_token");
        encoder.key("refresh_token");
        encoder.string_value(UTF8Convert::WideToUTF8(request.getRefreshToken()));
    }

    encoder.end_object();
    return s;
}

OauthTokenResponse* JsonConverter::oauthTokenResponseFromJson(std::string_view json)
{
    try {
        jsoncons::json response = jsoncons::json::parse(json);
        auto access_token = response["access_token"].as<CString>();
        auto expires_in = response["expires_in"].as<int>();
        auto token_type = response["token_type"].as<CString>();
        CString scope;
        const auto& json_scope = response["scope"];
        if (!json_scope.is_null())
            scope = response["scope"].as<CString>();
        auto refresh_token = response["refresh_token"].as<CString>();
        return new OauthTokenResponse(access_token, expires_in, token_type, scope, refresh_token);
    }
    catch (const jsoncons::json_exception& ex) {
        throw InvalidJsonException(ex.what());
    }
}

JSONCONS_ENUM_NAME_TRAITS(FileInfo::FileType, (File, "file"), (Directory, "directory"))

std::vector<FileInfo>* JsonConverter::fileInfoFromJson(std::string_view json_string)
{
    try {
        jsoncons::json json_array = jsoncons::json::parse(json_string);
        std::unique_ptr<std::vector<FileInfo>> file_infos(new std::vector<FileInfo>());
        for (const auto& json_file_info : json_array.array_range()) {
            FileInfo::FileType type = json_file_info.at("type").as<FileInfo::FileType>();
            CString name = json_file_info.at("name").as<CString>();
            CString directory = json_file_info.at("directory").as<CString>();
            if (type == FileInfo::FileType::File) {
                std::wstring md5 = json_file_info.at("md5").as<std::wstring>();
                int64_t size = json_file_info.at("size").as<int64_t>();
                time_t lastModified(0);
                if (json_file_info.contains("lastModified"))
                    lastModified = PortableFunctions::ParseRFC3339DateTime(json_file_info.at("lastModified").as<std::wstring>());
                file_infos->emplace_back(type, name, directory, size, lastModified, std::move(md5));
            }
            else {
                file_infos->emplace_back(type, name, directory);
            }
        }
        return file_infos.release();
    }
    catch (const jsoncons::json_exception& ex) {
        throw InvalidJsonException(ex.what());
    }
}

std::vector<FileInfo>& JsonConverter::dropboxDirectoryListingFromJson(std::string_view json_string, std::vector<FileInfo>& file_infos)
{
    try {
        jsoncons::json json_object = jsoncons::json::parse(json_string);
        auto entries = json_object["entries"];
        for (const auto& json_file_info : entries.array_range()) {
            file_infos.emplace_back(readDropboxFileInfo(json_file_info));
        }
        return file_infos;
    }
    catch (const jsoncons::json_exception& ex) {
        throw InvalidJsonException(ex.what());
    }
}

FileInfo JsonConverter::dropboxFileInfoFromJson(std::string_view json_string)
{
    try {
        jsoncons::json file_info = jsoncons::json::parse(json_string);
        return readDropboxFileInfo(file_info);
    }
    catch (const jsoncons::json_exception& ex) {
        throw InvalidJsonException(ex.what());
    }
}

std::string JsonConverter::toJson(const std::vector<FileInfo>& files)
{
    std::string s;
    auto encoder = MakeEncoder(s);
    encoder.begin_array();
    for (std::vector<FileInfo>::const_iterator i = files.begin(); i != files.end(); ++i) {
        encoder.begin_object();
        encoder.key("type");
        if (i->getType() == FileInfo::FileType::File)
            encoder.string_value("file");
        else
            encoder.string_value("directory");

        encoder.key("name");
        encoder.string_value(UTF8Convert::WideToUTF8(i->getName()));
        encoder.key("directory");
        encoder.string_value(UTF8Convert::WideToUTF8(i->getDirectory()));
        if (i->getType() == FileInfo::FileType::File) {
            encoder.key("md5");
            encoder.string_value(UTF8Convert::WideToUTF8(i->getMd5()));
            encoder.key("size");
            encoder.int64_value(i->getSize());
            encoder.key("lastModified");
            encoder.string_value(UTF8Convert::WideToUTF8(PortableFunctions::TimeToRFC3339String(i->getLastModified())));
        }
        encoder.end_object();
    }

    encoder.end_array();
    return s;
}

CString JsonConverter::dropboxAccountEmailFromJson(std::string_view json_string)
{
    try {
        jsoncons::json json_object = jsoncons::json::parse(json_string);
        return json_object["email"].as<CString>();
    }
    catch (const jsoncons::json_exception& ex) {
        throw InvalidJsonException(ex.what());
    }
}

CString JsonConverter::dropboxRefreshTokenFromJson(std::string_view json_string)
{
    CString refreshToken;
    try {
        jsoncons::json json_object = jsoncons::json::parse(json_string);
        refreshToken = json_object["refresh_token"].as<CString>();
        return refreshToken;
    }
    catch (const jsoncons::json_exception& ex) {
        throw InvalidJsonException(ex.what());
    }
}

CString JsonConverter::dropboxAccessTokenFromJson(std::string_view json_string)
{
    CString accessToken;
    try {
        jsoncons::json json_object = jsoncons::json::parse(json_string);
        accessToken = json_object["access_token"].as<CString>();
        return accessToken;
    }
    catch (const jsoncons::json_exception& ex) {
        throw InvalidJsonException(ex.what());
    }
}
SyncErrorResponse* JsonConverter::syncErrorResponseFromJson(std::string_view json_string)
{
    try {
        jsoncons::json json_object = jsoncons::json::parse(json_string);
        auto error = json_object["code"].as<CString>();
        auto description = json_object["message"].as<CString>();
        return new SyncErrorResponse(error, description);
    }
    catch (const jsoncons::json_exception& ex) {
        throw InvalidJsonException(ex.what());
    }
}

SyncErrorResponse* JsonConverter::dropboxSyncErrorResponseFromJson(std::string_view json_string)
{
    try {
        jsoncons::json json_object = jsoncons::json::parse(json_string);
        auto description = json_object["error_summary"].as<CString>();
        return new SyncErrorResponse(description);
    }
    catch (const jsoncons::json_exception& ex) {
        throw InvalidJsonException(ex.what());
    }
}

std::string JsonConverter::toJson(const SyncErrorResponse& response)
{
    std::string s;
    auto encoder = MakeEncoder(s);
    encoder.begin_object();
    encoder.key("code");
    encoder.string_value(UTF8Convert::WideToUTF8(response.GetError()));
    encoder.key("message");
    encoder.string_value(UTF8Convert::WideToUTF8(response.GetErrorDescription()));
    encoder.end_object();
    return s;
}

std::string JsonConverter::toJson(const DictionaryInfo& info)
{
    std::string s;
    auto encoder = MakeEncoder(s);
    encoder.begin_object();
    encoder.key("name");
    encoder.string_value(UTF8Convert::WideToUTF8(info.m_name));
    encoder.key("label");
    encoder.string_value(UTF8Convert::WideToUTF8(info.m_label));
    if (info.m_caseCount >= 0) {
        encoder.key("caseCount");
        encoder.int64_value(info.m_caseCount);
    }
    encoder.end_object();
    return s;
}

DictionaryInfo JsonConverter::dictionaryInfoFromJson(std::string_view json_string)
{
    try {
        jsoncons::json json_object = jsoncons::json::parse(json_string);
        return readDictionaryInfo(json_object);
    }
    catch (const jsoncons::json_exception& ex) {
        throw InvalidJsonException(ex.what());
    }
}

std::vector<DictionaryInfo> JsonConverter::dictionaryListFromJson(std::string_view json_string)
{
    try {
        jsoncons::json json_array = jsoncons::json::parse(json_string);
        std::vector<DictionaryInfo> dictionaries;
        for (const auto& json_dictionary_info : json_array.array_range())
            dictionaries.emplace_back(readDictionaryInfo(json_dictionary_info));
        return dictionaries;
    }
    catch (const jsoncons::json_exception& ex) {
        throw InvalidJsonException(ex.what());
    }
}

CString JsonConverter::sessionIdFromJson(std::string_view json_string)
{
    try {
        jsoncons::json json_object = jsoncons::json::parse(json_string);
        return json_object["session_id"].as<CString>();
    }
    catch (const jsoncons::json_exception& ex) {
        throw InvalidJsonException(ex.what());
    }
}

CString JsonConverter::cursorFromJson(std::string_view json_string)
{
    try {
        jsoncons::json json_object = jsoncons::json::parse(json_string);
        return json_object["cursor"].as<CString>();
    }
    catch (const jsoncons::json_exception& ex) {
        throw InvalidJsonException(ex.what());
    }
}

bool JsonConverter::hasMoreFromJson(std::string_view json_string)
{
    try {
        jsoncons::json json_object = jsoncons::json::parse(json_string);
        return json_object["has_more"].as<bool>();
    }
    catch (const jsoncons::json_exception& ex) {
        throw InvalidJsonException(ex.what());
    }
}

std::string JsonConverter::toJson(const SyncRequest& request)
{
    std::string s;
    auto encoder = MakeEncoder(s);
    encoder.begin_object();
    encoder.key("deviceId");
    encoder.string_value(UTF8Convert::WideToUTF8(request.getDeviceId()));
    encoder.key("universe");
    encoder.string_value(UTF8Convert::WideToUTF8(request.getUniverse()));
    encoder.end_object();
    return s;
}

std::string JsonConverter::toJson(const ApplicationPackage& package)
{
    std::string s;
    auto encoder = MakeEncoder(s);
    encoder.begin_object();
    encoder.key("name");
    encoder.string_value(UTF8Convert::WideToUTF8(package.getName()));
    encoder.key("description");
    encoder.string_value(UTF8Convert::WideToUTF8(package.getDescription()));
    encoder.key("buildTime");
    encoder.string_value(UTF8Convert::WideToUTF8(PortableFunctions::TimeToRFC3339String(package.getBuildTime())));
    encoder.key("files");
    writeFileSpecArray(encoder, package.getFiles());
    encoder.end_object();
    return s;
}

std::string JsonConverter::toJson(const std::vector<ApplicationPackage::FileSpec>& files)
{
    std::string s;
    auto encoder = MakeEncoder(s);
    writeFileSpecArray(encoder, files);
    return s;
}

ApplicationPackage JsonConverter::applicationPackageFromJson(std::string_view json_string)
{
    try {
        jsoncons::json json_object = jsoncons::json::parse(json_string);
        return readApplicationPackage(json_object);
    }
    catch (const jsoncons::json_exception& ex) {
        throw InvalidJsonException(ex.what());
    }
}

std::vector<ApplicationPackage> JsonConverter::applicationPackageListFromJson(std::string_view json_string)
{
    try {
        jsoncons::json json_array = jsoncons::json::parse(json_string);
        std::vector<ApplicationPackage> packages;
        for (const auto& json_package : json_array.array_range())
            packages.emplace_back(readApplicationPackage(json_package));
        return packages;
    }
    catch (const jsoncons::json_exception& ex) {
        throw InvalidJsonException(ex.what());
    }
}

std::vector<ApplicationPackage::FileSpec> JsonConverter::fileSpecListFromJson(std::string_view json_string)
{
    try {
        jsoncons::json json_array = jsoncons::json::parse(json_string);
        return readFileSpecArray(json_array);
    }
    catch (const jsoncons::json_exception& ex) {
        throw InvalidJsonException(ex.what());
    }
}

std::string JsonConverter::toJson(const SyncMessage& sync_message)
{
    std::string s;
    auto encoder = MakeEncoder(s);
    encoder.begin_object();

    encoder.key("type");
    encoder.string_value(sync_message.type);

    encoder.key("key");
    encoder.string_value(UTF8Convert::WideToUTF8(sync_message.key));

    encoder.key("value");
    encoder.string_value(UTF8Convert::WideToUTF8(sync_message.value));

    encoder.end_object();
    return s;
}

SyncMessage JsonConverter::syncMessageFromJson(std::string_view json_string)
{
    try {
        const jsoncons::json json_object = jsoncons::json::parse(json_string);

        if( json_object["type"].as<std::string_view>() != SyncMessage::type ) {
            throw InvalidJsonException(L"unknown sync message type");
        }

        return SyncMessage { json_object["key"].as<CString>(), json_object["value"].as<CString>() };
    }

    catch (const jsoncons::json_exception& ex) {
        throw InvalidJsonException(ex.what());
    }
}
