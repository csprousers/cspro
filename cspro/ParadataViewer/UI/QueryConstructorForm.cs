using System;
using System.Windows.Forms;

namespace ParadataViewer
{
    class QueryConstructorForm : TableForm
    {
        private QueryConstructorControl _queryConstructorControl;

        internal QueryConstructorForm(Controller controller,string initialSql) : base(controller)
        {
            this.Text = "Query Constructor";

            _queryConstructorControl = new QueryConstructorControl(this,initialSql);
            _queryConstructorControl.Dock = DockStyle.Fill;

            panelControls.Controls.Add(_queryConstructorControl);

            this.Shown += QueryConstructorForm_Shown;
        }

        private void QueryConstructorForm_Shown(object sender,EventArgs e)
        {
            _queryConstructorControl.Focus();
        }

        protected override bool QueryUsesFilters { get { return false; } }

        protected override bool IsTabularQuery { get { return true; } }

        internal async void ProcessNewQuery(string sql)
        {
            try
            {
                string[] columnNames = null;

                if( String.IsNullOrWhiteSpace(sql) )
                    _dbQuery = null;

                else
                {
                    _dbQuery = await _controller.CreateQueryAsync(sql);
                    columnNames = _dbQuery.ColumnNames;
                }

                _sql = sql;

                SetupTableColumns(columnNames);

                await ForceRefreshFormAsync();
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }
        }
    }
}
