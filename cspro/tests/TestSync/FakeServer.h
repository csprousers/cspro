#pragma once

#include <zCaseO/Case.h>
#include <zSyncO/ISyncServerConnection.h>
#include <zSyncO/IDataChunk.h>
#include <zSyncO/DefaultChunk.h>
#include <zSyncO/SyncRequest.h>
#include <zSyncO/ConnectResponse.h>
#include <zSyncO/DefaultChunk.h>
#include <zSyncO/CaseObservable.h>


class CDataDict;

// Fake sync server that always give the same response.
class FakeServer : public ISyncServerConnection {

public:

    FakeServer()
    {
    }

    FakeServer(const std::vector<std::shared_ptr<Case>>& responseCases)
        : m_responseCases(responseCases),
        m_responseServerRevision(_T("1")),
        m_dataChunk(DefaultDataChunk())
    {
    }

    // Connect to remote server
    virtual ConnectResponse* connect()
    {
        return new ConnectResponse(L"myserver");
    }

    // Disconnect from remote server
    virtual void disconnect()
    {
    }

    // Send request to download cases from server
    // Throws SyncException.
    virtual SyncGetResponse getData(const SyncRequest& request)
    {
        m_requestServerRevision = request.getLastServerRevision();
        return SyncGetResponse(SyncGetResponse::SyncGetResult::Complete,
            std::make_shared<CaseObservable>(rxcpp::observable<>::iterate(m_responseCases)),
            m_responseServerRevision);
    }

    // Send request to upload cases to server
    // Throws SyncException.
    virtual SyncPutResponse putData(const SyncRequest& request)
    {
        m_requestServerRevision = request.getLastServerRevision();
        m_requestCases = request.getClientCases();
        return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete,
            m_responseServerRevision);
    }

    virtual bool getFile(CString remoteFilePath, CString tempLocalFilePath, CString actualLocalFilePath, CString md5 = CString())
    {
        return true;
    }

    virtual void putFile(CString localPath, CString remotePath)
    {}

    virtual IDataChunk& getChunk()
    {
        return m_dataChunk;
    }

    virtual std::vector<FileInfo>* getDirectoryListing(CString remotePath)
    {
        return NULL;
    }

    const std::vector<std::shared_ptr<Case>>& getClientCasesFromLastRequest()
    {
        return m_requestCases;
    }

    virtual void setListener(ISyncListener*) {}

    virtual std::vector<DictionaryInfo> getDictionaries()
    {
        return std::vector<DictionaryInfo>();
    }

    virtual CString getDictionary(CString)
    {
        return CString();
    }

    virtual void putDictionary(CString) {}

    virtual void deleteDictionary(CString) {}

    void setDownloadPffServerParams(PFF&) {}

    std::vector<ApplicationPackage> listApplicationPackages()
    {
        return std::vector<ApplicationPackage>();
    }

    bool downloadApplicationPackage(CString, CString, const std::optional<ApplicationPackage>&, const CString&) override
    {
        return false;
    }

    void uploadApplicationPackage(CString, CString, CString)
    {}

    CString syncMessage(const CString&, const CString&) override
    {
        return CString();
    }

    std::unique_ptr<TemporaryFile> getFileIfExists(const CString&) override
    {
        return nullptr;
    }

    CString startParadataSync(const CString&) override
    {
        return CString();
    }

    void putParadata(const CString&) override {}

    std::vector<std::shared_ptr<TemporaryFile>> getParadata() override
    {
        return {};
    }

    void stopParadataSync() override {}

 private:

    std::vector<std::shared_ptr<Case>> m_requestCases, m_responseCases;
    CString m_responseServerRevision;
    CString m_requestServerRevision;
    DefaultDataChunk m_dataChunk;
};
