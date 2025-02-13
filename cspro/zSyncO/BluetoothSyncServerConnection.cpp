#include "stdafx.h"
#include <cassert>
#include <fstream>
#include "BluetoothSyncServerConnection.h"
#include "SyncRequest.h"
#include "ConnectResponse.h"
#include "JsonConverter.h"
#include "BluetoothObexConnection.h"
#include "ObexConstants.h"
#include "SyncException.h"
#include <sstream>
#include <zDataO/ISyncableDataRepository.h>
#include <easyloggingwrapper.h>
#include "SyncCustomHeaders.h"
#include <zDictO/DDClass.h>
#include "IChooseBluetoothDeviceDialog.h"
#include <zZipo/ZipUtility.h>
#include "IDataChunk.h"
#include "BluetoothChunk.h"
#include <zToolsO/base64.h>
#include <zUtilO/TemporaryFile.h>
#include "CaseObservable.h"
#include <zSyncO/ISyncListener.h>
#include <zSyncO/SyncLogCaseConstructionReporter.h>
#include <zUtilO/Versioning.h>

namespace {

    CString userAgentHeader = CString("User-Agent: CSPro sync client/") + Versioning::GetVersionDetailedString();

    JsonConverter jsonConverter;

    std::streamsize getStreamSize(std::istream& iStream)
    {
        iStream.seekg(0, std::ios::end);
        const std::streamsize streamSize = iStream.tellg();
        iStream.seekg(0, std::ios::beg);

        return streamSize;
    }

}

BluetoothSyncServerConnection::BluetoothSyncServerConnection(IBluetoothAdapter* pAdapter, BluetoothObexConnection* pObexConnection, const BluetoothDeviceInfo& deviceInfo)
    : m_pObexConnection(pObexConnection),
      m_pChooseDeviceDialog(nullptr),
      m_deviceInfo(deviceInfo),
      m_pAdapter(pAdapter),
      m_bWasBluetoothEnabled(pAdapter->isEnabled()),
      m_pListener(nullptr)
{
}

BluetoothSyncServerConnection::BluetoothSyncServerConnection(IBluetoothAdapter* pAdapter, BluetoothObexConnection* pObexConnection, IChooseBluetoothDeviceDialog* pChooseDeviceDialog)
    : m_pObexConnection(pObexConnection),
      m_pChooseDeviceDialog(pChooseDeviceDialog),
      m_pAdapter(pAdapter),
      m_bWasBluetoothEnabled(pAdapter && pAdapter->isEnabled()),
      m_pListener(nullptr)
{
}

BluetoothSyncServerConnection::~BluetoothSyncServerConnection()
{
    delete m_pObexConnection;
    if (m_pAdapter && !m_bWasBluetoothEnabled)
        m_pAdapter->disable();
}

ConnectResponse* BluetoothSyncServerConnection::connect()
{
    // No bluetooth support on this device
    if (!m_pAdapter)
        throw SyncError(100146);

    if (!m_pAdapter->isEnabled())
        m_pAdapter->enable();

    if (m_pChooseDeviceDialog) {
        if (!m_pChooseDeviceDialog->Show(m_deviceInfo))
            throw SyncCancelException();
    }

    bool status = m_pObexConnection->connect(m_deviceInfo);

    if (!status) {
        throw SyncError(100100, m_deviceInfo.csName);
    }
    else {
        // empty path is "default" object - for CSPro Obex
        // this is the server info
        std::ostringstream ss;
        HeaderList responseHeaders;
        m_pObexConnection->get(L"", L"", HeaderList(), ss, responseHeaders);
        std::string connectResponse = ss.str();
        try {
            auto response = jsonConverter.connectResponseFromJson(connectResponse);
            response->setServerName(m_deviceInfo.csName);
            m_server_cspro_version = response->getApiVersion();
            return response;
        }
        catch (const InvalidJsonException& e) {
            CLOG(ERROR, "sync") << "Invalid server response: " << e.what();
            CLOG(ERROR, "sync") << connectResponse;
            throw SyncError(100121);
        }
    }
}

void BluetoothSyncServerConnection::disconnect()
{
    m_pObexConnection->disconnect();
}

