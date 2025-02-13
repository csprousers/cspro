#include "Stdafx.h"
#include "SyncClient.h"
#include <zToolsO/Tools.h>
#include <zToolsO/Utf8Convert.h>
#include <zUtilO/TemporaryFile.h>
#include <zMessageO/SystemMessageIssuer.h>
#include <zDictO/DDClass.h>
#include <zSyncO/ApplicationPackage.h>
#include <zSyncO/ISyncListener.h>
#include <zSyncO/SyncCredentialStore.h>
#include <zSyncO/SyncServerConnectionFactory.h>
#include <zSyncO/WinBluetoothAdapter.h>
#include <fstream>

using namespace CSPro::Sync;
using namespace CSPro::Data;
using namespace CSPro::Dictionary;


namespace
{
    ::SyncDirection toNativeDirection(CSPro::Util::SyncDirection d)
    {
        switch (d) {
        case CSPro::Util::SyncDirection::GET:
            return ::SyncDirection::Get;
            break;
        case CSPro::Util::SyncDirection::PUT:
            return ::SyncDirection::Put;
            break;
        case CSPro::Util::SyncDirection::BOTH:
        default:
            return ::SyncDirection::Both;
            break;
        }
    }

    // the sync system message issuer will only be used for formatting
    class SyncSystemMessageIssuer : public SystemMessageIssuer
    {
        void OnIssue(MessageType, int, const std::wstring&) override { }
    };

    class Listener : public ISyncListener
    {
    public:
        Listener(System::IProgress<float>^ progressPercent,
            System::IProgress<System::String^>^ progressMessage,
            System::Threading::CancellationToken^ cancellationToken,
            OnSyncError^ onError) :
            m_progressPercent(progressPercent),
            m_progressMessage(progressMessage),
            m_cancellationToken(cancellationToken),
            m_onError(onError)
        {
        }

        // Inherited via ISyncListener
#pragma warning( push )
#pragma warning( disable: 4793 )

        virtual void onStart(int messageNumber, ...) override
        {
            va_list args;
            va_start(args, messageNumber);
            std::wstring msg = m_syncSystemMessageIssuer.GetFormattedMessageVA(messageNumber, args);
            va_end(args);
            reportProgress(msg);
        }

        virtual void onProgress(int64_t progress, int messageNumber, ...) override
        {
            va_list args;
            va_start(args, messageNumber);
            std::wstring msg = m_syncSystemMessageIssuer.GetFormattedMessageVA(messageNumber, args);
            va_end(args);
            reportProgress(progress);
            reportProgress(msg);
        }

        virtual void onError(int messageNumber, ...) override
        {
            va_list args;
            va_start(args, messageNumber);
            std::wstring msg = m_syncSystemMessageIssuer.GetFormattedMessageVA(messageNumber, args);
            va_end(args);
            reportError(msg);
        }

#pragma warning( pop )

        virtual void onFinish() override
        {
        }

        virtual void onProgress() override
        {
        }

        virtual void onProgress(int64_t progress) override
        {
            reportProgress(progress);
        }

        void setProgressTotal(int64_t total)
        {
            m_progressHelper.m_progressTotal = total;
        }

        int64_t getProgressTotal() const
        {
            return m_progressHelper.m_progressTotal;
        }

        void addToProgressPreviousStepsTotal(int64_t update)
        {
            m_progressHelper.m_progressPreviousStepsTotal += update;
        }

        // Pause updating the progress indicator and leave it at its current level
        void showProgressUpdates(bool show)
        {
            m_progressHelper.m_showProgressUpdates = show;
        }

        virtual bool isCancelled() const override
        {
            return m_cancellationToken->IsCancellationRequested;
        }

    private:
        void reportProgress(float progress)
        {
            float percent = m_progressHelper.progressPercent(progress);
            if (percent >= 0)
                m_progressPercent->Report(percent);
        }

        void reportProgress(NullTerminatedString msg)
        {
            m_progressMessage->Report(gcnew System::String(msg.c_str()));
        }

        void reportError(NullTerminatedString errorMessage)
        {
            auto msg = gcnew System::String(errorMessage.c_str());
            m_onError->Invoke(msg);
        }

        gcroot<System::IProgress<float>^> m_progressPercent;
        gcroot<System::IProgress<System::String^>^> m_progressMessage;
        gcroot<System::Threading::CancellationToken^> m_cancellationToken;
        gcroot<OnSyncError^> m_onError;
        SyncSystemMessageIssuer m_syncSystemMessageIssuer;
        SyncListenerProgressHelper m_progressHelper;
    };
}

CSPro::Sync::SyncClient::SyncClient()
{
    m_pBluetoothAdapter = WinBluetoothAdapter::create();
    m_pConnectionFactory = new SyncServerConnectionFactory(m_pBluetoothAdapter);
    m_pSyncCredentialStore = new SyncCredentialStore();
    m_pNativeClient = new ::SyncClient(GetDeviceId(), m_pConnectionFactory);
}

