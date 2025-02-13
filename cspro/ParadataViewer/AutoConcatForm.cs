using System;
using System.Windows.Forms;

namespace ParadataViewer
{
    partial class AutoConcatForm : Form
    {
        private string _outputLogFilename;

        internal bool IncludeCurrentLog { get { return checkBoxIncludeCurrentLog.Checked; } }
        internal bool UseTemporaryLog { get { return ( _outputLogFilename == null ); } }
        internal string OutputLogFilename { get { return _outputLogFilename; } }

        public AutoConcatForm(int numberLogs,bool logIsOpen)
        {
            InitializeComponent();

            labelText.Text = String.Format(labelText.Text,numberLogs);

            if( logIsOpen )
                checkBoxIncludeCurrentLog.Checked = true;

            else
                checkBoxIncludeCurrentLog.Enabled = false;
        }

        private void buttonConcatenateSaveResult_Click(object sender,EventArgs e)
        {
            SaveFileDialog sfd = new SaveFileDialog();
            sfd.Title = "Select Output Paradata Log";
            sfd.Filter = String.Format("Paradata Log ({0})|{0}",Controller.LogFilenameFilter);

            var dialogResult  = sfd.ShowDialog();

            if( dialogResult == DialogResult.OK )
                _outputLogFilename = sfd.FileName;

            this.DialogResult = dialogResult;
            Close();
        }
    }
}