SyncGetResponse BluetoothSyncServerConnection::getData(const SyncRequest& request)
{
    CString serverRevision = request.getLastServerRevision();
    CString lastGuid = request.getLastCaseUuid();
    const int currentChunkSize = getChunk().getSize();

    CLOG(INFO, "sync") << "Start case download with chunk size of " << currentChunkSize;

    if (!lastGuid.IsEmpty())
        CLOG(INFO, "sync") << "Download chunk starting from case " << UTF8Convert::WideToUTF8(lastGuid);

    while (true) {

        HeaderList responseHeaders;
        std::string responseBody;

        ObexResponseCode result = GetDataChunk(request.getDictionary().GetName(), request.getDeviceId(),
            request.getUniverse(), serverRevision, lastGuid,
            request.getExcludedServerRevisions(),
            responseHeaders, responseBody);

        if (ZipUtility::decompression(responseBody)) {
            throw SyncError(100139);
        }

        CLOG(INFO, "sync") << "Status : " << UTF8Convert::WideToUTF8(ObexResponseCodeToString(result));

        if (result == OBEX_OK || result == OBEX_PARTIAL_CONTENT) {
            try {
                JsonConverter converter;
                bool isBinaryJSON = converter.IsBinaryJson(responseBody);
                auto iss = std::make_shared<std::istringstream>();
                std::string binaryItemsContent;
                if (isBinaryJSON) {
                    iss->str(responseBody);
                    responseBody = converter.readCasesJsonFromBinary(*iss);
                }
                auto server_cases = jsonConverter.caseListFromJson(responseBody, request.getCaseAccess(), std::make_shared<SyncLogCaseConstructionReporter>());
                CLOG(INFO, "sync") << "Cases in chunk " << server_cases.size() << ". Server count = " << UTF8Convert::WideToUTF8(responseHeaders.value(SyncCustomHeaders::RANGE_COUNT_HEADER));

                SyncGetResponse::SyncGetResult responseResult = result == OBEX_OK ? SyncGetResponse::SyncGetResult::Complete : SyncGetResponse::SyncGetResult::MoreData;
                serverRevision = responseHeaders.value(_T("etag"));
                std::shared_ptr<CaseObservable> cases_observable;
                if (!isBinaryJSON) {
                    cases_observable = std::make_shared<CaseObservable>(rxcpp::observable<>::iterate(server_cases));
                }
                else {
                    cases_observable = std::make_shared<CaseObservable>(rxcpp::observable<>::create<std::shared_ptr<Case>>([server_cases, iss](rxcpp::subscriber<std::shared_ptr<Case>> case_subscriber){
                        JsonConverter converter;
                        for (const std::shared_ptr<Case>& data_case : server_cases) {
                            //load the binary items for the case before notifying the caseobservable subscriber
                                converter.readBinaryItemsForCase(*iss, *data_case);
                                case_subscriber.on_next(data_case);
                            }
                            case_subscriber.on_completed();
                        }));
                }

                return SyncGetResponse(responseResult, cases_observable, serverRevision);
            }
            catch (const InvalidJsonException& e) {
                CLOG(ERROR, "sync") << "Invalid server response: " << e.what();
                CLOG(ERROR, "sync") << responseBody;
                throw SyncError(100121);
            }
        } else if (result == OBEX_PRECONDITION_FAILED) {
            return SyncGetResponse::SyncGetResult::RevisionNotFound;
        } else if (result == OBEX_CANCELED_BY_USER) {
            throw SyncError(100122);
        } else {
            throw SyncError(100101, ObexResponseCodeToString(result));
        }
    }
}

