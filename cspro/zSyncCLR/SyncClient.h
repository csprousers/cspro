#pragma once

#include <zSyncO/SyncClient.h>
#include <zSyncCLR/DelegateLoginDialog.h>
#include <zSyncCLR/DelegateDropboxAuthDialog.h>
struct ISyncServerConnectionFactory;
struct IBluetoothAdapter;
class SyncCredentialStore;

namespace CSPro {

    namespace Sync {

        public ref class DictionaryInfo
        {
        public:

            DictionaryInfo(System::String^ name, System::String^ label, int caseCount);

            property System::String^ Name {
                System::String^ get();
            }

            property System::String^ Label {
                System::String^ get();
            }

            property int CaseCount {
                int get();
            }
        private:

            System::String^ m_name;
            System::String^ m_label;
            int m_caseCount;
        };

        public delegate void OnSyncError(String^ errorMessage);

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
            ///Connect to CSWeb sync server
            ///</summary>
            int connectWeb(System::String^ hostUrl, OnShowLoginDialog^ loginDialog,
                System::IProgress<float>^ progressPercent,
                System::IProgress<System::String^>^ progressMessage,
                System::Threading::CancellationToken^ cancellationToken,
                OnSyncError^ onError);

            ///<summary>
            ///Connect to FTP sync server
            ///</summary>
            int connectFtp(System::String^ hostUrl, OnShowLoginDialog^ loginDialog,
                System::IProgress<float>^ progressPercent,
                System::IProgress<System::String^>^ progressMessage,
                System::Threading::CancellationToken^ cancellationToken,
                OnSyncError^ onError);

            ///<summary>
            ///Connect to Dropbox sync server
            ///</summary>
            int connectDropbox(OnShowDropboxAuthDialog^ authDialog,
                System::IProgress<float>^ progressPercent,
                System::IProgress<System::String^>^ progressMessage,
                System::Threading::CancellationToken^ cancellationToken,
                OnSyncError^ onError);

            ///<summary>
            ///Connect to Dropbox folder on local machine
            ///</summary>
            int connectDropboxLocal(System::IProgress<float>^ progressPercent,
                System::IProgress<System::String^>^ progressMessage,
                System::Threading::CancellationToken^ cancellationToken,
                OnSyncError^ onError);

            ///<summary>
            ///Connect to folder on local machine
            ///</summary>
            int connectLocalFileSystem(System::String^ rootDirectory,
                System::IProgress<float>^ progressPercent,
                System::IProgress<System::String^>^ progressMessage,
                System::Threading::CancellationToken^ cancellationToken,
                OnSyncError^ onError);

            int disconnect();

            // Sync data file using smart sync
            int syncData(CSPro::Util::SyncDirection direction, CSPro::Data::DataRepository^ repository, System::String^ universe,
                System::IProgress<float>^ progressPercent,
                System::IProgress<System::String^>^ progressMessage,
                System::Threading::CancellationToken^ cancellationToken,
                OnSyncError^ onError);

            // Sync non-data file
            int syncFile(CSPro::Util::SyncDirection direction,
                System::String^ pathFrom,
                System::String^ pathTo,
                System::String^ clientFileRoot,
                System::IProgress<float>^ progressPercent,
                System::IProgress<System::String^>^ progressMessage,
                System::Threading::CancellationToken^ cancellationToken,
                OnSyncError^ onError);

            int uploadApplicationPackage(System::String^ localPath,
                System::String^ packageName,
                System::String^ packageSpecJson,
                System::IProgress<float>^ progressPercent,
                System::IProgress<System::String^>^ progressMessage,
                System::Threading::CancellationToken^ cancellationToken,
                OnSyncError^ onError);

            // Get list of dictionaries from server
            array<DictionaryInfo^>^ getDictionaries(
                System::IProgress<float>^ progressPercent,
                System::IProgress<System::String^>^ progressMessage,
                System::Threading::CancellationToken^ cancellationToken,
                OnSyncError^ onError);

            // Download a dictionary from server
            CSPro::Dictionary::DataDictionary^ getDictionary(
                System::String^ dictionaryName,
                System::IProgress<float>^ progressPercent,
                System::IProgress<System::String^>^ progressMessage,
                System::Threading::CancellationToken^ cancellationToken,
                OnSyncError^ onError);

            // Upload a dictionary to server
            int uploadDictionary(
                System::String^ dictionaryName,
                System::IProgress<float>^ progressPercent,
                System::IProgress<System::String^>^ progressMessage,
                System::Threading::CancellationToken^ cancellationToken,
                OnSyncError^ onError);

        private:
            IBluetoothAdapter* m_pBluetoothAdapter;
            ISyncServerConnectionFactory* m_pConnectionFactory;
            SyncCredentialStore* m_pSyncCredentialStore;
            ::SyncClient* m_pNativeClient;
        };
    }
}
