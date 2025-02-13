using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.Windows.Forms;
using Excel = Microsoft.Office.Interop.Excel;

namespace Excel2CSPro
{
    partial class CreateDictionaryControl : UserControl, ProgressFormWorker
    {
        private WorkbookController _workbookController;
        private Excel.Worksheet _worksheet;
        private List<ColumnAnalysis> _columnAnalysis;
        private List<CreateDictionaryItemControl> _itemSettings;
        private CSPro.Dictionary.SimpleDictionaryCreator _simpleDictionaryCreator;
        private string _lastSaveFilename;

        // information needed for ConvertDataPostDictionaryCreation
        private int _startingRow;
        private int _worksheetIndex;
        private string _recordName;

        public CreateDictionaryControl()
        {
            InitializeComponent();

            _workbookController = new WorkbookController();

            textBoxStartingRow.Text = "2";
        }

        private void buttonSelectExcelFile_Click(object sender,EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Title = Messages.ExcelOpenTitle;
            ofd.Filter = Messages.ExcelOpenFilter;

            if( ofd.ShowDialog() != DialogResult.OK )
                return;

            try
            {
                _workbookController.OpenWorkbook(ofd.FileName);

                comboBoxWorksheet.Items.Clear();

                foreach( string worksheetName in _workbookController.WorksheetNames )
                    comboBoxWorksheet.Items.Add(worksheetName);

                comboBoxWorksheet.SelectedIndex = 0;

                groupBoxExcelOptions.Text = String.Format(Messages.ExcelGroupTitle,ofd.FileName);
                buttonAnalyzeWorksheet.Enabled = true;
            }

            catch( Exception exception )
            {
                MessageBox.Show(String.Format(Messages.ExcelIOError,exception.Message));
            }
        }

        public class ColumnAnalysis
        {
            public int ColumnNumber = 0;
            public string ColumnAddress = "";
            public string HeaderText = "";
            public bool ContainsAlpha = false;
            public int DigitsBeforeDecimal = 0;
            public int DigitsAfterDecimal = 0;
            public int MaximumLength = 0;
            public int ValidValues = 0;

            public const int MaxValueSetValues = 500;
            public SortedSet<double> Values = new SortedSet<double>();
        }

        private void buttonAnalyzeWorksheet_Click(object sender,EventArgs e)
        {
            try
            {
                _worksheetIndex = comboBoxWorksheet.SelectedIndex;
                _worksheet = _workbookController.GetWorksheet(_worksheetIndex);

                List<ColumnAnalysis> previousColumnAnalysis = _columnAnalysis;

                ProgressForm progressDlg = new ProgressForm(FormStartPosition.CenterParent,String.Format(Messages.TitleReadingWorksheet,comboBoxWorksheet.SelectedItem.ToString()),false,this);

                if( progressDlg.ShowDialog() != DialogResult.OK || previousColumnAnalysis == _columnAnalysis )
                    return;

                // draw the data about the read items
                _itemSettings = new List<CreateDictionaryItemControl>();

                panelDictionaryContents.SuspendLayout();
                panelDictionaryContents.Controls.Clear();

                int yPos = 0;

                foreach( ColumnAnalysis ca in _columnAnalysis )
                {
                    CreateDictionaryItemControl cdic = new CreateDictionaryItemControl(ca);
                    cdic.Location = new Point(cdic.Margin.Left,cdic.Margin.Top + yPos);

                    panelDictionaryContents.Controls.Add(cdic);
                    _itemSettings.Add(cdic);

                    yPos += cdic.Height;
                }

                panelDictionaryContents.ResumeLayout();

                groupBoxDictionaryContents.Enabled = true;
                groupBoxCreateDictionary.Enabled = true;
                textBoxNamePrefix.Text = CSPro.Logic.Names.MakeName(comboBoxWorksheet.SelectedItem.ToString());
                buttonCreateDictionary.Enabled = true;
                _lastSaveFilename = null;
            }

            catch( Exception exception )
            {
                MessageBox.Show(String.Format(Messages.ExcelIOError,exception.Message));
            }
        }

