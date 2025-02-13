#include "stdafx.h"
#include "DropboxSyncServerConnection.h"
#include "ApplicationPackage.h"
#include "ConnectResponse.h"
#include "IDataChunk.h"
#include "IDropboxAuthDialog.h"
#include "FileBasedParadataSyncer.h"
#include "FileBasedSmartSyncer.h"
#include "JsonConverter.h"
#include "SyncErrorResponse.h"
#include "SyncException.h"
#include "SyncRequest.h"
#include <zToolsO/ApiKeys.h>
#include <zNetwork/IHttpConnection.h>
#include <zUtilO/CredentialStore.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/TemporaryFile.h>
#include <zUtilO/Versioning.h>
#include <zDictO/DDClass.h>
#include <zDataO/ISyncableDataRepository.h>
#include <easyloggingwrapper.h>
#include <zToolsO/base64.h>
#include <external/jsoncons/json.hpp>
#include <assert.h>
#include <sstream>

namespace {
    JsonConverter jsonConverter;

    CString createAuthHeader(CString token)
    {
        return L"Authorization: Bearer " + token;
    }

    void handleStreamError(CString localFilePath, CString remoteFilePath, CString streamError)
    {
        CString filename = PortableFunctions::PathGetFilename(localFilePath);
        CLOG(ERROR, "sync") << ": " << UTF8Convert::WideToUTF8(streamError) << " "
            << UTF8Convert::WideToUTF8(filename);

        CString msg;
        msg.Format(L"\"%s\": %s %s", (LPCTSTR)remoteFilePath, (LPCTSTR)streamError, (LPCTSTR)filename);
        throw SyncError(100111, msg);
    }

    const CString RPCEndpoint = _T("https://api.dropboxapi.com/2/");
    const CString ContentEndpoint = _T("https://content.dropboxapi.com/2/");

    // Dropbox API does not support paths with non-ascii chars unless they are escaped as JSON style hex char codes e.g. \uXXXX
    // See https://www.dropboxforum.com/t5/API-support/HTTP-header-quot-Dropbox-API-Arg-quot-could-not-decode-input-as/m-p/195155#M8876
    CString escapeNonAsciiChars(CString s)
    {
        CString result;
        for (const TCHAR* c = (LPCTSTR)s; *c != 0; ++c) {
            if (*c < 127)
                result += *c;
            else {
                result.AppendFormat(_T("\\u%04x"), *c);
            }
        }
        return result;
    }
}

DropboxSyncServerConnection::DropboxSyncServerConnection(std::shared_ptr<IHttpConnection> pConnection, CString accessToken) :
    m_pConnection(pConnection),
    m_accessToken(accessToken),
    m_refreshToken(),
    m_pAuthDialog(nullptr),
    m_pSyncCredentialStore(nullptr),
    m_pSmartSyncer(new FileBasedSmartSyncer<DropboxSyncServerConnection>(this)),
    m_dataChunk(DefaultDataChunk())
{
    init(pConnection);
}

DropboxSyncServerConnection::DropboxSyncServerConnection(std::shared_ptr<IHttpConnection> pConnection,
    IDropboxAuthDialog * pLoginDlg, ICredentialStore * pCredentialStore) :
    m_pConnection(pConnection),
    m_accessToken(),
    m_refreshToken(),
    m_pAuthDialog(pLoginDlg),
    m_pSyncCredentialStore(pCredentialStore),
    m_pSmartSyncer(new FileBasedSmartSyncer<DropboxSyncServerConnection>(this)),
    m_dataChunk(DefaultDataChunk())
{
    init(pConnection);
}

DropboxSyncServerConnection::~DropboxSyncServerConnection()
{
    delete m_pSmartSyncer;
}

void DropboxSyncServerConnection::init(std::shared_ptr<IHttpConnection> pConnection)
{
    m_isValidDb = true;
    try {
        m_dropboxDb.Init();
    }
    catch (const SQLiteError&) {
        // ALW - If database is in a bad state ignore it as it's
        // ALW - only an optimization for downloads.
        m_isValidDb = false;
    }
}