CSPro::Sync::SyncClient::!SyncClient()
{
    delete m_pNativeClient;
    delete m_pConnectionFactory;
    delete m_pBluetoothAdapter;
    delete m_pSyncCredentialStore;
}

int CSPro::Sync::SyncClient::connectWeb(System::String^ hostUrl, OnShowLoginDialog^ loginDialog,
    System::IProgress<float>^ progressPercent,
    System::IProgress<System::String^>^ progressMessage,
    System::Threading::CancellationToken^ cancellationToken,
    OnSyncError^ onError)
{
    DelegateLoginDialog loginDelegate;
    loginDelegate.OnLoginDelegate = loginDialog;

    Listener listener(progressPercent, progressMessage, cancellationToken, onError);
    m_pNativeClient->setListener(&listener);
    return m_pNativeClient->connectWeb((CString) hostUrl, &loginDelegate, m_pSyncCredentialStore) == ::SyncClient::SyncResult::SYNC_OK;
}

int CSPro::Sync::SyncClient::connectFtp(System::String^ hostUrl, OnShowLoginDialog^ loginDialog,
    System::IProgress<float>^ progressPercent,
    System::IProgress<System::String^>^ progressMessage,
    System::Threading::CancellationToken^ cancellationToken,
    OnSyncError^ onError)
{
    DelegateLoginDialog loginDelegate;
    loginDelegate.OnLoginDelegate = loginDialog;
    SyncCredentialStore syncCredentialStore;

    Listener listener(progressPercent, progressMessage, cancellationToken, onError);
    m_pNativeClient->setListener(&listener);
    return m_pNativeClient->connectFtp((CString) hostUrl, &loginDelegate, &syncCredentialStore) == ::SyncClient::SyncResult::SYNC_OK;
}

int CSPro::Sync::SyncClient::connectDropbox(OnShowDropboxAuthDialog^ authDialog,
    System::IProgress<float>^ progressPercent,
    System::IProgress<System::String^>^ progressMessage,
    System::Threading::CancellationToken^ cancellationToken,
    OnSyncError^ onError)
{
    DelegateDropboxAuthDialog authDelegate;
    authDelegate.OnShowDelegate = authDialog;
    SyncCredentialStore syncCredentialStore;

    Listener listener(progressPercent, progressMessage, cancellationToken, onError);
    m_pNativeClient->setListener(&listener);
    return m_pNativeClient->connectDropbox(&authDelegate, &syncCredentialStore) == ::SyncClient::SyncResult::SYNC_OK;
}

int CSPro::Sync::SyncClient::connectDropboxLocal(
    System::IProgress<float>^ progressPercent,
    System::IProgress<System::String^>^ progressMessage,
    System::Threading::CancellationToken^ cancellationToken,
    OnSyncError^ onError)
{
    Listener listener(progressPercent, progressMessage, cancellationToken, onError);
    m_pNativeClient->setListener(&listener);
    return m_pNativeClient->connectDropboxLocal() == ::SyncClient::SyncResult::SYNC_OK;
}

int CSPro::Sync::SyncClient::connectLocalFileSystem(System::String^ rootDirectory,
    System::IProgress<float>^ progressPercent,
    System::IProgress<System::String^>^ progressMessage,
    System::Threading::CancellationToken^ cancellationToken,
    OnSyncError^ onError)
{
    Listener listener(progressPercent, progressMessage, cancellationToken, onError);
    m_pNativeClient->setListener(&listener);
    return m_pNativeClient->connectLocalFileSystem((CString)rootDirectory) == ::SyncClient::SyncResult::SYNC_OK;
}

int CSPro::Sync::SyncClient::disconnect()
{
    m_pNativeClient->setListener(nullptr);
    return m_pNativeClient->disconnect() == ::SyncClient::SyncResult::SYNC_OK;
}

int CSPro::Sync::SyncClient::syncData(CSPro::Util::SyncDirection direction,
    CSPro::Data::DataRepository^ repository,
    System::String^ universe,
    System::IProgress<float>^ progressPercent,
    System::IProgress<System::String^>^ progressMessage,
    System::Threading::CancellationToken^ cancellationToken,
    OnSyncError^ onError)
{
    Listener listener(progressPercent, progressMessage, cancellationToken, onError);
    m_pNativeClient->setListener(&listener);

    ISyncableDataRepository* pSyncableRepo = repository->GetNativePointer()->GetSyncableDataRepository();

    if (pSyncableRepo != nullptr) {
        return m_pNativeClient->syncData(toNativeDirection(direction), *pSyncableRepo, (CString) universe) == ::SyncClient::SyncResult::SYNC_OK;
    } else {
        listener.onError(100116, (LPCTSTR) (CString) repository->Dictionary->Name);
        return 0;
    }
}