        public void RunTask(BackgroundWorker backgroundWorker)
        {
            try
            {
                _startingRow = _workbookController.ParseStartingRow(textBoxStartingRow.Text);

                int numRows = _worksheet.UsedRange.Rows.Count;
                int numCols = _worksheet.UsedRange.Columns.Count;
                int numDataRows = _worksheet.UsedRange.Rows.Count - _startingRow + 1;

                double progress = 0;
                double progressRowIncrementer = 100.0 / numDataRows / numCols;

                List<ColumnAnalysis> columnsAnalysis = new List<ColumnAnalysis>();

                // read the Excel file
                for( int col = 1; !backgroundWorker.CancellationPending && col <= numCols; col++ )
                {
                    ColumnAnalysis ca = new ColumnAnalysis
                    {
                        ColumnNumber = col
                    };

                    // the column address will appear in a format like $A:$A
                    ca.ColumnAddress = (string)_worksheet.Columns[col].Address;
                    ca.ColumnAddress = ca.ColumnAddress.Substring(1,ca.ColumnAddress.IndexOf(':') - 1);

                    // read the first row, which may be header text
                    object header = _worksheet.Cells[1,col].Value2;

                    if( header != null )
                        ca.HeaderText = header.ToString().Trim();

                    else
                        ca.HeaderText = String.Format("Column {0}",ca.ColumnAddress);

                    Excel.Range dataRange = _worksheet.Range[_worksheet.Cells[_startingRow,col],_worksheet.Cells[numRows,col]];
                    object[,] data = null;

                    // special processing for worksheets with only one row of data
                    if( numDataRows == 1 )
                    {
                        data = new object[2, 2];
                        data[1, 1] = dataRange.Value2;
                    }

                    else
                        data = dataRange.Value2;

                    if( data == null )
                        throw new Exception(Messages.ExcelNoData);

                    for( int row = 1; !backgroundWorker.CancellationPending && row <= numDataRows; row++ )
                    {
                        object cell = data[row,1];

                        if( cell != null )
                        {
                            ca.ValidValues++;

                            bool isValidNumeric = false;
                            bool isInvalidNumeric = false;
                            double doubleValue = 0;

                            // see if this value is a valid numeric
                            if( !ca.ContainsAlpha )
                            {
                                if( cell is double )
                                {
                                    doubleValue = (double)cell;
                                    isValidNumeric = true;
                                }

                                // invalid values like #VALUE! or #N/A appear as integers
                                else if( cell is int )
                                    isInvalidNumeric = true;

                                else if( cell is string )
                                    isValidNumeric = Double.TryParse((string)cell, out doubleValue);
                            }

                            if( isValidNumeric )
                            {
                                if( ca.Values.Count < ColumnAnalysis.MaxValueSetValues && !ca.Values.Contains(doubleValue) )
                                    ca.Values.Add(doubleValue);

                                // assume a maximum of six digits after the decimal point
                                string formattedDouble = doubleValue.ToString("0.000000",CultureInfo.InvariantCulture);
                                formattedDouble = formattedDouble.TrimEnd('0');

                                int digitsBeforeDecimal = formattedDouble.IndexOf('.');
                                int digitsAfterDecimal = formattedDouble.Length - digitsBeforeDecimal - 1;

                                ca.DigitsBeforeDecimal = Math.Max(ca.DigitsBeforeDecimal,digitsBeforeDecimal);
                                ca.DigitsAfterDecimal = Math.Max(ca.DigitsAfterDecimal,digitsAfterDecimal);

                                ca.MaximumLength = Math.Max(ca.MaximumLength,digitsBeforeDecimal + digitsAfterDecimal + ( ( digitsAfterDecimal > 0 ) ? 1 : 0 ));
                            }

                            else if( isInvalidNumeric )
                            {
                                // make sure a column with nothing but invalid numerics has a length
                                ca.DigitsBeforeDecimal = Math.Max(ca.DigitsBeforeDecimal, 1);
                                ca.MaximumLength = Math.Max(ca.MaximumLength, 1);
                            }

                            else
                            {
                                string sValue = (cell.ToString()).Trim();
                                ca.ContainsAlpha = true;
                                ca.MaximumLength = Math.Max(ca.MaximumLength,sValue.Length);
                            }
                        }

                        progress += progressRowIncrementer;
                        backgroundWorker.ReportProgress((int)progress);
                    }

                    if( ca.ValidValues != 0 )
                        columnsAnalysis.Add(ca);
                }

                if( !backgroundWorker.CancellationPending )
                    _columnAnalysis = columnsAnalysis;
            }

            catch( Exception exception )
            {
                MessageBox.Show(String.Format(Messages.ExcelIOError,exception.Message));
            }
        }


