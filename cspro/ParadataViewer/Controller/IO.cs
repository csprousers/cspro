using System;
using System.Collections.Generic;
using System.IO;
using System.Threading.Tasks;
using System.Windows.Forms;
using CSPro.ParadataViewer;

namespace ParadataViewer
{
    partial class Controller
    {
        internal const string LogFilenameExtension = ".cslog";
        internal const string LogFilenameFilter = "*" + LogFilenameExtension;

        private Database _db;
        private string _openLogFilename;
        private List<ReportQuery> _reportQueries;

        internal bool IsLogOpen { get { return ( _db != null ); } }
        internal string OpenLogFilename { get { return _openLogFilename; } }

        internal async Task LoadFileAsync(string filename)
        {
            try
            {
                if( IsLogOpen )
                    CloseFile();

                _db = new Database(filename);

                 ProcessMetadata(await ExecuteQueryAsync(Queries.QueryMetadata));

                _openLogFilename = filename;

                _mainForm.UpdateTitle();

                Int64 numberEvents = await ExecuteSingleQueryAsync(Queries.CountNumberEvents);
                _mainForm.UpdateStatusBarText(String.Format("{0} loaded with {1} events",Path.GetFileName(filename),numberEvents));

                _mainForm.InitializeFilters();
            }

            catch( Exception exception )
            {
                _db = null;
                MessageBox.Show(exception.Message);
            }
        }

        internal void CloseFile()
        {
            _db.Dispose();
            _db = null;

            _mainForm.UpdateStatusBarText(Path.GetFileName(_openLogFilename) + " closed");

            _openLogFilename = null;
            _mainForm.UpdateTitle();

            _reportQueries = null;

            _filterSql = null;
            _mainForm.HideFilters();

            _mainForm.CloseWindows();
        }

        internal Task<Int64> ExecuteSingleQueryAsync(string sql)
        {
            return Task.Run(() => _db.ExecuteSingleQuery(sql));
        }

        internal Task<List<object[]>> ExecuteQueryAsync(string sql)
        {
            return Task.Run(() => _db.ExecuteQuery(sql));
        }

        internal Task<DatabaseQuery> CreateQueryAsync(string sql)
        {
            return Task.Run(() => _db.CreateQuery(sql));
        }

        internal static string SqlEscape(string parameter)
        {
            return parameter.Replace("'","''");
        }

        private static readonly char[] SqlLikeCharsToEscape = new char[] { '%', '_' };

        internal static string CreateSqlLikeExpression(string formatter,string filter)
        {
            string escape = String.Empty;

            // only add an escape if necessary
            if( filter.IndexOfAny(SqlLikeCharsToEscape) >= 0 )
            {
                // determine what character to use while escaping
                const int StartingCharNumber = 48; // '0';
                int escapeCharNumber = StartingCharNumber;
                char escapeChar;

                do
                {
                    escapeChar = Convert.ToChar(escapeCharNumber++);
                } while( Array.IndexOf(SqlLikeCharsToEscape,escapeChar) >= 0 || filter.IndexOf(escapeChar) >= 0 );

                escape = $" ESCAPE '{escapeChar}'";

                // modify the filter to use this escape character
                for( int pos = 0; ( pos = filter.IndexOfAny(SqlLikeCharsToEscape,pos) ) >= 0; pos += 2 )
                    filter = filter.Substring(0,pos) + escapeChar + filter.Substring(pos);
            }

            return String.Format("LIKE '{0}'{1}",String.Format(formatter,filter),escape);
        }

        internal string GetConvertedTimestampColumnName(string columnName)
        {
            return String.Format("cspro_timestring('{0}',{1})",SqlEscape(Settings.TimestampFormatter),columnName);
        }

        internal List<ReportQuery> ReportQueries
        {
            get
            {
                if( _reportQueries == null && IsLogOpen )
                    _reportQueries = ReportManager.LoadParadataQueries(Path.GetDirectoryName(OpenLogFilename));

                return _reportQueries;
            }
        }
    }
}