SyncPutResponse BluetoothSyncServerConnection::putData(const SyncRequest& request)
{
    std::ostringstream oss;
    HeaderList requestHeaders;
    HeaderList responseHeaders;
    if (!request.getLastServerRevision().IsEmpty())
        requestHeaders.push_back(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER + _T(':') + request.getLastServerRevision());
    requestHeaders.push_back(SyncCustomHeaders::DEVICE_ID_HEADER + _T(": ") + request.getDeviceId());
    if (!request.getUniverse().IsEmpty())
        requestHeaders.push_back(SyncCustomHeaders::UNIVERSE_HEADER + _T(": ") + request.getUniverse());
    requestHeaders.push_back(userAgentHeader);

    bool cspro_74_comptability = m_server_cspro_version <= 7.4;
    std::string clientCasesJson = jsonConverter.toJson(request.getClientCases(), cspro_74_comptability);

    std::ostringstream cases_json_stream;
    auto binaryCaseItems = request.getBinaryCaseItems();

    if (binaryCaseItems.size() > 0) { //write the new binary json format only if cases have binary items
        std::ostringstream binary_cases_json_stream;
        JsonConverter converter;
        //cases have binary content send the questionnaire data using binary format
        converter.writeBinaryJson(binary_cases_json_stream, clientCasesJson, binaryCaseItems);
        clientCasesJson = binary_cases_json_stream.str();
    }

    if (ZipUtility::compression(clientCasesJson)) {
        throw SyncError(100138);
    }

    std::istringstream postDataStream(clientCasesJson);

    ObexResponseCode result = m_pObexConnection->put(OBEX_SYNC_DATA_MEDIA_TYPE, request.getDictionary().GetName(), false,
        postDataStream, clientCasesJson.size(), requestHeaders, oss, responseHeaders);
    if (result == OBEX_OK) {
        return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, responseHeaders.value(_T("etag")));
    } else if (result == OBEX_PRECONDITION_FAILED) {
        return SyncPutResponse(SyncPutResponse::SyncPutResult::RevisionNotFound);
    } else if (result == OBEX_CANCELED_BY_USER) {
        throw SyncError(100122);
    } else {
        throw SyncError(100101, ObexResponseCodeToString(result));
    }
}

IDataChunk& BluetoothSyncServerConnection::getChunk()
{
    return m_pObexConnection->getChunk();
}

std::vector<DictionaryInfo> BluetoothSyncServerConnection::getDictionaries()
{
    throw SyncError(100124);
}

CString BluetoothSyncServerConnection::getDictionary(CString dictionaryName)
{
    throw SyncError(100124);
}

void BluetoothSyncServerConnection::putDictionary(CString dictPath)
{
    throw SyncError(100124);
}

void BluetoothSyncServerConnection::deleteDictionary(CString dictName)
{
    throw SyncError(100124);
}

std::vector<FileInfo>* BluetoothSyncServerConnection::getDirectoryListing(CString remotePath)
{
    std::ostringstream ss;
    HeaderList responseHeaders;
    HeaderList requestHeaders;
    requestHeaders.push_back(userAgentHeader);

    ObexResponseCode result = m_pObexConnection->get(OBEX_DIRECTORY_LISTING_MEDIA_TYPE, remotePath, requestHeaders, ss, responseHeaders);
    switch (result) {
    case OBEX_OK:
        break;
    case OBEX_CANCELED_BY_USER:
        throw SyncError(100122);
        break;
    default:
        CString errMsg;
        errMsg.Format(_T("Server responded with error %d"), (int) result);
        throw SyncError(100101, errMsg);
    }
    std::string dirlistResponse = ss.str();
    try {
        return jsonConverter.fileInfoFromJson(dirlistResponse);
    }
    catch (const InvalidJsonException& e) {
        CLOG(ERROR, "sync") << "Invalid server response: " << e.what();
        CLOG(ERROR, "sync") << dirlistResponse;
        throw SyncError(100121);
    }
    return NULL;
}

void BluetoothSyncServerConnection::setListener(ISyncListener* pListener)
{
    m_pListener = pListener;
    m_pObexConnection->setListener(pListener);
}

