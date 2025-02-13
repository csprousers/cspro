using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using CSPro.ParadataViewer;

namespace ParadataViewer
{
    partial class FilterControl : UserControl
    {
        protected Controller _controller;
        protected Filter _filter;
        private DatabaseQuery _dbQuery;
        private bool _valuesRetrieved;
        private int _additionalResultsHeightAvailable;
        private bool _logicControllingChecks;
        private ListViewItem _additionalResultsLiewViewItem;
        private int _customControlsHeight;

        internal Filter Filter { get { return _filter; } }

        public FilterControl(Controller controller,Filter filter)
        {
             InitializeComponent();

            _controller = controller;
            _filter = filter;

            int listViewValuesBottomY = ( listViewValues.Location.Y + listViewValues.Height );
            int linkLabelLoadAdditionalResultsBottomY = ( linkLabelLoadAdditionalResults.Location.Y + linkLabelLoadAdditionalResults.Height );
            _additionalResultsHeightAvailable = ( linkLabelLoadAdditionalResultsBottomY - listViewValuesBottomY );
        }

        private async void listViewValues_MouseUp(object sender,MouseEventArgs e)
        {
            await ProcessClickAsync(sender,e);
        }

        internal async Task ProcessClickAsync(object sender,MouseEventArgs e)
        {
            bool clickedOnExpander = ( sender is Expander );

            if( e.Button == MouseButtons.Left )
            {
                if( clickedOnExpander )
                {
                    // if the query hasn't been executed, run it
                    if( !_valuesRetrieved )
                        await LoadInitialResultsAsync(true);
                }

                else if( listViewValues.SelectedItems.Count > 0 )
                {
                    // if the control key is pressed, select only the item the was was clicked on
                    if( ( Control.ModifierKeys & Keys.Control ) != 0 )
                        ProcessClick(ClickType.Only);
                }
            }

            else if( e.Button == MouseButtons.Right )
            {
                var menu = new ContextMenu();

                var menuItem = menu.MenuItems.Add("Select All");
                menuItem.Tag = ClickType.All;
                menuItem.Click += MenuItem_SelectClick;

                if( clickedOnExpander )
                {
                    menuItem = menu.MenuItems.Add("View SQL Query");
                    menuItem.Click += MenuItem_ViewSqlQueryClick;
                }

                if( !clickedOnExpander && listViewValues.SelectedItems.Count > 0 )
                {
                    string value = listViewValues.SelectedItems[0].Text;

                    menuItem = menu.MenuItems.Add($"Select Only '{value}'");
                    menuItem.Tag = ClickType.Only;
                    menuItem.Click += MenuItem_SelectClick;

                    menuItem = menu.MenuItems.Add($"Select All But '{value}'");
                    menuItem.Tag = ClickType.AllBut;
                    menuItem.Click += MenuItem_SelectClick;
                }

                menu.Show((Control)sender,e.Location);
            }
        }

        private enum ClickType { All, Only, AllBut };

        private void ProcessClick(ClickType clickType)
        {
            StartLogicControllingChecks();

            if( clickType == ClickType.All )
            {
                foreach( ListViewItem lvi in listViewValues.Items )
                    lvi.Checked = true;
            }

            else if( listViewValues.SelectedItems.Count > 0 )
            {
                var selectedLvi = listViewValues.SelectedItems[0];
                bool selectOnly = ( clickType == ClickType.Only );

                foreach( ListViewItem lvi in listViewValues.Items )
                    lvi.Checked = ( ( lvi == selectedLvi ) == selectOnly );
            }

            StopLogicControllingChecks();
        }

        private ListViewItem AddValue(string valueText)
        {
            var lvi = listViewValues.Items.Add(valueText);
            lvi.Checked = true;
            lvi.Tag = listViewValues.Items.Count - 1;
            return lvi;
        }

        protected async Task ResetResultsAsync()
        {
            // reset the additional results UI
            if( _additionalResultsLiewViewItem == null )
            {
                listViewValues.Height -= _additionalResultsHeightAvailable;
                linkLabelLoadAdditionalResults.Visible = true;
            }

            else
                _additionalResultsLiewViewItem = null;

            _filter.ResetValues();

            await LoadInitialResultsAsync(false);
        }

        private async Task LoadInitialResultsAsync(bool resizeControl)
        {
            _valuesRetrieved = true;

            // create the query and load the initial results
            _dbQuery = await _controller.CreateQueryAsync(_filter.GetPossibleValuesSqlQuery());

            await LoadResultsAsync();

            // resize the window based on the initial results
            const int MaximumListViewValuesHeight = 600;

            // the default height of listViewValues supports one item
            int lviHeight = ( listViewValues.Items.Count > 0 ) ? listViewValues.Items[0].GetBounds(ItemBoundsPortion.Entire).Height : 0;
            int additionalListViewValuesHeight = Math.Max(0,lviHeight * ( listViewValues.Items.Count - 1 ));
            int desiredListViewValuesHeight = Math.Min(MaximumListViewValuesHeight,listViewValues.Height + additionalListViewValuesHeight);

            if( !resizeControl )
                desiredListViewValuesHeight = listViewValues.Height;

            int additionalControlHeight = ( desiredListViewValuesHeight - listViewValues.Height );

            if( !linkLabelLoadAdditionalResults.Visible )
                additionalControlHeight -= _additionalResultsHeightAvailable;

            if( resizeControl )
                this.Height += additionalControlHeight + _customControlsHeight;

            listViewValues.Height = desiredListViewValuesHeight;

            labelGettingValues.Visible = false;
            listViewValues.Visible = true;
        }

