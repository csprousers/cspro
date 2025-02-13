using System;
using System.Linq;
using System.Windows.Forms;

namespace ParadataViewer
{
    partial class MetadataForm : Form, IQueryConstructorOptions
    {
        private Controller _controller;
        private ParadataTable _displayedTable;
        private ParadataColumn _displayedColumn;
        private string _initialColumnsHeaderFormatterText;
        private string _initialCodesHeaderFormatterText;

        public MetadataForm(Controller controller)
        {
            InitializeComponent();

            _controller = controller;

            _initialColumnsHeaderFormatterText = labelColumnsHeader.Text;
            _initialCodesHeaderFormatterText = labelCodesHeader.Text;

            foreach( Control control in panelTableMetadata.Controls )
            {
                if( control is CheckBox )
                    _controller.RestoreCheckBoxState((CheckBox)control);
            }

            comboBoxTableFilter.SelectedIndex = 0; // show all tables
        }

        private void UpdateMetadata(ParadataTable.TableType? tableType)
        {
            treeViewMetadata.SuspendLayout();

            treeViewMetadata.Nodes.Clear();

            foreach( var kp in _controller.TableDefinitions.OrderBy(x => x.Key) )
            {
                ParadataTable table = kp.Value;

                if( ( tableType != null ) && ( table.Type != tableType ) )
                    continue;

                var tableTreeNode = treeViewMetadata.Nodes.Add(table.Name);
                tableTreeNode.Tag = table;

                foreach( var column in table.Columns )
                {
                    var columnTreeNode = tableTreeNode.Nodes.Add(column.Name);
                    columnTreeNode.Tag = new Tuple<ParadataTable,ParadataColumn>(table,column);
                }
            }

            treeViewMetadata.ResumeLayout();

            treeViewMetadata.SelectedNode = treeViewMetadata.Nodes[0];
        }

        private void UpdateSelections(ParadataTable selectedTable,ParadataColumn selectedColumn)
        {
            if( _displayedTable != selectedTable )
            {
                labelColumnsHeader.Text = String.Format(_initialColumnsHeaderFormatterText,selectedTable.Name);

                listViewColumns.SuspendLayout();

                listViewColumns.Items.Clear();

                foreach( var column in selectedTable.Columns )
                {
                    var lvi = listViewColumns.Items.Add(column.Name);
                    lvi.SubItems.Add(column.Type.ToString().ToLower());

                    string sqliteType = ( column.Type == ParadataColumn.ColumnType.Text ) ? "text" :
                        ( column.Type == ParadataColumn.ColumnType.Double ) ? "real" :
                        "integer";

                    lvi.SubItems.Add(sqliteType + ( column.Nullable ? " / null" : "" ));
                    lvi.SubItems.Add(( column.LinkedTable != null ) ? column.LinkedTable.Name : "");
                    lvi.Tag = column;
                }

                listViewColumns.ResumeLayout();

                _displayedTable = selectedTable;

                UpdateSqlQuery();
            }

            if( _displayedColumn != selectedColumn )
            {
                labelCodesHeader.Text = String.Format(_initialCodesHeaderFormatterText,selectedColumn.Name);

                listViewCodes.SuspendLayout();

                listViewCodes.Items.Clear();

                if( selectedColumn.Codes != null )
                {
                    foreach( var kp in selectedColumn.Codes )
                    {
                        var lvi = listViewCodes.Items.Add(kp.Key.ToString());
                        lvi.SubItems.Add(kp.Value.ToString());
                    }
                }

                listViewCodes.ResumeLayout();

                _displayedColumn = selectedColumn;
            }
        }

        private void linkLabelExpandAllNodes_LinkClicked(object sender,LinkLabelLinkClickedEventArgs e)
        {
            TreeNode selectedNode = treeViewMetadata.SelectedNode;
            treeViewMetadata.ExpandAll();
            selectedNode.EnsureVisible();
        }

        private void treeViewMetadata_AfterSelect(object sender,TreeViewEventArgs e)
        {
            ParadataTable selectedTable = null;
            ParadataColumn selectedColumn = null;

            if( e.Node.Tag is ParadataTable )
            {
                selectedTable = (ParadataTable)e.Node.Tag;
                selectedColumn = selectedTable.Columns[0];
            }

            else
            {
                var tuple = (Tuple<ParadataTable,ParadataColumn>)e.Node.Tag;
                selectedTable = tuple.Item1;
                selectedColumn = tuple.Item2;
            }

            UpdateSelections(selectedTable,selectedColumn);
        }

        private void listViewColumns_SelectedIndexChanged(object sender,EventArgs e)
        {
            if( listViewColumns.SelectedItems.Count == 1 )
            {
                var selectedColumn = (ParadataColumn)listViewColumns.SelectedItems[0].Tag;
                UpdateSelections(_displayedTable,selectedColumn);
            }
        }

        private void comboBoxTableFilter_SelectedIndexChanged(object sender,EventArgs e)
        {
            UpdateMetadata(comboBoxTableFilter.SelectedIndex == 0 ? null : (ParadataTable.TableType?)comboBoxTableFilter.SelectedIndex);
        }

        private void linkLabelOpenInQueryConstructor_LinkClicked(object sender,LinkLabelLinkClickedEventArgs e)
        {
            ((MainForm)this.MdiParent).OpenQueryConstructor(textBoxQuerySql.Text);
        }

        private void checkBoxOption_Click(object sender,EventArgs e)
        {
            _controller.SaveCheckBoxState((CheckBox)sender);
            UpdateSqlQuery();
        }

        private void UpdateSqlQuery()
        {
            var queryConstructor = new QueryConstructor(_controller,this);

            if( checkBoxOptionLinkFromBaseEvent.Checked && ( _displayedTable.Type == ParadataTable.TableType.Event ) )
                textBoxQuerySql.Text = queryConstructor.ConstructEventQuery(_displayedTable);

            else
                textBoxQuerySql.Text = queryConstructor.ConstructTableQuery(_displayedTable);
        }

        // members of the IQueryConstructorOptions interface
        public bool ConvertTimestamps { get { return false; } }
        public bool ShowFullyLinkedTables { get { return checkBoxOptionFullyLinkTables.Checked; } }
        public bool ShowLinkingValues { get { return checkBoxOptionShowLinkedValue.Checked; } }
        public bool ExplicitlySelectColumns { get { return checkBoxOptionExplicitlySelectColumns.Checked; } }
        public bool IncludeBaseEventInstances { get { return checkBoxOptionIncludeBaseEventInstances.Checked; } }
        public string NewLine { get { return "\r\n\r\n"; } }
        public QueryConstructorNewColumnCallback NewColumnCallback { get { return null; } }
    }
}
