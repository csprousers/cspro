using System;
using System.Windows.Forms;
using CSPro.Util;
using Microsoft.WindowsAPICodePack.Dialogs;

namespace DataViewer
{
    public partial class SyncServerSelector : UserControl
    {
        private SyncServerParameters serverParameters = new SyncServerParameters(SyncServerType.CSWeb, "http://");

        internal SyncServerParameters ServerParameters {
            get => serverParameters;
            set
            {
                serverParameters = value;
                textBoxServerUrl.Text = serverParameters.Url;
                switch (serverParameters.Type)
                {
                    case SyncServerType.CSWeb:
                        radioButtonCSWeb.Checked = true;
                        break;
                    case SyncServerType.Dropbox:
                        radioButtonDropbox.Checked = true;
                        break;
                    case SyncServerType.FTP:
                        radioButtonFTP.Checked = true;
                        break;
                    case SyncServerType.LocalDropbox:
                        radioButtonDropboxFolder.Checked = true;
                        break;
                    case SyncServerType.LocalFiles:
                        radioButtonFTPFolder.Checked = true;
                        break;
                }
            }
        }

        public SyncServerSelector()
        {
            InitializeComponent();
        }
        
        private void radioButtonCSWeb_CheckedChanged(object sender, EventArgs e)
        {
            ServerParameters.ChangeType(SyncServerType.CSWeb);
            textBoxServerUrl.Text = ServerParameters.Url;
            updateEnabledControls();
        }

        private void radioButtonDropbox_CheckedChanged(object sender, EventArgs e)
        {
            ServerParameters.ChangeType(SyncServerType.Dropbox);
            textBoxServerUrl.Text = ServerParameters.Url;
            updateEnabledControls();
        }

        private void radioButtonFTP_CheckedChanged(object sender, EventArgs e)
        {
            ServerParameters.ChangeType(SyncServerType.FTP);
            textBoxServerUrl.Text = ServerParameters.Url;
            updateEnabledControls();
        }

        private void radioButtonDropboxFolder_CheckedChanged(object sender, EventArgs e)
        {
            ServerParameters.ChangeType(SyncServerType.LocalDropbox);
            textBoxServerUrl.Text = ServerParameters.Url;
            updateEnabledControls();
        }

        private void radioButtonFTPFolder_CheckedChanged(object sender, EventArgs e)
        {
            ServerParameters.ChangeType(SyncServerType.LocalFiles);
            textBoxServerUrl.Text = ServerParameters.Url;
            updateEnabledControls();
        }

        private void buttonFolderBrowse_Click(object sender, EventArgs e)
        {
            CommonOpenFileDialog dialog = new CommonOpenFileDialog();
            dialog.Title = "Please select a folder";
            dialog.IsFolderPicker = true;

            if (dialog.ShowDialog() == CommonFileDialogResult.Ok)
            {
                ServerParameters.Url = dialog.FileName;
                textBoxServerUrl.Text = ServerParameters.Url;
            }
        }

        private void textBoxServerUrl_TextChanged(object sender, EventArgs e)
        {
            ServerParameters.Url = textBoxServerUrl.Text.Trim();
        }

        private void updateEnabledControls()
        {
            textBoxServerUrl.Enabled = ServerParameters.TypeRequiresUrl;
            buttonFolderBrowse.Enabled = ServerParameters.Type == SyncServerType.LocalFiles;
        }
    }
}
