using System.Windows.Forms;

namespace DataViewer
{
    partial class DataSummaryForm : Form, ICaseProcessingForm
    {
        private CSPro.Data.DataViewerHtmlProvider _htmlProvider;

        public DataSummaryForm(CSPro.Data.DataViewerController controller)
        {
            InitializeComponent();

            _htmlProvider = new CSPro.Data.DataViewerHtmlProvider(controller, null);

            InitializeAsync();
        }

        private async void InitializeAsync()
        {
            await WebView2Helper.Initialize(webView);
            
            webView.Source = _htmlProvider.CreateDataSummaryUri();
        }

        // none of the interface's case features are used
        public CSPro.Data.Case CurrentCase { get { return null; } }
        public void RefreshCase(CSPro.Data.Case data_case) { }
        public void UpdateCaseFromRepository(CSPro.Data.Case data_case) { }
        public void RefreshSettings() { }
    }
}
