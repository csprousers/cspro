#include "stdafx.h"
#include "FtpSyncServerConnection.h"
#include "ApplicationPackage.h"
#include "ConnectResponse.h"
#include "IDataChunk.h"
#include "ILoginDialog.h"
#include "FileBasedParadataSyncer.h"
#include "FileBasedSmartSyncer.h"
#include "JsonConverter.h"
#include "SyncException.h"
#include "SyncRequest.h"
#include "SyncUtil.h"
#include <zNetwork/IFtpConnection.h>
#include <zUtilO/CredentialStore.h>
#include <zUtilO/TemporaryFile.h>
#include <zDictO/DDClass.h>
#include <zDataO/ISyncableDataRepository.h>
#include <easyloggingwrapper.h>
#include <sstream>


namespace {

    void parseUrl(CString url, CString& scheme, CString& host, CString& path)
    {
        int schemeEnd = url.Find(_T("://"));
        if (schemeEnd < 0) {
            schemeEnd = 0;
            scheme = CString();
        } else {
            schemeEnd += 3;
            scheme = url.Left(schemeEnd);
            scheme.MakeLower();
        }

        int hostEnd = url.Find(_T('/'), schemeEnd);
        if (hostEnd < 0) {
            host = url.Mid(schemeEnd);
            path = CString();
        } else {
            host = url.Mid(schemeEnd, hostEnd - schemeEnd);
            path = url.Mid(hostEnd);
        }

        if (path.IsEmpty())
            path = _T("/");
    }

}
FtpSyncServerConnection::FtpSyncServerConnection(std::shared_ptr<IFtpConnection> pConnection, CString hostUrl, CString username, CString password) :
    m_pConnection(pConnection),
    m_pLoginDialog(nullptr),
    m_pSyncCredentialStore(nullptr),
    m_hostUrl(forceEndWithSlash(hostUrl)),
    m_username(username),
    m_password(password),
    m_pSmartSyncer(new FileBasedSmartSyncer<FtpSyncServerConnection>(this)),
    m_dataChunk(DefaultDataChunk()),
    m_pListener(nullptr)
{
}

FtpSyncServerConnection::FtpSyncServerConnection(std::shared_ptr<IFtpConnection> pConnection, CString hostUrl, ILoginDialog* pLoginDlg, ICredentialStore* pCredentialStore) :
    m_pConnection(pConnection),
    m_pLoginDialog(pLoginDlg),
    m_pSyncCredentialStore(pCredentialStore),
    m_hostUrl(forceEndWithSlash(hostUrl)),
    m_pSmartSyncer(new FileBasedSmartSyncer<FtpSyncServerConnection>(this)),
    m_dataChunk(DefaultDataChunk())
{}

FtpSyncServerConnection::~FtpSyncServerConnection()
{
    delete m_pSmartSyncer;
}

ConnectResponse* FtpSyncServerConnection::connect()
{
    CString scheme, path, host;

    parseUrl(m_hostUrl, scheme, host, path);
    if (scheme.IsEmpty()) {
        scheme = _T("ftp://"); // default to FTP if no scheme provided
    } else {
        if (scheme != _T("ftp://") &&
            scheme != _T("ftps://") &&
            scheme != _T("ftpes://"))
                // An invalid url scheme - probably http
                throw SyncError(100125, m_hostUrl);
    }
    if (host.IsEmpty())
        throw SyncError(100125, m_hostUrl);

    m_hostUrl = scheme + host;
    m_basePath = path;

    if (m_pLoginDialog) {
        // No username/password specified

        // Try saved username/password first
        CString username = WS2CS(m_pSyncCredentialStore->Retrieve(CS2WS(host) + _T("_user")));
        CString password = WS2CS(m_pSyncCredentialStore->Retrieve(CS2WS(host) + _T("_pass")));
        if (!username.IsEmpty()) {
            try {
                m_pConnection->connect(m_hostUrl, username, password);
                m_username = username;
                return new ConnectResponse(m_hostUrl, m_hostUrl, m_username);
            }
            catch (const SyncLoginDeniedError&) {
                // Ignore login denied (bad password) errors, pass all others on
            }
            catch (const SyncError&) {
                throw;
            }
        }

        // Keep prompting for credentials until we get
        // successful login, cancel or error

        bool bHadFailure = false;

        while (true) {

            auto login_result = m_pLoginDialog->Show(m_hostUrl, bHadFailure);
            if (!login_result)
                throw SyncCancelException();

            username = std::get<0>(*login_result);
            password = std::get<1>(*login_result);

            try {
                m_pConnection->connect(m_hostUrl, username, password);
                m_pSyncCredentialStore->Store(CS2WS(host) + _T("_user"), CS2WS(username));
                m_pSyncCredentialStore->Store(CS2WS(host) + _T("_pass"), CS2WS(password));
                m_username = username;
                break;
            }
            catch (const SyncLoginDeniedError&) {
                // Ignore login denied (bad password) errors, pass all others on
                bHadFailure = true;
            }
            catch (const SyncError&) {
                throw;
            }
        }

    } else {
        // Use the username/pwd passed in
        m_pConnection->connect(m_hostUrl, m_username, m_password);
    }

    // FTP servers don't have an id so we just use the URL
    return new ConnectResponse(m_hostUrl, m_hostUrl, m_username);
}

