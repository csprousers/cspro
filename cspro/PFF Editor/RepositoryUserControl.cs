using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;

namespace PFF_Editor
{
    public partial class RepositoryUserControl : UserControl
    {
        public delegate void SetModifiedFunction();

        private TextBox _textBox;
        private string _initialFilename;
        private SetModifiedFunction _setModifiedFunction;

        private Dictionary<CSPro.Util.DataRepositoryType, int> _typeIndexMap;

        public RepositoryUserControl(TextBox textBox, string filename, SetModifiedFunction setModifiedFunction)
        {
            InitializeComponent();

            _textBox = textBox;
            _textBox.Text = filename;
            _initialFilename = filename;
            _setModifiedFunction = setModifiedFunction;

            // add the repository types
            var type_name_map = new Dictionary<string, CSPro.Util.DataRepositoryType>();

            for( CSPro.Util.DataRepositoryType type = CSPro.Util.DataRepositoryType.Null; type <= CSPro.Util.DataRepositoryType.Stata; ++type )
            {
                string type_name = CSPro.Util.ConnectionString.GetDataRepositoryTypeDisplayText(type);
                repositoryComboBox.Items.Add(type_name);
                type_name_map[type_name] = type;
            }

            _typeIndexMap = new Dictionary<CSPro.Util.DataRepositoryType, int>();

            for( int i = 0; i < repositoryComboBox.Items.Count; ++i )
                _typeIndexMap[type_name_map[repositoryComboBox.Items[i].ToString()]] = i;

            UpdateRepositoryType();
        }

        public void UpdateRepositoryType()
        {
            CSPro.Util.DataRepositoryType type = CSPro.Util.DataRepositoryType.Null;

            if( !string.IsNullOrWhiteSpace(_textBox.Text) )
                type = new CSPro.Util.ConnectionString(_textBox.Text).Type;

            repositoryComboBox.SelectedIndex = _typeIndexMap[type];
        }

        private void repositoryComboBox_SelectionChangeCommitted(object sender, EventArgs e)
        {
            CSPro.Util.DataRepositoryType type = _typeIndexMap.First(x => ( x.Value == repositoryComboBox.SelectedIndex )).Key;

            // if no filename is provided, use a default one
            string old_connection_string_text = string.IsNullOrWhiteSpace(_textBox.Text) ? ".\\data-file" : _textBox.Text;
            var old_connection_string = new CSPro.Util.ConnectionString(old_connection_string_text);

            _textBox.Text = old_connection_string.ToStringWithModifiedType(type);

            if( _initialFilename != _textBox.Text )
                _setModifiedFunction();
        }
    }
}
