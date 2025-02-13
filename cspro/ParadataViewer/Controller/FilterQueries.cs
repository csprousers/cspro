using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace ParadataViewer
{
    partial class Controller
    {
        private const string EventTableName = "event";
        private const string FilteredEventTableName = "filtered_event";

        private string _filterSql;
        internal string FilterSql { get { return _filterSql; } }

        internal void RefreshFilters()
        {
            bool updatedFilter = false;

            try
            {
                string sql = null;

                var sb = new StringBuilder(); // the where statements
                var joins = new List<string>();

                // construct the filter
                foreach( var filterControl in _mainForm.FilterControls )
                {
                    var whereSql = filterControl.GetWhereSql();

                    if( whereSql == null )
                        continue;

                    if( sb.Length > 0 )
                        sb.Append(" AND\r\n");

                    sb.Append(whereSql);

                    filterControl.Filter.AddJoinSqls(joins);
                }

                if( sb.Length > 0 )
                {
                    sql = $"CREATE TEMPORARY VIEW `{FilteredEventTableName}` AS\r\nSELECT `{EventTableName}`.* FROM `{EventTableName}`\r\n";

                    foreach( var joinSql in joins.Distinct() )
                        sql = sql + joinSql + "\r\n";

                    sql = sql + "WHERE " + sb.ToString() + ";";
                }

                // update the view if it has changed
                if( _filterSql != sql )
                {
                    // delete the existing view
                    _db.ExecuteNonQuery($"DROP VIEW IF EXISTS `{FilteredEventTableName}`;");

                    // and create a new one
                    if( sql != null )
                        _db.ExecuteNonQuery(sql);

                    updatedFilter = true;
                    _filterSql = sql;
                }
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
                _filterSql = null;
            }

            if( updatedFilter )
            {
                UpdateStatusBarText("Updated the filter");
                _mainForm.RefreshWindows();
            }
        }

        internal bool QueryCanUseFilters(string sql)
        {
            return ( sql.IndexOf($"`{EventTableName}`") >= 0 );
        }

        internal string ModifyQueryForFilters(string sql)
        {
            if( _filterSql != null )
                sql = sql.Replace($"`{EventTableName}`",$"`{FilteredEventTableName}`");

            return sql;
        }
    }
}
