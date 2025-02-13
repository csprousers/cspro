using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using CSPro.Dictionary;
using Microsoft.Office.Interop.Excel;

namespace Excel2CSPro
{
    class ExpandedDictionaryItem
    {
        public DictionaryRecord Record;
        public DictionaryItem Item;
        public int? Occurrence;
        public int Start;
        public bool IsId;

        public string Name { get { return Item.Name; } }
        public string Label { get { return Item.Label; } }
        public int Length { get { return Item.Length; } }

        private ExpandedDictionaryItem() { }

        private static DictionaryItem _lastItem;

        public static List<ExpandedDictionaryItem> GetItems(DictionaryRecord record,DictionaryItem item,bool isId)
        {
            // expand items out the given number of occurrences
            List<ExpandedDictionaryItem> expandedItems = new List<ExpandedDictionaryItem>();

            if( item.ItemType == ItemType.Item )
                _lastItem = item;

            int occurrences = _lastItem.Occurrences;
            int occurrenceStartOffset = _lastItem.Length;

            if( ( item.ItemType == ItemType.Subitem ) && ( item.Occurrences > 1 ) ) // a repeating subitem
            {
                occurrences = item.Occurrences;
                occurrenceStartOffset = item.Length;
            }

            for( int i = 0; i < occurrences; i++ )
            {
                ExpandedDictionaryItem expandedItem = new ExpandedDictionaryItem();

                expandedItem.Record = record;
                expandedItem.Item = item;
                expandedItem.Occurrence = ( occurrences == 1 ) ? (int?)null : ( i + 1 );
                expandedItem.Start = item.Start + ( i * occurrenceStartOffset );
                expandedItem.IsId = isId;

                expandedItems.Add(expandedItem);
            }

            return expandedItems;
        }
    }

    class Excel2CSProMappingManager
    {
        private WorkbookController _workbookController;
        private Dictionary<Worksheet,List<string>> _worksheetColumns;

        private DataDictionary _dictionary;
        private List<DictionaryRecord> _records;
        private Dictionary<DictionaryRecord,Worksheet> _recordMappings;
        private Dictionary<DictionaryRecord,Dictionary<ExpandedDictionaryItem,int?>> _itemMappings;

        public Excel2CSProMappingManager(WorkbookController workbookController,DataDictionary dictionary)
        {
            // setup the Excel objects
            _workbookController = workbookController;

            _worksheetColumns = new Dictionary<Worksheet,List<string>>();

            foreach( Worksheet worksheet in _workbookController.Workbook.Worksheets )
            {
                List<string> columnNames = new List<string>();

                // read in the column names
                for( int column = 1; column <= worksheet.UsedRange.Columns.Count; column++ )
                {
                    // the column address will appear in a format like $A:$A
                    string columnAddress = (string)worksheet.Columns[column].Address;
                    columnAddress = columnAddress.Substring(1,columnAddress.IndexOf(':') - 1);
                    string columnName = String.Format("Column {0}",columnAddress);

                    // read the first row, which may be header text
                    object header = worksheet.Cells[1,column].Value2;

                    if( header != null )
                        columnName = columnName + ": " + header.ToString().Trim();

                    columnNames.Add(columnName);
                }

                _worksheetColumns.Add(worksheet,columnNames);
            }


            // setup the dictionary objects
            _dictionary = dictionary;
            _records = new List<DictionaryRecord>();
            _recordMappings = new Dictionary<DictionaryRecord,Worksheet>();
            _itemMappings = new Dictionary<DictionaryRecord,Dictionary<ExpandedDictionaryItem,int?>>();

            // setup the records
            foreach( DictionaryRecord record in dictionary.Levels[0].Records )
            {
                _records.Add(record);
                _recordMappings.Add(record,null);
                _itemMappings.Add(record,new Dictionary<ExpandedDictionaryItem,int?>());
            }

            // add the IDs to the records
            foreach( DictionaryItem item in dictionary.Levels[0].IdItems.Items )
            {
                foreach( DictionaryRecord record in _records )
                {
                    foreach( ExpandedDictionaryItem expandedItem in ExpandedDictionaryItem.GetItems(record,item,true) )
                        _itemMappings[record].Add(expandedItem,null);
                }
            }

            // add the other items
            foreach( DictionaryRecord record in _records )
            {
                foreach( DictionaryItem item in record.Items )
                {
                    foreach( ExpandedDictionaryItem expandedItem in ExpandedDictionaryItem.GetItems(record,item,false) )
                        _itemMappings[record].Add(expandedItem,null);
                }
            }
        }