ObexResponseCode BluetoothSyncServerConnection::GetDataChunk(CString dictName, DeviceId deviceId, CString universe,
    CString lastServerRevision, CString lastGuid,
    const std::vector<CString>& excludeRevisions,
    HeaderList& responseHeaders, std::string & responseBody)
{
    std::ostringstream oss;
    HeaderList requestHeaders;
    if (!lastServerRevision.IsEmpty())
        requestHeaders.push_back(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER + _T(':') + lastServerRevision);
    requestHeaders.push_back(SyncCustomHeaders::DEVICE_ID_HEADER + _T(": ") + deviceId);
    if (!universe.IsEmpty())
        requestHeaders.push_back(SyncCustomHeaders::UNIVERSE_HEADER + _T(": ") + universe);
    if (!lastGuid.IsEmpty())
        requestHeaders.push_back(SyncCustomHeaders::START_AFTER_HEADER + _T(": ") + lastGuid);
    if (!excludeRevisions.empty()) {
        CString excludeHeader = SyncCustomHeaders::EXCLUDE_REVISIONS_HEADER + _T(": ") + WS2CS(SO::CreateSingleString(excludeRevisions, _T(",")));
        requestHeaders.push_back(excludeHeader);
    }
    CString countHeader;
    countHeader.Format(_T("%s: %d"), (LPCTSTR)SyncCustomHeaders::RANGE_COUNT_HEADER, getChunk().getSize());
    requestHeaders.push_back(countHeader);
    requestHeaders.push_back(userAgentHeader);

    ObexResponseCode result = m_pObexConnection->get(OBEX_SYNC_DATA_MEDIA_TYPE, dictName, requestHeaders, oss, responseHeaders);
    responseBody = oss.str();

    return result;
}


ObexResponseCode BluetoothSyncServerConnection::getFileWorker(const CString& type, const CString& remotePath, const CString& localPath,
    std::optional<HeaderList> requestHeaders/* = std::nullopt*/)
{
    if( !requestHeaders.has_value() )
    {
        requestHeaders = HeaderList();
        requestHeaders->push_back(userAgentHeader);
    }

    HeaderList responseHeaders;

    std::ofstream fileStream(localPath, std::ios::binary);

    while( true )
    {
        // The file is modified and the last file chunk has not been processed
        std::ostringstream response;
        ObexResponseCode getResult = m_pObexConnection->get(type, remotePath, *requestHeaders, response, responseHeaders);

        if( getResult == OBEX_OK || getResult == OBEX_IS_LAST_FILE_CHUNK )
        {
            std::istringstream inStrm(response.str());

            if( ZipUtility::decompression(inStrm, fileStream) != EXIT_SUCCESS )
                throw SyncError(100139);
        }

        else if( getResult == OBEX_CANCELED_BY_USER )
            throw SyncError(100122);

        if( getResult != OBEX_OK )
            return getResult;
    }
}

bool BluetoothSyncServerConnection::getFile(CString remoteFilePath, CString localFilePath, CString /*actualLocalFilePath*/, CString md5 /*= CString()*/)
{
    HeaderList requestHeaders;
    requestHeaders.push_back(userAgentHeader);

    if (!md5.IsEmpty())
        requestHeaders.push_back(CString("If-None-Match:") + md5);

    ObexResponseCode getResult = getFileWorker(OBEX_BINARY_FILE_MEDIA_TYPE, remoteFilePath, localFilePath, requestHeaders);

    if( getResult == OBEX_IS_LAST_FILE_CHUNK )
        return true;

    else if( getResult == OBEX_NOT_MODIFIED )
        return false;

    else
        throw SyncError(100101, ObexResponseCodeToString(getResult));
}

std::unique_ptr<TemporaryFile> BluetoothSyncServerConnection::getFileIfExists(const CString& /*remoteFilePath*/)
{
    // this method is only used by the FileBasedParadataSyncer ... implement it eventually if necessary
    ASSERT(false);
    throw ProgrammingErrorException();
}

