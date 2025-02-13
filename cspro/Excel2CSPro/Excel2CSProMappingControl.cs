using System;
using System.Collections.Generic;
using System.Windows.Forms;
using CSPro.Dictionary;
using Microsoft.Office.Interop.Excel;

namespace Excel2CSPro
{
    partial class Excel2CSProMappingControl : UserControl
    {
        private Excel2CSProMappingManager _mappingManager;
        private DictionaryRecord _record;
        private ExpandedDictionaryItem _item;
        private Dictionary<string,object> _possibleMappings;
        private Excel2CSProItemControl _parentForm;

        public Excel2CSProMappingControl(Excel2CSProItemControl parentForm,Excel2CSProMappingManager mappingManager,
            DictionaryRecord record,ExpandedDictionaryItem item = null)
        {
            InitializeComponent();

            _mappingManager = mappingManager;
            _record = record;
            _item = item;

            comboBoxMapping.BeginUpdate();

            if( _item == null ) // a record
            {
                labelLabel.Text = _record.Label;
                labelName.Text = _record.Name;

                _possibleMappings = _mappingManager.GetPossibleWorksheets();

                foreach( var kp in _possibleMappings )
                    comboBoxMapping.Items.Add(kp.Key);

                comboBoxMapping.SelectedItem = _mappingManager.GetMappedWorksheetName(_record);
            }

            else // an item
            {
                string occurrence = ( _item.Occurrence == null ) ? "" : String.Format(" ({0})",_item.Occurrence);

                labelLabel.Text = _item.Label + occurrence;
                labelName.Text = item.Name + occurrence;

                // remove the button
                this.Height -= ( buttonFillDefaultItemMappings.Location.Y + buttonFillDefaultItemMappings.Height ) -
                    ( comboBoxMapping.Location.Y + comboBoxMapping.Height );

                this.Controls.Remove(buttonResetMappings);
                this.Controls.Remove(buttonFillDefaultItemMappings);

                _possibleMappings = _mappingManager.GetPossibleColumns(_record,_item);

                foreach( var kp in _possibleMappings )
                    comboBoxMapping.Items.Add(kp.Key);

                comboBoxMapping.SelectedItem = _mappingManager.GetMappedColumn(_record,_item);
            }

            comboBoxMapping.EndUpdate();

            _parentForm = parentForm;
        }

        private void comboBoxMapping_SelectedIndexChanged(object sender,System.EventArgs e)
        {
            if( _item == null ) // a record
            {
                try
                {
                    _mappingManager.MapRecordToWorksheet(_record,(Worksheet)_possibleMappings[(string)comboBoxMapping.SelectedItem]);

                    if( _parentForm != null )
                        _parentForm.DrawItemMappings();
                }

                catch( Exception exception )
                {
                    MessageBox.Show(exception.Message);
                    comboBoxMapping.SelectedItem = _mappingManager.GetMappedWorksheetName(_record);
                }

                bool enableButtons = ( _mappingManager.GetMappedWorksheet(_record) != null );
                buttonResetMappings.Enabled = enableButtons;
                buttonFillDefaultItemMappings.Enabled = enableButtons;
            }

            else // an item
            {
                try
                {
                    _mappingManager.MapItemToColumn(_record,_item,comboBoxMapping.SelectedIndex);
                }

                catch( Exception exception )
                {
                    MessageBox.Show(exception.Message);
                    comboBoxMapping.SelectedItem = _mappingManager.GetMappedColumn(_record,_item);
                }
            }
        }

        private void buttonResetMappings_Click(object sender,EventArgs e)
        {
            _mappingManager.ResetRecordItemMappings(_record);

            if( _parentForm != null )
                _parentForm.DrawItemMappings();
        }

        private void buttonFillDefaultItemMappings_Click(object sender,System.EventArgs e)
        {
            _mappingManager.FillDefaultItemMappings(_record);

            if( _parentForm != null )
                _parentForm.DrawItemMappings();
        }
    }
}
