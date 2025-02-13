using System.Drawing;
using System.Windows.Forms;

namespace WinFormsShared
{
    partial class AboutForm : Form
    {
        public AboutForm(string applicationName, string aboutText, Icon icon)
        {
            InitializeComponent();

            this.Text = string.Format(this.Text, applicationName);

            labelName.Text = string.Format(labelName.Text, applicationName);

            labelVersion.Text = string.Format(labelVersion.Text, CSPro.Util.Versioning.DetailedString);
            labelReleaseDate.Text = string.Format(labelReleaseDate.Text, CSPro.Util.Versioning.ReleaseDateString);

            labelAbout.Text = aboutText;

            pictureBoxIcon.Image = icon.ToBitmap();
        }
    }
}
