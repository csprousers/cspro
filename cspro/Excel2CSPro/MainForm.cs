using System;
using System.IO;
using System.Windows.Forms;

namespace Excel2CSPro
{
    partial class MainForm : Form
    {
        private Excel2CSProManager _excel2CSProManager;
        private string _specFilename;
        private string _savedTitle;

        public MainForm(string filename)
        {
            InitializeComponent();

            _specFilename = filename;
            _savedTitle = this.Text;
        }

        private void MainForm_Shown(object sender,EventArgs e)
        {
            if( _specFilename != null )
                LoadSpecFile(null);

            else
                NewSpecFile();
        }

        private void tabControlMain_SelectedIndexChanged(object sender,EventArgs e)
        {
            bool menuOptionsEnabled = ( tabControlMain.SelectedTab == tabPageExcel2CSPro );
            menuItemNew.Enabled = menuOptionsEnabled;
            menuItemOpen.Enabled = menuOptionsEnabled;
            menuItemSave.Enabled = menuOptionsEnabled;
            menuItemSaveAs.Enabled = menuOptionsEnabled;
            ModifyTitle();
        }

        private void NewSpecFile()
        {
            _excel2CSProManager = new Excel2CSProManager();
            _specFilename = null;

            ModifyTitle();
            excel2CSProItemControl.UpdateManagerAndMappings(_excel2CSProManager);
        }

        private void menuItemNew_Click(object sender,EventArgs e)
        {
            NewSpecFile();
        }

        private void LoadSpecFile(Excel2CSProManager excel2CSProManager)
        {
            try
            {
                _excel2CSProManager = ( excel2CSProManager != null ) ? excel2CSProManager : new Excel2CSProManager();

                // the spec filename may actually be a dictionary (if passed on the command line)
                if( _specFilename != null && Path.GetExtension(_specFilename).ToLower() == CSPro.Dictionary.DataDictionary.Extension )
                {
                    string dictionary_filename = _specFilename;
                    _specFilename = null;
                    _excel2CSProManager.OpenDictionary(dictionary_filename);
                }

                else
                {
                    _excel2CSProManager.LoadSpecFile(_specFilename);
                }

                ModifyTitle();
                excel2CSProItemControl.UpdateManagerAndMappings(_excel2CSProManager);
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
                NewSpecFile();
            }
        }

        private void menuItemOpen_Click(object sender,EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Title = Messages.SpecFileOpenTitle;
            ofd.Filter = Messages.SpecFileFilter;

            if( ofd.ShowDialog() == DialogResult.OK )
            {
                _specFilename = ofd.FileName;
                LoadSpecFile(null);
            }
        }

        private void SaveSpecFile(string filename)
        {
            try
            {
                excel2CSProItemControl.UpdateOptions(true);
                _excel2CSProManager.SaveSpecFile(filename);
                _specFilename = filename;
                ModifyTitle();
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }
        }

        private void menuItemSave_Click(object sender,EventArgs e)
        {
            if( _specFilename == null )
                menuItemSaveAs_Click(sender,e);

            else
                SaveSpecFile(_specFilename);
        }

        private void menuItemSaveAs_Click(object sender,EventArgs e)
        {
            SaveFileDialog sfd = new SaveFileDialog();
            sfd.Title = Messages.SpecFileSaveTitle;
            sfd.Filter = Messages.SpecFileFilter;

            if( sfd.ShowDialog() == DialogResult.OK )
                SaveSpecFile(sfd.FileName);
        }

        private void menuItemExit_Click(object sender,EventArgs e)
        {
            this.Close();
        }

        private void menuItemHelp_Click(object sender,EventArgs e)
        {
            Help.ShowHelp(null,"Excel2CSPro.chm");
        }

        private void menuItemAbout_Click(object sender,EventArgs e)
        {
            new WinFormsShared.AboutForm("Excel to CSPro",
                "The Excel to CSPro tool is used to convert Excel worksheets to CSPro data files.",
                Properties.Resources.MainIcon).ShowDialog();
        }

        private void ModifyTitle()
        {
            if( tabControlMain.SelectedTab == tabPageCreateDictionary || _specFilename == null )
                this.Text = _savedTitle;

            else
                this.Text = String.Format("{0} - {1}",Path.GetFileNameWithoutExtension(_specFilename),_savedTitle);
        }

        internal void LoadSpecFileFromCreateDictionaryControl(Excel2CSProManager excel2CSProManager)
        {
            _specFilename = null;
            tabControlMain.SelectedTab = tabPageExcel2CSPro;
            LoadSpecFile(excel2CSProManager);
        }
    }
}
