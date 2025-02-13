using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace ParadataViewer
{
    class PluggableQueryForm : QueryForm, IPluggableOwner
    {
        private IPluggableQueryControl _pluggableQueryControl;

        internal PluggableQueryForm(Controller controller,IPluggableQueryControl pluggableQueryControl) : base(controller)
        {
            panelControls.Visible = false;
            splitterControls.Visible = false;

            // setup the control
            _pluggableQueryControl = pluggableQueryControl;

            this.Text = _pluggableQueryControl.WindowTitle;

            var control = _pluggableQueryControl.CreatePluginControl(this);
            panelResults.Controls.Add(control);
            control.Dock = DockStyle.Fill;
        }

        protected override bool QueryUsesFilters { get { return _pluggableQueryControl.IsSqlQueryEventBased; } }

        protected override bool IsTabularQuery { get { return false; } }

        protected override string GetSqlQuery()
        {
            _sql = _pluggableQueryControl.GetSqlQuery();

            if( _pluggableQueryControl.IsSqlQueryEventBased )
                _sql = _controller.ModifyQueryForFilters(_sql);

            return _sql;
        }

        public async Task ExecuteQuery()
        {
            await ForceRefreshFormAsync();
        }

        protected override async Task RefreshQueryAsync()
        {
            _dbQuery = await _controller.CreateQueryAsync(GetSqlQuery());
            await base.RefreshQueryAsync();
        }

        protected override void HandleResults(int startingRow)
        {
            _pluggableQueryControl.ProcessResults(_rows,startingRow);
        }

        protected override string SaveLabelText { get { return _pluggableQueryControl.SaveContentsText; } }

        protected override async void linkLabelSave_LinkClicked(object sender,LinkLabelLinkClickedEventArgs e)
        {
            await _pluggableQueryControl.SaveContentsAsync();
        }


        public QueryOptions QueryOptions { get { return _controller.Settings.QueryOptions; } }

        public Task<Int64> ExecuteSingleQueryAsync(string sql)
        {
            return _controller.ExecuteSingleQueryAsync(sql);
        }

        public Task<List<object[]>> ExecuteQueryAsync(string sql)
        {
            return _controller.ExecuteQueryAsync(sql);
        }

        public string GetConvertedTimestampColumnName(string columnName)
        {
            return QueryOptions.ConvertTimestamps ? _controller.GetConvertedTimestampColumnName(columnName) : columnName;
        }
    }
}