        public Dictionary<string,object> GetPossibleWorksheets()
        {
            Dictionary<string,object> possibleSelections = new Dictionary<string,object>();

            foreach( Worksheet worksheet in _workbookController.Workbook.Worksheets )
                possibleSelections.Add(worksheet.Name,worksheet);

            possibleSelections.Add(Messages.MappingUnassigned,null);

            return possibleSelections;
        }

        public string GetMappedWorksheetName(DictionaryRecord record)
        {
            return ( _recordMappings[record] == null ) ? Messages.MappingUnassigned : _recordMappings[record].Name;
        }

        public Worksheet GetMappedWorksheet(DictionaryRecord record)
        {
            return _recordMappings[record];
        }

        public void MapRecordToWorksheet(DictionaryRecord record,Worksheet worksheet)
        {
            if( _recordMappings[record] != worksheet ) // only check if there was a change
            {
                if( worksheet != null && _recordMappings.ContainsValue(worksheet) )
                    throw new Exception(String.Format(Messages.MappingWorksheetAlreadyAssigned,worksheet.Name));

                _recordMappings[record] = worksheet;

                ResetRecordItemMappings(record);
            }
        }

        public List<DictionaryRecord> Records { get { return _records; } }

        public List<DictionaryRecord> MappedRecords
        {
            get
            {
                List<DictionaryRecord> mappedRecords = new List<DictionaryRecord>();

                foreach( var kp in _recordMappings )
                {
                    if( kp.Value != null )
                        mappedRecords.Add(kp.Key);
                }

                return mappedRecords;
            }
        }

        public List<ExpandedDictionaryItem> GetRecordItems(DictionaryRecord record)
        {
            List<ExpandedDictionaryItem> items = new List<ExpandedDictionaryItem>();

            foreach( var kp in _itemMappings[record] )
                items.Add(kp.Key);

            return items;
        }

        public Dictionary<string,object> GetPossibleColumns(DictionaryRecord record,ExpandedDictionaryItem item)
        {
            Dictionary<string,object> possibleSelections = new Dictionary<string,object>();

            Worksheet worksheet = _recordMappings[record];
            List<string> columns = _worksheetColumns[worksheet];

            for( int i = 0; i < columns.Count; i++ )
                possibleSelections.Add(columns[i],i);

            possibleSelections.Add(Messages.MappingUnassigned,null);

            return possibleSelections;
        }

        public string GetMappedColumn(DictionaryRecord record,ExpandedDictionaryItem item)
        {
            int? col = _itemMappings[record][item];

            if( col == null )
                return Messages.MappingUnassigned;

            else
            {
                Worksheet worksheet = _recordMappings[record];
                List<string> columns = _worksheetColumns[worksheet];
                return columns[(int)col];
            }
        }

        public void ResetRecordItemMappings(DictionaryRecord record)
        {
            Dictionary<ExpandedDictionaryItem,int?> recordItemMappings = _itemMappings[record];

            foreach( ExpandedDictionaryItem item in recordItemMappings.Keys.ToList() )
                recordItemMappings[item] = null;
        }

        public void FillDefaultItemMappings(DictionaryRecord record)
        {
            Dictionary<ExpandedDictionaryItem,int?> recordItemMappings = _itemMappings[record];
            int numColumns = _worksheetColumns[_recordMappings[record]].Count;
            int column = 0;

            foreach( ExpandedDictionaryItem item in recordItemMappings.Keys.ToList() )
            {
                if( item.Item.ItemType == ItemType.Item )
                {
                    recordItemMappings[item] = ( column < numColumns ) ? (int?)column : null;
                    column++;
                }
            }
        }