        private void buttonCreateDictionary_Click(object sender,EventArgs e)
        {
            string namePrefix = textBoxNamePrefix.Text.ToUpper();

            if( !CSPro.Logic.Names.IsValid(namePrefix) )
            {
                string suggestedNamePrefix = CSPro.Logic.Names.MakeName(namePrefix);

                if( MessageBox.Show(String.Format(Messages.InvalidNameSuggestOther,namePrefix,suggestedNamePrefix),this.Text,MessageBoxButtons.YesNo) == DialogResult.No )
                    return;

                namePrefix = suggestedNamePrefix;
            }

            HashSet<string> usedNames = new HashSet<string>();

            usedNames.Add(CSPro.Logic.Names.MakeName(namePrefix + "_DICT"));
            usedNames.Add(CSPro.Logic.Names.MakeName(namePrefix + "_LEVEL"));

            _recordName = CSPro.Logic.Names.MakeName(namePrefix + "_REC");
            usedNames.Add(_recordName);

            List<CreateDictionaryItemControl.ItemSelections> ids = new List<CreateDictionaryItemControl.ItemSelections>();
            List<CreateDictionaryItemControl.ItemSelections> items = new List<CreateDictionaryItemControl.ItemSelections>();

            CreateDictionaryItemControl cdicForMessages = null;

            try
            {
                foreach( CreateDictionaryItemControl cdic in _itemSettings )
                {
                    cdicForMessages = cdic;

                    CreateDictionaryItemControl.ItemSelections selections = cdic.Selections;

                    if( !selections.IncludeItem )
                        continue;

                    if( !CSPro.Logic.Names.IsValid(selections.Name) )
                        throw new Exception(String.Format(Messages.InvalidName,selections.Name));

                    if( usedNames.Contains(selections.Name) )
                        throw new Exception(String.Format(Messages.AlreadyUsedName,selections.Name));

                    usedNames.Add(selections.Name);

                    int length = 0;

                    if( selections.IsNumeric )
                    {
                        length = selections.BeforeDecLength;

                        if( selections.AfterDecLength > 0 )
                        {
                            if( selections.IsId )
                                throw new Exception(Messages.InvalidIdNumericDecimal);

                            if( selections.AfterDecLength > CSPro.Dictionary.Rules.MaxLengthDecimal )
                                throw new Exception(String.Format(Messages.InvalidLengthNumericDecimal,CSPro.Dictionary.Rules.MaxLengthDecimal));

                            length += selections.AfterDecLength + ( checkBoxDecChar.Checked ? 1 : 0 );
                        }

                        if( length > CSPro.Dictionary.Rules.MaxLengthNumeric )
                            throw new Exception(String.Format(Messages.InvalidLengthNumeric,CSPro.Dictionary.Rules.MaxLengthNumeric));
                    }

                    else
                    {
                        length = selections.AlphaLength;

                        if( length > CSPro.Dictionary.Rules.MaxLengthAlpha )
                            throw new Exception(String.Format(Messages.InvalidLengthAlpha,CSPro.Dictionary.Rules.MaxLengthAlpha));
                    }

                    if( length == 0 )
                        throw new Exception(Messages.InvalidLengthZero);

                    if( selections.IsId )
                        ids.Add(selections);

                    else
                        items.Add(selections);
                }

                cdicForMessages = null;

                if( ids.Count == 0 )
                    throw new Exception(Messages.InvalidNumberIds);

                // save the dictionary
                SaveFileDialog sfd = new SaveFileDialog();
                sfd.Title = Messages.DictionarySaveTitle;
                sfd.Filter = Messages.DictionaryFileFilter;

                if( _lastSaveFilename != null )
                {
                    sfd.InitialDirectory = Path.GetDirectoryName(_lastSaveFilename);
                    sfd.FileName = Path.GetFileName(_lastSaveFilename);
                }

                if( sfd.ShowDialog() != DialogResult.OK )
                    return;

                _simpleDictionaryCreator = new CSPro.Dictionary.SimpleDictionaryCreator(namePrefix,checkBoxDecChar.Checked,checkBoxZeroFill.Checked);
                AddItems(true,ids);
                AddItems(false,items);

                _simpleDictionaryCreator.Save(sfd.FileName);
                _lastSaveFilename = sfd.FileName;

                // see if the user wants to now convert data using the new dictionary
                if( MessageBox.Show(Messages.ConvertDataPostDictionaryCreation, this.Text, MessageBoxButtons.YesNo) == DialogResult.Yes )
                    ConvertDataPostDictionaryCreation();
            }

            catch( Exception exception )
            {
                if( cdicForMessages != null )
                    MessageBox.Show(String.Format("{0}: {1}",cdicForMessages.ItemName,exception.Message));

                else
                    MessageBox.Show(exception.Message);
            }
        }

