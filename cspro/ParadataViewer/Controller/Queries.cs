using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text;

namespace ParadataViewer
{
    static class Queries
    {
        public const string CountNumberEvents =
            "SELECT COUNT(*) FROM `event`;";

        public const string QueryMetadata =
            "SELECT `metadata_table_info`.`table`, `metadata_table_info`.`code`, " +
            "`metadata_column_info`.`column`, `metadata_column_info`.`type`, `metadata_column_info`.`nullable`, " +
            "`metadata_code_info`.`code`, `metadata_code_info`.`value` " +
            "FROM `metadata_table_info` " +
            "LEFT JOIN `metadata_column_info` ON `metadata_table_info`.`id` = `metadata_column_info`.`metadata_table_info` " +
            "LEFT JOIN `metadata_code_info` ON `metadata_column_info`.`id` = `metadata_code_info`.`metadata_column_info` " +
            "ORDER BY `metadata_table_info`.`id`, `metadata_column_info`.`id`;";
    }


    delegate void QueryConstructorNewColumnCallback(ParadataColumn column,Stack<ParadataColumn> linkingColumns);

    interface IQueryConstructorOptions
    {
        bool ConvertTimestamps { get; }
        bool ShowFullyLinkedTables { get; }
        bool ShowLinkingValues { get; }
        bool IncludeBaseEventInstances { get; }
        bool ExplicitlySelectColumns { get; }
        string NewLine { get; }
        QueryConstructorNewColumnCallback NewColumnCallback { get; }
    }

    class QueryConstructor
    {
        private Controller _controller;
        private IQueryConstructorOptions _options;
        private StringBuilder _sb;
        private StringBuilder _sbSelectedColumns;
        private int _selectColumnsPos;
        private Stack<string> _tableNames;
        private Stack<bool> _nullableJoins;
        private Stack<ParadataColumn> _linkingColumns;

        internal QueryConstructor(Controller controller,IQueryConstructorOptions options)
        {
            _controller = controller;
            _options = options;
        }

        internal IQueryConstructorOptions QueryConstructorOptions { set { _options = value; } }

        private void ResetObjects()
        {
            _sb = new StringBuilder();
            _sbSelectedColumns = new StringBuilder();
            _tableNames = new Stack<string>();
            _nullableJoins = new Stack<bool>();
            _linkingColumns = new Stack<ParadataColumn>();
        }

        private void ConstructQueryPrelude(ParadataTable table)
        {
            ResetObjects();

            _sb.Append("SELECT ");

            _selectColumnsPos = _sb.Length;

            _sb.AppendFormat("{0}FROM `{1}`",_options.NewLine,table.Name);
        }

        private string ConstructQueryPostlude()
        {
            _sb.Insert(_selectColumnsPos,_options.ExplicitlySelectColumns ? _sbSelectedColumns.ToString() : "*");
            return _sb.ToString() + ";";
        }

        internal string ConstructTableQuery(ParadataTable table)
        {
            ConstructQueryPrelude(table);

            _tableNames.Push(table.Name);

            ProcessColumns(table);

            return ConstructQueryPostlude();
        }

        internal string ConstructEventQuery(ParadataTable table)
        {
            ConstructQueryPrelude(table);

            _sb.AppendFormat("{0}JOIN `{1}` ON `{2}`.`id` = `{1}`.`id`",_options.NewLine,_controller.BaseEventTable.Name,table.Name);

            // first add the event's ID
            Debug.Assert(table.Columns[0].IsId);

            _tableNames.Push(table.Name);
            ProcessColumn(table.Columns[0]);

            // then add the non-instance and non-ID base event columns
            _tableNames.Pop();
            _tableNames.Push(_controller.BaseEventTable.Name);

            foreach( var column in _controller.BaseEventTable.Columns )
            {
                if( !column.IsBaseEventInstance && !column.IsId )
                    ProcessColumn(column);
            }

            // then add the columns for the event
            _tableNames.Pop();
            _tableNames.Push(table.Name);

            foreach( var column in table.Columns )
            {
                if( !column.IsId )
                    ProcessColumn(column);
            }

            // the base event instances may not be added, but when creating a table for the application,
            // session, or case events, make sure that the corresponding instance is included
            ParadataTable baseEventInstanceTableOverride = null;
            string instanceTableName = table.Name.Replace("_event","_instance");
            _controller.TableDefinitions.TryGetValue(instanceTableName,out baseEventInstanceTableOverride);

            // finally add the instance base event columns
            _tableNames.Pop();
            _tableNames.Push(_controller.BaseEventTable.Name);

            foreach( var column in _controller.BaseEventTable.Columns )
            {
                if( column.IsBaseEventInstance )
                    ProcessColumn(column,column.LinkedTable == baseEventInstanceTableOverride);
            }

            return ConstructQueryPostlude();
        }