array<CSPro::Sync::DictionaryInfo^>^ CSPro::Sync::SyncClient::getDictionaries(System::IProgress<float>^ progressPercent, System::IProgress<System::String^>^ progressMessage, System::Threading::CancellationToken ^ cancellationToken, OnSyncError ^ onError)
{
    Listener listener(progressPercent, progressMessage, cancellationToken, onError);
    m_pNativeClient->setListener(&listener);
    std::vector<::DictionaryInfo> nativeDictionaries;
    if (m_pNativeClient->getDictionaries(nativeDictionaries) == ::SyncClient::SyncResult::SYNC_OK) {
        array<DictionaryInfo^>^ managedDicts = gcnew array<DictionaryInfo^>(nativeDictionaries.size());
        for (size_t i = 0; i < nativeDictionaries.size(); ++i) {
            managedDicts[i] = gcnew DictionaryInfo(
                gcnew System::String(nativeDictionaries[i].m_name),
                gcnew System::String(nativeDictionaries[i].m_label),
                nativeDictionaries[i].m_caseCount);
        }
        return managedDicts;
    }
    return nullptr;
}

// Sync non-data file
int CSPro::Sync::SyncClient::syncFile(CSPro::Util::SyncDirection direction,
    System::String^ pathFrom,
    System::String^ pathTo,
    System::String^ clientFileRoot,
    System::IProgress<float>^ progressPercent,
    System::IProgress<System::String^>^ progressMessage,
    System::Threading::CancellationToken^ cancellationToken,
    OnSyncError^ onError)
{
    Listener listener(progressPercent, progressMessage, cancellationToken, onError);
    m_pNativeClient->setListener(&listener);
    return m_pNativeClient->syncFile(toNativeDirection(direction), pathFrom, pathTo, clientFileRoot) == ::SyncClient::SyncResult::SYNC_OK;
}

int CSPro::Sync::SyncClient::uploadApplicationPackage(System::String^ localPath,
    System::String^ packageName,
    System::String^ packageSpecJson,
    System::IProgress<float>^ progressPercent,
    System::IProgress<System::String^>^ progressMessage,
    System::Threading::CancellationToken^ cancellationToken,
    OnSyncError^ onError)
{
    Listener listener(progressPercent, progressMessage, cancellationToken, onError);
    m_pNativeClient->setListener(&listener);
    return m_pNativeClient->uploadApplicationPackage(localPath, packageName, packageSpecJson) == ::SyncClient::SyncResult::SYNC_OK;
}

DataDictionary^ CSPro::Sync::SyncClient::getDictionary(System::String^ dictionaryName, System::IProgress<float>^ progressPercent, System::IProgress<System::String^>^ progressMessage, System::Threading::CancellationToken ^ cancellationToken, OnSyncError ^ onError)
{
    Listener listener(progressPercent, progressMessage, cancellationToken, onError);
    m_pNativeClient->setListener(&listener);
    CString dictionaryText;
    if (m_pNativeClient->downloadDictionary(dictionaryName, dictionaryText) == ::SyncClient::SyncResult::SYNC_OK) {
        TemporaryFile tempDictFile;
        std::ofstream tmpStream(tempDictFile.GetPath(), std::ios::binary);
        tmpStream << UTF8Convert::WideToUTF8(dictionaryText);
        tmpStream.close();
        try {
            return gcnew DataDictionary(gcnew System::String(tempDictFile.GetPath().c_str()));
        }
        catch (Exception^) {
            listener.onError(100137);
            return nullptr;
        }
    } else {
        return nullptr;
    }
}

int CSPro::Sync::SyncClient::uploadDictionary(System::String^ dictionaryName, System::IProgress<float>^ progressPercent, System::IProgress<System::String^>^ progressMessage, System::Threading::CancellationToken ^ cancellationToken, OnSyncError ^ onError)
{
    Listener listener(progressPercent, progressMessage, cancellationToken, onError);
    m_pNativeClient->setListener(&listener);
    CString dictionaryText;
    return m_pNativeClient->uploadDictionary(dictionaryName) == ::SyncClient::SyncResult::SYNC_OK;
}

System::String^ CSPro::Sync::DictionaryInfo::Name::get()
{
    return m_name;
}

System::String^ CSPro::Sync::DictionaryInfo::Label::get()
{
    return m_label;
}

int CSPro::Sync::DictionaryInfo::CaseCount::get()
{
    return m_caseCount;
}

CSPro::Sync::DictionaryInfo::DictionaryInfo(System::String ^ name, System::String ^ label, int caseCount) :
    m_name(name),
    m_label(label),
    m_caseCount(caseCount)
{}