ConnectResponse* DropboxSyncServerConnection::connect()
{
    bool bNewAccessToken = false;
    CString dbxCredentialV2;

    //Scenario when there is no saved access token.
    if (m_accessToken.IsEmpty()) {
        // Check for a saved token
        m_accessToken = WS2CS(m_pSyncCredentialStore->Retrieve(_T("DropboxV2")));
        if( m_accessToken.IsEmpty() )
            m_accessToken = WS2CS(m_pSyncCredentialStore->Retrieve(_T("Dropbox")));
        
        if (m_accessToken.IsEmpty()) {
            // No saved token, ask for a new authorization
            dbxCredentialV2 = m_accessToken = m_pAuthDialog->Show(DropboxKeys::client_id);
            if (m_accessToken.IsEmpty())
                throw SyncCancelException();
            bNewAccessToken = true;
        }
    }

    //if the credentials retrieved is a json string get the refreshToken from it.
    try {
        m_refreshToken = jsonConverter.dropboxRefreshTokenFromJson(UTF8Convert::WideToUTF8(m_accessToken));
        if(bNewAccessToken)
            m_accessToken = jsonConverter.dropboxAccessTokenFromJson(UTF8Convert::WideToUTF8(m_accessToken));
    }
    catch (const InvalidJsonException& /*ex*/) {
        //not a json string or old api credentials. Ignore the error and try old style authorization
        //throw InvalidJsonException(ex.what());
    }

    //scenario 2 - short lived tokens always use refresh token to get a new access token
    if(!m_refreshToken.IsEmpty() && !bNewAccessToken) {
        //get a new access  token and use it.
        std::string responseBody;
        int status = sendRefreshTokenConnectionRequest(responseBody);
        if (status == 200){
            try {
                m_accessToken = jsonConverter.dropboxAccessTokenFromJson(responseBody);
            }
            catch (const InvalidJsonException& /*ex*/) {
                //not a json string or invalid credentials. First try old method
                //throw InvalidJsonException(ex.what());
            }
        }
    }
    // Test that we can connect
    m_authHeader = createAuthHeader(m_accessToken);
    std::string responseBody;
    int status = testConnection(responseBody);
    if (status == 401) {
        // Invalid access token, reauthorize and try again
        dbxCredentialV2 = m_pAuthDialog->Show(DropboxKeys::client_id);
        if (m_accessToken.IsEmpty())
            throw SyncCancelException();
        bNewAccessToken = true;

        m_refreshToken = jsonConverter.dropboxRefreshTokenFromJson(UTF8Convert::WideToUTF8(dbxCredentialV2));
        m_accessToken = jsonConverter.dropboxAccessTokenFromJson(UTF8Convert::WideToUTF8(dbxCredentialV2));
        m_authHeader = createAuthHeader(m_accessToken);
        status = testConnection(responseBody);
    }

    if (status != 200) {
        // Failed to connect
        handleServerErrorResponse(100101, status, responseBody);
    }

    CString accountEmail = jsonConverter.dropboxAccountEmailFromJson(responseBody);

    CLOG(INFO, "sync") << "Dropbox account " << UTF8Convert::WideToUTF8(accountEmail);

    if (bNewAccessToken)
        m_pSyncCredentialStore->Store(_T("DropboxV2"), CS2WS(dbxCredentialV2));

    // Success
    return new ConnectResponse(L"Dropbox", L"Dropbox", accountEmail);
}

int DropboxSyncServerConnection::testConnection(std::string& responseBody)
{
    // ALW - Test connectivity by hitting a Dropbox endpoint and verifying the status.
    // ALW - The particular endpoint is unimportant.
    CString url = RPCEndpoint + "users/get_current_account";
    return sendTestConnectionRequest(url, responseBody);
}

int DropboxSyncServerConnection::sendRefreshTokenConnectionRequest(std::string& responseBody)
{
    const CString url = _T("https://api.dropbox.com/oauth2/token");

    const std::string requestBody = "grant_type=refresh_token&refresh_token=" + UTF8Convert::WideToUTF8(m_refreshToken);
    std::istringstream postDataStream(requestBody);

    HeaderList headers;
    CString keySecret = CString(DropboxKeys::client_id) + L":" + CString(DropboxKeys::client_secret);
    std::string utf8_text = UTF8Convert::WideToUTF8(keySecret);
    headers.push_back(CString(L"Authorization: Basic ") + Base64::Encode<std::wstring>(utf8_text.c_str(),utf8_text.length()).c_str());
    headers.push_back(L"Content-Type: application/x-www-form-urlencoded");

    auto request = HttpRequestBuilder(url).headers(headers).post(postDataStream, requestBody.size()).build();
    auto response = m_pConnection->Request(request);
    responseBody = response.body.ToString();

    return response.http_status;
}
int DropboxSyncServerConnection::sendTestConnectionRequest(CString url, std::string& responseBody)
{
    const std::string requestBody = "null";
    std::istringstream postDataStream(requestBody);

    HeaderList headers;
    headers.push_back(m_authHeader);
    headers.push_back(L"Content-Type: application/json; charset=utf-8");

    auto request = HttpRequestBuilder(url).headers(headers).post(postDataStream, requestBody.size()).build();
    auto response = m_pConnection->Request(request);

    responseBody = response.body.ToString();
    return response.http_status;
}

