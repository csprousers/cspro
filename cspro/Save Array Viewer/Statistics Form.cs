using System;
using System.Drawing;
using System.Globalization;
using System.Text;
using System.Windows.Forms;

namespace SaveArrayViewer
{
    partial class StatisticsForm : Form
    {
        SaveArrayFile saf;
        PutToGetCorrespondenceForm ptgcForm;

        internal StatisticsForm(SaveArrayFile saf,bool useZeroIndices)
        {
            InitializeComponent();

            this.saf = null;
            checkBoxIncludeZeroIndices.Checked = useZeroIndices;
            this.saf = saf;

            ptgcForm = new PutToGetCorrespondenceForm();
        }

        private void statisticsDataGridView_SelectionChanged(object sender,System.EventArgs e)
        {
            statisticsDataGridView.ClearSelection(); // disable cell highlighting
        }

        private void checkBoxIncludeZeroIndices_CheckedChanged(object sender,System.EventArgs e)
        {
            if( saf != null ) // only update the statistics when the request comes from a user click
                GenerateStatistics();
        }

        string FormatPercent(double percent)
        {
            return String.Format(percent != 0 && percent < 10 ? "{0:F1}%" : "{0:F0}%",percent);
        }

        internal void GenerateStatistics()
        {
            statisticsDataGridView.Rows.Clear();
            statisticsDataGridView.RowCount = saf.SaveArrays.Count;

            int startingIndex = checkBoxIncludeZeroIndices.Checked ? 0 : 1;

            for( int i = 0; i < saf.SaveArrays.Count; i++ )
            {
                SaveArray sa = (SaveArray)saf.SaveArrays[i];

                int numberCells = 0;
                int numberDefaults = 0;
                int numberGets = 0;
                int numberPuts = 0;
                int numberPositiveGetCells = 0;
                int numberPositivePutCells = 0;
                int numberGetWithoutPutCells = 0;

                for( int layer = ( sa.Dimension == 3 ? startingIndex : 0 ) ; layer < sa.Lay; layer++ )
                {
                    for( int c = ( sa.Dimension >= 2 ? startingIndex : 0 ); c < sa.Col; c++ )
                    {
                        for( int r = startingIndex; r < sa.Row; r++ )
                        {
                            numberCells++;

                            if( sa.Numeric && sa.GetCell(r,c,layer).Equals("DEFAULT") )
                                numberDefaults++;

                            numberGets += sa.GetGetCell(r,c,layer);
                            numberPuts += sa.GetPutCell(r,c,layer);

                            if( sa.GetGetCell(r,c,layer) > 0 )
                            {
                                numberPositiveGetCells++;

                                if( sa.GetPutCell(r,c,layer) == 0 )
                                    numberGetWithoutPutCells++;
                            }

                            if( sa.GetPutCell(r,c,layer) > 0 )
                                numberPositivePutCells++;
                        }
                    }
                }


                statisticsDataGridView.Rows[i].Cells[0].Value = sa.Name;
                statisticsDataGridView.Rows[i].Cells[1].Value = numberCells.ToString();

                if( sa.Numeric )
                {
                    statisticsDataGridView.Rows[i].Cells[2].Value = numberDefaults.ToString();

                    if( numberCells > 0 )
                    {
                        double defaultPercent = 100.0 * numberDefaults / numberCells;
                        statisticsDataGridView.Rows[i].Cells[3].Value = FormatPercent(defaultPercent);
                    }
                }

                else
                {
                    statisticsDataGridView.Rows[i].Cells[2].Style.BackColor = Color.LightBlue;
                    statisticsDataGridView.Rows[i].Cells[3].Style.BackColor = Color.LightBlue;
                }

                statisticsDataGridView.Rows[i].Cells[4].Value = sa.Runs;
                statisticsDataGridView.Rows[i].Cells[5].Value = sa.Cases;

                statisticsDataGridView.Rows[i].Cells[6].Value = numberPuts;
                statisticsDataGridView.Rows[i].Cells[7].Value = numberGets;

                if( numberGets > 0 )
                {
                    double putsPerGet = (double)numberPuts / numberGets;
                    statisticsDataGridView.Rows[i].Cells[8].Value = String.Format("{0:F1}",putsPerGet);
                }

                else
                    statisticsDataGridView.Rows[i].Cells[8].Style.BackColor = Color.LightBlue;

                if( sa.Cases > 0 )
                {
                    double putsPer100Cases = 100.0 * numberPuts / sa.Cases;
                    double getsPer100Cases = 100.0 * numberGets / sa.Cases;
                    statisticsDataGridView.Rows[i].Cells[9].Value = String.Format("{0:F1}",putsPer100Cases);
                    statisticsDataGridView.Rows[i].Cells[10].Value = String.Format("{0:F1}",getsPer100Cases);
                }

                else
                {
                    statisticsDataGridView.Rows[i].Cells[9].Style.BackColor = Color.LightBlue;
                    statisticsDataGridView.Rows[i].Cells[10].Style.BackColor = Color.LightBlue;
                }

                if( numberCells > 0 )
                {
                    double percentPositivePuts = 100.0 * numberPositivePutCells / numberCells;
                    double percentPositiveGets = 100.0 * numberPositiveGetCells / numberCells;
                    statisticsDataGridView.Rows[i].Cells[11].Value = FormatPercent(percentPositivePuts);
                    statisticsDataGridView.Rows[i].Cells[12].Value = FormatPercent(percentPositiveGets);
                }

                else
                {
                    statisticsDataGridView.Rows[i].Cells[11].Style.BackColor = Color.LightBlue;
                    statisticsDataGridView.Rows[i].Cells[12].Style.BackColor = Color.LightBlue;
                }

                statisticsDataGridView.Rows[i].Cells[13].Value = numberGetWithoutPutCells.ToString();

                if( numberPuts > 0 && numberGets > 0  )
                {
                    double putToGetCorrespondence = CalculatePutToGetCorrespondence(sa,numberPuts,numberGets);
                    statisticsDataGridView.Rows[i].Cells[14].Value = FormatPercent(putToGetCorrespondence);
                }

                else
                    statisticsDataGridView.Rows[i].Cells[14].Style.BackColor = Color.LightBlue;
            }

            // do a horizontal resize so that we don't show a lot of blank space

            // ( statisticsDataGridView.ColumnHeadersHeight isn't updated at this point so i tested values
            // to figure out what worked )
            int neededHeight = 62; // statisticsDataGridView.ColumnHeadersHeight;

            foreach( DataGridViewRow dgvr in statisticsDataGridView.Rows )
                neededHeight += dgvr.Height;

            if( neededHeight < statisticsDataGridView.Height )
                this.Height -= ( statisticsDataGridView.Height - neededHeight );
        }

