#pragma once

#include <zSyncO/SyncClient.h>
#include <zSyncCLR/SyncCLRLoginAccessor.h>


namespace CSPro
{
    namespace Sync
    {
        public delegate void OnSyncError(System::String^ errorMessage);

        ///<summary>
        ///Synchronization client
        ///</summary>
        public ref class SyncClient sealed
        {
        public:
            SyncClient();

            ~SyncClient() { this->!SyncClient(); }
            !SyncClient();

            ///<summary>
            ///Connect to CSWeb sync service
            ///</summary>
            int ConnectCSWeb(System::String^ hostUrl, OnQueryUsernamePassword^ on_query_username_password,
                System::IProgress<float>^ progressPercent,
                System::IProgress<System::String^>^ progressMessage,
                System::Threading::CancellationToken^ cancellationToken,
                OnSyncError^ onError);

            ///<summary>
            ///Connect to FTP sync service
            ///</summary>
            int ConnectFtp(System::String^ hostUrl, OnQueryUsernamePassword^ on_query_username_password,
                System::IProgress<float>^ progressPercent,
                System::IProgress<System::String^>^ progressMessage,
                System::Threading::CancellationToken^ cancellationToken,
                OnSyncError^ onError);

            ///<summary>
            ///Connect to Dropbox sync service
            ///</summary>
            int ConnectDropbox(System::IProgress<float>^ progressPercent,
                System::IProgress<System::String^>^ progressMessage,
                System::Threading::CancellationToken^ cancellationToken,
                OnSyncError^ onError);

            ///<summary>
            ///Connect to Dropbox folder on local machine
            ///</summary>
            int ConnectDropboxLocal(System::IProgress<float>^ progressPercent,
                System::IProgress<System::String^>^ progressMessage,
                System::Threading::CancellationToken^ cancellationToken,
                OnSyncError^ onError);

            int disconnect();

            int uploadApplicationPackage(System::String^ localPackageZipFilePath,
                System::String^ packageName,
                System::String^ packageSpecJson,
                System::String^ directoryForDictionaryUploadEvaluation,
                System::IProgress<float>^ progressPercent,
                System::IProgress<System::String^>^ progressMessage,
                System::Threading::CancellationToken^ cancellationToken,
                OnSyncError^ onError);

        private:
            std::shared_ptr<SyncCLRLoginAccessor>* m_syncCLRLoginAccessor;
            ::SyncClient* m_pNativeClient;
        };
    }
}
