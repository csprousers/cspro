using System;
using System.ComponentModel;
using System.Windows.Forms;

namespace DataViewer
{
    partial class SaveForm : Form
    {
        protected BackgroundWorker _backgroundWorker;

        public SaveForm(string action_type, string save_type)
        {
            InitializeComponent();

            this.Text = $"{action_type} {save_type}";
            labelFiles.Text = $"{action_type} {save_type.ToLower()}...";

            _backgroundWorker = new BackgroundWorker();
            _backgroundWorker.WorkerReportsProgress = true;
            _backgroundWorker.WorkerSupportsCancellation = true;

            _backgroundWorker.ProgressChanged += new ProgressChangedEventHandler(
                delegate(object o, ProgressChangedEventArgs args)
                {
                    if( args.ProgressPercentage == -1 )
                        labelFiles.Text = (string)args.UserState;

                    else
                    {
                        labelCases.Text = (string)args.UserState;
                        progressBar.Value = args.ProgressPercentage;
                    }
                });

            _backgroundWorker.RunWorkerCompleted += new RunWorkerCompletedEventHandler(
                delegate(object o, RunWorkerCompletedEventArgs args)
                {
                    if( !args.Cancelled && args.Result != null )
                        MessageBox.Show((string)args.Result, this.Text);

                    Close();
                });
        }

        private void SaveDataForm_Load(object sender, EventArgs e)
        {
            _backgroundWorker.RunWorkerAsync();
        }

        private void SaveDataForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            if( _backgroundWorker.IsBusy && !_backgroundWorker.CancellationPending )
                _backgroundWorker.CancelAsync();
        }

        private void buttonCancel_Click(object sender, EventArgs e)
        {
            _backgroundWorker.CancelAsync();
        }
    }
}
