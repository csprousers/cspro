using System;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace ParadataViewer
{
    abstract class ViewerForm : Form
    {
        private QueryOptions _previousQueryOptions;
        private string _previousFilterSql;
        private string _previousQuerySql;

        protected Controller _controller;

        public ViewerForm(Controller controller)
        {
            this.Activated += ViewerForm_Activated;
            this.FormClosing += ViewerForm_FormClosing;

            _controller = controller;
        }

        private async void ViewerForm_Activated(object sender,EventArgs e)
        {
            await RefreshFormAsync(true);
        }

        private void ViewerForm_FormClosing(object sender,FormClosingEventArgs e)
        {
            _controller.UpdateQuerySql("",false);
        }

        internal async Task RefreshFormAsync(bool updateSqlQuery)
        {
            try
            {
                bool mustRefreshQuery = false;

                if( QueryUsesFilters && ( _previousFilterSql != _controller.FilterSql ) )
                    mustRefreshQuery = true;

                string sql = GetSqlQuery();

                if( !mustRefreshQuery && _previousQuerySql != sql )
                    mustRefreshQuery = true;

                if( !mustRefreshQuery && _previousQueryOptions != null )
                    mustRefreshQuery = ForceQueryRefreshAfterOptionsChange(_previousQueryOptions);

                if( updateSqlQuery )
                    _controller.UpdateQuerySql(sql,QueryUsesFilters);

                if( mustRefreshQuery )
                    await RefreshQueryAsync();

                else if( ( _previousQueryOptions != null ) && !_previousQueryOptions.Equals(_controller.Settings.QueryOptions) )
                    RefreshAfterOptionsChange(_previousQueryOptions);

                _previousQueryOptions = new QueryOptions(_controller.Settings.QueryOptions);
                _previousFilterSql = _controller.FilterSql;
                _previousQuerySql = sql;
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }
        }

        protected async Task ForceRefreshFormAsync()
        {
            // force the refreshing of the form
            _previousQuerySql = null;
            await RefreshFormAsync(true);
        }

        protected abstract bool QueryUsesFilters { get; }

        protected abstract string GetSqlQuery();

        protected abstract Task RefreshQueryAsync();

        protected virtual void RefreshAfterOptionsChange(QueryOptions previousQueryOptions)
        {
        }

        protected virtual bool ForceQueryRefreshAfterOptionsChange(QueryOptions previousQueryOptions)
        {
            return false;
        }
    }
}
