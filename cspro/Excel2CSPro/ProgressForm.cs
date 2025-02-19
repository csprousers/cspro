using System;
using System.ComponentModel;
using WinFormsShared;
using System.Windows.Forms;

namespace Excel2CSPro
{
    public interface ProgressFormWorker
    {
        void RunTask(BackgroundWorker backgroundWorker, CSPro.Data.Excel2CSPro.OnQueryUsernamePassword onQueryUsernamePassword);
    }

    partial class ProgressForm : Form
    {
        private BackgroundWorker _backgroundWorker;
        private bool _workerReportedError;
        private string _savedLabelCasesText;

        public ProgressForm(FormStartPosition formStartPosition,string dialogTitle,bool showWritingProgressBar,ProgressFormWorker task)
        {
            InitializeComponent();

            this.StartPosition = formStartPosition;
            this.Text = dialogTitle;

            _workerReportedError = false;
            _savedLabelCasesText = labelCases.Text;

            if( !showWritingProgressBar )
            {
                this.Height -= panelWriting.Height;
                panelWriting.Visible = false;
            }

            _backgroundWorker = new BackgroundWorker();
            _backgroundWorker.WorkerReportsProgress = true;
            _backgroundWorker.WorkerSupportsCancellation = true;

            _backgroundWorker.DoWork += new DoWorkEventHandler(
                delegate(object o,DoWorkEventArgs args)
                {
                    CSPro.Data.Excel2CSPro.OnQueryUsernamePassword onQueryUsernamePassword = (bool showError) =>
                    {
                        return (CSPro.Data.Excel2CSPro.UsernamePassword)this.Invoke(new Func<CSPro.Data.Excel2CSPro.UsernamePassword>(() =>
                        {
                            var loginDlg = new LoginDialog();
                            loginDlg.ShowError = showError;

                            if( loginDlg.ShowDialog(this) != DialogResult.OK )
                                return null;

                            return new CSPro.Data.Excel2CSPro.UsernamePassword { username = loginDlg.Username, password = loginDlg.Password };
                        }));
                    };

                    task.RunTask(_backgroundWorker, onQueryUsernamePassword);
                });

            _backgroundWorker.ProgressChanged += new ProgressChangedEventHandler(
                delegate(object o,ProgressChangedEventArgs args)
                {
                    if( args.UserState == null )
                    {
                        progressBarReading.Value = Math.Min(args.ProgressPercentage,100);
                    }

                    else if( args.UserState is CSPro.Data.Excel2CSPro.ConversionCounts )
                    {
                        var counts = (CSPro.Data.Excel2CSPro.ConversionCounts)args.UserState;
                        progressBarWriting.Value = Math.Min(counts.WritingProgressPercentage, 100);
                        labelCases.Text = String.Format(_savedLabelCasesText, counts.Converted);
                        labelCases.Visible = true;
                    }

                    else
                    {
                        _workerReportedError = true;
                        new ErrorDisplayForm((string)args.UserState).ShowDialog();
                    }
                });

            _backgroundWorker.RunWorkerCompleted += new RunWorkerCompletedEventHandler(
                delegate(object o,RunWorkerCompletedEventArgs args)
                {
                    this.DialogResult = ( args.Cancelled || _workerReportedError ) ? DialogResult.Cancel : DialogResult.OK;
                    Close();
                });
        }

        private void ProgressForm_Load(object sender,EventArgs e)
        {
            _backgroundWorker.RunWorkerAsync();
        }

        private bool ConfirmExit()
        {
            if( MessageBox.Show(Messages.ConfirmCancelTask,this.Text,MessageBoxButtons.YesNo) == DialogResult.Yes )
            {
                _backgroundWorker.CancelAsync();
                this.DialogResult = DialogResult.Cancel;
                return true;
            }

            else
                return false;
        }

        private void ProgressForm_FormClosing(object sender,FormClosingEventArgs e)
        {
            if( _backgroundWorker.IsBusy && !_backgroundWorker.CancellationPending )
            {
                if( !ConfirmExit() )
                    e.Cancel = true;
            }
        }

        private void buttonCancel_Click(object sender,EventArgs e)
        {
            if( ConfirmExit() )
                Close();
        }
    }
}
