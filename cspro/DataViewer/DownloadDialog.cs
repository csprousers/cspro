using System;
using System.IO;
using System.Linq;
using System.Windows.Forms;
using CSPro.Sync;
using CSPro.Util;
using WinFormsShared;

namespace DataViewer
{
    public partial class DownloadDialog : Form
    {
        public SyncServerParameters ServerParameters { get; private set; }

        private Synchronizer _synchronizer = new Synchronizer();
        private DictionaryInfo[] _dictionaries;

        public String SelectedDictionary {
            get { return _dictionaries != null ? _dictionaries[comboBoxDictionaries.SelectedIndex].Name : null; }
        }

        public string FilePath { get; private set; }

        public DownloadDialog(SyncServerParameters server_parameters)
        {
            InitializeComponent();
            if (server_parameters != null)
                serverSelector.ServerParameters = server_parameters;
            updateEnabled();
        }

        private void buttonSaveAsBrowse_Click(object sender, EventArgs e)
        {
            var saveFileDialog = new SaveFileDialog();
            saveFileDialog.Filter = FileFilters.CsdbFileFilter;
            saveFileDialog.OverwritePrompt = false;
            if (saveFileDialog.ShowDialog() == DialogResult.OK)
                textBoxSaveAsFilename.Text = saveFileDialog.FileName;
        }

        private async void buttonConnect_Click(object sender, EventArgs e)
        {
            if (!serverSelector.ServerParameters.IsValidServerUrl())
            {
                MessageBox.Show("Enter a valid url for the server.");
                return;
            }

            ProgressDialog progressDialog = new ProgressDialog();
            progressDialog.Show(this);
            try
            {
                _dictionaries = await _synchronizer.ConnectAndGetDictionariesAsync(
                    serverSelector.ServerParameters,
                    progressDialog.ProgressPercent, progressDialog.ProgressMessage,
                    progressDialog.CancellationToken, this);
                if (_dictionaries != null)
                {
                    // TODO saveLastServerSettings();

                    // Only CSWeb provides case counts, for others just show labels
                    if (serverSelector.ServerParameters.Type == SyncServerType.CSWeb)
                        comboBoxDictionaries.DataSource = _dictionaries.Select(d => String.Format("{0} ({1} case{2})", d.Label, d.CaseCount, d.CaseCount == 1 ? "" : "s")).ToList();
                    else
                        comboBoxDictionaries.DataSource = _dictionaries.Select(d => d.Label).ToList();
                }
                else
                {
                    comboBoxDictionaries.DataSource = null;
                }

                updateEnabled();

                if (_dictionaries != null && _dictionaries.Length == 0)
                {
                    MessageBox.Show("There are no data files on the server. Upload a data file to the server or use a different server and try again.");
                }
            }
            finally
            {
                progressDialog.Hide();
            }
        }

        private bool IsValidFilePath()
        {
            try
            {
                if (FilePath != null && Path.IsPathRooted(FilePath) && Directory.Exists(Path.GetDirectoryName(FilePath)))
                    return (FileFilters.CsdbFileFilter.IndexOf("*" + Path.GetExtension(FilePath).ToLower()) >= 0);
            }
            catch { }

            return false;
        }

        private async void buttonDownload_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.None;

            if (!IsValidFilePath())
            {
                MessageBox.Show("Enter a valid file path. Must be a full path, the directory must exist and the extension must be csdb.");
            }
            else
            {
                ProgressDialog progressDialog = new ProgressDialog();
                progressDialog.Show(this);
                try
                {
                    var download_result = await _synchronizer.DownloadAsync(serverSelector.ServerParameters,
                        SelectedDictionary, FilePath,
                        progressDialog.ProgressPercent, progressDialog.ProgressMessage,
                        progressDialog.CancellationToken, this);
                    if (download_result)
                        DialogResult = DialogResult.OK;
                    ServerParameters = serverSelector.ServerParameters;
                } finally
                {
                    progressDialog.Hide();
                }
            }
        }

        private void updateEnabled()
        {
            buttonDownload.Enabled = _dictionaries != null && _dictionaries.Length > 0 && FilePath != null && FilePath.Length > 0;
        }

        private void textBoxSaveAsFilename_TextChanged(object sender, EventArgs e)
        {
            FilePath = textBoxSaveAsFilename.Text;
            updateEnabled();
        }

        private void DownloadDialog_FormClosed(object sender, FormClosedEventArgs e)
        {
            _synchronizer.Disconnect();
        }
    }
}
