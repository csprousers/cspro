#include "stdafx.h"
#include "LocalFileSyncServerConnection.h"
#include "ConnectResponse.h"
#include "FileBasedParadataSyncer.h"
#include "FileBasedSmartSyncer.h"
#include "SyncException.h"
#include "SyncRequest.h"
#include <zToolsO/Encoders.h>
#include <zDictO/DDClass.h>
#include <easyloggingwrapper.h>
#include <assert.h>
#include <sstream>


LocalFileSyncServerConnection::LocalFileSyncServerConnection(const CString& root_directory,
    std::string local_file_sync_type/* = "file"*/)
    :   m_localFileSyncType(local_file_sync_type),
        m_pSmartSyncer(new FileBasedSmartSyncer<LocalFileSyncServerConnection>(this)),
        m_pListener(nullptr)
{
    // the root directory can be either a file URL or a straight filename
    m_rootDirectory = WS2CS(Encoders::FromFileUrl(root_directory).value_or(CS2WS(root_directory)));
}

LocalFileSyncServerConnection::~LocalFileSyncServerConnection()
{
}

ConnectResponse* LocalFileSyncServerConnection::connect()
{
    if (!PortableFunctions::FileIsDirectory(m_rootDirectory)) {
        CLOG(ERROR, "sync") << "Root directory " << UTF8Convert::WideToUTF8(m_rootDirectory) << " not found on this computer";
        throw SyncError(100147);
    }

    CString directory_file_url = WS2CS(Encoders::ToFileUrl(CS2WS(m_rootDirectory)));
    return new ConnectResponse(directory_file_url, directory_file_url);
}

void LocalFileSyncServerConnection::disconnect()
{
}

SyncGetResponse LocalFileSyncServerConnection::getData(const SyncRequest& syncRequest)
{
    CLOG(INFO, "sync") << "Start local " << m_localFileSyncType << " case download for dictionary " << UTF8Convert::WideToUTF8(syncRequest.getDictionary().GetName());

    SyncGetResponse response = m_pSmartSyncer->getData(syncRequest);

    CLOG(INFO, "sync") << "Local " << m_localFileSyncType << " case download complete ";

    return response;
}

SyncPutResponse LocalFileSyncServerConnection::putData(const SyncRequest& syncRequest)
{
    CLOG(INFO, "sync") << "Start local" << m_localFileSyncType << " case upload for dictionary " << UTF8Convert::WideToUTF8(syncRequest.getDictionary().GetName());

    SyncPutResponse response = m_pSmartSyncer->putData(syncRequest);

    CLOG(INFO, "sync") << "Local " << m_localFileSyncType << " case upload complete.";

    return response;
}

IDataChunk& LocalFileSyncServerConnection::getChunk()
{
    return m_dataChunk;
}

std::vector<DictionaryInfo> LocalFileSyncServerConnection::getDictionaries()
{
    return m_pSmartSyncer->getDictionaries();
}

CString LocalFileSyncServerConnection::getDictionary(CString dictionaryName)
{
    return m_pSmartSyncer->getDictionary(dictionaryName);
}

void LocalFileSyncServerConnection::putDictionary(CString dictPath)
{
    m_pSmartSyncer->putDictionary(dictPath);
}

void LocalFileSyncServerConnection::deleteDictionary(CString dictName)
{
    throw SyncError(100124);
}

bool LocalFileSyncServerConnection::getFile(CString remoteFilePath, CString tempLocalFilePath, CString actualLocalFilePath, CString md5)
{
    CString fullRemotePath = PortableFunctions::PathToNativeSlash(PortableFunctions::PathAppendToPath(m_rootDirectory, remoteFilePath));

    if (!PortableFunctions::FileCopy(fullRemotePath, tempLocalFilePath, false)) {
        CLOG(ERROR, "sync") << "Failed to download file " << UTF8Convert::WideToUTF8(fullRemotePath) << " to " << UTF8Convert::WideToUTF8(actualLocalFilePath);
        throw SyncError(100110, remoteFilePath);
    }
    return true;
}

std::unique_ptr<TemporaryFile> LocalFileSyncServerConnection::getFileIfExists(const CString& remoteFilePath)
{
    if( fileExists(remoteFilePath) )
    {
        auto temporary_file = std::make_unique<TemporaryFile>();

        getFile(remoteFilePath, WS2CS(temporary_file->GetPath()), WS2CS(temporary_file->GetPath()), CString());

        return temporary_file;
    }

    return nullptr;
}

void LocalFileSyncServerConnection::putFile(CString localPath, CString remotePath)
{
    CString fullRemotePath = PortableFunctions::PathToNativeSlash(PortableFunctions::PathAppendToPath(m_rootDirectory, remotePath));
    CString remoteDir = PortableFunctions::PathGetDirectory<CString>(fullRemotePath);
    PortableFunctions::PathMakeDirectories(remoteDir);

    if (!PortableFunctions::FileCopy(localPath, fullRemotePath, false)) {
        CLOG(ERROR, "sync") << "Failed to upload file " << UTF8Convert::WideToUTF8(localPath) << " to " << UTF8Convert::WideToUTF8(fullRemotePath);
        throw SyncError(100111, localPath);
    }
}

