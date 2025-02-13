using System;
using System.IO;
using System.Windows.Forms;

namespace Operator_Statistics_Viewer
{
    public partial class BulkAddForm : Form
    {
        public bool Subfolders { get { return checkBoxSubfolders.Checked; } }
        public string FolderName { get { return textBoxFolderName.Text; } }

        public BulkAddForm()
        {
            InitializeComponent();
        }

        private void textBoxFolderName_TextChanged(object sender,EventArgs e)
        {
            buttonOK.Enabled = Directory.Exists(textBoxFolderName.Text);
        }

        private void buttonBrowse_Click(object sender,EventArgs e)
        {
            FolderBrowserDialog fbd = new FolderBrowserDialog();

            if( fbd.ShowDialog() == DialogResult.OK )
                textBoxFolderName.Text = fbd.SelectedPath;
        }
    }
}