void DropboxSyncServerConnection::disconnect()
{
}

SyncGetResponse DropboxSyncServerConnection::getData(const SyncRequest& syncRequest)
{
    CLOG(INFO, "sync") << "Start Dropbox case download for dictionary " << UTF8Convert::WideToUTF8(syncRequest.getDictionary().GetName());

    SyncGetResponse response = m_pSmartSyncer->getData(syncRequest);

    CLOG(INFO, "sync") << "Dropbox case download complete ";

    return response;
}

SyncPutResponse DropboxSyncServerConnection::putData(const SyncRequest& syncRequest)
{
    CLOG(INFO, "sync") << "Start Dropbox case upload for dictionary " << UTF8Convert::WideToUTF8(syncRequest.getDictionary().GetName());

    SyncPutResponse response = m_pSmartSyncer->putData(syncRequest);

    CLOG(INFO, "sync") << "Dropbox case upload complete.";

    return response;

}

IDataChunk& DropboxSyncServerConnection::getChunk()
{
    return m_dataChunk;
}

std::vector<DictionaryInfo> DropboxSyncServerConnection::getDictionaries()
{
    return m_pSmartSyncer->getDictionaries();
}

CString DropboxSyncServerConnection::getDictionary(CString dictionaryName)
{
    return m_pSmartSyncer->getDictionary(dictionaryName);
}

void DropboxSyncServerConnection::putDictionary(CString dictPath)
{
    m_pSmartSyncer->putDictionary(dictPath);
}

void DropboxSyncServerConnection::deleteDictionary(CString dictName)
{
    throw SyncError(100124);
}

bool DropboxSyncServerConnection::getFile(CString remoteFilePath, CString tempLocalFilePath, CString actualLocalFilePath, CString md5 /*= CString()*/)
{
    CString url = ContentEndpoint + "files/download";

    int status = sendGetFileRequest(url, tempLocalFilePath, remoteFilePath, actualLocalFilePath, md5);

    if (status != 200 && status != 304) {
        throw SyncError(100110, remoteFilePath);
    }

    return status == 200;
}

std::unique_ptr<TemporaryFile> DropboxSyncServerConnection::getFileIfExists(const CString& remoteFilePath)
{
    if( fileExists(remoteFilePath) )
    {
        auto temporary_file = std::make_unique<TemporaryFile>();

        if( getFile(remoteFilePath, WS2CS(temporary_file->GetPath()), WS2CS(temporary_file->GetPath()), CString()) )
            return temporary_file;
    }

    return nullptr;
}

int DropboxSyncServerConnection::sendGetFileRequest(CString url, CString localFilePath, CString remoteFilePath, CString actualFilePath, CString checksum)
{
    const std::string requestBody = "";
    std::istringstream postDataStream(requestBody);

    HeaderList headers;
    headers.push_back(m_authHeader);
    if (remoteFilePath.Left(1) != _T(".")) {
        // ALW - Prepend forward slash to paths that don't start with "." or ".."
        remoteFilePath = PortableFunctions::PathAppendForwardSlashToPath<CString>(L"/", remoteFilePath);
    }
    headers.push_back(L"Dropbox-API-Arg: {\"path\": \"" + escapeNonAsciiChars(remoteFilePath) + "\"}");
    headers.push_back(L"Content-Type: ");
    addEtag(actualFilePath, checksum, headers);

    auto request = HttpRequestBuilder(url).headers(headers).post(postDataStream, requestBody.size()).build();
    auto response = m_pConnection->Request(request);

    if (response.http_status == 200) {
        std::ofstream fileStream(localFilePath, std::ios::binary);
        fileStream << response.body;
        saveEtag(localFilePath, actualFilePath, response.headers, fileStream);
    }
    else if (response.http_status != 304) {
        CLOG(ERROR, "sync") << "Error downloading file " << remoteFilePath;
        CLOG(ERROR, "sync") << response.body.ToString();
    }

    return response.http_status;
}

