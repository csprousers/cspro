using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.Windows.Forms;
using CSPro.ParadataViewer;

namespace ParadataViewer
{
    abstract partial class QueryForm : ViewerForm
    {
        protected string _sql = "";
        protected DatabaseQuery _dbQuery;
        protected List<object[]> _rows;
        private bool _formClosed;

        internal QueryForm(Controller controller) : base(controller)
        {
            InitializeComponent();
        }

        private void QueryForm_Load(object sender,EventArgs e)
        {
            if( SaveLabelText == null )
                linkLabelSave.Visible = false;

            else
                linkLabelSave.Text = SaveLabelText;

            _controller.RestoreSplitterState(this,panelControls,splitterControls);
        }

        private void QueryForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            _formClosed = true;
        }

        private void splitterControls_SplitterMoved(object sender,SplitterEventArgs e)
        {
            _controller.SaveSplitterState(this,panelControls,splitterControls);
        }

        protected override string GetSqlQuery()
        {
            return _sql;
        }

        protected abstract string SaveLabelText { get; }

        protected abstract bool IsTabularQuery { get; }

        protected abstract void linkLabelSave_LinkClicked(object sender,LinkLabelLinkClickedEventArgs e);

        protected override async Task RefreshQueryAsync()
        {
            labelResultsIgnoreFilters.Visible = !QueryUsesFilters;

            if( _dbQuery == null )
                _rows = null;

            else
            {
                int initialNumberRows = IsTabularQuery ? _controller.Settings.TabularQueryInitialNumberRows :
                    _controller.Settings.GeneralQueryInitialNumberRows;

                _rows = await GetResultsAsync(initialNumberRows);

                if( !_formClosed )
                {
                    HandleResults(0);
                    linkLabelLoadCompleteResults.Visible = ( _dbQuery.AdditionalResultsAvailable == true );
                }
            }
        }

        private async void linkLabelLoadCompleteResults_LinkClicked(object sender,LinkLabelLinkClickedEventArgs e)
        {
            try
            {
                int initialNumberRowsRetrieved = _rows.Count;

                int maximumNumberRows = IsTabularQuery ? _controller.Settings.TabularQueryMaximumNumberRows :
                    _controller.Settings.GeneralQueryMaximumNumberRows;

                var additionalRows = await GetResultsAsync(maximumNumberRows - initialNumberRowsRetrieved);

                lock( _rows )
                {
                    _rows.AddRange(additionalRows);
                }

                HandleResults(initialNumberRowsRetrieved);
                linkLabelLoadCompleteResults.Visible = false;
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }
        }

        private async Task<List<object[]>> GetResultsAsync(int maxNumberResults)
        {
            DateTime startTime = DateTime.Now;
            _controller.UpdateStatusBarText("Executing query...");

            List<object[]> rows = await Task.Run(() => _dbQuery.GetResults(maxNumberResults));

            double seconds = DateTime.Now.Subtract(startTime).TotalSeconds;

            _controller.UpdateStatusBarText(String.Format("{0} row{1} returned in {2:F3} seconds",
                rows.Count,( rows.Count == 1 ) ? "" : "s",seconds));

            return rows;
        }

        protected abstract void HandleResults(int startingRow);
    }
}