        // calculate a measure of how closely the distribution of puts matching with the distribution of gets
        // 0 = the gets come from cells where there were no puts
        // 1 = the distributions of puts and gets is the same
        double CalculatePutToGetCorrespondence(SaveArray sa,int numberPuts,int numberGets)
        {
            int startingIndex = checkBoxIncludeZeroIndices.Checked ? 0 : 1;

            double putsMatchingsGets = 0;

            for( int layer = ( sa.Dimension == 3 ? startingIndex : 0 ); layer < sa.Lay; layer++ )
            {
                for( int c = ( sa.Dimension >= 2 ? startingIndex : 0 ); c < sa.Col; c++ )
                {
                    for( int r = startingIndex; r < sa.Row; r++ )
                    {
                        int thisGets = sa.GetGetCell(r,c,layer);
                        int thisPuts = sa.GetPutCell(r,c,layer);

                        putsMatchingsGets += Math.Min((double)thisGets / numberGets,(double)thisPuts / numberPuts);
                    }
                }
            }

            return 100 * putsMatchingsGets;
        }

        private void StatisticsForm_FormClosing(object sender,FormClosingEventArgs e)
        {
            ptgcForm.Close();
        }

        private void linkLabelCorrespondenceHelp_LinkClicked(object sender,LinkLabelLinkClickedEventArgs e)
        {
            if( ptgcForm.IsDisposed )
                ptgcForm = new PutToGetCorrespondenceForm();

            ptgcForm.Show();
            ptgcForm.Activate();
        }

        private void linkLabelCopy_LinkClicked(object sender,LinkLabelLinkClickedEventArgs e)
        {
            // copy the table to the clipboard
            StringBuilder sb = new StringBuilder();

            foreach( DataGridViewColumn dgvc in statisticsDataGridView.Columns )
            {
                sb.Append(dgvc.HeaderText);
                sb.Append('\t');
            }

            sb.Remove(sb.Length - 1,1); // remove the last tab added

            foreach( DataGridViewRow dgvr in statisticsDataGridView.Rows )
            {
                sb.Append("\r\n");

                foreach( DataGridViewCell dgvc in dgvr.Cells )
                {
                    sb.Append(dgvc.Value);
                    sb.Append('\t');
                }

                sb.Remove(sb.Length - 1,1); // remove the last tab added
            }

            Clipboard.SetText(sb.ToString());
        }

        private void statisticsDataGridView_SortCompare(object sender,DataGridViewSortCompareEventArgs e)
        {
            if( e.CellValue1 == null || e.CellValue2 == null )
                return;

            // do a natural sort if the first character is a digit
            string str1 = e.CellValue1.ToString();
            string str2 = e.CellValue2.ToString();

            if( str1.Length == 0 || !char.IsNumber(str1[0]) || str2.Length == 0 || !char.IsNumber(str2[0]) )
                return; // we won't handle the sort

            double dbl1 = Double.Parse(str1.Replace('%',' '), NumberStyles.Any, CultureInfo.InvariantCulture); // get rid of any percent signs
            double dbl2 = Double.Parse(str2.Replace('%',' '), NumberStyles.Any, CultureInfo.InvariantCulture);

            if( dbl1 > dbl2 )
                e.SortResult = 1;

            else if( dbl1 < dbl2 )
                e.SortResult = -1;

            else
                e.SortResult = 0;

            e.Handled = true;
        }
    }
}
