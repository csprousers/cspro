using System;
using System.IO;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace ParadataViewer
{
    abstract class TableForm : QueryForm
    {
        protected DataGridView _dgvResults;

        internal TableForm(Controller controller) : base(controller)
        {
            _dgvResults = new DataGridView();
            _dgvResults.AllowUserToAddRows = false;
            _dgvResults.AllowUserToDeleteRows = false;
            _dgvResults.AllowUserToOrderColumns = true;
            _dgvResults.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.AllCells;
            _dgvResults.AutoSizeRowsMode = System.Windows.Forms.DataGridViewAutoSizeRowsMode.AllCells;
            _dgvResults.ColumnHeadersDefaultCellStyle.Alignment = DataGridViewContentAlignment.BottomLeft;
            _dgvResults.ColumnHeadersHeightSizeMode = DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            _dgvResults.Dock = DockStyle.Fill;
            _dgvResults.ReadOnly = true;
            _dgvResults.RowHeadersVisible = false;
            _dgvResults.SelectionMode = DataGridViewSelectionMode.FullRowSelect;
            _dgvResults.ShowCellErrors = false;
            _dgvResults.ShowCellToolTips = false;
            _dgvResults.ShowEditingIcon = false;
            _dgvResults.ShowRowErrors = false;

            panelResults.Controls.Add(_dgvResults);
        }

        protected void SetupTableColumns(string[] columnNames,string[] columnFormats = null)
        {
            _dgvResults.SuspendLayout();

            _dgvResults.Columns.Clear();

            if( columnNames != null )
            {
                for( int i = 0; i < columnNames.Length; i++ )
                {
                    var column = new DataGridViewTextBoxColumn();

                    column.HeaderText = columnNames[i];

                    if( columnFormats != null && columnFormats[i] != null )
                        column.DefaultCellStyle.Format = columnFormats[i];

                    _dgvResults.Columns.Add(column);
                }
            }

            _dgvResults.ResumeLayout();
        }

        protected override async Task RefreshQueryAsync()
        {
            _dgvResults.Rows.Clear();
            await base.RefreshQueryAsync();
        }

        protected override void HandleResults(int startingRow)
        {
            _dgvResults.SuspendLayout();

            for( int i = startingRow; i < _rows.Count; i++ )
                _dgvResults.Rows.Add(_rows[i]);

            _dgvResults.ResumeLayout();
        }

        protected override string SaveLabelText { get { return "Save results to Excel"; } }

        protected override async void linkLabelSave_LinkClicked(object sender,LinkLabelLinkClickedEventArgs e)
        {
            await SaveResultsToExcelPromptAsync(_dbQuery.ColumnNames, null);
        }

        protected async Task SaveResultsToExcelPromptAsync(string[] columnLabels, string[] columnFormats)
        {
            try
            {
                if( ( _rows == null ) || ( _rows.Count == 0 ) )
                    throw new Exception("There are no rows to save");

                var sfd = new SaveFileDialog();
                sfd.Title = "Save As";
                sfd.Filter = "Excel Worksheets (*.xlsx;*.xlsm;*.xlsb;*.xls)|*.xlsx;*.xlsm;*.xlsb;*.xls";

                if( sfd.ShowDialog() == DialogResult.OK )
                {
                    if( File.Exists(sfd.FileName) )
                        File.Delete(sfd.FileName);

                    _controller.UpdateStatusBarText("Saving to Excel...");

                    await Task.Run(() => SaveResultsToExcel(sfd.FileName, columnLabels, columnFormats));

                    _controller.UpdateStatusBarText(Path.GetFileName(sfd.FileName) + " saved");
                }
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }
        }

        private void SaveResultsToExcel(string filename, string[] columnLabels, string[] columnFormats)
        {
            Microsoft.Office.Interop.Excel.Workbook workbook = null;

            try
            {
                int numberColumns = _dbQuery.ColumnCount;
                object[,] rowsArray = null;

                lock( _rows )
                {
                    rowsArray = new object[_rows.Count,numberColumns];

                    for( int i = 0 ; i < _rows.Count; i++ )
                    {
                        var row = _rows[i];

                        for( int j = 0; j < numberColumns; j++ )
                            rowsArray[i,j] = row[j];
                    }
                }

                var excel = _controller.GetExcelApplication();
                workbook = excel.Workbooks.Add(Type.Missing);
                var worksheet = workbook.ActiveSheet;

                worksheet.Name = this.Text;

                // add the header
                for( int i = 0; i < numberColumns; ++i )
                {
                    worksheet.Cells[1, i + 1] = columnLabels[i];

                    string excelFormat = ( columnFormats != null ) ? ColumnFormatToExcelFormat(columnFormats[i]) : null;

                    if( excelFormat != null )
                        worksheet.Cells[1, i + 1].EntireColumn.NumberFormat = excelFormat;
                }

                worksheet.Cells[1,1].EntireRow.Font.Bold = true;

                // add the rows
                var topLeftCell = (Microsoft.Office.Interop.Excel.Range)worksheet.Cells[2,1];
                var bottomRightCell = (Microsoft.Office.Interop.Excel.Range)worksheet.Cells[1 + _rows.Count,numberColumns];
                var range = (Microsoft.Office.Interop.Excel.Range)worksheet.Range[topLeftCell,bottomRightCell];
                range.Value = rowsArray;
                range.EntireColumn.AutoFit();

                workbook.SaveAs(filename);
            }

            finally
            {
                if( workbook != null )
                {
                    workbook.Saved = true;
                    workbook.Close();
                }
            }
        }

        private string ColumnFormatToExcelFormat(string columnFormat)
        {
            if( !String.IsNullOrEmpty(columnFormat) )
            {
                if( columnFormat == "P" )
                {
                    return "0.00%";
                }

                else if( columnFormat[0] == 'F' )
                {
                    try
                    {
                        int decimals = Int32.Parse(columnFormat.Substring(1));
                        return ( decimals == 0 ) ? "0" : $"0.{new string('0', decimals)}";
                    }

                    catch( Exception )
                    {
                        // ignore parsing errors
                    }
                }
            }

            return null;
        }
    }
}
