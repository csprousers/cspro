using System;
using System.Drawing;
using System.IO;
using System.Windows.Forms;
using CSPro.Dictionary;

namespace Excel2CSPro
{
    partial class Excel2CSProItemControl : UserControl
    {
        private Excel2CSProManager _excel2CSProManager;

        private string _savedLabelTextExcelFile;
        private string _savedLabelTextCSProDictionary;
        private string _savedLabelTextDataFile;

        public Excel2CSProItemControl()
        {
            InitializeComponent();

            _savedLabelTextExcelFile = labelExcelFile.Text;
            _savedLabelTextCSProDictionary = labelCSProDictionary.Text;
            _savedLabelTextDataFile = labelDataFile.Text;
        }

        private void SetFileDialogPath(FileDialog fd,string filename)
        {
            try
            {
                DirectoryInfo di = new DirectoryInfo(Path.GetDirectoryName(filename));

                if( di.Exists )
                {
                    fd.InitialDirectory = di.FullName;
                    fd.FileName = Path.GetFileName(filename);
                }
            }
            catch { }
        }


        private void buttonSelectExcelFile_Click(object sender,EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Title = Messages.ExcelOpenTitle;
            ofd.Filter = Messages.ExcelOpenFilter;
            SetFileDialogPath(ofd, _excel2CSProManager._spec.ExcelFilename);

            if( ofd.ShowDialog() != DialogResult.OK )
                return;

            try
            {
                _excel2CSProManager.OpenExcel(ofd.FileName);
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }

            RefreshMappings(true);
        }

        private void buttonSelectDictionary_Click(object sender,EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Title = Messages.DictionaryOpenTitle;
            ofd.Filter = Messages.DictionaryFileFilter;
            SetFileDialogPath(ofd,_excel2CSProManager._spec.DictionaryFilename);

            if( ofd.ShowDialog() != DialogResult.OK )
                return;

            try
            {
                _excel2CSProManager.OpenDictionary(ofd.FileName);
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }

            RefreshMappings(true);
        }

        private void buttonSelectOutputFile_Click(object sender,EventArgs e)
        {
            SaveFileDialog sfd = new SaveFileDialog();
            sfd.Title = Messages.DataSaveTitle;
            sfd.Filter = Messages.DataFileFilter;

            if( _excel2CSProManager._spec.OutputConnectionString != null )
                SetFileDialogPath(sfd, _excel2CSProManager._spec.OutputConnectionString.Filename);

            if( sfd.ShowDialog() != DialogResult.OK )
                return;

            if( _excel2CSProManager._spec.OutputConnectionString == null ||
                !_excel2CSProManager._spec.OutputConnectionString.FilenameMatches(sfd.FileName) )
            {
                _excel2CSProManager._spec.OutputConnectionString = new CSPro.Util.ConnectionString(sfd.FileName);
            }

            RefreshMappings(false);
        }

        public void UpdateOptions(bool saveAndValidate)
        {
            if( saveAndValidate )
            {
                int startingRow;

                if( !Int32.TryParse(textBoxStartingRow.Text, out startingRow) || ( startingRow < 1 ) )
                    throw new Exception(Messages.PrerunCheckInvalidStartingRow);

                _excel2CSProManager._spec.StartingRow = startingRow;

                _excel2CSProManager._spec.CaseManagement = (CSPro.Data.Excel2CSPro.CaseManagement)comboBoxCaseManagement.SelectedIndex;

                _excel2CSProManager._spec.RunOnlyIfNewer = checkBoxRunOnlyIfNewer.Checked;
            }

            else
            {
                textBoxStartingRow.Text = _excel2CSProManager._spec.StartingRow.ToString();

                comboBoxCaseManagement.SelectedIndex = (int)_excel2CSProManager._spec.CaseManagement;

                checkBoxRunOnlyIfNewer.Checked = _excel2CSProManager._spec.RunOnlyIfNewer;
            }
        }