std::vector<FileInfo>* LocalFileSyncServerConnection::getDirectoryListing(CString remotePath)
{
    auto directoryListing = std::make_unique<std::vector<FileInfo>>();

    CString directory = PortableFunctions::PathAppendToPath(m_rootDirectory,
        PortableFunctions::PathToNativeSlash(remotePath));

    // Store the relative path that when concatenated with the root path will construct the full path
    CString pathFromRoot = PortableFunctions::PathToForwardSlash(remotePath);
    if (pathFromRoot.Left(2) != _T("./") && pathFromRoot.Left(3) != _T("../")) {
        pathFromRoot = PortableFunctions::PathAppendForwardSlashToPath<CString>(L"/", pathFromRoot);
    }

    for( const std::wstring& fullPath : DirectoryLister().SetIncludeDirectories()
                                                         .GetPaths(directory) )
    {
        CString filename = PortableFunctions::PathRemoveTrailingSlash<CString>(PortableFunctions::PathGetFilename(fullPath));
        FileInfo::FileType type = PortableFunctions::FileIsDirectory(fullPath) ? FileInfo::FileType::Directory : FileInfo::FileType::File;
        if (type == FileInfo::FileType::File) {
            int64_t size = PortableFunctions::FileSize(fullPath);
            std::wstring md5 = PortableFunctions::FileMd5(fullPath);
            time_t time = PortableFunctions::FileModifiedTime(fullPath);
            directoryListing->emplace_back(type, filename, pathFromRoot, size, time, std::move(md5));
        }
        else {
            directoryListing->emplace_back(type, filename, pathFromRoot);
        }
    }
    return directoryListing.release();
}

std::vector<ApplicationPackage> LocalFileSyncServerConnection::listApplicationPackages()
{
    throw SyncError(100124);
}

bool LocalFileSyncServerConnection::downloadApplicationPackage(CString packageName, CString localPath, const std::optional<ApplicationPackage>&, const CString&)
{
    throw SyncError(100124);
}

void LocalFileSyncServerConnection::uploadApplicationPackage(CString localPath, CString packageName, CString packageSpecJson)
{
    throw SyncError(100124);
}

CString LocalFileSyncServerConnection::syncMessage(const CString&, const CString&)
{
    throw SyncError(100154);
}

void LocalFileSyncServerConnection::setListener(ISyncListener* listener)
{
    m_pListener = listener;
}

bool LocalFileSyncServerConnection::fileExists(CString remoteFilePath)
{
    return PortableFunctions::FileIsRegular(PortableFunctions::PathAppendToPath(m_rootDirectory, remoteFilePath));
}

bool LocalFileSyncServerConnection::directoryExists(CString remoteDirectoryPath)
{
    return PortableFunctions::FileIsDirectory(PortableFunctions::PathAppendToPath(m_rootDirectory, remoteDirectoryPath));
}

void LocalFileSyncServerConnection::UpdateProgress(uint64_t bytes_so_far)
{
    if (m_pListener) {
        m_pListener->onProgress(bytes_so_far);

        if (m_pListener->isCancelled()) {
            throw SyncCancelException();
        }
    }
}

void LocalFileSyncServerConnection::CopyStreams(std::istream& src, std::ostream& dst, uint64_t total_bytes)
{
    constexpr size_t BUFFER_SIZE = 32768;
    auto buffer = std::make_unique_for_overwrite<char[]>(BUFFER_SIZE);

    if (m_pListener) {
        m_pListener->setProgressTotal(total_bytes);
        UpdateProgress(0);
    }

    uint64_t so_far = 0;

    do {
        src.read(buffer.get(), BUFFER_SIZE);
        dst.write(buffer.get(), src.gcount());
        so_far += src.gcount();
        UpdateProgress(so_far);
    } while (src.gcount() > 0);
}

void LocalFileSyncServerConnection::getStream(CString remotePath, std::ostream& stream)
{
    CString path = PortableFunctions::PathToNativeSlash(PortableFunctions::PathAppendToPath(m_rootDirectory, remotePath));
    uint64_t total = PortableFunctions::FileSize(path);
    std::ifstream src(path, std::ios::binary);
    if (src.fail())
        throw SyncError(100111, (LPCTSTR)path);

    CopyStreams(src, stream, total);

    src.close();
}

void LocalFileSyncServerConnection::putStream(std::istream& stream, int64_t /*sizeBytes*/, CString remotePath)
{
    CString path = PortableFunctions::PathToNativeSlash(PortableFunctions::PathAppendToPath(m_rootDirectory, remotePath));
    PortableFunctions::PathMakeDirectories(PortableFunctions::PathGetDirectory(path));

    stream.seekg(0, std::ios::end);
    auto total = stream.tellg();
    stream.seekg(0, std::ios::beg);

    std::ofstream dest(path, std::ios::binary);
    if (dest.fail())
        throw SyncError(100111, (LPCTSTR)path);

    CopyStreams(stream, dest, total);

    dest.close();
}

void LocalFileSyncServerConnection::setDownloadPffServerParams(PFF& pff)
{
    pff.SetSyncServerType(SyncServerType::LocalFiles);
    pff.SetSyncUrl(m_rootDirectory);
}


CString LocalFileSyncServerConnection::startParadataSync(const CString& log_uuid)
{
    m_paradataSyncer = std::make_unique<FileBasedParadataSyncer<LocalFileSyncServerConnection>>(*this);

    return m_paradataSyncer->startParadataSync(log_uuid);
}

void LocalFileSyncServerConnection::putParadata(const CString& filename)
{
    return m_paradataSyncer->putParadata(filename);
}

std::vector<std::shared_ptr<TemporaryFile>> LocalFileSyncServerConnection::getParadata()
{
    return m_paradataSyncer->getParadata();
}

void LocalFileSyncServerConnection::stopParadataSync()
{
    m_paradataSyncer->stopParadataSync();

    m_paradataSyncer.reset();
}
