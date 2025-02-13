using System;
using System.Drawing;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;
using CSPro.ParadataViewer;
using System.Windows.Forms.DataVisualization.Charting;
using System.IO;

namespace ParadataViewer
{
    partial class ReportViewerForm : TableForm
    {
        private ReportViewerControl _reportViewerControl;
        private ReportQuery _reportQuery;
        private ReportType _reportType;
        private string[] _columnLabels;
        private string[] _columnFormats;
        private int[] _timestampColumnIndices;
        private Chart _chart;
        private int[] _lastProcessedNumberRows;

        internal ReportViewerForm(Controller controller) : base(controller)
        {
            this.Text = "Report Viewer";

            panelControls.Width = 230;
            panelControls.Dock = DockStyle.Left;
            splitterControls.Dock = DockStyle.Left;

            panelResults.BackColor = SystemColors.AppWorkspace;

            _reportType = ReportType.Table;

            this.Shown += ReportViewerForm_Shown;
        }

        private void ReportViewerForm_Shown(object sender,EventArgs e)
        {
            try
            {
                // the reports are loaded by the control, and the loading routine can throw an exception
                _reportViewerControl = new ReportViewerControl(this,_controller);
                _reportViewerControl.Dock = DockStyle.Fill;
                panelControls.Controls.Add(_reportViewerControl);
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
                Close();
            }
        }

        protected override bool IsTabularQuery
        {
            get
            {
                return ( _reportQuery != null ) ? _reportQuery.IsTabularQuery : true;
            }
        }

        protected override bool QueryUsesFilters
        {
            get
            {
                return ( _reportQuery != null ) ? _controller.QueryCanUseFilters(_reportQuery.SqlQuery) : true;
            }
        }

        protected override string GetSqlQuery()
        {
            if( _reportQuery != null )
                _sql = _controller.ModifyQueryForFilters(_reportQuery.SqlQuery);

            return _sql;
        }

        protected override async Task RefreshQueryAsync()
        {
            if( _reportQuery != null )
            {
                _dbQuery = await _controller.CreateQueryAsync(_sql);

                // calculate the column labels
                _columnLabels = new string[_dbQuery.ColumnCount];
                _columnFormats = new string[_dbQuery.ColumnCount];

                for( int i = 0; i < _columnLabels.Length; i++ )
                {
                    string label = null;

                    if( i < _reportQuery.Columns.Count )
                    {
                        if( !String.IsNullOrWhiteSpace(_reportQuery.Columns[i].Label) )
                            label = _reportQuery.Columns[i].Label;

                        if( !String.IsNullOrWhiteSpace(_reportQuery.Columns[i].Format) )
                            _columnFormats[i] = _reportQuery.Columns[i].Format;
                    }

                    _columnLabels[i] = label ?? _dbQuery.ColumnNames[i];
                }

                // determine which columns are timestamps
                _timestampColumnIndices = _reportQuery.Columns
                    .Select((v,i) => new { Index = i, Value = v })
                    .Where(x => x.Value.IsTimestamp)
                    .Select(x => x.Index)
                    .ToArray();

                // setup the table
                SetupTableColumns(_columnLabels,_columnFormats);

                // remove the chart and reset the starting row information
                RemoveExistingChart();
                _lastProcessedNumberRows = new int[2];

                await base.RefreshQueryAsync();
            }
        }

        private bool ShowingTable { get { return ( _reportType == ReportType.Table ); } }
        private bool ShowingChart { get { return ( _reportType == ReportType.Chart ); } }
        private int lastProcessedNumberRowsIndex { get { return ( ShowingTable ? 0 : 1 ); } }

        protected override void HandleResults(int startingRow)
        {
            if( _controller.Settings.QueryOptions.ConvertTimestamps )
                ConvertTimestamps(startingRow);

            ProcessResults();
        }

        private void ProcessResults()
        {
            linkLabelSave.Text = SaveLabelText;

            _dgvResults.Visible = ShowingTable;

            if( _chart != null )
                _chart.Visible = ShowingChart;

            if( ShowingTable )
                base.HandleResults(_lastProcessedNumberRows[lastProcessedNumberRowsIndex]);

            else
                HandleChartResults(_lastProcessedNumberRows[lastProcessedNumberRowsIndex]);

            _lastProcessedNumberRows[lastProcessedNumberRowsIndex] = _rows.Count;
        }

        protected override bool ForceQueryRefreshAfterOptionsChange(QueryOptions previousQueryOptions)
        {
            bool forceQueryRefresh = false;

            if( previousQueryOptions.ConvertTimestamps != _controller.Settings.QueryOptions.ConvertTimestamps )
                forceQueryRefresh = ( _timestampColumnIndices != null && _timestampColumnIndices.Length > 0 );

            return forceQueryRefresh;
        }