        private void ProcessColumn(ParadataColumn column,bool overrideBaseEventInstance = false)
        {
            // don't include the ID of any linked tables
            if( column.IsId && ( _tableNames.Count > 1 ) )
                return;

            bool willReturnBeforeIncludingBaseEventInstances = ( !overrideBaseEventInstance && !_options.IncludeBaseEventInstances && column.IsBaseEventInstance );

            if( !column.HasLinkedTable || _options.ShowLinkingValues || !_options.ShowFullyLinkedTables || willReturnBeforeIncludingBaseEventInstances )
                SelectColumn(column);

            if( willReturnBeforeIncludingBaseEventInstances )
                return;

            if( column.HasLinkedTable && _options.ShowFullyLinkedTables )
            {
                bool nullableJoin = column.Nullable;

                if( _nullableJoins.Count > 0 )
                    nullableJoin |= _nullableJoins.Peek();

                string joinType = nullableJoin ? "LEFT JOIN" : "JOIN";
                string joinWithTableName = column.LinkedTable.Name;

                // create a new name for the table
                string joinNewTableName = column.Name;

                // if not on the base table append the column name to the currently-being-processed joined table
                if( _tableNames.Count > 1 )
                    joinNewTableName = _tableNames.Peek() + "_" + joinNewTableName;

                _sb.AppendFormat("{0}{1} `{2}`",_options.NewLine,joinType,joinWithTableName);

                if( joinWithTableName != joinNewTableName )
                    _sb.AppendFormat(" `{0}`",joinNewTableName);

                _sb.AppendFormat(" ON {0} = `{1}`.`id`",GetEvaluatedColumnName(column),joinNewTableName);

                _nullableJoins.Push(nullableJoin);
                _tableNames.Push(joinNewTableName);
                _linkingColumns.Push(column);

                ProcessColumns(column.LinkedTable);

                _nullableJoins.Pop();
                _tableNames.Pop();
                _linkingColumns.Pop();
            }
        }

        private void ProcessColumns(ParadataTable table)
        {
            foreach( var column in table.Columns )
                ProcessColumn(column);
        }

        private void SelectColumn(ParadataColumn column)
        {
            _options.NewColumnCallback?.Invoke(column,_linkingColumns);

            if( _sbSelectedColumns.Length > 0 )
                _sbSelectedColumns.Append(", ");

            string columnName = GetEvaluatedColumnName(column);

            if( column.IsTimestamp && _options.ConvertTimestamps )
                _sbSelectedColumns.Append(_controller.GetConvertedTimestampColumnName(columnName));

            else
                _sbSelectedColumns.Append(columnName);
        }

        private string GetEvaluatedColumnName(ParadataColumn column)
        {
            return String.Format("`{0}`.`{1}`",_tableNames.Peek(),column.Name);
        }
    }

    class QueryConstructorFromOptions : QueryConstructor, IQueryConstructorOptions
    {
        private QueryOptions _queryOptions;

        internal QueryConstructorFromOptions(Controller controller) : base(controller,null)
        {
            base.QueryConstructorOptions = this;

            _queryOptions = controller.Settings.QueryOptions;
            IncludeBaseEventInstances = false;
            ExplicitlySelectColumns = true;
            NewLine = "\r\n";
        }

        public bool ConvertTimestamps { get { return _queryOptions.ConvertTimestamps; } }
        public bool ShowFullyLinkedTables { get { return _queryOptions.ShowFullyLinkedTables; } }
        public bool ShowLinkingValues {  get { return _queryOptions.ShowLinkingValues; } }
        public bool IncludeBaseEventInstances { get; set; }
        public bool ExplicitlySelectColumns { get; private set; }
        public string NewLine { get; private set; }
        public QueryConstructorNewColumnCallback NewColumnCallback { get; set; }
    }
}
