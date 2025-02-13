#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/ApplicationPackage.h>
#include <zSyncO/DictionaryInfo.h>
#include <zSyncO/SyncMessage.h>
#include <zNetwork/FileInfo.h>
#include <zToolsO/CSProException.h>
#include <external/jsoncons/json.hpp>

class BinaryCaseItem;
class Case;
class CaseAccess;
class CaseConstructionReporter;
class CaseItemIndex;
class ConnectResponse;
class OauthTokenRequest;
class OauthTokenResponse;
class SyncErrorResponse;
class SyncRequest;


// Exception thrown by the fromJson methods
struct InvalidJsonException : public CSProException
{
    InvalidJsonException(CString msg)
        : CSProException(msg)
    {}
    InvalidJsonException(const char* msg)
        : CSProException(msg)
    {}
};


// Convert requests/responses to/from server between JSON text and corresponding C++ classes.
//
class SYNC_API JsonConverter
{
public:
    bool IsBinaryJson(std::string_view json);
    std::vector<std::shared_ptr<Case>> caseListFromJson(std::string_view json, const CaseAccess& case_access, std::shared_ptr<CaseConstructionReporter> case_construction_reporter = nullptr);
    std::string readCasesJsonFromBinary(std::istringstream& iss);
    void readBinaryCaseItems(std::istringstream& iss, std::vector<std::shared_ptr<Case>>& cases);
    void readBinaryItemsForCase(std::istringstream& iss, Case& binaryCase);
    std::map<std::wstring, std::tuple<const BinaryCaseItem*, CaseItemIndex>> filterUniqueBinaryItemsForCase(const Case& binaryCase);

    std::string toJson(const std::vector<std::shared_ptr<Case>>& cases, bool CSPro74BackwardsCompatible = false);
    void writeBinaryCaseItems(std::ostringstream& oss, const std::vector<std::pair<const BinaryCaseItem*, CaseItemIndex>>& binaryCases, bool cspro74_compatible = false);
    void writeBinaryJson(std::ostringstream& oss, const std::string& casesJson, const std::vector<std::pair<const BinaryCaseItem*, CaseItemIndex>>& binaryCases, bool cspro74_compatible = false);

    void writeBinaryJsonHeader(std::ostringstream& oss);
    void readBinaryJsonHeader(std::istringstream& iss, int& version);

    std::string toJson(const Case& data_case, bool CSPro74BackwardsCompatible);
    std::string toJson(const BinaryCaseItem& binary_case_item, const CaseItemIndex& index, bool CSPro74BackwardsCompatible = false);

    std::string toJson(const ConnectResponse& response);
    ConnectResponse* connectResponseFromJson(std::string_view json);

    std::string toJson(const OauthTokenRequest& response);

    OauthTokenResponse* oauthTokenResponseFromJson(std::string_view json);

    std::vector<FileInfo>* fileInfoFromJson(std::string_view json);
    std::vector<FileInfo>& dropboxDirectoryListingFromJson(std::string_view json, std::vector<FileInfo>& fileInfos);
    FileInfo dropboxFileInfoFromJson(std::string_view json);
    std::string toJson(const std::vector<FileInfo>& info);
    CString dropboxAccountEmailFromJson(std::string_view json);
    CString dropboxRefreshTokenFromJson(std::string_view json);
    CString dropboxAccessTokenFromJson(std::string_view json);

    SyncErrorResponse* syncErrorResponseFromJson(std::string_view json);
    SyncErrorResponse* dropboxSyncErrorResponseFromJson(std::string_view json);
    std::string toJson(const SyncErrorResponse& response);

    std::string toJson(const DictionaryInfo& info);
    DictionaryInfo dictionaryInfoFromJson(std::string_view json);

    CString sessionIdFromJson(std::string_view json);
    std::vector<DictionaryInfo> dictionaryListFromJson(std::string_view json);
    CString cursorFromJson(std::string_view json);
    bool hasMoreFromJson(std::string_view json);

    std::string toJson(const SyncRequest& request);

    std::string toJson(const ApplicationPackage& package);
    std::string toJson(const std::vector<ApplicationPackage::FileSpec>& files);
    std::vector<ApplicationPackage::FileSpec> fileSpecListFromJson(std::string_view json);
    ApplicationPackage applicationPackageFromJson(std::string_view json);
    std::vector<ApplicationPackage> applicationPackageListFromJson(std::string_view json);

    std::string toJson(const SyncMessage& sync_message);
    SyncMessage syncMessageFromJson(std::string_view json);
};