void DropboxSyncServerConnection::addEtag(CString actualFilePath, CString checksum, HeaderList& headers)
{
    if (m_isValidDb) {
        try {
            DropboxDb::Record record;
            m_dropboxDb.SelectRecord(actualFilePath, record);
            if (!record.isEmpty()) {
                // ALW - File has previously been downloaded
                if (PortableFunctions::FileExists(actualFilePath)) {
                    // ALW - Local file still exists
                    if (checksum == record.getChecksum()) {
                        // ALW - Local file has not been modified, so use etag.
                        headers.push_back(CString(_T("If-None-Match: ")) + record.getEtag());
                    }
                }
            }
        }
        catch (const SQLiteError&) {
            // ALW - If database is in a bad state ignore it as it's
            // ALW - only an optimization for downloads.
            m_isValidDb = false;
        }
    }
}

void DropboxSyncServerConnection::saveEtag(CString localFilePath, CString actualFilePath, HeaderList responseHeaders, std::ofstream& fileStream)
{
    if (m_isValidDb) {
        try {
            // ALW - The file was downloaded. Update database.
            fileStream.close(); // ALW - Manually close, so pending output is written to file
            const CString checksum = WS2CS(PortableFunctions::FileMd5(localFilePath));
            const CString etag = responseHeaders.value("etag");

            DropboxDb::Record record;
            m_dropboxDb.SelectRecord(actualFilePath, record);
            if (record.isEmpty()) {
                // ALW - Record does not exist, so insert new record
                m_dropboxDb.InsertRecord(actualFilePath, checksum, etag);
            }
            else {
                // ALW - Record already exists, so update record
                if (checksum != record.getChecksum()) {
                    m_dropboxDb.UpdateField("checksum", checksum, "local_file_path", actualFilePath);
                }

                if (etag != record.getEtag()) {
                    m_dropboxDb.UpdateField("etag", etag, "local_file_path", actualFilePath);
                }
            }
        }
        catch (const SQLiteError&) {
            // ALW - If database is in a bad state ignore it as it's
            // ALW - only an optimization for downloads.
            m_isValidDb = false;
        }
    }
}

void DropboxSyncServerConnection::putFile(CString localPath, CString remotePath)
{
    int64_t fileSizeBytes = PortableFunctions::FileSize(localPath);
    CLOG(INFO, "sync") << "Uploading " << fileSizeBytes << " bytes";

#ifdef ANDROID
    // Due to limitations in Android httpconnection API we can only post
    // data up to 2GB. The Java method HttpUrlConnection.setFixedLengthStreamingMode
    // takes a Java int. In Android API 19 a method that takes a long was added but
    // in order to support earlier versions of Android we use the older version
    // of the method that takes an int which means we can only handle posts up to 2GB.
    if (fileSizeBytes > 0x7FFFFFFF) {
        throw SyncError(100117, localPath);
    }
#endif

    const int64_t chunkSize = 10 * 1024 * 1024;
    std::string responseBody;
    int status = 0;
    if (fileSizeBytes <= chunkSize) {
        CString url = ContentEndpoint + "files/upload";
        status = sendPutFileRequest(url, localPath, remotePath, fileSizeBytes, responseBody);
    }
    else {
        status = sendChunkedPutFileRequest(localPath, remotePath, fileSizeBytes, chunkSize, responseBody);
    }

    if (status != 200) {
        handleServerErrorResponse(100111, status, remotePath, responseBody);
    }
}

#include <chrono>

int DropboxSyncServerConnection::sendPutFileRequest(CString url, std::istream& uploadStream, CString remoteFilePath, int64_t fileSizeBytes, std::string& responseBody)
{
    HeaderList headers;
    headers.push_back(m_authHeader);
    if (remoteFilePath.Left(1) != _T(".")) {
        // ALW - Prepend forward slash to paths that don't start with "." or ".."
        remoteFilePath = PortableFunctions::PathAppendForwardSlashToPath<CString>(L"/", remoteFilePath);
    }
    headers.push_back(CString(L"Dropbox-API-Arg: {\"path\": \"") + escapeNonAsciiChars(remoteFilePath) + CString("\""
        ", \"mode\": \"overwrite\", \"autorename\": true, \"mute\": true}"));
    headers.push_back(L"Content-Type: application/octet-stream");

    auto request = HttpRequestBuilder(url).headers(headers).post(uploadStream, fileSizeBytes).build();
    auto response = m_pConnection->Request(request);

    responseBody = response.body.ToString();
    return response.http_status;
}

