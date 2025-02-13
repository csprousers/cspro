using System;
using System.Windows.Forms;

namespace Operator_Statistics_Viewer
{
    public partial class SpecifyOperationStatisticsForm : Form
    {
        public bool UseParameters { get { return checkBoxUseParameters.Checked; } }
        public DateTime DesiredEndDate { get { return dateTimePickerEndDate.Value; } }
        public int ExpectedCases { get { return Int32.Parse(textBoxCases.Text); } }
        
        public SpecifyOperationStatisticsForm(DateTime operationDesiredEndDate,int operationExpectedCases)
        {
            InitializeComponent();

            checkBoxUseParameters.Checked = operationExpectedCases > 0;
            checkBoxUseParameters_CheckedChanged(null,null);

            if( checkBoxUseParameters.Checked )
            {
                dateTimePickerEndDate.Value = operationDesiredEndDate;
                textBoxCases.Text = operationExpectedCases.ToString();
            }
        }

        private void textBoxCases_TextChanged(object sender,EventArgs e)
        {
            int intVal;
            buttonOK.Enabled = Int32.TryParse(textBoxCases.Text,out intVal) && intVal > 0;
        }

        private void checkBoxUseParameters_CheckedChanged(object sender,EventArgs e)
        {
            dateTimePickerEndDate.Enabled = checkBoxUseParameters.Checked;
            textBoxCases.Enabled = checkBoxUseParameters.Checked;

            if( checkBoxUseParameters.Checked )
                textBoxCases_TextChanged(null,null);

            else
                buttonOK.Enabled = true;
        }
    }
}
