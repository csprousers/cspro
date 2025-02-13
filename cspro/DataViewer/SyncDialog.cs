using CSPro.Util;
using System.Windows.Forms;

namespace DataViewer
{
    public partial class SyncDialog : Form
    {
        public SyncServerParameters ServerParameters { get; private set; }
        public SyncDirection Direction { get; private set; }

        public SyncDialog(SyncServerParameters server_parameters, SyncDirection direction)
        {
            InitializeComponent();

            syncServerSelector.ServerParameters = server_parameters;
            switch (direction)
            {
                case SyncDirection.GET:
                    comboBoxDirection.SelectedIndex = 0;
                    break;
                case SyncDirection.PUT:
                    comboBoxDirection.SelectedIndex = 1;
                    break;
                case SyncDirection.BOTH:
                    comboBoxDirection.SelectedIndex = 2;
                    break;
            }
        }

        private void buttonOk_Click(object sender, System.EventArgs e)
        {
            ServerParameters = syncServerSelector.ServerParameters;
            switch (comboBoxDirection.SelectedIndex) {
                case 0:
                    Direction = SyncDirection.GET;
                    break;
                case 1:
                    Direction = SyncDirection.PUT;
                    break;
                case 2:
                    Direction = SyncDirection.BOTH;
                    break;
            }
            DialogResult = DialogResult.OK;
        }
    }
}
