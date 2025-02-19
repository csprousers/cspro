#include "Stdafx.h"
#include "SyncClient.h"
#include <zToolsO/Tools.h>
#include <zNetwork/LoginCredentials.h>
#include <zNetwork/SyncListener.h>
#include <zSyncO/ApplicationPackage.h>
#include <zSyncO/BluetoothDeviceInfo.h>
#include <zSyncO/SyncServiceFactory.h>
#include <fstream>


namespace
{
    class SyncCLRListener : public SyncListener
    {
    public:
        SyncCLRListener(System::IProgress<float>^ progressPercent,
                        System::IProgress<System::String^>^ progressMessage,
                        System::Threading::CancellationToken^ cancellationToken,
                        CSPro::Sync::OnSyncError^ onError)
            :   m_progressPercent(progressPercent),
                m_progressMessage(progressMessage),
                m_cancellationToken(cancellationToken),
                m_onError(onError)
        {
        }

    protected:
        void OnStart(const std::string& message_text) override
        {
            m_progressMessage->Report(clr_helpers::to_SystemString(message_text));
        }

        void OnFinish() override
        {
        }

        void OnProgress(const int reportable_progress, const cs::cref_optional<std::string> message_text) override
        {
            // the progress reporter expects values from 0 - 1
            if( reportable_progress >= 0 )
                m_progressPercent->Report(static_cast<float>(reportable_progress) / 100);

            if( message_text.has_value() )
                m_progressMessage->Report(clr_helpers::to_SystemString(*message_text));
        }

        bool IsCanceled() const override
        {
            return m_cancellationToken->IsCancellationRequested;
        }

        void OnError(int /*message_number*/, const std::string& message_text) override
        {
            m_onError->Invoke(clr_helpers::to_SystemString(message_text));
        }

    private:
        gcroot<System::IProgress<float>^> m_progressPercent;
        gcroot<System::IProgress<System::String^>^> m_progressMessage;
        gcroot<System::Threading::CancellationToken^> m_cancellationToken;
        gcroot<CSPro::Sync::OnSyncError^> m_onError;
    };
}


CSPro::Sync::SyncClient::SyncClient()
    :   m_syncCLRLoginAccessor(new std::shared_ptr<SyncCLRLoginAccessor>(std::make_shared<SyncCLRLoginAccessor>())),
        m_pNativeClient(new ::SyncClient(GetDeviceId(), std::make_unique<SyncServiceFactory>(*m_syncCLRLoginAccessor)))
{
}


CSPro::Sync::SyncClient::!SyncClient()
{
    delete m_pNativeClient;
    delete m_syncCLRLoginAccessor;
}


int CSPro::Sync::SyncClient::ConnectCSWeb(System::String^ hostUrl, OnQueryUsernamePassword^ on_query_username_password,
    System::IProgress<float>^ progressPercent,
    System::IProgress<System::String^>^ progressMessage,
    System::Threading::CancellationToken^ cancellationToken,
    OnSyncError^ onError)
{
    (*m_syncCLRLoginAccessor)->on_query_username_password = on_query_username_password;

    m_pNativeClient->SetSyncListener(std::make_unique<SyncCLRListener>(progressPercent, progressMessage, cancellationToken, onError));

    return ( m_pNativeClient->ConnectCSWeb(clr_helpers::to_string(hostUrl), nullptr) == ::SyncClient::SyncResult::SYNC_OK );
}


int CSPro::Sync::SyncClient::ConnectFtp(System::String^ hostUrl, OnQueryUsernamePassword^ on_query_username_password,
    System::IProgress<float>^ progressPercent,
    System::IProgress<System::String^>^ progressMessage,
    System::Threading::CancellationToken^ cancellationToken,
    OnSyncError^ onError)
{
    (*m_syncCLRLoginAccessor)->on_query_username_password = on_query_username_password;

    m_pNativeClient->SetSyncListener(std::make_unique<SyncCLRListener>(progressPercent, progressMessage, cancellationToken, onError));

    return ( m_pNativeClient->ConnectFtp(clr_helpers::to_string(hostUrl), nullptr) == ::SyncClient::SyncResult::SYNC_OK );
}


int CSPro::Sync::SyncClient::ConnectDropbox(System::IProgress<float>^ progressPercent,
    System::IProgress<System::String^>^ progressMessage,
    System::Threading::CancellationToken^ cancellationToken,
    OnSyncError^ onError)
{
    m_pNativeClient->SetSyncListener(std::make_unique<SyncCLRListener>(progressPercent, progressMessage, cancellationToken, onError));

    return ( m_pNativeClient->ConnectDropbox() == ::SyncClient::SyncResult::SYNC_OK );
}


int CSPro::Sync::SyncClient::ConnectDropboxLocal(
    System::IProgress<float>^ progressPercent,
    System::IProgress<System::String^>^ progressMessage,
    System::Threading::CancellationToken^ cancellationToken,
    OnSyncError^ onError)
{
    m_pNativeClient->SetSyncListener(std::make_unique<SyncCLRListener>(progressPercent, progressMessage, cancellationToken, onError));

    const SyncConnectionString sync_connection_string = SyncConnectionString::CreateLocalDropboxSyncConnectionString();

    return ( m_pNativeClient->ConnectDropbox(sync_connection_string) == ::SyncClient::SyncResult::SYNC_OK );
}


int CSPro::Sync::SyncClient::disconnect()
{
    m_pNativeClient->SetSyncListener(nullptr);
    return m_pNativeClient->Disconnect() == ::SyncClient::SyncResult::SYNC_OK;
}


int CSPro::Sync::SyncClient::uploadApplicationPackage(System::String^ localPackageZipFilePath,
    System::String^ packageName,
    System::String^ packageSpecJson,
    System::String^ directoryForDictionaryUploadEvaluation,
    System::IProgress<float>^ progressPercent,
    System::IProgress<System::String^>^ progressMessage,
    System::Threading::CancellationToken^ cancellationToken,
    OnSyncError^ onError)
{
    m_pNativeClient->SetSyncListener(std::make_unique<SyncCLRListener>(progressPercent, progressMessage, cancellationToken, onError));

    return ( m_pNativeClient->UploadApplicationPackage(clr_helpers::to_string(localPackageZipFilePath), clr_helpers::to_string(packageName),
                                                       clr_helpers::to_string(packageSpecJson), clr_helpers::to_string(directoryForDictionaryUploadEvaluation)) == ::SyncClient::SyncResult::SYNC_OK );
}
