using System.Windows.Forms;

namespace DataViewer
{
    partial class LogicHelperForm : Form, ICaseProcessingForm
    {
        private CSPro.Data.DataViewerController _controller;
        private CSPro.Data.DataViewerHtmlProvider _htmlProvider;

        public CSPro.Data.Case CurrentCase { get; private set; }

        public LogicHelperForm(CSPro.Data.DataViewerController controller, CSPro.Data.Case data_case)
        {
            InitializeComponent();

            _controller = controller;
            CurrentCase = data_case;

            InitializeAsync();
        }

        private async void InitializeAsync()
        {
            await WebView2Helper.Initialize(webView);

            RefreshHtml();
        }

        public void RefreshCase(CSPro.Data.Case data_case)
        {
            CurrentCase = data_case;
            RefreshHtml();
        }

        public void UpdateCaseFromRepository(CSPro.Data.Case data_case)
        {
            RefreshCase(data_case);
        }

        public void RefreshSettings()
        {
            RefreshHtml();
        }

        private void RefreshHtml()
        {
            _htmlProvider = new CSPro.Data.DataViewerHtmlProvider(_controller, CurrentCase);

            webView.Source = _htmlProvider.CreateLogicHelperUri();
        }
    }
}