        public void MapItemToColumn(DictionaryRecord record,ExpandedDictionaryItem item,int column)
        {
            Dictionary<ExpandedDictionaryItem,int?> recordItemMappings = _itemMappings[record];
            List<string> columnNames = _worksheetColumns[_recordMappings[record]];

            int? nullableColumn = ( column == columnNames.Count ) ? null : (int?)column;

            if( recordItemMappings[item] != nullableColumn ) // only check if there was a change
            {
                // make sure the selection doesn't overlap with another selection
                if( nullableColumn != null )
                {
                    int overlapStartPos = item.Start;
                    int overlapEndPos = item.Start + item.Length;

                    foreach( ExpandedDictionaryItem thisItem in recordItemMappings.Keys )
                    {
                        if( thisItem != item && recordItemMappings[thisItem] != null )
                        {
                            int endPos = thisItem.Start + thisItem.Length;

                            if( ( thisItem.Start >= overlapStartPos && thisItem.Start < overlapEndPos ) ||
                                ( endPos > overlapStartPos && thisItem.Start < overlapStartPos ) )
                            {
                                throw new Exception(String.Format(Messages.MappingItemOverlap,item.Name,thisItem.Name));
                            }
                        }
                    }
                }

                recordItemMappings[item] = nullableColumn;
            }
        }

        public List<CSPro.Data.Excel2CSPro.RecordMapping> GetSerializableMappings()
        {
            var mappings = new List<CSPro.Data.Excel2CSPro.RecordMapping>();

            foreach( var kpRW in _recordMappings )
            {
                if( kpRW.Value != null )
                {
                    DictionaryRecord record = kpRW.Key;
                    Worksheet worksheet = kpRW.Value;

                    // add the record mapping
                    var record_mapping = new CSPro.Data.Excel2CSPro.RecordMapping
                    {
                        RecordName = record.Name,
                        WorksheetName = worksheet.Name,
                        WorksheetIndex = worksheet.Index
                    };

                    mappings.Add(record_mapping);

                    // add any item mappings
                    foreach( var kp in _itemMappings[record] )
                    {
                        ExpandedDictionaryItem item = kp.Key;
                        int? column = kp.Value;

                        if( column != null )
                        {
                            record_mapping.ItemMappings.Add(new CSPro.Data.Excel2CSPro.ItemMapping()
                            {
                                ItemName = item.Name,
                                Occurrence = item.Occurrence,
                                ColumnIndex = (int)column + 1,
                            });                         
                        }
                    }
                }
            }

            return mappings;
        }

        public string ProcessSerializableMappings(List<CSPro.Data.Excel2CSPro.RecordMapping> mappings)
        {
            // the function will return a list of mapping errors (if any)
            StringBuilder sb = new StringBuilder();

            foreach( var record_mapping in mappings )
            {
                try
                {
                    // find the record
                    var record = _records.Find(x => x.Name.Equals(record_mapping.RecordName, StringComparison.InvariantCulture));

                    if( record == null )
                        throw new Exception($"The record {record_mapping.RecordName} because it is not a record in the dictionary");

                    if( _recordMappings[record] != null )
                        throw new Exception($"The record {record.Name} because it is already mapped to {_recordMappings[record].Name}");

                    // find the worksheet
                    Worksheet worksheet = null;

                    // ...first by checking the name
                    if( record_mapping.WorksheetName != null )
                    {
                        foreach( Worksheet thisWorksheet in _workbookController.Workbook.Worksheets )
                        {
                            if( thisWorksheet.Name.Equals(record_mapping.WorksheetName) )
                            {
                                worksheet = thisWorksheet;
                                break;
                            }
                        }
                    }

                    // ...and then the index
                    if( worksheet == null )
                    {
                        if( record_mapping.WorksheetIndex >= 1 && record_mapping.WorksheetIndex <= _workbookController.Workbook.Worksheets.Count )
                            worksheet = _workbookController.Workbook.Worksheets[record_mapping.WorksheetIndex];

                        if( worksheet == null )
                            throw new Exception($"The record {record_mapping.RecordName} because the worksheet could not be found");
                    }

                    if( _recordMappings.ContainsValue(worksheet) )
                        throw new Exception($"The record {record_mapping.RecordName} because the worksheet {worksheet.Name} is already mapped");

                    MapRecordToWorksheet(record, worksheet);


                    // process the item mappings
                    var item_mappings = _itemMappings[record];

                    foreach( var item_mapping in record_mapping.ItemMappings )
                    {
                        try
                        {
                            if( item_mapping.ColumnIndex < 1 || item_mapping.ColumnIndex > _worksheetColumns[worksheet].Count )
                                throw new Exception($"The item {item_mapping.ItemName} because its mapping to column {item_mapping.ColumnIndex} is not valid (must be 1-{_worksheetColumns[worksheet].Count})");

                            var item = item_mappings.FirstOrDefault(x => x.Key.Name.Equals(item_mapping.ItemName, StringComparison.InvariantCulture) &&
                                                                         x.Key.Occurrence == item_mapping.Occurrence);

                            if( item.Key == null )
                            {
                                if( item_mapping.Occurrence.HasValue &&
                                    item_mappings.FirstOrDefault(x => x.Key.Name.Equals(item_mapping.ItemName, StringComparison.InvariantCulture)).Key != null )
                                {
                                    throw new Exception($"The item {item_mapping.ItemName}({item_mapping.Occurrence}) because the occurrence is not valid");
                                }

                                else
                                {
                                    throw new Exception($"The item {item_mapping.ItemName} because it is not an ID item and does not belong to {record.Name}");
                                }
                            }

                            if( item.Value != null )
                                throw new Exception($"The item {item_mapping.ItemName} because it is already mapped to column {item.Value + 1}");

                            MapItemToColumn(record, item.Key, item_mapping.ColumnIndex - 1);
                        }

                        catch( Exception exception )
                        {
                            sb.AppendLine(exception.Message);
                        }
                    }
                }

                catch( Exception exception )
                {
                    sb.AppendLine(exception.Message);
                }
            }

            return sb.ToString();
        }