int DropboxSyncServerConnection::sendPutFileRequest(CString url, CString localFilePath, CString remoteFilePath, int64_t fileSizeBytes, std::string& responseBody)
{
    std::ifstream uploadStream(localFilePath, std::ios::binary);
    return sendPutFileRequest(url, uploadStream, remoteFilePath, fileSizeBytes, responseBody);
}

int DropboxSyncServerConnection::sendChunkedPutFileRequest(CString localFilePath, CString remoteFilePath, int64_t fileSizeBytes, int64_t chunkSize, std::string& responseBody)
{
    std::ifstream fileStream(localFilePath, std::ios::binary);
    if (!fileStream) {
        handleStreamError(localFilePath, remoteFilePath, "Unable to open");
    }

    if (m_pListener) {
        m_pListener->setProgressTotal(fileSizeBytes);
    }

    // Start upload session
    int status = uploadSessionStart(localFilePath, remoteFilePath, chunkSize, fileStream, responseBody);
    if (m_pListener) {
        m_pListener->addToProgressPreviousStepsTotal(chunkSize);
    }

    if (status == 200) {
        const CString sessionId = jsonConverter.sessionIdFromJson(responseBody);
        int64_t offset = chunkSize;
        while (fileStream.tellg() < fileSizeBytes) {
            int64_t unread = fileSizeBytes - fileStream.tellg();
            if (unread <= chunkSize) {
                // Finish upload session
                status = uploadSessionAppend(localFilePath, remoteFilePath, sessionId, L"true", unread, offset, fileStream, responseBody);
                if (status == 200) {
                    if (m_pListener) {
                        m_pListener->addToProgressPreviousStepsTotal(unread);
                    }
                    status = uploadSessionFinish(sessionId, remoteFilePath, fileSizeBytes, responseBody);
                }
            }
            else {
                // Append data to upload session
                status = uploadSessionAppend(localFilePath, remoteFilePath, sessionId, L"false", chunkSize, offset, fileStream, responseBody);
                if (m_pListener) {
                    m_pListener->addToProgressPreviousStepsTotal(chunkSize);
                }
            }

            if (status != 200) {
                break;
            }
        }
    }

    return status;
}

int DropboxSyncServerConnection::uploadSessionStart(CString localFilePath, CString remoteFilePath, int64_t chunkSize, std::ifstream& fileStream, std::string& responseBody)
{
    const CString url = ContentEndpoint + "files/upload_session/start";

    HeaderList headers;
    headers.push_back(m_authHeader);
    headers.push_back(L"Dropbox-API-Arg: {\"close\": false}");
    headers.push_back(L"Content-Type: application/octet-stream");

    std::vector<char> buffer(static_cast<size_t>(chunkSize));
    fileStream.read(&buffer.front(), chunkSize);
    std::istringstream uploadStream(std::string(buffer.begin(), buffer.end()));
    if (!uploadStream) {
        handleStreamError(localFilePath, remoteFilePath, "Unable to open");
    };

    auto request = HttpRequestBuilder(url).headers(headers).post(uploadStream, chunkSize).build();
    auto response = m_pConnection->Request(request);
    responseBody = response.body.ToString();

    return response.http_status;
}

int DropboxSyncServerConnection::uploadSessionAppend(CString localFilePath, CString remoteFilePath, CString sessionId, CString closeSession, int64_t chunkSize, int64_t& offset, std::ifstream& fileStream, std::string& responseBody)
{
    const CString url = ContentEndpoint + "files/upload_session/append_v2";

    std::stringstream sstm;
    sstm << offset;
    CString strOffset = sstm.str().c_str();
    offset += chunkSize;

    HeaderList headers;
    headers.push_back(m_authHeader);
    headers.push_back(L"Dropbox-API-Arg: {\"cursor\": {\"session_id\": \"" + sessionId
        + "\", \"offset\": " + strOffset + "}, \"close\": " + closeSession + "}");
    headers.push_back(L"Content-Type: application/octet-stream");

    std::vector<char> buffer(static_cast<size_t>(chunkSize));
    fileStream.read(&buffer.front(), chunkSize);
    std::istringstream uploadStream(std::string(buffer.begin(), buffer.end()));
    if (!uploadStream) {
        handleStreamError(localFilePath, remoteFilePath, "Unable to open");
    };

    auto request = HttpRequestBuilder(url).headers(headers).post(uploadStream, chunkSize).build();
    auto response = m_pConnection->Request(request);

    responseBody = response.body.ToString();
    return response.http_status;
}

