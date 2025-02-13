using System;
using System.Collections.Generic;

namespace ParadataViewer
{
    partial class Controller
    {
        private MainForm _mainForm;

        internal Dictionary<string,ParadataTable> TableDefinitions { get; private set; }
        internal ParadataTable BaseEventTable { get; private set; }

        internal Settings Settings { get; private set; }

        internal Controller(MainForm mainForm)
        {
            _mainForm = mainForm;
            Settings = Settings.Load();
        }

        ~Controller()
        {
            DeleteTemporaryFiles();

            Settings.Save();

            if( _excelApplication != null )
            {
                try
                {
                    _excelApplication.Quit();
                }
                catch { }
            }
        }

        internal void UpdateQuerySql(string sql,bool useFilters)
        {
            if( sql != String.Empty && useFilters && _filterSql != null )
                sql = _filterSql + "\r\n\r\n" + sql;

            _mainForm.UpdateQuerySql(sql);
        }

        internal void UpdateStatusBarText(string text)
        {
            _mainForm.UpdateStatusBarText(text);
        }


        private Microsoft.Office.Interop.Excel.Application _excelApplication;

        internal Microsoft.Office.Interop.Excel.Application GetExcelApplication()
        {
            return _excelApplication ?? ( _excelApplication = new Microsoft.Office.Interop.Excel.Application() );
        }
    }
}
