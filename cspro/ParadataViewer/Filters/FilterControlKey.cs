using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace ParadataViewer
{
    class KeyFilter : InstanceFilter
    {
        private const string CaseInfoTableName = "case_info";
        private const string CaseKeyTableName = "case_key_info";
        private const string UuidColumnName = "uuid";
        private const string KeyColumnName = "key";

        private const string CasesTemporaryTableName = "cases";
        private const string MaxIdTemporaryColumnName = "max_id";

        internal string KeyFilterText { get; set; }

        internal KeyFilter()
            : base(FilterType.Case,CaseInfoTableName,UuidColumnName)
        {
            KeyFilterText = String.Empty;
        }

        internal override bool FilterIsShowingAllRows { get { return ( KeyFilterText == String.Empty ); } }

        internal override string GetPossibleValuesSqlQuery()
        {
            string sql =
                $"SELECT `{CasesTemporaryTableName}`.`{UuidColumnName}`, `{CaseKeyTableName}`.`{KeyColumnName}` " +
                $"FROM ( SELECT MAX(`{CaseKeyTableName}`.`id`) AS `{MaxIdTemporaryColumnName}`, `{InfoTableName}`.`{UuidColumnName}` " +
                $"FROM `{InstanceTableName}` " +
                $"JOIN `{InfoTableName}` ON `{InstanceTableName}`.`{InfoTableName}` = `{InfoTableName}`.`id` " +
                $"JOIN `{CaseKeyTableName}` ON `{InfoTableName}`.`id` = `{CaseKeyTableName}`.`{InfoTableName}` " +
                $"GROUP BY `{InfoTableName}`.`{UuidColumnName}` ) AS `{CasesTemporaryTableName}` " +
                $"JOIN `{CaseKeyTableName}` ON `{CasesTemporaryTableName}`.`{MaxIdTemporaryColumnName}` = `{CaseKeyTableName}`.`id` ";

            if( !String.IsNullOrEmpty(KeyFilterText) )
                sql += $"WHERE `{CaseKeyTableName}`.`{KeyColumnName}` {Controller.CreateSqlLikeExpression("{0}%",KeyFilterText)} ";

            sql += $"ORDER BY `{CaseKeyTableName}`.`{KeyColumnName}`;";

            return sql;
        }
    }

    class FilterControlKey: FilterControl
    {
        private KeyFilter _keyFilter;
        private TextBox _textBoxFilter;
        private bool _filterUsed;
        private bool _resettingResults;

        internal FilterControlKey(Controller controller)
            : base(controller,new KeyFilter())
        {
            _keyFilter = (KeyFilter)Filter;

            // add the key filter text box
            _textBoxFilter = new CueTextBox()
            {
                Cue = "Filter by case ID"
            };

            _textBoxFilter.KeyUp += textBoxFilter_KeyUp;

            AddCustomControl(_textBoxFilter);
        }

        private async void textBoxFilter_KeyUp(object sender,KeyEventArgs e)
        {
            _filterUsed = true;

            if( _resettingResults || _textBoxFilter.Text == _keyFilter.KeyFilterText )
                return;

            _resettingResults = true;

            _keyFilter.KeyFilterText = _textBoxFilter.Text;

            await ResetResultsAsync();

            _resettingResults = false;

            _controller.RefreshFilters();
        }

        internal override void ResetFilter()
        {
            if( _filterUsed )
            {
                _textBoxFilter.Text = String.Empty;
                _keyFilter.KeyFilterText = String.Empty;

                ResetResultsAsync();

                _filterUsed = false;
            }

            else
                base.ResetFilter();
        }
    }

    // from https://stackoverflow.com/questions/4902565/watermark-textbox-in-winforms
    class CueTextBox : TextBox
    {
        internal string Cue
        {
            get { return mCue; }
            set { mCue = value; updateCue(); }
        }

        private void updateCue()
        {
            if ( this.IsHandleCreated && mCue != null )
                SendMessage(this.Handle,0x1501,(IntPtr)1,mCue);
        }

        protected override void OnHandleCreated(EventArgs e)
        {
            base.OnHandleCreated(e);
            updateCue();
        }

        private string mCue;

        // PInvoke
        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern IntPtr SendMessage(IntPtr hWnd,int msg,IntPtr wp,string lp);
    }
}