void FtpSyncServerConnection::disconnect()
{
    m_pConnection->disconnect();
}

SyncGetResponse FtpSyncServerConnection::getData(const SyncRequest& syncRequest)
{
    CLOG(INFO, "sync") << "Start FTP case download for dictionary " << UTF8Convert::WideToUTF8(syncRequest.getDictionary().GetName());

    SyncGetResponse response = m_pSmartSyncer->getData(syncRequest);

    CLOG(INFO, "sync") << "FTP case download complete ";

    return response;
}

SyncPutResponse FtpSyncServerConnection::putData(const SyncRequest& syncRequest)
{
    CLOG(INFO, "sync") << "Start FTP case upload for dictionary " << UTF8Convert::WideToUTF8(syncRequest.getDictionary().GetName());

    SyncPutResponse response = m_pSmartSyncer->putData(syncRequest);

    CLOG(INFO, "sync") << "FTP case upload complete.";

    return response;
}

IDataChunk& FtpSyncServerConnection::getChunk()
{
    return m_dataChunk;
}

bool FtpSyncServerConnection::fileExists(CString path)
{
    try {
        path = PortableFunctions::PathAppendForwardSlashToPath(m_basePath, path);
        m_pConnection->getLastModifiedTime(path);
        return true;
    } catch (const SyncError&)
    { }
    return false;
}

bool FtpSyncServerConnection::directoryExists(CString path)
{
    path = PortableFunctions::PathAppendForwardSlashToPath(m_basePath, path);

    // getLastModified time doesn't work on directories (at least not on all servers)
    // so we need to do directory listing of parent here.
    path = (path.Right(1) == _T("/")) ? path.Left(path.GetLength() - 1) : path;
    CString parentPath = PortableFunctions::PathGetDirectory<CString>(path);
    CString dirName = PortableFunctions::PathGetFilename(path);
    std::unique_ptr<std::vector<FileInfo>> pParentListing(m_pConnection->getDirectoryListing(parentPath));
    auto iDir =
        std::find_if(pParentListing->begin(), pParentListing->end(),
            [dirName](const FileInfo& fi) { return fi.getType() == FileInfo::FileType::Directory && fi.getName() == dirName; });
    return iDir != pParentListing->end();
}

void FtpSyncServerConnection::getStream(CString remotePath, std::ostream& stream)
{
    remotePath = PortableFunctions::PathAppendForwardSlashToPath(m_basePath, remotePath);
    m_pConnection->download(remotePath, stream);
}

void FtpSyncServerConnection::putStream(std::istream& stream, int64_t sizeBytes, CString remotePath)
{
    remotePath = PortableFunctions::PathAppendForwardSlashToPath(m_basePath, remotePath);
    m_pConnection->upload(stream, sizeBytes, remotePath);
}

void FtpSyncServerConnection::setDownloadPffServerParams(PFF& pff)
{
    pff.SetSyncServerType(SyncServerType::FTP);
    pff.SetSyncUrl(m_hostUrl);
}

