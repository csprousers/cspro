using System;
using System.Collections.Generic;
using System.Linq;

namespace ParadataViewer
{
    abstract class Filter
    {
        internal enum FilterType { Application, Session, Case, Event };

        private FilterType _filterType;
        protected List<object[]> _values;

        internal Filter(FilterType filterType)
        {
            _filterType = filterType;
            ResetValues();
        }

        internal void ResetValues()
        {
            _values = new List<object[]>();
        }

        internal FilterType Type { get { return _filterType; } }

        protected static string GetSuffixTableName(FilterType filterType,string suffix)
        {
            if( filterType == FilterType.Event )
                throw new InvalidOperationException();

            else
                return filterType.ToString().ToLower() + suffix;
        }

        protected const string NameTableAndColumnName = "name";
        protected static string EventTableName { get { return FilterType.Event.ToString().ToLower(); } }
        protected string InstanceTableName { get { return GetSuffixTableName(_filterType,"_instance"); } }
        protected string InfoTableName { get { return GetSuffixTableName(_filterType,"_info"); } }

        internal virtual bool ForceRetrieveAllRows { get { return false; } }

        internal virtual bool FilterIsShowingAllRows { get { return true; } }

        internal abstract string GetPossibleValuesSqlQuery();

        protected static string PossibleValueToString(object value)
        {
            return ( value != null ) ? value.ToString() : "<undefined>";
        }

        internal virtual List<string> ProcessPossibleValues(List<object[]> values)
        {
            _values.AddRange(values);

            var displayedValues = values.Select(x => PossibleValueToString(x.Last())).ToList();

            return displayedValues;
        }

        internal virtual void AddJoinSqls(List<string> joinsSqls)
        {
        }

        internal abstract string GetWhereSql(int index);
    }

    class InstanceFilter : Filter
    {
        protected string _tableName;
        protected string _columnName;

        internal InstanceFilter(FilterType filterType,string tableName,string columnName)
            :   base(filterType)
        {
            _tableName = tableName;
            _columnName = columnName;
        }

        internal override string GetPossibleValuesSqlQuery()
        {
            return $"SELECT `{_columnName}` FROM `{_tableName}` GROUP BY `{_columnName}` ORDER BY `{_columnName}`;";
        }

        internal override void AddJoinSqls(List<string> joinsSqls)
        {
            joinsSqls.Add($"JOIN `{InstanceTableName}` ON `{EventTableName}`.`{InstanceTableName}` = `{InstanceTableName}`.`id`");
            joinsSqls.Add($"JOIN `{_tableName}` ON `{InstanceTableName}`.`{_tableName}` = `{_tableName}`.`id`");
        }

        internal override string GetWhereSql(int index)
        {
            object value = _values[index][0];

            string lhs = $"`{_tableName}`.`{_columnName}`";

            if( value is string )
                return $"{lhs} = '{Controller.SqlEscape((string)value)}'";

            else if( value is double )
                return $"{lhs} = {value}";

            else
                return $"{lhs} IS NULL";
        }
    }

    class InstanceFilterWithCodes : InstanceFilter
    {
        private Dictionary<double,string> _codes;

        internal InstanceFilterWithCodes(FilterType filterType,string tableName,string columnName,Dictionary<string,ParadataTable> tableDefinitions)
            :   base(filterType,tableName,columnName)
        {
            _tableName = tableName;
            _columnName = columnName;

            // get the code set
            ParadataTable paradataTable = null;

            if( tableDefinitions.TryGetValue(_tableName,out paradataTable) )
            {
                var paradataColumn = paradataTable.Columns.FirstOrDefault(x => x.Name.Equals(_columnName));

                if( paradataColumn != null )
                    _codes = paradataColumn.Codes;
            }
        }

        internal override List<string> ProcessPossibleValues(List<object[]> values)
        {
            _values.AddRange(values);
            var displayedValues = new List<string>();

            foreach( var row in values )
            {
                object value = row.Last();
                string text = null;

                if( !( value is double ) || _codes == null || !_codes.TryGetValue((double)value,out text) )
                    text = PossibleValueToString(value);

                displayedValues.Add(text);
            }

            return displayedValues;
        }
    }

    class InfoFilter : InstanceFilter
    {
        internal InfoFilter(FilterType filterType,string tableName,string columnName)
            :   base(filterType,tableName,columnName)
        {
        }

        internal override void AddJoinSqls(List<string> joinsSqls)
        {
            joinsSqls.Add($"JOIN `{InstanceTableName}` ON `{EventTableName}`.`{InstanceTableName}` = `{InstanceTableName}`.`id`");
            joinsSqls.Add($"JOIN `{InfoTableName}` ON `{InstanceTableName}`.`{InfoTableName}` = `{InfoTableName}`.`id`");
            joinsSqls.Add($"JOIN `{_tableName}` ON `{InfoTableName}`.`{_tableName}` = `{_tableName}`.`id`");
        }
    }

    class DictionaryNameFilter : InstanceFilter
    {
        internal DictionaryNameFilter()
            :   base(FilterType.Case,"case_info","dictionary_name")
        {
        }

        internal override string GetPossibleValuesSqlQuery()
        {
            return  $"SELECT `{_columnName}`, `{NameTableAndColumnName}` FROM `{_tableName}` " +
                    $"JOIN `{NameTableAndColumnName}` ON `{_columnName}` = `{NameTableAndColumnName}`.`id` " +
                    $"GROUP BY `{_columnName}` ORDER BY `{NameTableAndColumnName}`;";
        }
    }

    class ProcNameFilter : Filter
    {
        private const string ProcColumnName = "proc_name";

        internal ProcNameFilter()
            :   base(FilterType.Event)
        {
        }

        internal override string GetPossibleValuesSqlQuery()
        {
            return  $"SELECT `{ProcColumnName}`, `{NameTableAndColumnName}` FROM `{EventTableName}` " +
                    $"JOIN `{NameTableAndColumnName}` ON `{ProcColumnName}` = `{NameTableAndColumnName}`.`id` " +
                    $"GROUP BY `{ProcColumnName}` ORDER BY `{NameTableAndColumnName}`;";
        }

        internal override string GetWhereSql(int index)
        {
            object value = _values[index][0];
            return $"`{EventTableName}`.`{ProcColumnName}` = {(double)value}";
        }
    }
}
