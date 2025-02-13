using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace PFF_Editor
{
    public partial class AddParameterForm : Form
    {
        Dictionary<string,string> map;
        bool makeUppercase;

        public AddParameterForm(string category,Dictionary<string,string> parameters,bool parameterCaseSensitive = false)
        {
            InitializeComponent();

            map = parameters;
            this.makeUppercase = !parameterCaseSensitive;
            labelCategory.Text = category + " Name";
        }

        private void buttonOK_Click(object sender,EventArgs e)
        {
            string category = textBoxCategory.Text.Trim();
            
            if( makeUppercase )
                category = category.ToUpper();

            category = category.Replace(" ",""); // replace any spaces

            if( category.Length == 0 )
                MessageBox.Show("You must enter a valid category.");

            else if( map.ContainsKey(category) )
                MessageBox.Show(String.Format("{0} has already been added.",category));

            else
            {
                map[category] = "";
                DialogResult = DialogResult.OK;
                Close();
            }
        }
    }
}
