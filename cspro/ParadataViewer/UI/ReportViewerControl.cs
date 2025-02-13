using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;
using CSPro.ParadataViewer;

namespace ParadataViewer
{
    partial class ReportViewerControl : UserControl
    {
        private ReportViewerForm _reportViewerForm;
        private Controller _controller;
        private List<ReportQuery> _reportQueries;
        private Dictionary<string,List<ReportQuery>> _reportQueryCategoriesMap;
        private ReportQuery _selectedReportQuery;

        internal ReportViewerControl(ReportViewerForm reportViewerForm,Controller controller)
        {
            InitializeComponent();

            _reportViewerForm = reportViewerForm;
            _controller = controller;
            _reportQueries = _controller.ReportQueries;

            _controller.RestoreCheckBoxState(checkBoxListReportsAlphabetically);

            UpdateReportListing();
        }

        private void checkBoxListReportsAlphabetically_CheckedChanged(object sender,EventArgs e)
        {
            _controller.SaveCheckBoxState(checkBoxListReportsAlphabetically);
            UpdateReportListing();
        }

        internal void UpdateReportListing()
        {
            treeViewReports.SuspendLayout();

            treeViewReports.Nodes.Clear();

            if( checkBoxListReportsAlphabetically.Checked )
                UpdateReportListingAlphabetically();

            else
                UpdateReportListingCategorically();

            treeViewReports.ExpandAll();

            treeViewReports.ResumeLayout();
        }

        private void UpdateReportListingAlphabetically()
        {
            foreach( var reportQuery in _controller.ReportQueries.OrderBy(x => x.Description) )
            {
                var reportTreeNode = treeViewReports.Nodes.Add(reportQuery.Description);
                reportTreeNode.Tag = reportQuery;
            }
        }

        private void UpdateReportListingCategorically()
        {
            // categorize the reports
            if( _reportQueryCategoriesMap == null )
            {
                _reportQueryCategoriesMap = new Dictionary<string,List<ReportQuery>>();

                foreach( var reportQuery in _controller.ReportQueries.OrderBy(x => x.Description) )
                {
                    List<ReportQuery> groupReportQueries = null;
                    string grouping = reportQuery.Grouping.ToLower();

                    if( !_reportQueryCategoriesMap.TryGetValue(grouping,out groupReportQueries) )
                    {
                        groupReportQueries = new List<ReportQuery>();
                        _reportQueryCategoriesMap.Add(grouping,groupReportQueries);
                    }

                    groupReportQueries.Add(reportQuery);
                }
            }

            // add the categorized reports
            foreach( var kp in _reportQueryCategoriesMap.OrderBy(x => x.Key) )
            {
                var groupReportQueries = kp.Value;
                var categoryTreeNode = treeViewReports.Nodes.Add(groupReportQueries.First().Grouping);

                foreach( var reportQuery in groupReportQueries )
                {
                    var reportQueryTreeNode = categoryTreeNode.Nodes.Add(reportQuery.Description);
                    reportQueryTreeNode.Tag = reportQuery;
                }
            }
        }

        private void treeViewReports_NodeMouseClick(object sender,TreeNodeMouseClickEventArgs e)
        {
            // select the node on a right-click and also display the context menu
            if( e.Button == MouseButtons.Right )
            {
                treeViewReports.SelectedNode = e.Node;

                if( e.Node.Tag != null )
                {
                    contextMenuReports.Tag = e.Node.Tag;
                    contextMenuReports.Show((Control)sender,e.Location);
                }
            }
        }

        private void menuItemReportsCopyQueryName_Click(object sender,EventArgs e)
        {
            var reportQuery = (ReportQuery)contextMenuReports.Tag;
            Clipboard.SetText(reportQuery.Name);
        }

        private async void treeViewReports_AfterSelect(object sender,TreeViewEventArgs e)
        {
            if( treeViewReports.SelectedNode.Tag != null )
            {
                _selectedReportQuery = (ReportQuery)treeViewReports.SelectedNode.Tag;

                linkLabelViewAsTable.Enabled = _selectedReportQuery.CanViewAsTable;
                linkLabelViewAsChart.Enabled = _selectedReportQuery.CanViewAsChart;

                labelDisplayedReportName.Text = String.Format("Displaying {0}",_selectedReportQuery.Description);

                await _reportViewerForm.DisplayReportAsync(_selectedReportQuery,_selectedReportQuery.DefaultReportType);
            }
        }

        private void linkLabelViewAsTable_LinkClicked(object sender,LinkLabelLinkClickedEventArgs e)
        {
            _reportViewerForm.ChangeReportType(ReportType.Table);
        }

        private void linkLabelViewAsChart_LinkClicked(object sender,LinkLabelLinkClickedEventArgs e)
        {
            _reportViewerForm.ChangeReportType(ReportType.Chart);
        }
    }
}