std::vector<DictionaryInfo> FtpSyncServerConnection::getDictionaries()
{
    return m_pSmartSyncer->getDictionaries();
}

CString FtpSyncServerConnection::getDictionary(CString dictionaryName)
{
    return m_pSmartSyncer->getDictionary(dictionaryName);
}

void FtpSyncServerConnection::putDictionary(CString dictPath)
{
    m_pSmartSyncer->putDictionary(dictPath);
}

void FtpSyncServerConnection::deleteDictionary(CString dictionaryName)
{
    throw SyncError(100124);
}

bool FtpSyncServerConnection::getFile(CString remoteFilePath, CString tempLocalFilePath, CString /*actualLocalFilePath*/, CString md5 /*= CString()*/)
{
    remoteFilePath = PortableFunctions::PathAppendForwardSlashToPath(m_basePath, remoteFilePath);
    m_pConnection->download(remoteFilePath, tempLocalFilePath);

    return true;
}

std::unique_ptr<TemporaryFile> FtpSyncServerConnection::getFileIfExists(const CString& remoteFilePath)
{
    if( fileExists(remoteFilePath) )
    {
        auto temporary_file = std::make_unique<TemporaryFile>();

        if( getFile(remoteFilePath, WS2CS(temporary_file->GetPath()), WS2CS(temporary_file->GetPath()), CString()) )
            return temporary_file;
    }

    return nullptr;
}

void FtpSyncServerConnection::putFile(CString localFilePath, CString remoteFilePath)
{
    remoteFilePath = PortableFunctions::PathAppendForwardSlashToPath(m_basePath, remoteFilePath);
    return m_pConnection->upload(localFilePath, remoteFilePath);
}

std::vector<FileInfo>* FtpSyncServerConnection::getDirectoryListing(CString remotePath)
{
    remotePath = PortableFunctions::PathAppendForwardSlashToPath(m_basePath, remotePath);
    return m_pConnection->getDirectoryListing(remotePath);
}

std::vector<ApplicationPackage> FtpSyncServerConnection::listApplicationPackages()
{
    CLOG(INFO, "sync") << "FTP list application packages";
    std::vector<ApplicationPackage> packageList = m_pSmartSyncer->listApplicationPackages();
    CLOG(INFO, "sync") << "FTP download application package complete";
    return packageList;
}

bool FtpSyncServerConnection::downloadApplicationPackage(CString packageName, CString localPath, const std::optional<ApplicationPackage>& currentPackage, const CString&)
{
    CLOG(INFO, "sync") << "FTP download application package" << UTF8Convert::WideToUTF8(packageName) << " to " << UTF8Convert::WideToUTF8(localPath);
    return m_pSmartSyncer->downloadApplicationPackage(packageName, localPath, currentPackage);
}

void FtpSyncServerConnection::uploadApplicationPackage(CString localPath, CString packageName, CString packageSpecJson)
{
    CLOG(INFO, "sync") << "FTP upload application package" << UTF8Convert::WideToUTF8(packageName) << " from " << UTF8Convert::WideToUTF8(localPath);
    m_pSmartSyncer->uploadApplicationPackage(localPath, packageName, packageSpecJson);
    CLOG(INFO, "sync") << "FTP upload application package complete";
}

CString FtpSyncServerConnection::syncMessage(const CString&, const CString&)
{
    throw SyncError(100154);
}


CString FtpSyncServerConnection::startParadataSync(const CString& log_uuid)
{
    m_paradataSyncer = std::make_unique<FileBasedParadataSyncer<FtpSyncServerConnection>>(*this);

    return m_paradataSyncer->startParadataSync(log_uuid);
}

void FtpSyncServerConnection::putParadata(const CString& filename)
{
    return m_paradataSyncer->putParadata(filename);
}

std::vector<std::shared_ptr<TemporaryFile>> FtpSyncServerConnection::getParadata()
{
    return m_paradataSyncer->getParadata();
}

void FtpSyncServerConnection::stopParadataSync()
{
    m_paradataSyncer->stopParadataSync();

    m_paradataSyncer.reset();
}


void FtpSyncServerConnection::setListener(ISyncListener* pListener)
{
    m_pListener = pListener;
    m_pConnection->setListener(pListener);
}
