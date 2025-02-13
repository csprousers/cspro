using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace ParadataViewer
{
    class TableBrowserForm : TableForm
    {
        private TableBrowserControl _tableBrowserControl;
        private List<string> _columnNames;

        private class CodeMappings
        {
            public int Column;
            public Dictionary<double,string> Codes;
            public object[] OriginalCodes;
        }

        private List<CodeMappings> _queriedCodeMappings;
        private List<CodeMappings> _executedCodeMappings;

        internal TableBrowserForm(Controller controller) : base(controller)
        {
            this.Text = "Table Browser";

            panelControls.Width = 230;
            panelControls.Dock = DockStyle.Left;
            splitterControls.Dock = DockStyle.Left;

            _tableBrowserControl = new TableBrowserControl(this,controller);
            _tableBrowserControl.Dock = DockStyle.Fill;

            panelControls.Controls.Add(_tableBrowserControl);
        }

        protected override bool QueryUsesFilters
        {
            get
            {
                bool queryUsesFilters = true;

                if( _tableBrowserControl.SelectedTable != null )
                {
                    queryUsesFilters = ( _tableBrowserControl.SelectedTable.Type == ParadataTable.TableType.BaseEvent ) ||
                        ( _tableBrowserControl.SelectedTable.Type == ParadataTable.TableType.Event );
                }

                return queryUsesFilters;
            }
        }

        protected override bool IsTabularQuery { get { return true; } }

        protected override string GetSqlQuery()
        {
            if( _tableBrowserControl.SelectedTable != null )
            {
                var queryConstructor =  new QueryConstructorFromOptions(_controller);
                queryConstructor.IncludeBaseEventInstances = _tableBrowserControl.IncludeBaseEventInstances;

                _columnNames = new List<string>();
                _queriedCodeMappings = new List<CodeMappings>();
                queryConstructor.NewColumnCallback = NewColumnCallback;

                _sql = ( _tableBrowserControl.SelectedTable.Type == ParadataTable.TableType.Event ) ?
                    queryConstructor.ConstructEventQuery(_tableBrowserControl.SelectedTable) :
                    queryConstructor.ConstructTableQuery(_tableBrowserControl.SelectedTable);

                if( QueryUsesFilters )
                    _sql = _controller.ModifyQueryForFilters(_sql);
            }

            return _sql;
        }

        private void NewColumnCallback(ParadataColumn column,Stack<ParadataColumn> linkingColumns)
        {
            string columnName = QualifyColumnName(column,linkingColumns);

            // now show any linking
            foreach( var linkingColumn in linkingColumns.Reverse() )
                columnName = String.Format("{0}⇒\r\n{1}",QualifyColumnName(linkingColumn,linkingColumns),columnName);

            _columnNames.Add(columnName);

            if( column.Codes != null )
            {
                _queriedCodeMappings.Add(new CodeMappings()
                {
                    Column = _columnNames.Count - 1,
                    Codes = column.Codes
                });
            }
        }

        private string QualifyColumnName(ParadataColumn column,Stack<ParadataColumn> linkingColumns)
        {
            // columns from the selected table will not be qualified
            if( ( column.Table == _tableBrowserControl.SelectedTable ) && ( linkingColumns.Count == 0 ) )
                return column.Name;

            else
                return String.Format("{0}.{1}",column.Table.Name,column.Name);
        }

        protected override async Task RefreshQueryAsync()
        {
            if( _columnNames != null )
            {
                SetupTableColumns(_columnNames.ToArray());

                _executedCodeMappings = _queriedCodeMappings;

                _dbQuery = await _controller.CreateQueryAsync(_sql);

                await base.RefreshQueryAsync();
            }
        }

        protected override void HandleResults(int startingRow)
        {
            if( _controller.Settings.QueryOptions.ViewValueLabels )
                ConvertCodesToLabels();

            base.HandleResults(startingRow);
        }

        private void ConvertCodesToLabels()
        {
            lock( _rows )
            {
                // change the rows from codes to values
                foreach( var mapping in _executedCodeMappings )
                {
                    int startingRow = 0;

                    if( mapping.OriginalCodes == null )
                        mapping.OriginalCodes = new object[_rows.Count];

                    else if( mapping.OriginalCodes.Length != _rows.Count )
                    {
                        // this will happen when additional rows are retrieved after the first query
                        startingRow = mapping.OriginalCodes.Length;
                        object[] resizedArray = new object[_rows.Count];
                        Array.Copy(mapping.OriginalCodes,resizedArray,startingRow);
                        mapping.OriginalCodes = resizedArray;
                    }

                    // keep track of the last lookup to prevent repeat use of the
                    // code lookup for situations in which a lot of codes are the same
                    double lastCode = Double.MinValue;
                    string lastValue = null;

                    for( int i = startingRow; i < _rows.Count; i++ )
                    {
                        mapping.OriginalCodes[i] = _rows[i][mapping.Column];

                        if( mapping.OriginalCodes[i] == null )
                            continue;

                        double code = (double)mapping.OriginalCodes[i];

                        if( code != lastCode )
                        {
                            if( !mapping.Codes.TryGetValue(code,out lastValue) )
                                continue;

                            lastCode = code;
                        }

                        _rows[i][mapping.Column] = lastValue;
                    }
                }
            }
        }

        private void ConvertLabelsToCodes()
        {
            lock( _rows )
            {
                // change the rows from values to codes
                for( int i = 0; i < _rows.Count; i++ )
                {
                    var row = _rows[i];

                    foreach( var mapping in _executedCodeMappings )
                        row[mapping.Column] = mapping.OriginalCodes[i];
                }
            }
        }

        protected override void RefreshAfterOptionsChange(QueryOptions previousQueryOptions)
        {
            if( previousQueryOptions.ViewValueLabels == _controller.Settings.QueryOptions.ViewValueLabels )
                return;

            // process a change in viewing the value associated with codes
            if( ( _executedCodeMappings == null ) || ( _executedCodeMappings.Count == 0 ) )
                return;

            if( _controller.Settings.QueryOptions.ViewValueLabels )
                ConvertCodesToLabels();

            else
                ConvertLabelsToCodes();

            // redraw the cells
            Debug.Assert(_dgvResults.Rows.Count == _rows.Count);

            _dgvResults.SuspendLayout();

            for( int i = 0; i < _rows.Count; i++ )
            {
                var row = _dgvResults.Rows[i];

                foreach( var mapping in _executedCodeMappings )
                    row.Cells[mapping.Column].Value = _rows[i][mapping.Column];
            }

            _dgvResults.ResumeLayout();
        }

        internal async Task<bool> ProcessNewQuery()
        {
            try
            {
                await ForceRefreshFormAsync();
                return true;
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
                return false;
            }
        }
    }
}
