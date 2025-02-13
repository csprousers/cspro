using System;
using System.Windows.Forms;

namespace Excel2CSPro
{
    partial class ErrorDisplayForm : Form
    {
        public ErrorDisplayForm(string text)
        {
            InitializeComponent();

            // if the text only comes in with \n instead of \r\n, add the carriage returns
            for( int newline_pos = 0; ( newline_pos = text.IndexOf('\n', newline_pos) ) >= 0; ++newline_pos )
            {
                if( newline_pos > 0 && text[newline_pos - 1] != '\r' )
                {
                    text = text.Insert(newline_pos, "\r");
                    ++newline_pos;
                }
            }

            textBox.Text = text;
        }

        private void buttonCopyToClipboard_Click(object sender, EventArgs e)
        {
            Clipboard.SetText(textBox.Text);
        }
    }
}
