using System;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace WinFormsShared
{
    public partial class DropboxAuthDialog : Form
    {

        public string AccessToken
        {
            get; private set;
        }

        public string Error
        {
            get; private set;
        }

        public DropboxAuthDialog(string clientId)
        {
            InitializeComponent();
        }

        private void buttonCancel_Click(object sender, EventArgs e)
        {
            m_cancellationTokenSource.Cancel();
        }

        private async void DropboxAuthDialog_Load(object sender, EventArgs e)
        {
            try
            {
                var result = await RunDropboxAuthProcessAsync(m_cancellationTokenSource.Token);

                Activate();

                if (!result && !String.IsNullOrEmpty(Error))
                    MessageBox.Show($"Error connecting to Dropbox: {Error}");
                DialogResult = DialogResult.OK;
            } catch (TaskCanceledException) {
                DialogResult = DialogResult.Cancel;
            }
        }

        private async Task<bool> RunDropboxAuthProcessAsync(CancellationToken cancellationToken)
        {
            m_authProcessCompleted = new TaskCompletionSource<bool>();

            using (m_authProcess = new Process())
            {
                try
                {
                    m_authProcess.StartInfo.FileName = "DropboxAuth.exe";
                    m_authProcess.StartInfo.CreateNoWindow = true;
                    m_authProcess.StartInfo.UseShellExecute = false;
                    m_authProcess.StartInfo.RedirectStandardOutput = true;
                    m_authProcess.StartInfo.RedirectStandardError = true;
                    m_authProcess.EnableRaisingEvents = true;
                    m_authProcess.Exited += ProcessExited;
                    m_authProcess.Start();
                    cancellationToken.Register(() =>
                    {
                        m_authProcess.Exited -= ProcessExited;
                        m_authProcess.Kill();
                        m_authProcessCompleted.TrySetCanceled();
                    });

                } catch (Exception ex)
                {
                    Error = ex.Message;
                    return false;
                }
                return await m_authProcessCompleted.Task;
            }
        }

        private void ProcessExited(object sender, System.EventArgs e)
        {
            AccessToken = m_authProcess.StandardOutput.ReadToEnd();
            Error = m_authProcess.StandardError.ReadToEnd();
            m_authProcessCompleted.TrySetResult(m_authProcess.ExitCode == 0);
        }

        private CancellationTokenSource m_cancellationTokenSource = new CancellationTokenSource();
        private TaskCompletionSource<bool> m_authProcessCompleted;
        private Process m_authProcess;
    }
}
