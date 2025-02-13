using System;
using System.Drawing;
using System.Threading;
using System.Windows.Forms;

namespace WinFormsShared
{
    public partial class ProgressDialog : Form
    {
        public IProgress<float> ProgressPercent { get; private set; }
        public IProgress<string> ProgressMessage { get; private set; }

        private CancellationTokenSource cancellationTokenSource = new CancellationTokenSource();
        public CancellationToken CancellationToken
        {
            get
            {
                return cancellationTokenSource.Token;
            }
        }

        void OnProgressPercent(float percentComplete)
        {
            if (percentComplete == -1.0f)
            {
                progressBar.Style = ProgressBarStyle.Marquee;
            }
            else if (percentComplete >= 0)
            {
                progressBar.Value = Math.Min((int)(100 * percentComplete), 100);
                progressBar.Style = ProgressBarStyle.Blocks;
            }
        }

        void OnProgressMessage(string message)
        {
            labelProgressMessage.Text = message;
        }

        public ProgressDialog()
        {
            InitializeComponent();
            ProgressPercent = new Progress<float>(OnProgressPercent);
            ProgressMessage = new Progress<string>(OnProgressMessage);
        }

        private void buttonCancel_Click(object sender, EventArgs e)
        {
            buttonCancel.Enabled = false;
            cancellationTokenSource.Cancel();
        }

        private void ProgressDialog_Load(object sender, EventArgs e)
        {
            if (Owner != null)
                Location = new Point(Owner.Location.X + Owner.Width / 2 - Width / 2,
                    Owner.Location.Y + Owner.Height / 2 - Height / 2);
        }
    }
}