        private void AddItems(bool areIdItems,List<CreateDictionaryItemControl.ItemSelections> items)
        {
            foreach( CreateDictionaryItemControl.ItemSelections selections in items )
            {
                int length = selections.IsNumeric ? selections.BeforeDecLength : selections.AlphaLength;

                if( selections.IsNumeric && selections.AfterDecLength > 0 )
                    length += selections.AfterDecLength + ( checkBoxDecChar.Checked ? 1 : 0 );

                _simpleDictionaryCreator.AddItem(areIdItems,selections.Name,selections.IsNumeric,length,
                    selections.IsNumeric ? selections.AfterDecLength : 0,selections.Values);
            }
        }

        private void ConvertDataPostDictionaryCreation()
        {
            // create a spec with the settings used when creating the dictionary
            var excel2CSProManager = new Excel2CSProManager();

            excel2CSProManager._spec.ExcelFilename = _workbookController.WorkbookFilename;
            excel2CSProManager._spec.DictionaryFilename = _lastSaveFilename;
            excel2CSProManager._spec.StartingRow = _startingRow;

            var record_mapping = new CSPro.Data.Excel2CSPro.RecordMapping()
            {
                RecordName = _recordName,
                WorksheetIndex = _worksheetIndex + 1
            };

            for( int i = 0; i < _itemSettings.Count; i++ )
            {
                CreateDictionaryItemControl.ItemSelections selections = _itemSettings[i].Selections;

                if( selections.IncludeItem )
                {
                    record_mapping.ItemMappings.Add(new CSPro.Data.Excel2CSPro.ItemMapping()
                    {
                        ItemName = selections.Name,
                        ColumnIndex = _columnAnalysis[i].ColumnNumber
                    });
                }
            }

            excel2CSProManager._spec.Mappings.Add(record_mapping);            

            ((MainForm)this.TopLevelControl).LoadSpecFileFromCreateDictionaryControl(excel2CSProManager);
        }
    }
}