        private void buttonCreateDataFile_Click(object sender,EventArgs e)
        {
            try
            {
                UpdateOptions(true);

                var counts = _excel2CSProManager.RunConversion(FormStartPosition.CenterParent);

                if( counts != null )
                {
                    string messageFormatter = ( _excel2CSProManager._spec.CaseManagement == CSPro.Data.Excel2CSPro.CaseManagement.CreateNewFile ) ?
                        Messages.ConversionSuccess : Messages.ConversionSuccessDetailed;

                    MessageBox.Show(String.Format(messageFormatter,counts.Converted,counts.Added,counts.Modified,
                        counts.Skipped,counts.Deleted));
                }
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }
        }

        public void UpdateManagerAndMappings(Excel2CSProManager excel2CSProManager)
        {
            _excel2CSProManager = excel2CSProManager;
            UpdateOptions(false);
            RefreshMappings(true);
        }

        private void RefreshMappings(bool updateMappings)
        {
            labelExcelFile.Text = ( _excel2CSProManager._spec.ExcelFilename == null ) ? _savedLabelTextExcelFile : String.Format(Messages.ExcelLabelFilter,_excel2CSProManager._spec.ExcelFilename);
            labelCSProDictionary.Text = ( _excel2CSProManager._spec.DictionaryFilename == null ) ? _savedLabelTextCSProDictionary : String.Format(Messages.DictionaryLabelFilter,_excel2CSProManager._dictionary.Name,_excel2CSProManager._spec.DictionaryFilename);
            labelDataFile.Text = ( _excel2CSProManager._spec.OutputConnectionString == null ) ? _savedLabelTextDataFile : String.Format(Messages.DataLabelFilter,_excel2CSProManager._spec.OutputConnectionString.Filename);

            if( updateMappings )
            {
                bool canCreateMapping = ( _excel2CSProManager._mappingManager != null );
                groupBoxRecordToWorksheetMapping.Enabled = canCreateMapping;
                groupBoxItemToColumnMapping.Enabled = canCreateMapping;

                if( canCreateMapping )
                {
                    DrawRecordMappings();
                    DrawItemMappings();
                }

                else
                {
                    panelRecordToWorksheetMapping.Controls.Clear();
                    panelItemToColumnMapping.Controls.Clear();
                }
            }
        }

        private void DrawRecordMappings()
        {
            panelRecordToWorksheetMapping.SuspendLayout();
            panelRecordToWorksheetMapping.Controls.Clear();

            int yPos = 0;

            foreach( DictionaryRecord record in _excel2CSProManager._mappingManager.Records )
            {
                Excel2CSProMappingControl mappingControl = new Excel2CSProMappingControl(this,_excel2CSProManager._mappingManager,record);

                mappingControl.Location = new Point(mappingControl.Margin.Left,mappingControl.Margin.Top + yPos);

                panelRecordToWorksheetMapping.Controls.Add(mappingControl);

                yPos += mappingControl.Height;
            }

            panelRecordToWorksheetMapping.ResumeLayout();
        }

        public void DrawItemMappings()
        {
            panelItemToColumnMapping.SuspendLayout();
            panelItemToColumnMapping.Controls.Clear();

            int yPos = 0;

            foreach( DictionaryRecord record in _excel2CSProManager._mappingManager.MappedRecords )
            {
                // add a label with the record name
                const int LabelMargin = 3;
                const int ItemMargin = 25;
                const int SpacingBetweenRecords = 20;

                Label label = new Label();
                label.Text = record.Name;
                label.Font = new System.Drawing.Font("Microsoft Sans Serif",8.25F,System.Drawing.FontStyle.Bold);
                label.Location = new Point(LabelMargin,LabelMargin + yPos);

                panelItemToColumnMapping.Controls.Add(label);

                yPos += label.Height;
                label.AutoSize = true; // to show the whole record name

                // add the items
                foreach( ExpandedDictionaryItem item in _excel2CSProManager._mappingManager.GetRecordItems(record) )
                {
                    Excel2CSProMappingControl mappingControl = new Excel2CSProMappingControl(this,_excel2CSProManager._mappingManager,record,item);

                    mappingControl.Location = new Point(mappingControl.Margin.Left + ItemMargin,mappingControl.Margin.Top + yPos);

                    panelItemToColumnMapping.Controls.Add(mappingControl);

                    yPos += mappingControl.Height;
                }

                yPos += SpacingBetweenRecords;
            }

            panelItemToColumnMapping.ResumeLayout();
        }
    }
}
