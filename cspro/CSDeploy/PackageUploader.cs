using System;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using CSPro.Sync;
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

        public Task<bool> UploadPackage(string packagePath, string packageName, string packageSpecJson, string inputsRootPath,
            ServerType serverType, string serverUrl, Form parentForm)
        {
            return Task.Run(() =>
            {
                OnSyncError showSyncError = (string errorMessage) =>
                {
                    parentForm.Invoke(new Action(() => { MessageBox.Show(errorMessage); }));
                };

                OnQueryUsernamePassword showLogin = (bool bShowError) =>
                {
                     return (UsernamePassword) parentForm.Invoke(new Func<UsernamePassword>(() =>
                     {
                         var loginDlg = new LoginDialog();
                         loginDlg.ShowError = bShowError;

                         if (loginDlg.ShowDialog(parentForm) != DialogResult.OK)
                             return null;

                         return new UsernamePassword { username = loginDlg.Username, password = loginDlg.Password };
                     }));
                };

                int result = 0;
                using (var syncClient = new SyncClient())
                {
                    switch (serverType)
                    {
                        case ServerType.CSWeb:
                            result = syncClient.ConnectCSWeb(serverUrl, showLogin, progressPercent, progressMessage, cancellationToken, showSyncError);
                            break;
                        case ServerType.Dropbox:
                            result = syncClient.ConnectDropbox(progressPercent, progressMessage, cancellationToken, showSyncError);
                            break;
                        case ServerType.FTP:
                            result = syncClient.ConnectFtp(serverUrl, showLogin, progressPercent, progressMessage, cancellationToken, showSyncError);
                            break;
                    }
                    if (result == 0)
                        return false;

                    result = syncClient.uploadApplicationPackage(packagePath, packageName, packageSpecJson, inputsRootPath, progressPercent, progressMessage, cancellationToken, showSyncError);

                    syncClient.disconnect();
                }

                return result != 0;
            });
        }
    }
}