int DropboxSyncServerConnection::uploadSessionFinish(CString sessionId, CString remoteFilePath, int64_t fileSizeBytes, std::string& responseBody)
{
    const CString url = ContentEndpoint + "files/upload_session/finish";

    std::stringstream sstm;
    sstm.str("");
    sstm << fileSizeBytes;
    CString strOffset = sstm.str().c_str();

    HeaderList headers;
    headers.push_back(m_authHeader);
    if (remoteFilePath.Left(1) != _T(".")) {
        // ALW - Prepend forward slash to paths that don't start with "." or ".."
        remoteFilePath = PortableFunctions::PathAppendForwardSlashToPath<CString>(L"/", remoteFilePath);
    }
    headers.push_back(L"Dropbox-API-Arg: {\"cursor\": {\"session_id\": \"" + sessionId
        + "\", \"offset\": " + strOffset + "}, \"commit\" : {\"path\": \"" + escapeNonAsciiChars(remoteFilePath) + "\""
        ", \"mode\" : \"overwrite\", \"autorename\" : true, \"mute\" : false}}");
    headers.push_back(L"Content-Type: application/octet-stream");

    std::istringstream uploadStream(std::string(""));

    auto request = HttpRequestBuilder(url).headers(headers).post(uploadStream, 0).build();
    auto response = m_pConnection->Request(request);

    responseBody = response.body.ToString();
    return response.http_status;
}

std::vector<FileInfo>* DropboxSyncServerConnection::getDirectoryListing(CString remotePath)
{
    std::string responseBody;
    std::vector<FileInfo> directoryListing;
    int status = sendGetDirectoryListingRequest(remotePath, directoryListing, responseBody);

    if (status == 200) {
        // The following is a workaround to keep the current interface while maintaining
        // exception safety. Alternatively, I could have passed/returned a unique_ptr by
        // value and avoided this additional copy.
        std::unique_ptr<std::vector<FileInfo>> pDirectoryListing(new std::vector<FileInfo>());
        pDirectoryListing->insert(pDirectoryListing->end(), directoryListing.begin(), directoryListing.end());
        return pDirectoryListing.release();
    }
    else {
        handleServerErrorResponse(100112, status, remotePath, responseBody);
        return nullptr; // Avoid warning C4715
    }
}

int DropboxSyncServerConnection::sendGetDirectoryListingRequest(CString remotePath, std::vector<FileInfo>& directoryListing, std::string& responseBody)
{
    if (remotePath.Left(1) != _T(".")) {
        // ALW - Prepend forward slash to paths that don't start with "." or ".."
        remotePath = PortableFunctions::PathAppendForwardSlashToPath<CString>(L"/", remotePath);
    }
    if (remotePath == _T("/")) {
        // ALW - Dropbox requires the root folder specified as an empty string rather than a forward slash
        remotePath = "";
    }
    int status = listFolder(remotePath, responseBody);
    if (status == 200) {
        buildDirectoryListing(responseBody, directoryListing);
        while (jsonConverter.hasMoreFromJson(responseBody)) {
            CString cursor = jsonConverter.cursorFromJson(responseBody);
            status = listFolderContinue(cursor, responseBody);
            if (status != 200) {
                break;
            }

            buildDirectoryListing(responseBody, directoryListing);
        }
    }

    return status;
}

int DropboxSyncServerConnection::listFolder(CString remotePath, std::string& responseBody)
{
    CString url = RPCEndpoint + "files/list_folder";

    const std::string requestBody = "{\"path\": \"" + UTF8Convert::WideToUTF8(remotePath) + "\", "
        "\"recursive\": false, \"include_media_info\": false, \"include_deleted\": false, "
        "\"include_has_explicit_shared_members\": false}";
    std::istringstream postDataStream(requestBody);

    HeaderList headers;
    headers.push_back(m_authHeader);
    headers.push_back(L"Content-Type: application/json; charset=utf-8");

    auto request = HttpRequestBuilder(url).headers(headers).post(postDataStream, requestBody.size()).build();
    auto response = m_pConnection->Request(request);

    responseBody = response.body.ToString();
    return response.http_status;
}