        private void ConvertTimestamps(int startingRow)
        {
            lock( _rows )
            {
                if( _rows.Count == 0 )
                    return;

                int numberColumns = _rows.First().Length;

                foreach( var index in _timestampColumnIndices )
                {
                    if( index >= numberColumns )
                        return;

                    for( int i = startingRow; i < _rows.Count; i++ )
                    {
                        if( _rows[i][index] is double )
                            _rows[i][index] = Helper.FormatTimestamp(_controller.Settings.TimestampFormatter,(double)_rows[i][index]);
                    }
                }
            }
        }

        private void RemoveExistingChart()
        {
            if( _chart != null )
            {
                panelResults.Controls.Remove(_chart);
                _chart = null;
            }
        }

        private void HandleChartResults(int startingRow)
        {
            const float TitleFontMultiplier = 1.6f;
            const float AxisLabelFontMultiplier = 1.2f;

            // the startingRow will be ignored and the chart will always be created from scratch;
            // this should be fine because charts should be created from summary data, in which case
            // this will only get called once
            RemoveExistingChart();

            _chart = new Chart()
            {
                Dock = DockStyle.Fill
            };

            var title = _chart.Titles.Add(_reportQuery.Description);
            title.Font = new Font(title.Font.FontFamily,title.Font.Size * TitleFontMultiplier,FontStyle.Bold);

            panelResults.Controls.Add(_chart);

            // the chart can only be created if there are at least two columns of data
            if( _columnLabels.Length < 2 )
                return;

            _chart.SuspendLayout();

            var chartArea = new ChartArea();
            _chart.ChartAreas.Add(chartArea);

            var series = new Series
            {
                Color = Color.SteelBlue,
                IsVisibleInLegend = false,
                IsXValueIndexed = true,
                ChartType = SeriesChartType.Column,
            };

            _chart.Series.Add(series);

            // the x-axis values come from the first column of results
            chartArea.AxisX.Title = _columnLabels[0];
            chartArea.AxisX.TitleFont = new Font(chartArea.AxisX.TitleFont.FontFamily,chartArea.AxisX.TitleFont.Size * AxisLabelFontMultiplier,FontStyle.Bold);
            chartArea.AxisX.Interval = 1;
            chartArea.AxisX.MajorGrid.Enabled = false;

            // the y-axis values come from the second column of results
            chartArea.AxisY.Title = _columnLabels[1];
            chartArea.AxisY.TitleFont = chartArea.AxisX.TitleFont;

            // add the data
            for( int i = 0; i < _rows.Count; i++ )
            {
                object valueX = _rows[i][0];
                string labelX =
                    ( valueX is string ) ? ((string)valueX) :
                    ( valueX is double ) ? ((double)valueX).ToString() :
                    "";

                object valueY = _rows[i][1];
                double frequencyY =
                    ( valueY is double ) ? (double)valueY :
                    0;

                int pos = series.Points.AddXY(i,valueY);
                series.Points[pos].AxisLabel = labelX;
            }

            _chart.ResumeLayout();
        }

        private void SaveChart()
        {
            try
            {
                if( _chart == null || _rows == null || _rows.Count == 0 )
                    throw new Exception("There are no rows to save");

                var sfd = new SaveFileDialog();
                sfd.Title = "Save As";
                sfd.Filter = "PNG (*.png)|*.png|JPEG (*.jpg;*.jpeg)|*.jpg;*.jpeg)|Bitmap (*.bmp)|*.bmp|" +
                    "GIF (*.gif)|*.gif|TIFF (*.tif;*.tiff)|*.tif;*.tiff|All Files (*.*)|*.*";

                if( sfd.ShowDialog() != DialogResult.OK )
                    return;

                string extension = Path.GetExtension(sfd.FileName).ToLower();

                var chartImageFormat =
                    extension.Equals(".png") ? ChartImageFormat.Png :
                    ( extension.Equals(".jpg") || extension.Equals(".jpeg") ) ? ChartImageFormat.Jpeg :
                    extension.Equals(".bmp") ? ChartImageFormat.Bmp :
                    extension.Equals(".gif") ? ChartImageFormat.Gif :
                    ( extension.Equals(".tif") || extension.Equals(".tiff") ) ? ChartImageFormat.Tiff :
                    throw new Exception("Unsupported image format " + extension);

                    _chart.SaveImage(sfd.FileName,chartImageFormat);
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }
        }

        protected override string SaveLabelText
        {
            get
            {
                return ( _reportType == ReportType.Table ) ? base.SaveLabelText : "Save chart image";
            }
        }

        protected override async void linkLabelSave_LinkClicked(object sender,LinkLabelLinkClickedEventArgs e)
        {
            if( _reportType == ReportType.Table )
                await SaveResultsToExcelPromptAsync(_columnLabels, _columnFormats);

            else
                SaveChart();
        }

        internal async Task DisplayReportAsync(ReportQuery reportQuery,ReportType reportType)
        {
            try
            {
                _reportQuery = reportQuery;
                _reportType = reportType;

                await ForceRefreshFormAsync();
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }
        }

        internal void ChangeReportType(ReportType reportType)
        {
            if( _reportType != reportType )
            {
                _reportType = reportType;
                ProcessResults();
            }
        }
    }
}
