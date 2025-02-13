using System;
using System.ComponentModel;
using System.Linq;
using System.Text;
using Microsoft.Office.Interop.Excel;

namespace Excel2CSPro
{
    class Excel2CSProConverter : ProgressFormWorker
    {
        private const int MaxRowsPerRead = 25000;

        private Excel2CSProManager _excel2CSProManager;
        private CSPro.Data.Excel2CSPro.Worker _worker;

        public CSPro.Data.Excel2CSPro.ConversionCounts Counts { get { return _worker.Counts; } }

        public Excel2CSProConverter(Excel2CSProManager excel2CSProManager)
        {
            _excel2CSProManager = excel2CSProManager;
            _worker = new CSPro.Data.Excel2CSPro.Worker();
        }

        public void RunTask(BackgroundWorker backgroundWorker)
        {
            try
            {
                // create the information needed for the conversion
                var record_conversion_information_list = _excel2CSProManager._mappingManager.GenerateConversionInformation();

                double cellsRead = 0;
                double totalCellsToRead = 0;

                foreach( var record_conversion_information in record_conversion_information_list )
                {
                    Worksheet worksheet = (Worksheet)record_conversion_information.ExcelWorksheet;

                    record_conversion_information.LastRow = worksheet.UsedRange.Rows.Count;
                    record_conversion_information.NextRow = Math.Min(_excel2CSProManager._spec.StartingRow, record_conversion_information.LastRow);

                    totalCellsToRead += ( record_conversion_information.LastRow - record_conversion_information.NextRow + 1 ) * record_conversion_information.Items.Count;
                }

                // initialize the worker
                _worker.Initialize(_excel2CSProManager._dictionary, record_conversion_information_list, _excel2CSProManager._spec);

                // convert the data
                while( !backgroundWorker.CancellationPending && cellsRead < totalCellsToRead )
                {
                    // read the Excel data sheet-by-sheet...
                    foreach( var record_conversion_information in record_conversion_information_list.Where(x => x.ReadMoreRows) )
                    {
                        if( backgroundWorker.CancellationPending )
                            break;

                        int rows_to_read = Math.Min(record_conversion_information.LastRow - record_conversion_information.NextRow + 1, MaxRowsPerRead);

                        if( rows_to_read <= 0 )
                            continue;

                        record_conversion_information.SetRowsRead(rows_to_read);

                        Worksheet worksheet = (Worksheet)record_conversion_information.ExcelWorksheet;                        

                        // ...column-by-column
                        foreach( var item_conversion_information in record_conversion_information.Items )
                        {
                            if( backgroundWorker.CancellationPending )
                                break;

                            Range dataRange = worksheet.Range[
                                worksheet.Cells[record_conversion_information.NextRow, item_conversion_information.ExcelColumn],
                                worksheet.Cells[record_conversion_information.NextRow + rows_to_read - 1, item_conversion_information.ExcelColumn]];

                            item_conversion_information.ExcelData = ( rows_to_read == 1 ) ? new object[1, 1] { { dataRange.Value2 } } : dataRange.Value2;

                            cellsRead += rows_to_read;
                            backgroundWorker.ReportProgress((int)( 100 * cellsRead / totalCellsToRead ));
                        }

                        record_conversion_information.NextRow += rows_to_read;
                    }

                    _worker.ConstructCases(backgroundWorker);
                }

                // construct cases one last time to take care of any final rows of data
                _worker.ConstructCases(backgroundWorker);

                // delete any cases (if applicable) and close the file
                _worker.FinishConversion(backgroundWorker);

                // display an error message about duplicate keys
                if( _worker.Counts.DuplicateKeys.Count > 0 )
                {
                    StringBuilder sb = new StringBuilder();
                    sb.AppendFormat(Messages.ConversionDuplicateKeys, Counts.Converted);
                    sb.AppendLine();
                    sb.AppendLine();

                    foreach( string key in _worker.Counts.DuplicateKeys )
                        sb.AppendLine("• " + key);

                    throw new Exception(sb.ToString());
                }
            }

            catch( Exception exception )
            {
                string message = ( exception.Message.Length == 0 ) ? Messages.DataWriteError : exception.Message;
                backgroundWorker.ReportProgress(Int32.MinValue,message);
            }
        }
    }
}