        public void PrerunChecks()
        {
            // checks:
            // - at least one ID item has to be defined
            // - an ID item defined in one record has to be defined in all of the records
            int numRecordsDefined = 0;
            Dictionary<string,int> idsDefined = new Dictionary<string,int>();

            foreach( var kpRW in _recordMappings )
            {
                if( kpRW.Value != null )
                {
                    DictionaryRecord record = kpRW.Key;
                    numRecordsDefined++;

                    foreach( var kp in _itemMappings[record] )
                    {
                        ExpandedDictionaryItem item = kp.Key;
                        int? column = kp.Value;

                        if( item.IsId && column != null )
                        {
                            if( idsDefined.ContainsKey(item.Name) )
                                idsDefined[item.Name]++;

                            else
                                idsDefined[item.Name] = 1;
                        }
                    }
                }
            }

            if( numRecordsDefined == 0 )
                throw new Exception(Messages.PrerunCheckMappingMustDefineRecord);

            else if( idsDefined.Count == 0 )
                throw new Exception(Messages.PrerunCheckMappingNoIdsDefined);

            else
            {
                foreach( var kp in idsDefined )
                {
                    if( kp.Value != numRecordsDefined )
                        throw new Exception(String.Format(Messages.PrerunCheckMappingIdsMustBeDefinedForEachRecord,kp.Key));
                }
            }
        }

        public List<CSPro.Data.Excel2CSPro.RecordConversionInformation> GenerateConversionInformation()
        {
            var record_conversion_information_list = new List<CSPro.Data.Excel2CSPro.RecordConversionInformation>();

            foreach( var kpRW in _recordMappings )
            {
                if( kpRW.Value != null )
                {
                    DictionaryRecord record = kpRW.Key;
                    var record_conversion_information = new CSPro.Data.Excel2CSPro.RecordConversionInformation(record, kpRW.Value);

                    foreach( var kp in _itemMappings[record] )
                    {
                        ExpandedDictionaryItem item = kp.Key;
                        int? column = kp.Value;

                        if( column != null )
                        {
                            uint occurrence = ( item.Occurrence == null ) ? 0 : ( (uint)item.Occurrence - 1 );
                            var item_conversion_information = new CSPro.Data.Excel2CSPro.ItemConversionInformation(item.Item, (int)column + 1, occurrence);
                            record_conversion_information.AddItem(item_conversion_information);
                        }
                    }

                    record_conversion_information_list.Add(record_conversion_information);
                }
            }

            return record_conversion_information_list;
        }
    }
}
