using System;
using System.Collections.Generic;
using System.Linq;
using Excel = Microsoft.Office.Interop.Excel;

namespace Excel2CSPro
{
    class WorkbookController
    {
        private class WorkbookCounter
        {
            public Excel.Workbook workbook;
            public int count;
        };

        private static Excel.Application _application = null;
        private static Dictionary<string, WorkbookCounter> _openWorkbooks = new Dictionary<string, WorkbookCounter>();
        private string _workbookFilename = null;
        private Excel.Workbook _workbook = null;
        private Excel.Worksheet _worksheet = null;

        public static void Close()
        {
            if( _application != null )
            {
                // ignore any errors closing the workbooks or Excel
                foreach( var workbook_counter in _openWorkbooks.Values )
                {
                    try
                    {
                        CloseWorkbook(workbook_counter.workbook);
                    }
                    catch { }
                }

                try
                {
                    _application.Quit();
                }
                catch { }
            }
        }

        public void OpenWorkbook(string filename)
        {
            Excel.Workbook workbook_to_be_closed = _workbook;
            WorkbookCounter workbook_counter;

            // if the workbook is already open, use it
            if( _openWorkbooks.TryGetValue(filename, out workbook_counter) )
            {
                // if they are reopening the same workbook, quit out
                if( workbook_counter.workbook == _workbook )
                    return;
            }

            // otherwise, open the workbook
            else
            {
                if( _application == null )
                    _application = new Excel.Application();

                workbook_counter = new WorkbookCounter()
                {
                    workbook = _application.Workbooks.Open(filename, Type.Missing, true),
                    count = 0
                };

                _openWorkbooks.Add(filename, workbook_counter);
            }

            // potentially close the previously opened workbook
            if( workbook_to_be_closed != null )
            {
                WorkbookCounter workbook_counter_to_be_closed = _openWorkbooks[_workbookFilename];
                workbook_counter_to_be_closed.count--;

                if( workbook_counter_to_be_closed.count == 0 )
                {
                    CloseWorkbook(workbook_counter_to_be_closed.workbook);
                    _openWorkbooks.Remove(_workbookFilename);
                }
            }

            // increment the counter of objects using this workbook
            workbook_counter.count++;

            _workbook = workbook_counter.workbook;
            _workbookFilename = filename;
        }

        public static void CloseWorkbook(Excel.Workbook workbook)
        {
            workbook.Saved = true; // disable any prompts to save the data
            workbook.Close();
        }

        public string WorkbookFilename { get { return _workbookFilename; } }

        public Excel.Workbook Workbook { get { return _workbook; } }

        public List<string> WorksheetNames
        {
            get
            {
                List<string> names = new List<string>();

                foreach( Excel.Worksheet worksheet in _workbook.Worksheets )
                    names.Add(worksheet.Name);

                return names;
            }
        }

        public Excel.Worksheet GetWorksheet(int i)
        {
            _worksheet = _workbook.Worksheets[i + 1]; // the worksheets are one-based
            return _worksheet;
        }

        public int ParseStartingRow(string text)
        {
            int value;

            if( !Int32.TryParse(text,out value) )
                throw new Exception(Messages.StartingRowInvalid);

            else if( value < 1 || value > _worksheet.UsedRange.Rows.Count )
                throw new Exception(String.Format(Messages.StartingRowOutOfRange,_worksheet.UsedRange.Rows.Count));

            return value;
        }
    }
}
