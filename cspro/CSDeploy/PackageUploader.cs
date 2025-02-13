using CSPro.Sync;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Threading;
using System.Threading;
using WinFormsShared;

namespace CSDeploy
{
    public class PackageUploader
    {
        private IProgress<float> progressPercent;
        private IProgress<string> progressMessage;
        private CancellationToken cancellationToken;

        public PackageUploader(IProgress<float> progressPercent, IProgress<string> progressMessage, CancellationToken cancellationToken)
        {
            this.progressPercent = progressPercent;
            this.progressMessage = progressMessage;
            this.cancellationToken = cancellationToken;
        }

        public enum ServerType
        {
            CSWeb, Dropbox, FTP
        }

        public Task<bool> UploadPackage(string packagePath, string packageName, string packageSpecJson,
            ServerType serverType, string serverUrl, IEnumerable<string> dictionariesToUpload,
            Form parentForm)
        {
            return Task.Run(() =>
            {
                OnSyncError showSyncError = (string errorMessage) =>
                {
                    parentForm.Invoke(new Action(() => { MessageBox.Show(errorMessage); }));
                };

                OnShowLoginDialog showLogin = (bool bShowError) =>
                {

                     return (LoginInfo) parentForm.Invoke(new Func<LoginInfo>(() =>
                     {
                         var loginDlg = new LoginDialog();
                         loginDlg.ShowError = bShowError;

                         if (loginDlg.ShowDialog(parentForm) != DialogResult.OK)
                             return null;

                         return new LoginInfo { username = loginDlg.Username, password = loginDlg.Password };
                     }));
                };

                OnShowDropboxAuthDialog showDropboxAuth = (string clientId) =>
                {
                    return (string) parentForm.Invoke(new Func<String>(() =>
                    {
                        var authDlg = new DropboxAuthDialog(clientId);
                        if (authDlg.ShowDialog(parentForm) != DialogResult.OK)
                            return null;

                        return authDlg.AccessToken;
                    }));
                };

                int result = 0;
                using (var syncClient = new SyncClient())
                {
                    switch (serverType)
                    {
                        case ServerType.CSWeb:
                            result = syncClient.connectWeb(serverUrl, showLogin, progressPercent, progressMessage, cancellationToken, showSyncError);
                            break;
                        case ServerType.Dropbox:
                            result = syncClient.connectDropbox(showDropboxAuth, progressPercent, progressMessage, cancellationToken, showSyncError);
                            break;
                        case ServerType.FTP:
                            result = syncClient.connectFtp(serverUrl, showLogin, progressPercent, progressMessage, cancellationToken, showSyncError);
                            break;
                    }
                    if (result == 0)
                        return false;

                    result = syncClient.uploadApplicationPackage(packagePath, packageName, packageSpecJson, progressPercent, progressMessage, cancellationToken, showSyncError);
                    if (result != 0)
                    {
                        foreach (var dictionary in dictionariesToUpload)
                        {
                            if (syncClient.uploadDictionary(dictionary, progressPercent, progressMessage, cancellationToken, showSyncError) == 0)
                            {
                                result = 0;
                                break;
                            }
                        }
                    }

                    syncClient.disconnect();
                }

                return result != 0;
            });
        }
    }
}
