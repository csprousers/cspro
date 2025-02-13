using System.Windows.Forms;

namespace WinFormsShared
{
    public partial class LoginDialog : Form
    {
        public LoginDialog()
        {
            InitializeComponent();
        }

        public bool ShowError
        {
            get { return labelLoginError.Visible; }
            set { labelLoginError.Visible = value; }
        }

        public string Username
        {
            get { return textBoxUsername.Text; }
        }

        public string Password
        {
            get { return textBoxPassword.Text; }
        }
    }
}
