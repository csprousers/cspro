using System;
using System.Collections.Generic;
using System.Diagnostics;

namespace ParadataViewer
{
    class ParadataTable
    {
        // starts at 1 because "All" will be displayed in a drop down on the MetadataForm
        public enum TableType { BaseEvent = 1, Event, Instance, Information };

        public string Name;
        public TableType Type;
        public int? EventTypeCode;
        public List<ParadataColumn> Columns;
    }

    class ParadataColumn
    {
        public enum ColumnType { Boolean, Integer, Long, Double, Text };

        public ParadataTable Table;
        public string Name;
        public ColumnType Type;
        public bool Nullable;
        public Dictionary<double,string> Codes;

        // calculated fields
        public bool IsId;
        public bool IsTimestamp;
        public ParadataTable LinkedTable;
        public bool HasLinkedTable { get { return ( LinkedTable != null ); } }
        public bool IsBaseEventInstance { get { return ( ( Table.Type == ParadataTable.TableType.BaseEvent ) &&
                        HasLinkedTable && ( LinkedTable.Type == ParadataTable.TableType.Instance ) ); } }
    }

    partial class Controller
    {
        private void ProcessMetadata(List<object[]> metadataRows)
        {
            TableDefinitions = new Dictionary<string,ParadataTable>();

            var potentiallyLinkedColumns = new List<ParadataColumn>();

            ParadataTable currentTable = null;
            ParadataColumn currentColumn = null;

            foreach( var row in metadataRows )
            {
                // process the table
                string tableName = row[0].ToString();

                if( ( currentTable == null ) || ( tableName != currentTable.Name ) )
                {
                    currentTable = new ParadataTable()
                    {
                        Name = tableName
                    };

                    TableDefinitions.Add(tableName,currentTable);

                    int tableCode = (int)(double)row[1];

                    if( tableCode != -1 )
                    {
                        Debug.Assert(StringIsAtEnd("_event",tableName));
                        currentTable.Type = ParadataTable.TableType.Event;
                        currentTable.EventTypeCode = tableCode;
                    }

                    else if( tableName.IndexOf("event") == 0 )
                    {
                        currentTable.Type = ParadataTable.TableType.BaseEvent;
                        BaseEventTable = currentTable;
                    }

                    else
                    {
                        if( StringIsAtEnd("_instance",tableName) )
                            currentTable.Type = ParadataTable.TableType.Instance;

                        else
                            currentTable.Type = ParadataTable.TableType.Information;
                    }

                    // every table has an ID field
                    currentColumn = new ParadataColumn()
                    {
                        Table = currentTable,
                        Name = "id",
                        Type = ParadataColumn.ColumnType.Long,
                        IsId = true
                    };

                    currentTable.Columns = new List<ParadataColumn>();
                    currentTable.Columns.Add(currentColumn);
                }

                // process the column
                string columnName = row[2].ToString();

                if( columnName != currentColumn.Name )
                {
                    string type = row[3].ToString();

                    currentColumn = new ParadataColumn()
                    {
                        Table = currentTable,

                        Name = columnName,

                        Type =
                        ( type == "boolean" ) ? ParadataColumn.ColumnType.Boolean :
                        ( type == "integer" ) ? ParadataColumn.ColumnType.Integer :
                        ( type == "long" ) ? ParadataColumn.ColumnType.Long :
                        ( type == "double" ) ? ParadataColumn.ColumnType.Double :
                        ( type == "text" ) ? ParadataColumn.ColumnType.Text :
                        throw new InvalidOperationException(),

                        Nullable = ( (int)(double)row[4] == 1 ),

                        IsTimestamp = ( StringIsAtEnd("time",columnName) && ( columnName != "readtime" ) )
                    };

                    currentTable.Columns.Add(currentColumn);

                    if( currentColumn.Type == ParadataColumn.ColumnType.Long )
                        potentiallyLinkedColumns.Add(currentColumn);
                }

                // process the code (if available)
                if( row[5] != null )
                {
                    Debug.Assert(currentColumn.Type != ParadataColumn.ColumnType.Text);

                    if( currentColumn.Codes == null )
                        currentColumn.Codes = new Dictionary<double,string>();

                    currentColumn.Codes[(double)row[5]] = row[6].ToString();
                }
            }

            // determine any table links
            foreach( var column in potentiallyLinkedColumns )
            {
                foreach( var kp in TableDefinitions )
                {
                    string tableName = kp.Key;

                    // if the table name is at the end (and this isn't recursive), it's a match
                    if( StringIsAtEnd(tableName,column.Name) && ( column.Table != kp.Value ) )
                    {
                        column.LinkedTable = kp.Value;
                        break;
                    }
                }
            }
        }

        private static bool StringIsAtEnd(string endString,string fullString)
        {
            int index = fullString.IndexOf(endString);
            return ( ( index >= 0 ) && ( ( index + endString.Length ) == fullString.Length ) );
        }
    }
}