void BluetoothSyncServerConnection::putFileWorker(const CString& type, const CString& localPath, const CString& remotePath)
{
    HeaderList requestHeaders;
    requestHeaders.push_back(userAgentHeader);

    if (m_pListener)
        m_pListener->setProgressTotal(PortableFunctions::FileSize(localPath));

    std::ifstream fileStream(localPath, std::ios::binary);
    const std::vector<char>::size_type bufferSize = BluetoothFileChunk::size;
    std::streamsize chunkSize = bufferSize;
    std::vector<char> chunk(bufferSize, 0);

    while (!fileStream.eof()) {
        fileStream.read(chunk.data(), bufferSize);

        bool isLastFileChunk = false;
        if (fileStream.bad()) {
            throw SyncError(100134);
        }
        else if (fileStream.eof()) {
            isLastFileChunk = true;
            chunkSize = fileStream.gcount();
        }

        std::string stringChunk(chunk.data(), chunkSize);

        if (ZipUtility::compression(stringChunk)) {
            throw SyncError(100138);
        }

        std::istringstream iStreamChunk(stringChunk);
        std::streamsize iStreamChunkSize = getStreamSize(iStreamChunk);

        std::ostringstream oss;
        HeaderList responseHeaders;

        ObexResponseCode putResult = m_pObexConnection->put(type, remotePath, isLastFileChunk,
            iStreamChunk, iStreamChunkSize, requestHeaders, oss, responseHeaders);

        switch (putResult) {
        case OBEX_OK:
            break;
        case OBEX_CANCELED_BY_USER:
            throw SyncError(100122);
            break;
        default:
            CString errMsg;
            errMsg.Format(_T("Server responded with error %d"), (int) putResult);
            throw SyncError(100101, errMsg);
        }

        if (m_pListener)
            m_pListener->addToProgressPreviousStepsTotal(chunkSize);
    }
}

void BluetoothSyncServerConnection::putFile(CString localPath, CString remotePath)
{
    putFileWorker(OBEX_BINARY_FILE_MEDIA_TYPE, localPath, remotePath);
}

bool BluetoothSyncServerConnection::downloadApplicationPackage(CString packageName, CString localPath,
                                                               const std::optional<ApplicationPackage> &currentPackage,
                                                               const CString& currentPackageSignature)
{
    HeaderList responseHeaders;
    HeaderList requestHeaders;
    requestHeaders.push_back(userAgentHeader);

    if (currentPackage) {
        // For an existing package we add the build time and the package file list in headers
        requestHeaders.push_back(CString(_T("If-None-Match: ")) + currentPackageSignature);
        requestHeaders.push_back(SyncCustomHeaders::APP_PACKAGE_BUILD_TIME_HEADER + _T(": ") + PortableFunctions::TimeToRFC3339String(currentPackage->getBuildTime()));

        // Convert the file list to base64 encoded, compressed json
        auto packageFiles = currentPackage->getFiles();
        // No need to send the only on first install flag to server, setting to false removes from JSON
        for (auto& f : packageFiles) {
            f.OnlyOnFirstInstall = false;
        }

        std::string fileJson = jsonConverter.toJson(packageFiles);
        if (ZipUtility::compression(fileJson)) {
            CLOG(ERROR, "sync") << "Failed to compress package json, fallback to full package download";
        } else {
            requestHeaders.push_back(SyncCustomHeaders::APP_PACKAGE_FILES_HEADER + _T(": ") +
                Base64::Encode<std::wstring>(fileJson.data(), fileJson.length()).c_str());
        }
    }

    TemporaryFile tmpZip(PortableFunctions::PathGetDirectory<CString>(localPath));
    std::ofstream fileStream(tmpZip.GetPath().c_str(), std::ios::binary);

    ObexResponseCode getResult = OBEX_OK;
    while (getResult == OBEX_OK) {
        // The file is modified and the last file chunk has not been processed
        std::ostringstream response;
        getResult = m_pObexConnection->get(OBEX_SYNC_APP_MEDIA_TYPE, packageName, requestHeaders, response, responseHeaders);

        switch (getResult) {
            case OBEX_OK:
            case OBEX_IS_LAST_FILE_CHUNK: {
                std::istringstream inStrm(response.str());
                if (ZipUtility::decompression(inStrm, fileStream)) {
                    throw SyncError(100139);
                }
                break;
            }
            case OBEX_NOT_MODIFIED:
                CLOG(INFO, "sync") << "Application package already up to date ";
                break;
            case OBEX_CANCELED_BY_USER:
                throw SyncError(100122);
            case OBEX_NOT_IMPLEMENTED:
                CLOG(WARNING, "sync") << "syncapp not implemented by the Bluetooth server device";
                break;
            default:
                CString errMsg;
                errMsg.Format(_T("Server responded with error %d"), (int) getResult);
                throw SyncError(100101, errMsg);
        }
    }

    if (getResult != OBEX_IS_LAST_FILE_CHUNK) {
        return false;
    } else {
        if (PortableFunctions::FileExists(localPath))
            PortableFunctions::FileDelete(localPath);
        if (!tmpZip.Rename(CS2WS(localPath))) {
            throw SyncError(100109, localPath);
        }
        CLOG(INFO, "sync") << "Downloaded application package " << UTF8Convert::WideToUTF8(packageName);
        return true;
    }
}


