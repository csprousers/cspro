using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;

namespace ParadataViewer
{
    partial class TableBrowserControl : UserControl
    {
        private TableBrowserForm _tableBrowserForm;
        private Controller _controller;
        private Dictionary<ParadataTable.TableType,List<ParadataTable>> _tableCategoriesMap;
        private ParadataTable _selectedTable;

        internal TableBrowserControl(TableBrowserForm tableBrowserForm,Controller controller)
        {
            InitializeComponent();

            _tableBrowserForm = tableBrowserForm;
            _controller = controller;

            _controller.RestoreCheckBoxState(checkBoxListTablesAlphabetically);
            _controller.RestoreCheckBoxState(checkBoxOptionIncludeBaseEventInstances);

            UpdateTableListing();
        }

        internal ParadataTable SelectedTable { get { return _selectedTable; } }
        internal bool IncludeBaseEventInstances{ get { return checkBoxOptionIncludeBaseEventInstances.Checked; } }

        private void checkBoxListTablesAlphabetically_CheckedChanged(object sender,EventArgs e)
        {
            _controller.SaveCheckBoxState(checkBoxListTablesAlphabetically);
            UpdateTableListing();
        }

        private void UpdateTableListing()
        {
            treeViewTables.SuspendLayout();

            treeViewTables.Nodes.Clear();

            if( checkBoxListTablesAlphabetically.Checked )
                UpdateTableListingAlphabetically();

            else
                UpdateTableListingCategorically();

            treeViewTables.ExpandAll();

            treeViewTables.ResumeLayout();
        }

        private void UpdateTableListingAlphabetically()
        {
            foreach( var kp in _controller.TableDefinitions.OrderBy(x => x.Key) )
            {
                ParadataTable table = kp.Value;

                var tableTreeNode = treeViewTables.Nodes.Add(table.Name);
                tableTreeNode.Tag = table;
            }
        }

        private void UpdateTableListingCategorically()
        {
            // categorize the tables
            if( _tableCategoriesMap == null )
            {
                _tableCategoriesMap = new Dictionary<ParadataTable.TableType,List<ParadataTable>>();

                foreach( var kp in _controller.TableDefinitions )
                {
                    ParadataTable table = kp.Value;

                    List<ParadataTable> tablesList = null;

                    if( !_tableCategoriesMap.TryGetValue(table.Type,out tablesList) )
                    {
                        tablesList = new List<ParadataTable>();
                        _tableCategoriesMap.Add(table.Type,tablesList);
                    }

                    tablesList.Add(table);
                }
            }

            // add the categories in order
            object[,] order = new object[,]
            {
                { ParadataTable.TableType.Event, "Event Tables" },
                { ParadataTable.TableType.BaseEvent, "Base Event Table" },
                { ParadataTable.TableType.Information, "Information Tables" },
                { ParadataTable.TableType.Instance, "Instance Tables" }
            };

            for( int i = 0; i < order.GetLength(0); i++ )
            {
                var categoryTreeNode = treeViewTables.Nodes.Add((string)order[i,1]);

                foreach( var table in _tableCategoriesMap[(ParadataTable.TableType)order[i,0]].OrderBy(x => x.Name) )
                {
                    var tableTreeNode = categoryTreeNode.Nodes.Add(table.Name);
                    tableTreeNode.Tag = table;
                }
            }
        }

        private void treeViewTables_AfterSelect(object sender,TreeViewEventArgs e)
        {
            if( treeViewTables.SelectedNode.Tag != null )
            {
                _selectedTable = (ParadataTable)treeViewTables.SelectedNode.Tag;
                ProcessSelection();
            }
        }

        private void checkBoxOptionIncludeBaseEventInstances_Click(object sender,EventArgs e)
        {
            _controller.SaveCheckBoxState(checkBoxOptionIncludeBaseEventInstances);
            ProcessSelection();
        }

        private async void ProcessSelection()
        {
            if( _selectedTable == null )
                return;

            if( await _tableBrowserForm.ProcessNewQuery() )
                labelDisplayedTableName.Text = String.Format("Displaying {0}",_selectedTable.Name);
        }
    }
}
