using System;
using System.Collections.Generic;
using System.IO;
using System.Windows.Forms;
using CSPro.Dictionary;

namespace Excel2CSPro
{
    class Excel2CSProManager
    {
        internal Excel2CSProMappingManager _mappingManager;

        internal WorkbookController _workbookController;
        internal List<string> _worksheetNames;

        internal CSPro.Data.Excel2CSPro.Spec _spec;
        internal DataDictionary _dictionary;

        public Excel2CSProManager()
        {
            _workbookController = new WorkbookController();

            _spec = new CSPro.Data.Excel2CSPro.Spec();
        }

        public void LoadSpecFile(string filename)
        {
            if( filename != null )
                _spec.Load(filename);

            if( _spec.ExcelFilename != null )
                OpenExcel(_spec.ExcelFilename);

            if( _spec.DictionaryFilename != null )
                OpenDictionary(_spec.DictionaryFilename);

            // if an Excel file and a dictionary were loaded, process the mappings
            if( _mappingManager != null )
            {
                string mappingErrors = _mappingManager.ProcessSerializableMappings(_spec.Mappings);

                if( !String.IsNullOrWhiteSpace(mappingErrors) )
                    MessageBox.Show("The following mappings were ignored:\n\n" + mappingErrors);
            }
        }

        public void SaveSpecFile(string filename)
        {
            if( _mappingManager != null )
                _spec.Mappings = _mappingManager.GetSerializableMappings();
            
            _spec.Save(filename);
        }

        internal void OpenExcel(string filename)
        {
            _worksheetNames = null;
            _spec.ExcelFilename = null;

            try
            {
                _workbookController.OpenWorkbook(filename);
                _worksheetNames = _workbookController.WorksheetNames;
                _spec.ExcelFilename = filename;

                RefreshMappingManager();
            }

            catch( Exception exception )
            {
                throw new Exception(String.Format(Messages.ExcelIOError,exception.Message));
            }
        }

        internal void OpenDictionary(string filename)
        {
            _dictionary = null;
            _spec.DictionaryFilename = null;

            try
            {
                DataDictionary readDictionary = new DataDictionary(filename);

                if( readDictionary.Levels.Length > 1 )
                    throw new Exception(Messages.DictionaryUnsupportedNumberLevels);

                _dictionary = readDictionary;
                _spec.DictionaryFilename = filename;

                RefreshMappingManager();
            }

            catch( Exception exception )
            {
                throw ( exception.Message.Length == 0 ) ? new Exception(Messages.DictionaryReadError) : exception;
            }
        }

        private void RefreshMappingManager()
        {
            if( _spec.ExcelFilename != null && _dictionary != null )
                _mappingManager = new Excel2CSProMappingManager(_workbookController,_dictionary);
        }

        public CSPro.Data.Excel2CSPro.ConversionCounts RunConversion(FormStartPosition formStartPosition = FormStartPosition.CenterScreen)
        {
            if( _spec.ExcelFilename == null )
                throw new Exception(Messages.PrerunCheckNoExcel);

            if( _spec.DictionaryFilename == null )
                throw new Exception(Messages.PrerunCheckNoDictionary);

            if( _spec.OutputConnectionString == null || _spec.OutputConnectionString.Type == CSPro.Util.DataRepositoryType.Null )
                throw new Exception(Messages.PrerunCheckNoDataFile);

            if( _spec.StartingRow < 1 )
                throw new Exception(Messages.PrerunCheckInvalidStartingRow);

            _mappingManager.PrerunChecks();

            Excel2CSProConverter converter = new Excel2CSProConverter(this);
            bool runConversion = true;

            if( _spec.RunOnlyIfNewer )
            {
                try
                {
                    FileInfo excelFileInfo = new FileInfo(_spec.ExcelFilename);
                    FileInfo dataFileInfo = new FileInfo(_spec.OutputConnectionString.Filename);

                    if( dataFileInfo.LastWriteTimeUtc > excelFileInfo.LastWriteTimeUtc )
                        runConversion = false;
                }
                catch { } // ignore any FileInfo exceptions
            }

            if( runConversion )
            {
                ProgressForm progressDlg = new ProgressForm(formStartPosition,Messages.TitleConvertingData,true,converter);

                if( progressDlg.ShowDialog() != DialogResult.OK )
                    return null; // the user canceled the operation
            }

            return converter.Counts;
        }
    }
}
