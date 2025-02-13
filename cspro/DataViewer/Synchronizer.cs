using CSPro.Dictionary;
using CSPro.Sync;
using CSPro.Util;
using WinFormsShared;
using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace DataViewer
{
    class Synchronizer
    {
        private SyncClient syncClient = new SyncClient();

        public Task<DictionaryInfo[]> ConnectAndGetDictionariesAsync(
            SyncServerParameters server_params,
            IProgress<float> progress_percent,
            IProgress<string> progress_message,
            CancellationToken cancellation_token,
            Form parent_form)
        {
            return Task.Run(() =>
            {
                OnSyncError showSyncError = (string errorMessage) =>
                {
                    parent_form.Invoke(new Action(() => { MessageBox.Show(errorMessage); }));
                };

                OnShowLoginDialog showLogin = (bool bShowError) =>
                {

                    return (LoginInfo)parent_form.Invoke(new Func<LoginInfo>(() =>
                    {
                        var loginDlg = new LoginDialog();
                        loginDlg.ShowError = bShowError;

                        if (loginDlg.ShowDialog(parent_form) != DialogResult.OK)
                            return null;

                        return new LoginInfo { username = loginDlg.Username, password = loginDlg.Password };
                    }));
                };

                OnShowDropboxAuthDialog showDropboxAuth = (string clientId) =>
                {
                    return (string)parent_form.Invoke(new Func<String>(() =>
                    {
                        var authDlg = new DropboxAuthDialog(clientId);
                        if (authDlg.ShowDialog(parent_form) != DialogResult.OK)
                            return null;

                        return authDlg.AccessToken;
                    }));
                };

                int connectResult = 0;
                switch (server_params.Type)
                {
                    case SyncServerType.CSWeb:
                        connectResult = syncClient.connectWeb(server_params.Url, showLogin, progress_percent, progress_message, cancellation_token, showSyncError);
                        break;
                    case SyncServerType.FTP:
                        connectResult = syncClient.connectFtp(server_params.Url, showLogin, progress_percent, progress_message, cancellation_token, showSyncError);
                        break;
                    case SyncServerType.Dropbox:
                        connectResult = syncClient.connectDropbox(showDropboxAuth, progress_percent, progress_message, cancellation_token, showSyncError);
                        break;
                    case SyncServerType.LocalDropbox:
                        connectResult = syncClient.connectDropboxLocal(progress_percent, progress_message, cancellation_token, showSyncError);
                        break;
                    case SyncServerType.LocalFiles:
                        connectResult = syncClient.connectLocalFileSystem(server_params.Url, progress_percent, progress_message, cancellation_token, showSyncError);
                        break;
                }
                if (connectResult != 0)
                {
                    return syncClient.getDictionaries(progress_percent, progress_message, cancellation_token, showSyncError);
                }
                return null;
            });

        }

        public async Task<bool> DownloadAsync(
            SyncServerParameters server_params,
            String dictionary_name,
            String file_path,
            IProgress<float> progress_percent,
            IProgress<string> progress_message,
            CancellationToken cancellation_token,
            Form parent_form)
        {
            var connection_string = new CSPro.Util.ConnectionString(file_path);
            return await SyncDataAsync(null, server_params, dictionary_name, true,
                SyncDirection.GET, connection_string, progress_percent, progress_message, cancellation_token, parent_form);
        }

        public async Task<bool> SyncAsync(
            CSPro.Data.DataRepository repository,
            CSPro.Util.ConnectionString connection_string,
            SyncServerParameters server_params,
            String dictionary_name,
            SyncDirection direction,
            IProgress<float> progress_percent,
            IProgress<string> progress_message,
            CancellationToken cancellation_token,
            Form parent_form)
        {
            return await SyncDataAsync(repository, server_params, dictionary_name, false,
                direction, connection_string, progress_percent, progress_message, cancellation_token, parent_form);
        }

        private Task<bool> SyncDataAsync(
            CSPro.Data.DataRepository repository,
            SyncServerParameters server_params,
            String dictionary_name,
            bool alreadyConnected,
            SyncDirection direction,
            CSPro.Util.ConnectionString connection_string,
            IProgress<float> progress_percent,
            IProgress<string> progress_message,
            CancellationToken cancellation_token,
            Form parent_form)
        {
            return Task.Run(() =>
            {
                OnSyncError showSyncError = (string errorMessage) =>
                {
                    parent_form.Invoke(new Action(() => { MessageBox.Show(errorMessage); }));
                };

                OnSyncError ignoreSyncError = (string errorMessage) =>
                {
                };

                OnShowLoginDialog showLogin = (bool bShowError) =>
                {

                    return (LoginInfo)parent_form.Invoke(new Func<LoginInfo>(() =>
                    {
                        var loginDlg = new LoginDialog();
                        loginDlg.ShowError = bShowError;

                        if (loginDlg.ShowDialog(parent_form) != DialogResult.OK)
                            return null;

                        return new LoginInfo { username = loginDlg.Username, password = loginDlg.Password };
                    }));
                };

                OnShowDropboxAuthDialog showDropboxAuth = (string clientId) =>
                {
                    return (string)parent_form.Invoke(new Func<String>(() =>
                    {
                        var authDlg = new DropboxAuthDialog(clientId);
                        if (authDlg.ShowDialog(parent_form) != DialogResult.OK)
                            return null;

                        return authDlg.AccessToken;
                    }));
                };

                if (!alreadyConnected)
                {
                    int connectResult = 0;
                    switch (server_params.Type)
                    {
                        case SyncServerType.CSWeb:
                            connectResult = syncClient.connectWeb(server_params.Url, showLogin, progress_percent, progress_message, cancellation_token, showSyncError);
                            break;
                        case SyncServerType.Dropbox:
                            connectResult = syncClient.connectDropbox(showDropboxAuth, progress_percent, progress_message, cancellation_token, showSyncError);
                            break;
                        case SyncServerType.FTP:
                            connectResult = syncClient.connectFtp(server_params.Url, showLogin, progress_percent, progress_message, cancellation_token, showSyncError);
                            break;
                        case SyncServerType.LocalDropbox:
                            connectResult = syncClient.connectDropboxLocal(progress_percent, progress_message, cancellation_token, showSyncError);
                            break;
                        case SyncServerType.LocalFiles:
                            connectResult = syncClient.connectLocalFileSystem(server_params.Url, progress_percent, progress_message, cancellation_token, showSyncError);
                            break;
                    }
                    if (connectResult == 0)
                        return false;
                }

                // Try to get server dictionary - we only really need it for GET but for PUT and BOTH we still want
                // it to compare the name against embedded dcf in case someone tries to sync to the wrong dictionary
                var errorHandler = direction == SyncDirection.GET ? showSyncError : ignoreSyncError;
                var dictionary = syncClient.getDictionary(dictionary_name, progress_percent, progress_message, cancellation_token, errorHandler);
                if (dictionary == null && direction == SyncDirection.GET)
                    return false;

                repository = new CSPro.Data.DataRepository();

                try
                {
                    if (File.Exists(connection_string.Filename))
                    {
                        DataDictionary embedded_dictionary = CSPro.Data.DataRepository.GetEmbeddedDictionary(connection_string);

                        if (embedded_dictionary != null && embedded_dictionary.Name != dictionary_name)
                        {
                            showSyncError.Invoke("The file " + System.IO.Path.GetFileName(connection_string.Filename) + " uses dictionary " + embedded_dictionary.Name + ", not " + dictionary_name + ". Unable to sync to a file with a different dictionary.");
                            return false;
                        }

                        if (dictionary == null && embedded_dictionary == null)
                            return false;

                        if (dictionary == null)
                            dictionary = embedded_dictionary;

                        repository.OpenForReadingAndWriting(dictionary, connection_string);
                    }
                    else
                    {
                        if (dictionary == null)
                            return false;
                        repository.CreateForReadingAndWriting(dictionary, connection_string);
                    }
                }
                catch (Exception ex)
                {
                    showSyncError.Invoke("Error opening file " + connection_string.Filename + " : " + ex.Message);
                    return false;
                }

                var result = syncClient.syncData(direction, repository, "", progress_percent, progress_message, cancellation_token, showSyncError);

                syncClient.disconnect();

                // Reopen in read-only mode
                repository.Close();
                repository.OpenForReading(dictionary, connection_string);

                return result != 0;

            });
        }

        internal void Disconnect()
        {
            syncClient.disconnect();
        }
    }
}
