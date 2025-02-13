using System;
using System.Diagnostics;
using System.Windows.Forms;
using CSPro.ParadataViewer;

namespace ParadataViewer
{
    partial class SettingsForm : Form
    {
        private Controller _controller;

        public SettingsForm(Controller controller)
        {
            InitializeComponent();

            _controller = controller;

            SetFormValues(_controller.Settings);
        }

        private void SetFormValues(Settings settings)
        {
            textBoxFilterQueryRows.Text = settings.FilterQueryNumberRows.ToString();

            textBoxGeneralQueryInitialRows.Text = settings.GeneralQueryInitialNumberRows.ToString();
            textBoxGeneralQueryMaximumRows.Text = settings.GeneralQueryMaximumNumberRows.ToString();

            textBoxTabularQueryInitialRows.Text = settings.TabularQueryInitialNumberRows.ToString();
            textBoxTabularQueryMaximumRows.Text = settings.TabularQueryMaximumNumberRows.ToString();

            textBoxTimestampConversionFormat.Text = settings.TimestampFormatter;
            textBoxTimestampConversionFormatForFilters.Text = settings.TimestampFormatterForFilters;
        }

        private void linkLabelTimestampFormatters_LinkClicked(object sender,LinkLabelLinkClickedEventArgs e)
        {
            Process.Start("http://www.cplusplus.com/reference/ctime/strftime/");
        }

        private void buttonOK_Click(object sender,EventArgs e)
        {
            try
            {
                int fr = ValidateInteger("filter rows",textBoxFilterQueryRows.Text,1);

                int ginr = ValidateInteger("general initial rows",textBoxGeneralQueryInitialRows.Text,1);
                int gmnr = ValidateInteger("general maximum rows",textBoxGeneralQueryMaximumRows.Text,ginr + 1);

                int tinr = ValidateInteger("tabular initial rows",textBoxTabularQueryInitialRows.Text,1);
                int tmnr = ValidateInteger("tabular maximum rows",textBoxTabularQueryMaximumRows.Text,tinr + 1);

                string tf = ValidateFormatter("timestamp format",textBoxTimestampConversionFormat.Text);
                string tfff = ValidateFormatter("timestamp format for filters",textBoxTimestampConversionFormatForFilters.Text);

                _controller.Settings.FilterQueryNumberRows = fr;

                _controller.Settings.GeneralQueryInitialNumberRows = ginr;
                _controller.Settings.GeneralQueryMaximumNumberRows = gmnr;

                _controller.Settings.TabularQueryInitialNumberRows = tinr;
                _controller.Settings.TabularQueryMaximumNumberRows = tmnr;

                _controller.Settings.TimestampFormatter = tf;
                _controller.Settings.TimestampFormatterForFilters = tfff;

                this.DialogResult = DialogResult.OK;
                Close();
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }
        }

        private int ValidateInteger(string setting,string value,int minValue)
        {
            int intValue;

            if( !Int32.TryParse(value,out intValue) )
                throw new Exception(String.Format("The {0} must be valid number",setting));

            if( intValue < minValue )
                throw new Exception(String.Format("The {0} must be {1} or greater",setting,minValue));

            return intValue;
        }

        private string ValidateFormatter(string setting,string value)
        {
            value = value.Trim();

            if( value.Length == 0 )
                throw new Exception(String.Format("The {0} cannot be blank",setting));

            return value;
        }

        private void textBoxTimestampConversionFormat_TextChanged(object sender,EventArgs e)
        {
            labelTimestampConversionFormat.Text = Helper.FormatTimestamp(textBoxTimestampConversionFormat.Text);
        }

        private void textBoxTimestampConversionFormatForFilters_TextChanged(object sender,EventArgs e)
        {
            labelTimestampConversionFormatForFilters.Text = Helper.FormatTimestamp(textBoxTimestampConversionFormatForFilters.Text);
        }

        private void buttonRestoreDefaults_Click(object sender,EventArgs e)
        {
            SetFormValues(new Settings());
        }
    }
}