CString BluetoothSyncServerConnection::syncMessage(const CString& message_key, const CString& message_value)
{
    SyncMessage sent_sync_message = { message_key, message_value };
    std::string message_json = jsonConverter.toJson(sent_sync_message);

    HeaderList request_headers;
    request_headers.push_back(SyncCustomHeaders::MESSAGE_HEADER + _T(": ") + UTF8Convert::UTF8ToWide<CString>(message_json));
    request_headers.push_back(userAgentHeader);

    std::ostringstream ss;
    HeaderList response_headers;
    ObexResponseCode get_result = m_pObexConnection->get(OBEX_SYNC_MESSAGE_MEDIA_TYPE,
        _T("a message"), request_headers, ss, response_headers);

    if( get_result == OBEX_OK )
    {
        try
        {
            SyncMessage received_sync_message = jsonConverter.syncMessageFromJson(ss.str());
            ASSERT(received_sync_message.key.Compare(sent_sync_message.key) == 0);
            return received_sync_message.value;
        }

        catch( const InvalidJsonException& exception )
        {
            CLOG(ERROR, "sync") << "Invalid server response: " << exception.what();
            CLOG(ERROR, "sync") << get_result;
            throw SyncError(100121);
        }
    }

    else if( get_result == OBEX_METHOD_NOT_ALLOWED )
        throw SyncError(100156);

    else
        throw SyncError(100101, ObexResponseCodeToString(get_result));
}


CString BluetoothSyncServerConnection::startParadataSync(const CString& log_uuid)
{
    HeaderList request_headers;
    request_headers.push_back(userAgentHeader);
    request_headers.push_back(SyncCustomHeaders::PARADATA_LOG_UUID + _T(": ") + log_uuid);

    std::ostringstream ss;
    HeaderList response_headers;

    ObexResponseCode get_result = m_pObexConnection->get(OBEX_SYNC_PARADATA_SYNC_HANDSHAKE,
        _T("paradata log details"), request_headers, ss, response_headers);

    if( get_result == OBEX_OK )
    {
        CString server_log_uuid = response_headers.value(SyncCustomHeaders::PARADATA_LOG_UUID);

        // throw an error if no paradata log is open on the server
        if( server_log_uuid.IsEmpty() )
            throw SyncError(100172);

        return server_log_uuid;
    }

    else
        throw SyncError(100101, ObexResponseCodeToString(get_result));
}

void BluetoothSyncServerConnection::putParadata(const CString& filename)
{
    putFileWorker(OBEX_SYNC_PARADATA_TYPE, filename, CString());
}

std::vector<std::shared_ptr<TemporaryFile>> BluetoothSyncServerConnection::getParadata()
{
    auto temporary_file = std::make_shared<TemporaryFile>();

    ObexResponseCode get_result = getFileWorker(OBEX_SYNC_PARADATA_TYPE, _T("paradata log"), WS2CS(temporary_file->GetPath()));

    if( get_result == OBEX_NOT_MODIFIED )
        return { };

    else if( get_result == OBEX_IS_LAST_FILE_CHUNK )
        return { temporary_file };

    else
        throw SyncError(100101, ObexResponseCodeToString(get_result));
}

void BluetoothSyncServerConnection::stopParadataSync()
{
    HeaderList request_headers;
    request_headers.push_back(userAgentHeader);

    HeaderList response_headers;
    std::istringstream iss;
    std::ostringstream oss;

    ObexResponseCode put_result = m_pObexConnection->put(OBEX_SYNC_PARADATA_SYNC_HANDSHAKE, _T("paradata sync"),
        true, iss, 0, request_headers, oss, response_headers);

    if( put_result != OBEX_OK )
        throw SyncError(100101, ObexResponseCodeToString(put_result));
}
