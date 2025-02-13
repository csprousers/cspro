using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;

namespace DataViewer
{
    class CaseListingProvider
    {
        private const int NumberCaseSummariesToLoadEachRequest = 1000;

        private class ListViewItemWithComputedWidth
        {
            public ListViewItem ListViewItem;
            public bool ComputedWidth;
        }

        private class CachedListViewItemSet
        {
            public int StartIndex;
            public int EndIndex;
            public List<ListViewItemWithComputedWidth> ListViewItemWithComputedWidths;
        }

        private CSPro.Data.DataRepository _repository;
        private ListView _listViewCases;
        private Panel _panelCaseListing;
        private CSPro.Data.DataViewerSettings _settings;

        private Graphics _listViewCasesGraphics;
        private int _maxCaseLabelWidth;
        private int _maxKeyWidth;

        private List<CachedListViewItemSet> _cachedListViewItemSet;

        public CaseListingProvider(CSPro.Data.DataRepository repository, ListView list_view_cases, Panel panel_case_listing, CSPro.Data.DataViewerSettings settings)
        {
            _repository = repository;
            _listViewCases = list_view_cases;
            _panelCaseListing = panel_case_listing;
            _settings = settings;

            _cachedListViewItemSet = new List<CachedListViewItemSet>();

            if( _listViewCases.SmallImageList == null )
            {
                _listViewCases.SmallImageList = new ImageList();
                _listViewCases.SmallImageList.Images.Add(Properties.Resources.CaseComplete);
                _listViewCases.SmallImageList.Images.Add(Properties.Resources.CaseCompleteVerified);
                _listViewCases.SmallImageList.Images.Add(Properties.Resources.CasePartialAdd);
                _listViewCases.SmallImageList.Images.Add(Properties.Resources.CasePartialModify);
                _listViewCases.SmallImageList.Images.Add(Properties.Resources.CasePartialVerify);
                _listViewCases.SmallImageList.Images.Add(Properties.Resources.CaseDeleted);
            }

            _listViewCasesGraphics = _listViewCases.CreateGraphics();
            _maxCaseLabelWidth = 0;
            _maxKeyWidth = 0;
        }

        public void Refresh()
        {
            _cachedListViewItemSet.Clear();

            _listViewCases.BeginUpdate();
            _listViewCases.VirtualListSize = _repository.GetNumberCases(_settings);
            _listViewCases.SelectedIndices.Clear();
            _listViewCases.EndUpdate();
        }

        private string GetDisplayText(CSPro.Data.CaseSummary case_summary)
        {
            return _settings.ShowCaseLabels ? case_summary.CaseLabelForSingleLineDisplay :
                                              case_summary.KeyForSingleLineDisplay;
        }

        private void AdjustWidth()
        {
            const int MinimumWidth = 80;
            const int IconTextSpacingWidth = 10;

            int new_column_width = _listViewCases.SmallImageList.Images[0].Width + IconTextSpacingWidth + ( _settings.ShowCaseLabels ? _maxCaseLabelWidth : _maxKeyWidth );

            int new_width_for_list_view = Math.Max(new_column_width, MinimumWidth) + SystemInformation.VerticalScrollBarWidth;
            int panel_width_adjustment = new_width_for_list_view - _listViewCases.Width;

            _listViewCases.Columns[0].Width = new_column_width;

            _panelCaseListing.Width = _panelCaseListing.Width + panel_width_adjustment;
        }

        public void ToggleShowCaseLabels()
        {
            _listViewCases.BeginUpdate();

            foreach( var cached_list_view_item_set in _cachedListViewItemSet )
            {
                foreach( var lvi_with_computed_width in cached_list_view_item_set.ListViewItemWithComputedWidths )
                {
                    var lvi = lvi_with_computed_width.ListViewItem;
                    lvi.Text = GetDisplayText((CSPro.Data.CaseSummary)lvi.Tag);
                }
            }

            AdjustWidth();

            _listViewCases.EndUpdate();
        }

        public void RetrieveVirtualItem(object sender, RetrieveVirtualItemEventArgs e)
        {
            // check if the item is in the cache
            foreach( var cached_list_view_item_set in _cachedListViewItemSet )
            {
                if( e.ItemIndex >= cached_list_view_item_set.StartIndex && e.ItemIndex <= cached_list_view_item_set.EndIndex )
                {
                    var lvi_with_computed_width = cached_list_view_item_set.ListViewItemWithComputedWidths[e.ItemIndex - cached_list_view_item_set.StartIndex];

                    e.Item = lvi_with_computed_width.ListViewItem;

                    // adjust the width of the list view and panel if necessary
                    if( !lvi_with_computed_width.ComputedWidth )
                    {
                        var case_summary = (CSPro.Data.CaseSummary)lvi_with_computed_width.ListViewItem.Tag;

                        // the + "." is because spaces at the end will get ignored otherwise
                        int case_label_width = (int)_listViewCasesGraphics.MeasureString(case_summary.CaseLabelForSingleLineDisplay + ".", _listViewCases.Font).Width;
                        int key_width = (int)_listViewCasesGraphics.MeasureString(case_summary.KeyForSingleLineDisplay + ".", _listViewCases.Font).Width;
                        bool adjust_width = false;

                        if( case_label_width > _maxCaseLabelWidth )
                        {
                            _maxCaseLabelWidth = case_label_width;
                            adjust_width = true;
                        }

                        if( key_width > _maxKeyWidth )
                        {
                            _maxKeyWidth = key_width;
                            adjust_width = true;
                        }

                        if( adjust_width )
                            AdjustWidth();

                        lvi_with_computed_width.ComputedWidth = true;
                    }

                    return;
                }
            }

            // if not in the cache, read in more case summaries and then call this function again,
            // which should get the value from the newly read set
            CacheCaseSummaries(e.ItemIndex);
            RetrieveVirtualItem(sender, e);
        }

        private void CacheCaseSummaries(int start_index)
        {
            // instead of starting right at the start index, read in blocks
            start_index = start_index / NumberCaseSummariesToLoadEachRequest * NumberCaseSummariesToLoadEachRequest;

            var case_summaries = _repository.GetCaseSummaries(_settings, start_index, NumberCaseSummariesToLoadEachRequest);
            System.Diagnostics.Debug.Assert(case_summaries.Count > 0);

            var cached_list_view_item_set = new CachedListViewItemSet
            {
                StartIndex = start_index,
                EndIndex = start_index + case_summaries.Count - 1,
                ListViewItemWithComputedWidths = new List<ListViewItemWithComputedWidth>()
            };

            foreach( var case_summary in case_summaries )
            {
                var lvi_with_computed_width = new ListViewItemWithComputedWidth
                {
                    ListViewItem = new ListViewItem(GetDisplayText(case_summary)),
                    ComputedWidth = false
                };

                lvi_with_computed_width.ListViewItem.Tag = case_summary;

                lvi_with_computed_width.ListViewItem.ImageIndex =
                      case_summary.Deleted                                                ? 5 :
                    ( case_summary.PartialSaveMode == CSPro.Data.PartialSaveMode.Add )    ? 2 :
                    ( case_summary.PartialSaveMode == CSPro.Data.PartialSaveMode.Modify ) ? 3 :
                    ( case_summary.PartialSaveMode == CSPro.Data.PartialSaveMode.Verify ) ? 4 :
                      case_summary.Verified                                               ? 1 :
                                                                                            0;

                cached_list_view_item_set.ListViewItemWithComputedWidths.Add(lvi_with_computed_width);
            }

            _cachedListViewItemSet.Add(cached_list_view_item_set);
        }
    }
}