int DropboxSyncServerConnection::listFolderContinue(CString cursor, std::string& responseBody)
{
    CString url = RPCEndpoint + "files/list_folder/continue";

    const std::string requestBody = "{\"cursor\": \"" + UTF8Convert::WideToUTF8(cursor) + "\"}";
    std::istringstream postDataStream(requestBody);

    HeaderList headers;
    headers.push_back(m_authHeader);
    headers.push_back(L"Content-Type: application/json; charset=utf-8");

    auto response = m_pConnection->Request(HttpRequestBuilder(url).headers(headers).post(postDataStream, requestBody.size()).build());
    responseBody = response.body.ToString();
    return response.http_status;
}

void DropboxSyncServerConnection::buildDirectoryListing(const std::string& responseBody, std::vector<FileInfo>& directoryListing)
{
    try {
        jsonConverter.dropboxDirectoryListingFromJson(responseBody, directoryListing);
    }
    catch (const InvalidJsonException& e) {
        CLOG(ERROR, "sync") << "Invalid server response: " << e.what();
        CLOG(ERROR, "sync") << responseBody;
        throw SyncError(100121);
    }
}

void DropboxSyncServerConnection::setListener(ISyncListener* listener)
{
    m_pListener = listener;
    m_pConnection->setListener(listener);
}

void DropboxSyncServerConnection::handleServerErrorResponse(int msgNum, int httpResponseCode, const std::string &responseBody)
{
    try {
        // try to parse error from http response
        std::unique_ptr<SyncErrorResponse> response(jsonConverter.dropboxSyncErrorResponseFromJson(responseBody));
        CString msg;
        msg.Format(L"%s. (%d)", (LPCTSTR)response->GetErrorDescription(), httpResponseCode);
        throw SyncError(msgNum, msg);
    }
    catch (const InvalidJsonException&) {
        CLOG(ERROR, "sync") << "Error from server: ";
        CLOG(ERROR, "sync") << responseBody;

        // If can't parse message then just use the http error code. Don't show the body since
        // it is probably html.
        CString msg;
        msg.Format(L"Error %d", httpResponseCode);
        throw SyncError(msgNum, msg);
    }
}

void DropboxSyncServerConnection::handleServerErrorResponse(int msgNum, int httpResponseCode, CString remotePath, const std::string &responseBody)
{
    try {
        // try to parse error from http response
        std::unique_ptr<SyncErrorResponse> response(jsonConverter.dropboxSyncErrorResponseFromJson(responseBody));
        CString msg;
        msg.Format(L"\"%s\": %s (%d)", (LPCTSTR)remotePath, (LPCTSTR)response->GetErrorDescription(), httpResponseCode);
        throw SyncError(msgNum, msg);
    }
    catch (const InvalidJsonException& e) {
        CLOG(ERROR, "sync") << "Invalid server response: " << e.what();
        CLOG(ERROR, "sync") << responseBody;
        // If can't parse message then just use the http error code. Don't show the body since
        // it is probably html.
        CString msg;
        msg.Format(L"\"%s\": Error %d", (LPCTSTR)remotePath, httpResponseCode);
        throw SyncError(msgNum, msg);
    }
}

FileInfo DropboxSyncServerConnection::getFileInfo(CString remoteFilePath)
{
    CString url = RPCEndpoint + "files/get_metadata";

    if (!remoteFilePath.IsEmpty() && remoteFilePath.Left(1) != _T(".")) {
        // ALW - Prepend forward slash to paths that don't start with "." or ".."
        remoteFilePath = PortableFunctions::PathAppendForwardSlashToPath<CString>(L"/", remoteFilePath);
    }

    // Paths ending in "/" are considered malformed in Dropbox API
    if (!remoteFilePath.IsEmpty() && remoteFilePath.Right(1) == _T("/")) {
        remoteFilePath = remoteFilePath.Mid(0, remoteFilePath.GetLength() - 1);
    }

    HeaderList headers;
    headers.push_back(m_authHeader);
    headers.push_back(L"Content-Type: application/json");

    std::string postData = UTF8Convert::WideToUTF8(L"{\"path\": \"" + remoteFilePath + "\"}");
    std::istringstream postDataStream(postData);

    auto response = m_pConnection->Request(HttpRequestBuilder(url).headers(headers).post(postDataStream, postData.size()).build());
    std::string responseBody = response.body.ToString();

    if (response.http_status == 200) {
        return jsonConverter.dropboxFileInfoFromJson(responseBody);
    } else {
        handleServerErrorResponse(100112, response.http_status, remoteFilePath, responseBody);
        return FileInfo();
    }
}