        private async Task LoadResultsAsync()
        {
            listViewValues.SuspendLayout();

            StartLogicControllingChecks();

            // clear the values (if necessary)
            if( _additionalResultsLiewViewItem == null )
                listViewValues.Items.Clear();

            // remove the additional rows item if it was previously added
            else
            {
                listViewValues.Items.Remove(_additionalResultsLiewViewItem);
                _additionalResultsLiewViewItem = null;
            }

            // run the query
            int initialNumberRows = _filter.ForceRetrieveAllRows ? Int32.MaxValue :
                _controller.Settings.FilterQueryNumberRows;

            var values = await Task.Run(() => _dbQuery.GetResults(initialNumberRows));
            bool additionalResultsAvailable = ( _dbQuery.AdditionalResultsAvailable == true );

            // add the values
            foreach( var valueText in _filter.ProcessPossibleValues(values) )
                AddValue(valueText);

            // add the additional rows item if needed
            if( additionalResultsAvailable )
                _additionalResultsLiewViewItem = AddValue("<Other Values>");

            else
            {
                _dbQuery = null;

                // if no additional rows are available, resize the checked list box to use the space that the label used
                if( linkLabelLoadAdditionalResults.Visible )
                    listViewValues.Height += _additionalResultsHeightAvailable;
            }

            StopLogicControllingChecks(false);

            listViewValues.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);

            listViewValues.ResumeLayout();

            linkLabelLoadAdditionalResults.Visible = additionalResultsAvailable;
        }

        protected void ReplaceResults(List<string> valueTexts)
        {
            listViewValues.SuspendLayout();

            StartLogicControllingChecks();

            listViewValues.Items.Clear();

            // add the values
            foreach( var valueText in valueTexts )
                AddValue(valueText);

            StopLogicControllingChecks();

            listViewValues.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);

            listViewValues.ResumeLayout();
        }

        private async void linkLabelLoadAdditionalResults_LinkClicked(object sender,LinkLabelLinkClickedEventArgs e)
        {
            await LoadResultsAsync();
        }

        private void MenuItem_SelectClick(object sender,EventArgs e)
        {
            var menuItem = (MenuItem)sender;
            ProcessClick((ClickType)menuItem.Tag);
        }

        private void MenuItem_ViewSqlQueryClick(object sender,EventArgs e)
        {
            _controller.UpdateQuerySql(_filter.GetPossibleValuesSqlQuery(),false);
        }

        private void listViewValues_ItemChecked(object sender,ItemCheckedEventArgs e)
        {
            if( !_logicControllingChecks )
                StopLogicControllingChecks();
        }

        private void StartLogicControllingChecks()
        {
            _logicControllingChecks = true;
        }

        private void StopLogicControllingChecks(bool runCompletedActions = true)
        {
            _logicControllingChecks = false;

            if( runCompletedActions )
                _controller.RefreshFilters();
        }

        internal string GetWhereSql()
        {
            // if the values haven't been retrieved, then nothing in this area has been filtered
            if( !_valuesRetrieved )
                return null;

            int checkedValues = listViewValues.CheckedItems.Count;

            // if nothing is unchecked, then there is no reason to filter
            if( _filter.FilterIsShowingAllRows && checkedValues == listViewValues.Items.Count )
                return null;

            // if nothing is checked, then there should be no results
            else if( checkedValues == 0 )
                return "0";

            // by default, if most values are checked, negate the WHERE instead of having to
            // list a majority of the values
            bool negateWhere = ( checkedValues > ( listViewValues.Items.Count / 2 ) );

            // however, if the additional rows item is displayed, we must negate based on its selection
            if( _additionalResultsLiewViewItem != null )
                negateWhere = _additionalResultsLiewViewItem.Checked;

            // alternatively, if not all rows are being shown, then it is not possible to do the negation
            if( !_filter.FilterIsShowingAllRows )
                negateWhere = false;

            var sb = new StringBuilder();

            if( negateWhere )
                sb.Append("NOT ");

            sb.Append("( ");

            bool firstWhere = true;

            foreach( ListViewItem lvi in listViewValues.Items )
            {
                if( lvi == _additionalResultsLiewViewItem )
                    continue;

                if( lvi.Checked != negateWhere )
                {
                    if( !firstWhere )
                        sb.Append(" OR ");

                    sb.Append("( ");
                    sb.Append(_filter.GetWhereSql((int)lvi.Tag));
                    sb.Append(" )");

                    firstWhere = false;
                }
            }

            sb.Append(" )");

            return sb.ToString();
        }

        internal virtual void ResetFilter()
        {
            if( !_valuesRetrieved )
                return;

            StartLogicControllingChecks();

            foreach( ListViewItem lvi in listViewValues.Items )
                lvi.Checked = true;

            StopLogicControllingChecks(false);
        }

        internal void AddCustomControl(Control customControl)
        {
            // put the custom control where listViewValues is and then move things down
            customControl.Anchor = AnchorStyles.Left | AnchorStyles.Top | AnchorStyles.Right;
            customControl.Width = listViewValues.Width;
            customControl.Location = listViewValues.Location;

            const int MarginSize = 5;
            int movingHeight = customControl.Height + MarginSize;

            listViewValues.Location = new Point(listViewValues.Location.X,listViewValues.Location.Y + movingHeight);

            _customControlsHeight += movingHeight;

            this.Controls.Add(customControl);
        }
    }
}
