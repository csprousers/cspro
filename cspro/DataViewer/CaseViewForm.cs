using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace DataViewer
{
    partial class CaseViewForm : Form, ICaseProcessingForm
    {
        private CSPro.Data.DataViewerController _controller;
        private CSPro.Data.DataViewerHtmlProvider _htmlProvider;

        public CSPro.Data.Case CurrentCase { get; private set; }

        public bool DefaultViewer { get; private set; }

        public CaseViewForm(CSPro.Data.DataViewerController controller, bool default_viewer, CSPro.Data.Case data_case)
        {
            InitializeComponent();

            _controller = controller;

            DefaultViewer = default_viewer;

            InitializeAsync(data_case);
        }

        async void InitializeAsync(CSPro.Data.Case data_case)
        {
            await WebView2Helper.Initialize(webView);

            RefreshCase(data_case, false);
        }

        public void RefreshCase(CSPro.Data.Case data_case)
        {
            RefreshCase(data_case, true);
        }

        public void UpdateCaseFromRepository(CSPro.Data.Case data_case)
        {
            // if a case that is being viewed has been deleted, close the form
            if( data_case == null )
                Close();

            else
                RefreshCase(data_case, false);
        }

        public void RefreshSettings()
        {
            RefreshHtml();
        }

        private void RefreshCase(CSPro.Data.Case data_case, bool ignore_if_not_default_viewer)
        {
            if( ignore_if_not_default_viewer && !DefaultViewer )
                return;

            CurrentCase = data_case;
            this.Text = CurrentCase.KeyForSingleLineDisplay;

            RefreshHtml();
        }

        private void RefreshHtml()
        {
            _htmlProvider = new CSPro.Data.DataViewerHtmlProvider(_controller, CurrentCase);

            webView.Source = _htmlProvider.CreateCaseViewUri();
        }

        private void webView2_NavigationStarting(object sender, Microsoft.Web.WebView2.Core.CoreWebView2NavigationStartingEventArgs e)
        {
            try
            {
                var uri = new Uri(e.Uri);

                if( uri.Authority.Equals("binary-data-access", System.StringComparison.InvariantCultureIgnoreCase) )
                {
                    // there should be three segments
                    var segments = uri.Segments.Select(x => x.TrimEnd('/'))
                                      .Where(x => x.Length > 0)
                                      .Select(x => System.Uri.UnescapeDataString(x))
                                      .ToList();
                
                    if( segments.Count == 3 )
                    {
                        e.Cancel = true;

                        // run the task in the UI thread
                        var task = new Task(() => ProcessBinaryDataAccess(segments[0], segments[1], segments[2]));
                        task.Start(TaskScheduler.FromCurrentSynchronizationContext());
                    }
                }
            }

            catch { }
        }

        private void ProcessBinaryDataAccess(string case_item_identifier, string suggested_filename, string action)
        {
            try
            {
                if( action == "open" )
                {
                    string temporary_filename = _controller.GetTemporaryBinaryDataFile(CurrentCase,
                        case_item_identifier, suggested_filename);

                    var file_opener = new Process();
                    file_opener.StartInfo.FileName = "explorer";
                    file_opener.StartInfo.Arguments = $"\"{temporary_filename}\"";
                    file_opener.Start();
                }

                else if( action == "save" )
                {
                    var save_file_dialog = new SaveFileDialog();
                    save_file_dialog.Title = "Save File As";
                    save_file_dialog.FileName = suggested_filename;
                    save_file_dialog.DefaultExt = Path.GetExtension(suggested_filename);

                    if( save_file_dialog.ShowDialog() != DialogResult.OK )
                        return;

                    _controller.SaveBinaryData(CurrentCase, case_item_identifier, save_file_dialog.FileName);
                }
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }
        }
    }
}
