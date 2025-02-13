using System;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using System.Web;
using System.Windows.Forms;
using Microsoft.Web.WebView2.Core;
using Newtonsoft.Json;
using CSPro.ParadataViewer;

namespace ParadataViewer
{
    class LocationMapperForm : QueryForm
    {
        private MappingSharedHtmlLocalFileServer localFileServer;
        private Microsoft.Web.WebView2.WinForms.WebView2 webView;
        private bool webViewHtmlSet;

        internal LocationMapperForm(Controller controller) : base(controller)
        {
            SetupControl();
        }

        private async void SetupControl()
        {
            panelControls.Visible = false;
            splitterControls.Visible = false;
            
            webView = new Microsoft.Web.WebView2.WinForms.WebView2();
            await WinFormsShared.WebView2Initializer.Initialize(webView);
            WinFormsShared.WebView2Initializer.ApplyDefaultSettings(webView, true);

            webView.NavigationCompleted += webView_NavigationCompleted;
            webView.WebMessageReceived += WebView_WebMessageReceived;

            panelResults.Controls.Add(webView);
            webView.Dock = DockStyle.Fill;

            localFileServer = new MappingSharedHtmlLocalFileServer();
            webView.Source = new Uri(localFileServer.GetProjectUrl("paradata-viewer-location-mapper.html"));
        }

        private void webView_NavigationCompleted(object sender, CoreWebView2NavigationCompletedEventArgs e)
        {
            webViewHtmlSet = true;
        }

        protected override bool QueryUsesFilters { get { return true; } }

        protected override string GetSqlQuery()
        {
            _sql =
                $"SELECT {N.Table.Event}.{N.Column.Id}, {N.Column.GpsAction}, {N.Column.GpsReadingLatitude}, {N.Column.GpsReadingLongitude}, {N.Column.GpsReadingAltitude} " +
                $"FROM {N.Table.GpsEvent} " +
                $"JOIN {N.Table.Event} ON {N.Table.GpsEvent}.{N.Column.Id} = {N.Table.Event}.{N.Column.Id} " +
                $"JOIN {N.Table.GpsReadingInstance} ON {N.Table.GpsEvent}.{N.Table.GpsReadingInstance} = {N.Table.GpsReadingInstance}.{N.Column.Id} " +
                $"WHERE {N.Column.GpsReadingLatitude} IS NOT NULL AND {N.Column.GpsReadingLongitude} IS NOT NULL;";

            _sql = _controller.ModifyQueryForFilters(_sql);

            return _sql;
        }

        protected override bool IsTabularQuery { get { return true; } }

        protected override async Task RefreshQueryAsync()
        {
            // wait until the web view is initialized
            while( !webViewHtmlSet )
                await Task.Delay(5);

            _dbQuery = await _controller.CreateQueryAsync(GetSqlQuery());
            await base.RefreshQueryAsync();
        }

        protected override void HandleResults(int startingRow)
        {
            // clear any existing points in the case that the results were modified by a filter
            if( startingRow == 0 )
                webView.ExecuteScriptAsync("clearMarkers();");

            for( int i = startingRow; i < _rows.Count; ++i )
            {
                var row = _rows[i];

                long id = (long)(double)row[0];
                string backgroundReading = ( (double)row[1] == 4 ) ? "true" : "false";
                double latitude = (double)row[2];
                double longitude = (double)row[3];
                double? altitude = (double?)row[4];

                if( altitude.HasValue )
                    webView.ExecuteScriptAsync($"addMarker({latitude}, {longitude}, {id}, {backgroundReading}, {altitude});");

                else
                    webView.ExecuteScriptAsync($"addMarker({latitude}, {longitude}, {id}, {backgroundReading});");
                
            }

            webView.ExecuteScriptAsync("fitPoints();");
        }

        protected override string SaveLabelText { get { return "Save map image"; } }

        protected override async void linkLabelSave_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            var sfd = new SaveFileDialog();
            sfd.Title = "Save As";
            sfd.Filter = "PNG (*.png)|*.png|JPEG (*.jpg)|*.jpg|All Files (*.*)|*.*";

            if( sfd.ShowDialog() != DialogResult.OK )
                return;

            try
            {
                string extension = Path.GetExtension(sfd.FileName).ToLower();
                var imageFormat = CoreWebView2CapturePreviewImageFormat.Png;

                if( extension == ".jpg" )
                    imageFormat = CoreWebView2CapturePreviewImageFormat.Jpeg;

                else if( extension != ".png" )
                    throw new Exception($"The image format with the extension {extension} is not supported.");

                using( var file = File.Create(sfd.FileName) )
                    await webView.CoreWebView2.CapturePreviewAsync(imageFormat, file);
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }
        }

        // classes for communicating back and forth with JavaScript
        class Action
        {
            public string action { get; set; }
            public long dbId { get; set; }
            public long? markerId { get; set; }
        }

        class PointDetails
        {
            public long dbId { get; set; }
            public string time { get; set; }
            public string username { get; set; }
            public string operatorId { get; set; }
            public string gpsAction { get; set; }
            public double? requestDuration { get; set; }
            public double? requestAccuracy { get; set; }
            public double? readingDuration { get; set; }
            public double? latitude { get; set; }
            public double? longitude { get; set; }
            public double? altitude { get; set; }
            public double? satellites { get; set; }
            public double? accuracy { get; set; }
        }

