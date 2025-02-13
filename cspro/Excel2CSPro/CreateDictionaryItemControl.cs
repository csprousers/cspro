using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace Excel2CSPro
{
    partial class CreateDictionaryItemControl : UserControl
    {
        private CreateDictionaryControl.ColumnAnalysis _columnAnalysis;
        private bool _canCreateValueSet;

        public CreateDictionaryItemControl(CreateDictionaryControl.ColumnAnalysis columnAnalysis)
        {
            InitializeComponent();

            _columnAnalysis = columnAnalysis;
            _canCreateValueSet = false;

            if( !_columnAnalysis.ContainsAlpha && _columnAnalysis.Values.Count >= 1 && _columnAnalysis.Values.Count < CreateDictionaryControl.ColumnAnalysis.MaxValueSetValues )
            {
                _canCreateValueSet = true;
                checkBoxItemCreateValueSet.Text = String.Format(checkBoxItemCreateValueSet.Text,_columnAnalysis.Values.Count);
            }

            textBoxItemName.Text =  CSPro.Logic.Names.MakeName(_columnAnalysis.HeaderText);
            textBoxItemLength.Text = _columnAnalysis.MaximumLength.ToString();

            if( !_columnAnalysis.ContainsAlpha )
            {
                checkBoxItemNumeric.Checked = true;
                textBoxItemBeforeDecLength.Text = _columnAnalysis.DigitsBeforeDecimal.ToString();

                if( _columnAnalysis.DigitsAfterDecimal > 0 )
                    textBoxItemAfterDecLength.Text = _columnAnalysis.DigitsAfterDecimal.ToString();
            }
        }

        private void checkBoxIncludeItem_CheckedChanged(object sender,EventArgs e)
        {
            panelItem.Enabled = checkBoxIncludeItem.Checked;
        }

        private void checkBoxItemNumeric_CheckedChanged(object sender,EventArgs e)
        {
            bool isNumeric = checkBoxItemNumeric.Checked;

            textBoxItemLength.Enabled = !isNumeric;

            textBoxItemBeforeDecLength.Enabled = isNumeric;
            textBoxItemAfterDecLength.Enabled = isNumeric;

            checkBoxItemCreateValueSet.Visible = isNumeric && _canCreateValueSet;
        }

        public class ItemSelections
        {
            public bool IncludeItem = false;
            public string Name = null;
            public bool IsId = false;
            public bool IsNumeric = false;
            public int AlphaLength = 0;
            public int BeforeDecLength = 0;
            public int AfterDecLength = 0;
            public bool CreateValueSet = false;
            public SortedSet<double> Values = null;
        }

        private int ParseLengthText(string text)
        {
            if( String.IsNullOrWhiteSpace(text) )
                return 0;

            int length = 0;

            if( Int32.TryParse(text,out length) )
                return length;

            else
                throw new Exception(String.Format(Messages.InvalidLength,text));
        }

        public string ItemName { get { return textBoxItemName.Text.Trim().ToUpper(); } }

        public ItemSelections Selections
        {
            get
            {
                ItemSelections selections = new ItemSelections();

                selections.IncludeItem = checkBoxIncludeItem.Checked;

                if( selections.IncludeItem )
                {
                    selections.Name = this.ItemName;
                    selections.IsId = checkBoxItemID.Checked;
                    selections.IsNumeric = checkBoxItemNumeric.Checked;

                    if( selections.IsNumeric )
                    {
                        selections.BeforeDecLength = ParseLengthText(textBoxItemBeforeDecLength.Text);
                        selections.AfterDecLength = ParseLengthText(textBoxItemAfterDecLength.Text);
                        selections.CreateValueSet = checkBoxItemCreateValueSet.Checked;
                        selections.Values = selections.CreateValueSet ? _columnAnalysis.Values : null;
                    }

                    else
                        selections.AlphaLength = ParseLengthText(textBoxItemLength.Text);
                }

                return selections;
            }
        }
    }
}
