using System;
using System.Windows.Forms;

namespace ToolbarCreator
{
    partial class MainForm : Form
    {
        public MainForm()
        {
            InitializeComponent();
        }

        private void textBoxInputs_TextChanged(object sender, EventArgs e)
        {
            try
            {
                textBoxOutput.Text = Creator.GetOutputFilename(textBoxInputs.Text);
            }

            catch( Exception exception )
            {
                textBoxOutput.Text = "Fix the input: " + exception.Message;
            }
        }

        private void buttonCreate_Click(object sender, EventArgs e)
        {
            try
            {
                var creator = new Creator(textBoxInputs.Text, textBoxOutput.Text);
                creator.Create();
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }
        }
    }
}
