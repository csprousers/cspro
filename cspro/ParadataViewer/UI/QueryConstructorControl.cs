using System.Windows.Forms;

namespace ParadataViewer
{
    partial class QueryConstructorControl : UserControl
    {
        private QueryConstructorForm _queryConstructorForm;

        public QueryConstructorControl(QueryConstructorForm queryConstructorForm,string initialSql)
        {
            InitializeComponent();

            _queryConstructorForm = queryConstructorForm;

            textBoxQuerySql.Text = initialSql;
        }

        private void QueryConstructorControl_Load(object sender,System.EventArgs e)
        {
            // open the control without the initial SQL query highlighted
            textBoxQuerySql.SelectionStart = textBoxQuerySql.Text.Length;
            textBoxQuerySql.DeselectAll();

            // run the query if one has been specified
            if( textBoxQuerySql.Text.Length > 0 )
                _queryConstructorForm.ProcessNewQuery(textBoxQuerySql.Text);
        }

        private void textBoxQuerySql_KeyDown(object sender,KeyEventArgs e)
        {
            if( ( e.KeyCode == Keys.F5 ) || ( e.Control && ( e.KeyCode == Keys.Return ) ) )
            {
                e.SuppressKeyPress = true;
                _queryConstructorForm.ProcessNewQuery(textBoxQuerySql.Text);
            }
        }
    }
}