        private void WebView_WebMessageReceived(object sender, CoreWebView2WebMessageReceivedEventArgs e)
        {
            var action = JsonConvert.DeserializeObject<Action>(e.TryGetWebMessageAsString());

            if( action.action == "markerClick" )
            {
                GetLocationDetails(action);
            }

            else
            {
                bool copyCoordinates = ( action.action == "copyCoordinates" );

                if( copyCoordinates || action.action == "openGoogleMaps" )
                    CopyCoordinatesOrOpenInGoogleMaps(action, copyCoordinates);
            }
        }

        private async void GetLocationDetails(Action action)
        {
            string sql =
                $"SELECT {_controller.GetConvertedTimestampColumnName(N.Column.EventTime)}, {N.Column.DeviceUsername}, {N.Column.OperatoridOperatoridInfo}, {N.Column.GpsAction}, " +
                $"{N.Column.GpsReadRequestMaxReadDuration}, {N.Column.GpsReadRequestDesiredAccuracy}, {N.Column.GpsReadRequestReadDuration}, " +
                $"{N.Column.GpsReadingLatitude}, {N.Column.GpsReadingLongitude}, {N.Column.GpsReadingAltitude}, {N.Column.GpsReadingSatellites}, {N.Column.GpsReadingAccuracy} " +
                $"FROM {N.Table.GpsEvent} " +
                $"JOIN {N.Table.Event} ON {N.Table.GpsEvent}.{N.Column.Id} = {N.Table.Event}.{N.Column.Id} " +
                $"LEFT JOIN {N.Table.GpsReadRequestInstance} ON {N.Table.GpsEvent}.{N.Table.GpsReadRequestInstance} = {N.Table.GpsReadRequestInstance}.{N.Column.Id} " +
                $"JOIN {N.Table.GpsReadingInstance} ON {N.Table.GpsEvent}.{N.Table.GpsReadingInstance} = {N.Table.GpsReadingInstance}.{N.Column.Id} " +
                $"LEFT JOIN {N.Table.ApplicationInstance} ON {N.Table.Event}.{N.Table.ApplicationInstance} = {N.Table.ApplicationInstance}.{N.Column.Id} " +
                $"LEFT JOIN {N.Table.DeviceInfo} ON {N.Table.ApplicationInstance}.{N.Table.DeviceInfo} = {N.Table.DeviceInfo}.{N.Column.Id} " +
                $"LEFT JOIN {N.Table.SessionInstance} ON {N.Table.Event}.{N.Table.SessionInstance} = {N.Table.SessionInstance}.{N.Column.Id} " +
                $"LEFT JOIN {N.Table.SessionInfo} ON {N.Table.SessionInstance}.{N.Table.SessionInfo} = {N.Table.SessionInfo}.{N.Column.Id} " +
                $"LEFT JOIN {N.Table.OperatoridInfo} ON {N.Table.SessionInfo}.{N.Table.OperatoridInfo} = {N.Table.OperatoridInfo}.{N.Column.Id} " +
                $"WHERE {N.Table.GpsEvent}.{N.Column.Id} = {action.dbId};";

            var values = ( await _controller.ExecuteQueryAsync(sql) ).FirstOrDefault();

            if( values == null )
                return;

            Func<int, double?> getNullableDouble = x => ( values[x] is double ) ? (double?)values[x] : null;

            Func<int, string> getGpsAction = x =>
            {
                double? gps_action = getNullableDouble(x);
                return ( gps_action == 2 ) ? "Get New Reading" :
                       ( gps_action == 3 ) ? "Save Previous Reading" :
                       ( gps_action == 4 ) ? "Store Background Reading" :
                                             String.Empty;
            };

            var pointDetails = new PointDetails()
            {
                dbId = action.dbId,
                time = (string)values[0],
                username = (string)values[1],
                operatorId = (string)values[2],
                gpsAction = getGpsAction(3),
                requestDuration = getNullableDouble(4),
                requestAccuracy = getNullableDouble(5),
                readingDuration = getNullableDouble(6),
                latitude = getNullableDouble(7),
                longitude = getNullableDouble(8),
                altitude = getNullableDouble(9),
                satellites = getNullableDouble(10),
                accuracy = getNullableDouble(11)
            };

            string pointDetailsJson = JsonConvert.SerializeObject(pointDetails);
            await webView.ExecuteScriptAsync($"addMarkerDetails({action.markerId}, \"{HttpUtility.JavaScriptStringEncode(pointDetailsJson)}\");");
        }

        private async void CopyCoordinatesOrOpenInGoogleMaps(Action action, bool copyCoordinates)
        {
            string sql =
                $"SELECT {N.Column.GpsReadingLatitude}, {N.Column.GpsReadingLongitude} " +
                $"FROM {N.Table.GpsEvent} " +
                $"JOIN {N.Table.GpsReadingInstance} ON {N.Table.GpsEvent}.{N.Table.GpsReadingInstance} = {N.Table.GpsReadingInstance}.{N.Column.Id} " +
                $"WHERE {N.Table.GpsEvent}.{N.Column.Id} = {action.dbId};";

            var values = ( await _controller.ExecuteQueryAsync(sql) ).FirstOrDefault();

            if( values == null )
                return;

            double latitude = (double)values[0];
            double longitude = (double)values[1];

            if( copyCoordinates )
                Clipboard.SetText($"{latitude}, {longitude}");

            else
                System.Diagnostics.Process.Start($"https://maps.google.com/maps?q={latitude},{longitude}");
        }
    }
}