bool DropboxSyncServerConnection::fileExists(CString remoteFilePath)
{
    try {
        getFileInfo(remoteFilePath);
        return true; // If we got here with no exception the file exists
    }
    catch (const SyncError&) {
    }
    return false;
}

bool DropboxSyncServerConnection::directoryExists(CString remoteDirectoryPath)
{
    return fileExists(remoteDirectoryPath);
}

void DropboxSyncServerConnection::getStream(CString remoteFilePath, std::ostream& responseStream)
{
    CString url = ContentEndpoint + "files/download";

    if (remoteFilePath.Left(1) != _T(".")) {
        // ALW - Prepend forward slash to paths that don't start with "." or ".."
        remoteFilePath = PortableFunctions::PathAppendForwardSlashToPath<CString>(L"/", remoteFilePath);
    }

    HeaderList headers;
    headers.push_back(m_authHeader);
    headers.push_back(L"Dropbox-API-Arg: {\"path\": \"" + escapeNonAsciiChars(remoteFilePath) + "\"}");
    headers.push_back(L"Content-Type: ");

    std::string postData = "";
    std::istringstream postDataStream(postData);

    auto response = m_pConnection->Request(HttpRequestBuilder(url).headers(headers).post(postDataStream, postData.size()).build());
    responseStream << response.body;

    if (response.http_status != 200) {
        handleServerErrorResponse(100110, response.http_status, remoteFilePath, std::string());
    }
}

void DropboxSyncServerConnection::putStream(std::istream& uploadStream, int64_t sizeBytes, CString remoteFilePath)
{
    CString url = ContentEndpoint + "files/upload";

    std::string responseBody;
    int status = sendPutFileRequest(url, uploadStream, remoteFilePath, sizeBytes, responseBody);
    if (status != 200) {
        handleServerErrorResponse(100111, status, remoteFilePath, responseBody);
    }
}

void DropboxSyncServerConnection::setDownloadPffServerParams(PFF& pff)
{
    pff.SetSyncServerType(SyncServerType::Dropbox);
}

std::vector<ApplicationPackage> DropboxSyncServerConnection::listApplicationPackages()
{
    CLOG(INFO, "sync") << "Dropbox list application packages";
    std::vector<ApplicationPackage> packageList = m_pSmartSyncer->listApplicationPackages();
    CLOG(INFO, "sync") << "Dropbox download application package complete";
    return packageList;
}

bool DropboxSyncServerConnection::downloadApplicationPackage(CString packageName, CString localPath, const std::optional<ApplicationPackage>& currentPackage, const CString&)
{
    CLOG(INFO, "sync") << "Dropbox download application package" << UTF8Convert::WideToUTF8(packageName) << " to " << UTF8Convert::WideToUTF8(localPath);
    return m_pSmartSyncer->downloadApplicationPackage(packageName, localPath, currentPackage);
}

void DropboxSyncServerConnection::uploadApplicationPackage(CString localPath, CString packageName, CString packageSpecJson)
{
    CLOG(INFO, "sync") << "Dropbox upload application package" << UTF8Convert::WideToUTF8(packageName) << " from " << UTF8Convert::WideToUTF8(localPath);
    m_pSmartSyncer->uploadApplicationPackage(localPath, packageName, packageSpecJson);
    CLOG(INFO, "sync") << "Dropbox upload application package complete";
}

CString DropboxSyncServerConnection::syncMessage(const CString&, const CString&)
{
    throw SyncError(100154);
}


CString DropboxSyncServerConnection::startParadataSync(const CString& log_uuid)
{
    m_paradataSyncer = std::make_unique<FileBasedParadataSyncer<DropboxSyncServerConnection>>(*this);

    return m_paradataSyncer->startParadataSync(log_uuid);
}

void DropboxSyncServerConnection::putParadata(const CString& filename)
{
    return m_paradataSyncer->putParadata(filename);
}

std::vector<std::shared_ptr<TemporaryFile>> DropboxSyncServerConnection::getParadata()
{
    return m_paradataSyncer->getParadata();
}

void DropboxSyncServerConnection::stopParadataSync()
{
    m_paradataSyncer->stopParadataSync();

    m_paradataSyncer.reset();
}
