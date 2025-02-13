using System;
using System.IO;
using System.Windows.Forms;

namespace Operator_Statistics_Viewer
{
    public partial class SpecifyOperatorNamesFile : Form
    {
        public bool UseNamesFile { get { return checkBoxUseNamesFile.Checked; } }
        public string NamesFile { get { return textBoxFilename.Text; } }

        public SpecifyOperatorNamesFile(string filename)
        {
            InitializeComponent();

            checkBoxUseNamesFile.Checked = filename != null;
            checkBoxUseNamesFile_CheckedChanged(null,null);

            if( checkBoxUseNamesFile.Checked )
                textBoxFilename.Text = filename;
        }

        private void checkBoxUseNamesFile_CheckedChanged(object sender,EventArgs e)
        {
            textBoxFilename.Enabled = checkBoxUseNamesFile.Checked;
            buttonBrowse.Enabled = checkBoxUseNamesFile.Checked;

            if( checkBoxUseNamesFile.Checked )
                textBoxFilename_TextChanged(null,null);

            else
                buttonOK.Enabled = true;
        }

        private void buttonBrowse_Click(object sender,EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Filter = "Name Files (*.csv)|*.csv|All Files (*.*)|*.*";

            if( ofd.ShowDialog() == DialogResult.OK )
                textBoxFilename.Text = ofd.FileName;
        }

        private void textBoxFilename_TextChanged(object sender,EventArgs e)
        {
            buttonOK.Enabled = File.Exists(textBoxFilename.Text);
        }
    }
}
