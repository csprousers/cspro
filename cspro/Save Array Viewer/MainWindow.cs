using System;
using System.Collections;
using System.Drawing;
using System.Windows.Forms;

namespace SaveArrayViewer
{
    partial class MainWindow : Form
    {
        const string SaveArrayFilter = "Save Array Files (*.sva)|*.sva|All Files (*.*)|*.*";

        string applicationName;

        SaveArrayFile saf;
        bool changesMade;
        bool disableCellValidation;

        SaveArray displayedSA;
        int displayedStartingRow,displayedStartingCol,displayedStartingLay;

        enum DisplayedValues { ArrayValues, GetValues, PutValues };
        DisplayedValues valuesViewing;

        StatisticsForm statisticsForm;

        double getPercentile25,getPercentile50,getPercentile75;
        double putPercentile25,putPercentile50,putPercentile75;
        double maxPutGetRatio;


        public MainWindow()
        {
            InitializeComponent();

            applicationName = this.Text;
            changesMade = false;
            disableCellValidation = false;

            EnableMenus(false);

            valuesViewing = DisplayedValues.ArrayValues;
            SetChecksForViewMenu();

            statisticsForm = null;
        }

        void EnableMenus(bool fileIsOpen)
        {
            saveToolStripMenuItem.Enabled = fileIsOpen;
            saveAsToolStripMenuItem.Enabled = fileIsOpen;
            optionsToolStripMenuItem.Enabled = fileIsOpen;
            arrayControlToolStripMenuItem.Enabled = fileIsOpen;
            statisticsToolStripMenuItem.Enabled = fileIsOpen;
            viewToolStripMenuItem.Enabled = fileIsOpen;
        }

        void UncheckAllHighlights()
        {
            highlightDefaultValuesToolStripMenuItem.Checked = false;
            highlightCellsWhereGetPutToolStripMenuItem.Checked = false;
            highlightCellsWhereGet0ToolStripMenuItem.Checked = false;
            highlightCellsWherePut0ToolStripMenuItem.Checked = false;
            highlightCellsWhereGet0AndPut0ToolStripMenuItem.Checked = false;
            top25GetToolStripMenuItem.Checked = false;
            top50GetToolStripMenuItem.Checked = false;
            least50GetToolStripMenuItem.Checked = false;
            least25GetToolStripMenuItem.Checked = false;
            top25PutToolStripMenuItem.Checked = false;
            top50PutToolStripMenuItem.Checked = false;
            least50PutToolStripMenuItem.Checked = false;
            least25PutToolStripMenuItem.Checked = false;
            shadeCellsByPutGetRatioToolStripMenuItem.Checked = false;
        }

        private void MainWindow_Shown(object sender,EventArgs e)
        {
            Array commandArgs = Environment.GetCommandLineArgs();

            if( commandArgs.Length >= 2 )
                LoadFile((string)commandArgs.GetValue(1));
        }

        private void MarkChangesMade()
        {
            if( !changesMade )
            {
                this.Text += "*";
                changesMade = true;
            }
        }

        private bool ConfirmFileClose()
        {
            return !changesMade || MessageBox.Show("Discard changes to the opened file?",applicationName,MessageBoxButtons.OKCancel) == DialogResult.OK;
        }

        private void LoadFile(string filename)
        {
            Cursor prevCursor = Cursor.Current;
            Cursor.Current = Cursors.WaitCursor;

            try
            {
                bool readExtendedInformation;

                saf = FileLoader.LoadFile(filename,out readExtendedInformation);
                this.Text = applicationName + " - " + saf.DisplayName;
                changesMade = false;
                EnableMenus(true);

                UpdateTree();

                usedInLabel.Visible = readExtendedInformation;
                procsLabel.Visible = readExtendedInformation;
                defLabel.Visible = readExtendedInformation;
                definitionLabel.Visible = readExtendedInformation;

                if( !readExtendedInformation )
                    showValueSetLabelsForDeckArraysToolStripMenuItem.Checked = false;

                showValueSetLabelsForDeckArraysToolStripMenuItem.Enabled = readExtendedInformation;

                if( statisticsForm != null && !statisticsForm.IsDisposed )
                    statisticsForm.Close();

                statisticsForm = null;
            }

            catch( Exception e )
            {
                MessageBox.Show(e.Message,applicationName,MessageBoxButtons.OK,MessageBoxIcon.Error);
            }

            Cursor.Current = prevCursor;
        }

        void UpdateTree()
        {
            Cursor prevCursor = Cursor.Current;
            Cursor.Current = Cursors.WaitCursor;

            arraysTreeView.Nodes.Clear();

            foreach( SaveArray sa in saf.SaveArrays )
            {
                TreeNode arrayNode = arraysTreeView.Nodes.Add(sa.Name);

                int startLay = showZeroIndicesToolStripMenuItem.Checked ? 0 : 1;

                if( sa.Lay > 1 )
                {
                    bool useVSLabels = showValueSetLabelsForDeckArraysToolStripMenuItem.Checked;

                    for( int l = startLay; l < sa.Lay; l++ )
                        arrayNode.Nodes.Add(sa.GetLayLabel(l,useVSLabels));
                }
            }

            if( arraysTreeView.Nodes.Count > 0 )
            {
                disableCellValidation = true;
                arraysTreeView.SelectedNode = arraysTreeView.Nodes[0];
                disableCellValidation = false;
            }

            else
            {
                displayedSA = null;
                arrayDataGridView.Rows.Clear();
                arrayDataGridView.ColumnCount = 0;
                definitionLabel.Text = "";
                procsLabel.Text = "";
            }

            Cursor.Current = prevCursor;
        }

        void IdentifySelectedNameAndLayer(out string name,out int layer)
        {
            name = displayedSA.Name;

            layer = -1;

            if( arraysTreeView.SelectedNode.Parent != null ) // a layer is selected, figure out what index it is
            {
                for( int i = 0; layer < 0; i++ )
                {
                    if( arraysTreeView.SelectedNode.Parent.Nodes[i] == arraysTreeView.SelectedNode )
                        layer = i;
                }
            }
        }

        void SelectByNameAndLayer(string oldArrayName,int newLayerRequested)
        {
            // find the selected array
            foreach( TreeNode tn in arraysTreeView.Nodes )
            {
                if( String.Equals(tn.Text,oldArrayName) )
                {
                    if( tn.Nodes.Count > 0 )
                        arraysTreeView.SelectedNode = tn.Nodes[newLayerRequested];

                    else
                        arraysTreeView.SelectedNode = tn;

                    return;
                }
            }
        }

        void ReselectTreeAndCellAfterOptionsUpdate(int indexAdjustment)
        {
            if( displayedSA != null )
            {
                string oldArrayName;
                int newLayerRequested;

                IdentifySelectedNameAndLayer(out oldArrayName,out newLayerRequested);

                if( arraysTreeView.SelectedNode.Parent != null )
                    newLayerRequested = Math.Max(0,newLayerRequested + indexAdjustment);

                UpdateTree();

                SelectByNameAndLayer(oldArrayName,newLayerRequested);
            }
        }

        private void arraysTreeView_AfterSelect(object sender,TreeViewEventArgs e)
        {
            SaveArray sa;
            int selectedLayer;

            if( arraysTreeView.SelectedNode.Nodes.Count > 0 ) // if selecting the array name of an array with layers ... select the first layer
            {
                arraysTreeView.SelectedNode.Expand();
                arraysTreeView.SelectedNode = arraysTreeView.SelectedNode.Nodes[0];
                return;
            }

            if( arraysTreeView.SelectedNode.Parent != null ) // selected a layer
            {
                sa = saf.GetSaveArrayByName(arraysTreeView.SelectedNode.Parent.Text);
                selectedLayer = arraysTreeView.SelectedNode.Parent.Nodes.IndexOf(arraysTreeView.SelectedNode);

                if( !showZeroIndicesToolStripMenuItem.Checked )
                    selectedLayer++;
            }

            else
            {
                sa = saf.GetSaveArrayByName(arraysTreeView.SelectedNode.Text);
                selectedLayer = 0;
            }

            string procs = "";
            IEnumerator itr = sa.GetProcReferences();

            while( itr.MoveNext() )
            {
                if( procs.Length == 0 )
                    procs = (string)itr.Current;

                else
                    procs = String.Format("{0}, {1}",procs,(string)itr.Current);
            }

            if( procs.Length > 0 )
                procsLabel.Text = procs;

            else
                procsLabel.Text = "No PROCs or Functions";

            string def = "array";

            if( sa.Alpha )
                def = String.Format("{0} alpha ({1})",def,sa.GetCell(0,0,0).Length);

            def = String.Format("{0} {1}(",def,sa.Name);

            for( int i = 0; i < sa.Dimension; i++ )
                def = String.Format("{0}{1}{2}{3}",def,i > 0 ? "," : "",sa.GetDimensionReference(i),sa.GetUsingSpillover(i) ? "(+)" : "");

            definitionLabel.Text = String.Format("{0})",def);

            DrawSpreadsheet(sa,selectedLayer);

        }

        string GetCellByValuesViewing(SaveArray sa,int r,int c,int layer)
        {
            if( valuesViewing == DisplayedValues.ArrayValues )
                return sa.GetCell(r,c,layer);

            else if( valuesViewing == DisplayedValues.GetValues )
                return sa.GetGetCell(r,c,layer).ToString();

            else
                return sa.GetPutCell(r,c,layer).ToString();
        }

        void SetCellByValuesViewing(string val,SaveArray sa,int r,int c,int layer)
        {
            if( valuesViewing == DisplayedValues.ArrayValues )
                sa.SetCell(val,r,c,layer);

            else if( valuesViewing == DisplayedValues.GetValues )
                sa.SetGetCell(Convert.ToInt32(val),r,c,layer);

            else
                sa.SetPutCell(Convert.ToInt32(val),r,c,layer);
        }

        void DrawSpreadsheet(SaveArray sa,int layer)
        {
            Cursor prevCursor = Cursor.Current;
            Cursor.Current = Cursors.WaitCursor;

            int startingRow = showZeroIndicesToolStripMenuItem.Checked ? 0 : 1;
            int startingCol = showZeroIndicesToolStripMenuItem.Checked || sa.Col == 1 ? 0 : 1;

            arrayDataGridView.Rows.Clear();

            arrayDataGridView.ColumnCount = sa.Col - startingCol;
            arrayDataGridView.RowCount = sa.Row - startingRow;

            bool highlightDefaults = sa.Numeric && highlightDefaultValuesToolStripMenuItem.Checked;
            bool otherHighlightSet1 = highlightCellsWhereGetPutToolStripMenuItem.Checked || highlightCellsWhereGet0ToolStripMenuItem.Checked || highlightCellsWherePut0ToolStripMenuItem.Checked || highlightCellsWhereGet0AndPut0ToolStripMenuItem.Checked;
            bool otherHighlightSet2 = top25GetToolStripMenuItem.Checked || top50GetToolStripMenuItem.Checked || least50GetToolStripMenuItem.Checked || least25GetToolStripMenuItem.Checked;
            bool otherHighlightSet3 = top25PutToolStripMenuItem.Checked || top50PutToolStripMenuItem.Checked || least50PutToolStripMenuItem.Checked || least25PutToolStripMenuItem.Checked;
            bool otherHighlightSet4 = shadeCellsByPutGetRatioToolStripMenuItem.Checked;

            if( otherHighlightSet2 )
                CalculateGetPercentiles(sa);

            else if( otherHighlightSet3 )
                CalculatePutPercentiles(sa);

            else if( otherHighlightSet4 )
                CalculatePutGetRatio(sa);

            bool useVSLabels = showValueSetLabelsForDeckArraysToolStripMenuItem.Checked;

            for( int c = startingCol; c < sa.Col; c++ )
            {
                arrayDataGridView.Columns[c - startingCol].Name = sa.GetColLabel(c,useVSLabels);
                arrayDataGridView.Columns[c - startingCol].SortMode = DataGridViewColumnSortMode.NotSortable;

                for( int r = startingRow; r < sa.Row; r++ )
                {
                    if( c == startingCol )
                        arrayDataGridView.Rows[r - startingRow].HeaderCell.Value = sa.GetRowLabel(r,useVSLabels);

                    string cellVal = GetCellByValuesViewing(sa,r,c,layer);

                    arrayDataGridView.Rows[r - startingRow].Cells[c - startingCol].Value = cellVal;

                    if( highlightDefaults && String.Compare("DEFAULT",cellVal,true) == 0 )
                        arrayDataGridView.Rows[r - startingRow].Cells[c - startingCol].Style.BackColor = Color.Orange;

                    else if( otherHighlightSet1 )
                    {
                        bool highlight = false;

                        if( highlightCellsWhereGetPutToolStripMenuItem.Checked )
                            highlight = sa.GetGetCell(r,c,layer) > sa.GetPutCell(r,c,layer);

                        else if( highlightCellsWhereGet0ToolStripMenuItem.Checked )
                            highlight = sa.GetGetCell(r,c,layer) == 0;

                        else if( highlightCellsWherePut0ToolStripMenuItem.Checked )
                            highlight = sa.GetPutCell(r,c,layer) == 0;

                        else if( highlightCellsWhereGet0AndPut0ToolStripMenuItem.Checked )
                            highlight = sa.GetGetCell(r,c,layer) == 0 && sa.GetPutCell(r,c,layer) == 0;

                        if( highlight )
                            arrayDataGridView.Rows[r - startingRow].Cells[c - startingCol].Style.BackColor = Color.Cyan;
                    }

                    else if( otherHighlightSet2 )
                    {
                        bool highlight = false;

                        if( top25GetToolStripMenuItem.Checked )
                            highlight = sa.GetGetCell(r,c,layer) >= getPercentile75;

                        else if( top50GetToolStripMenuItem.Checked )
                            highlight = sa.GetGetCell(r,c,layer) >= getPercentile50;

                        else if( least50GetToolStripMenuItem.Checked )
                            highlight = sa.GetGetCell(r,c,layer) < getPercentile50;

                        else if( least25GetToolStripMenuItem.Checked )
                            highlight = sa.GetGetCell(r,c,layer) <= getPercentile25;

                        if( highlight )
                            arrayDataGridView.Rows[r - startingRow].Cells[c - startingCol].Style.BackColor = Color.Cyan;
                    }

                    else if( otherHighlightSet3 )
                    {
                        bool highlight = false;

                        if( top25PutToolStripMenuItem.Checked )
                            highlight = sa.GetPutCell(r,c,layer) >= putPercentile75;

                        else if( top50PutToolStripMenuItem.Checked )
                            highlight = sa.GetPutCell(r,c,layer) >= putPercentile50;

                        else if( least50PutToolStripMenuItem.Checked )
                            highlight = sa.GetPutCell(r,c,layer) < putPercentile50;

                        else if( least25PutToolStripMenuItem.Checked )
                            highlight = sa.GetPutCell(r,c,layer) <= putPercentile25;

                        if( highlight )
                            arrayDataGridView.Rows[r - startingRow].Cells[c - startingCol].Style.BackColor = Color.Cyan;
                    }

                    else if( otherHighlightSet4 )
                    {
                        int get = sa.GetGetCell(r,c,layer);

                        if( get > 0 && maxPutGetRatio != 0 )
                        {
                            double ratio = sa.GetPutCell(r,c,layer) / (double)get;
                            double factor = ratio / maxPutGetRatio;
                            arrayDataGridView.Rows[r - startingRow].Cells[c - startingCol].Style.BackColor = Color.FromArgb(255 - (int)( 255 * factor ),
                                                                                                                            255 - (int)( 55 * factor ),
                                                                                                                            255 - (int)( 100 * factor ));
                        }
                    }
                }
            }

            displayedSA = sa;
            displayedStartingRow = startingRow;
            displayedStartingCol = startingCol;
            displayedStartingLay = layer;

            labelTypeValues.Text = "Showing " +    ( valuesViewing == DisplayedValues.ArrayValues ? "Array" :
                                                    ( valuesViewing == DisplayedValues.GetValues ? "Get" : "Put" ) ) + " Values";

            labelTypeValues.Text += String.Format(" ({0} Run{1}, {2} Case{3})",sa.Runs,sa.Runs == 1 ? "" : "s",sa.Cases,sa.Cases == 1 ? "" : "s");

            // right justify the text
            labelTypeValues.Location = new Point(arrayDataGridView.Location.X + arrayDataGridView.Size.Width - labelTypeValues.Width,labelTypeValues.Location.Y);

            Cursor.Current = prevCursor;
        }

        bool CheckValidCellChange(string str)
        {
            bool showingStatistics = valuesViewing != DisplayedValues.ArrayValues;

            if( displayedSA.Numeric || showingStatistics )
            {
                try
                {
                    if( !showingStatistics && ( String.Equals(str, "default", StringComparison.CurrentCultureIgnoreCase) ||
                                                String.Equals(str, "missing", StringComparison.CurrentCultureIgnoreCase) ||
                                                String.Equals(str, "refused", StringComparison.CurrentCultureIgnoreCase) ||
                                                String.Equals(str, "notappl", StringComparison.CurrentCultureIgnoreCase) ) )
                    {
                        // fine
                    }

                    else
                        Convert.ToDouble(str);
                }

                catch
                {
                    return false;
                }
            }

            return true;
        }

        private void arrayDataGridView_CellValidating(object sender,DataGridViewCellValidatingEventArgs e)
        {
            if( !disableCellValidation && displayedSA != null && !CheckValidCellChange((string)e.FormattedValue) )
            {
                MessageBox.Show("Enter a valid numeric" + ( valuesViewing == DisplayedValues.ArrayValues ? " (or special)" : "" ) + " value.");
                e.Cancel = true;
            }
        }

        private void arrayDataGridView_CellEndEdit(object sender,DataGridViewCellEventArgs e)
        {
            if( disableCellValidation )
                return;

            string val = (string)arrayDataGridView.Rows[e.RowIndex].Cells[e.ColumnIndex].Value;

            string cellVal = GetCellByValuesViewing(displayedSA,displayedStartingRow + e.RowIndex,displayedStartingCol + e.ColumnIndex,displayedStartingLay);

            if( String.Equals(val,cellVal) )
                return; // the value wasn't changed

            MarkChangesMade();

            bool showingStatistics = valuesViewing != DisplayedValues.ArrayValues;

            bool highlightAfterValueChange = false;

            if( showingStatistics || displayedSA.Numeric )
            {
                val = val.ToUpper();

                bool highlightDefaults = highlightDefaultValuesToolStripMenuItem.Checked;

                if( highlightDefaults )
                {
                    arrayDataGridView.Rows[e.RowIndex].Cells[e.ColumnIndex].Style.BackColor =
                        String.Equals("DEFAULT",val,StringComparison.CurrentCultureIgnoreCase) ? Color.Orange : arrayDataGridView.DefaultCellStyle.BackColor;
                }

                else
                    highlightAfterValueChange = true;

                // get rid of any trailing zeros
                try
                {
                    double dVal = Convert.ToDouble(val);
                    val = String.Format("{0}",dVal);
                }

                catch(Exception) // it was a special value
                {
                }

            }

            else // truncate the string at the item length
            {
                int strLen = displayedSA.GetCell(displayedStartingRow + e.RowIndex,displayedStartingCol + e.ColumnIndex,displayedStartingLay).Length;

                if( val.Length < strLen )
                    val = val.PadRight(strLen);

                else if( val.Length > strLen )
                    val = val.Substring(0,strLen);
            }

            arrayDataGridView.Rows[e.RowIndex].Cells[e.ColumnIndex].Value = val;
            SetCellByValuesViewing(val,displayedSA,displayedStartingRow + e.RowIndex,displayedStartingCol + e.ColumnIndex,displayedStartingLay);

            if( highlightAfterValueChange )
                arrayDataGridView.Rows[e.RowIndex].Cells[e.ColumnIndex].Style.BackColor = GetCellColor(displayedStartingRow + e.RowIndex,displayedStartingCol + e.ColumnIndex);
        }

        Color GetCellColor(int r,int c)
        {
            bool highlight = false;

            if( highlightCellsWhereGetPutToolStripMenuItem.Checked )
                highlight = displayedSA.GetGetCell(r,c,displayedStartingLay) > displayedSA.GetPutCell(r,c,displayedStartingLay);

            else if( highlightCellsWhereGet0ToolStripMenuItem.Checked )
                highlight = displayedSA.GetGetCell(r,c,displayedStartingLay) == 0;

            else if( highlightCellsWherePut0ToolStripMenuItem.Checked )
                highlight = displayedSA.GetPutCell(r,c,displayedStartingLay) == 0;

            else if( highlightCellsWhereGet0AndPut0ToolStripMenuItem.Checked )
                highlight = displayedSA.GetGetCell(r,c,displayedStartingLay) == 0 && displayedSA.GetPutCell(r,c,displayedStartingLay) == 0;

            else if( top25GetToolStripMenuItem.Checked )
            {
                CalculateGetPercentiles(displayedSA);
                highlight = displayedSA.GetGetCell(r,c,displayedStartingLay) >= getPercentile75;
            }

            else if( top50GetToolStripMenuItem.Checked )
            {
                CalculateGetPercentiles(displayedSA);
                highlight = displayedSA.GetGetCell(r,c,displayedStartingLay) >= getPercentile50;
            }

            else if( least50GetToolStripMenuItem.Checked )
            {
                CalculateGetPercentiles(displayedSA);
                highlight = displayedSA.GetGetCell(r,c,displayedStartingLay) < getPercentile50;
            }

            else if( least25GetToolStripMenuItem.Checked )
            {
                CalculateGetPercentiles(displayedSA);
                highlight = displayedSA.GetGetCell(r,c,displayedStartingLay) <= getPercentile25;
            }

            else if( top25PutToolStripMenuItem.Checked )
            {
                CalculatePutPercentiles(displayedSA);
                highlight = displayedSA.GetPutCell(r,c,displayedStartingLay) >= putPercentile75;
            }

            else if( top50PutToolStripMenuItem.Checked )
            {
                CalculatePutPercentiles(displayedSA);
                highlight = displayedSA.GetPutCell(r,c,displayedStartingLay) >= putPercentile50;
            }

            else if( least50PutToolStripMenuItem.Checked )
            {
                CalculatePutPercentiles(displayedSA);
                highlight = displayedSA.GetPutCell(r,c,displayedStartingLay) < putPercentile50;
            }

            else if( least25PutToolStripMenuItem.Checked )
            {
                CalculatePutPercentiles(displayedSA);
                highlight = displayedSA.GetPutCell(r,c,displayedStartingLay) <= putPercentile25;
            }

            else if( shadeCellsByPutGetRatioToolStripMenuItem.Checked )
            {
                CalculatePutGetRatio(displayedSA);

                int get = displayedSA.GetGetCell(r,c,displayedStartingLay);

                if( get > 0 && maxPutGetRatio != 0 )
                {
                    double ratio = displayedSA.GetPutCell(r,c,displayedStartingLay) / (double)get;
                    double factor = ratio / maxPutGetRatio;
                    return Color.FromArgb(255 - (int)( 255 * factor ),255 - (int)( 55 * factor ),255 - (int)( 100 * factor ));
                }
             }

            return highlight ? Color.Cyan : arrayDataGridView.DefaultCellStyle.BackColor;
        }

        void OnPasteCells()
        {
            if( !Clipboard.ContainsText() )
                return;

            string clipboard = Clipboard.GetText();

            bool usingHeaders = clipboard[0] == '\t';

            string[] lines = clipboard.Replace("\r","").Split('\n');

            int row = arrayDataGridView.CurrentCell.RowIndex;
            int col = arrayDataGridView.CurrentCell.ColumnIndex;

            if( usingHeaders )
            {
                row--;
                col--;
            }

            for( int step = 0; step < 2; step++ )
            {
                for( int r = ( usingHeaders ? 1 : 0 ); r < lines.Length; r++ )
                {
                    string[] cells = lines[r].Split('\t');

                    if( r == ( lines.Length - 1 ) && cells.Length == 1 && cells[0] == "" ) // excel can add a blank line at the end of the output
                        continue;

                    int thisRow = row + r;

                    if( thisRow >= arrayDataGridView.RowCount )
                        break;

                    for( int c = ( usingHeaders ? 1 : 0 ); c < cells.Length; c++ )
                    {
                        int thisCol = col + c;

                        if( thisCol >= arrayDataGridView.ColumnCount )
                            break;

                        if( step == 0 )
                        {
                            if( !CheckValidCellChange(cells[c]) )
                            {
                                MessageBox.Show(String.Format("There are values (\"{0}\") in the clipboard that cannot be pasted.",cells[c]));
                                return;
                            }
                        }

                        else
                        {
                            arrayDataGridView.Rows[thisRow].Cells[thisCol].Value = cells[c];
                            arrayDataGridView_CellEndEdit(null,new DataGridViewCellEventArgs(thisCol,thisRow));
                        }
                    }
                }
            }

            arrayDataGridView.Invalidate();
        }

        private void arrayDataGridView_KeyDown(object sender,KeyEventArgs e)
        {
            if( e.Control && e.KeyCode == Keys.V )
            {
                OnPasteCells();
                e.Handled = true;
            }

            else
                e.Handled = false;
        }

        // File -> Open
        private void openToolStripMenuItem_Click(object sender,EventArgs e)
        {
            if( !ConfirmFileClose() )
                return;

            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Filter = SaveArrayFilter;

            if( ofd.ShowDialog() == DialogResult.OK )
                LoadFile(ofd.FileName);
        }

        // File -> Save
        private void saveToolStripMenuItem_Click(object sender,EventArgs e)
        {
            if( !changesMade )
                return;

            Cursor prevCursor = Cursor.Current;
            Cursor.Current = Cursors.WaitCursor;

            try
            {
                saf.Save();
                this.Text = applicationName + " - " + saf.DisplayName;
                changesMade = false;
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message,applicationName,MessageBoxButtons.OK,MessageBoxIcon.Error);
            }

            Cursor.Current = prevCursor;
        }

        // File -> Save As
        private void saveAsToolStripMenuItem_Click(object sender,EventArgs e)
        {
            SaveFileDialog sfd = new SaveFileDialog();
            sfd.Filter = SaveArrayFilter;

            if( sfd.ShowDialog() == DialogResult.OK )
            {
                Cursor prevCursor = Cursor.Current;
                Cursor.Current = Cursors.WaitCursor;

                try
                {
                    saf.Filename = sfd.FileName;
                    saf.Save();
                    this.Text = applicationName + " - " + saf.DisplayName;
                    changesMade = false;
                }

                catch( Exception exception )
                {
                    MessageBox.Show(exception.Message,applicationName,MessageBoxButtons.OK,MessageBoxIcon.Error);
                }

                Cursor.Current = prevCursor;
            }
        }

        private void MainWindow_FormClosing(object sender,FormClosingEventArgs e)
        {
            if( !ConfirmFileClose() )
                e.Cancel = true;
        }

        // File -> Exit
        private void exitToolStripMenuItem_Click(object sender,EventArgs e)
        {
            Close();
        }

        // Options -> Show Zero Indices
        private void showZeroIndicesToolStripMenuItem_Click(object sender,EventArgs e)
        {
            showZeroIndicesToolStripMenuItem.Checked = !showZeroIndicesToolStripMenuItem.Checked;
            ReselectTreeAndCellAfterOptionsUpdate(showZeroIndicesToolStripMenuItem.Checked ? 1 : -1);
        }

        // Options -> Highlight Default Values
        private void highlightDefaultValuesToolStripMenuItem_Click(object sender,EventArgs e)
        {
            bool temp = highlightDefaultValuesToolStripMenuItem.Checked;
            UncheckAllHighlights();
            highlightDefaultValuesToolStripMenuItem.Checked = !temp;
            ReselectTreeAndCellAfterOptionsUpdate(0);
        }

        // Options -> Show Value Set Labels for DeckArrays
        private void showValueSetLabelsForDeckArraysToolStripMenuItem_Click(object sender,EventArgs e)
        {
            showValueSetLabelsForDeckArraysToolStripMenuItem.Checked = !showValueSetLabelsForDeckArraysToolStripMenuItem.Checked;
            ReselectTreeAndCellAfterOptionsUpdate(0);
        }

        // Array Control -> Reload Arrays
        private void reloadFileToolStripMenuItem_Click(object sender,EventArgs e)
        {
            if( saf != null && ConfirmFileClose() )
            {
                SaveArray selectedArray = displayedSA;

                string oldArrayName = "";
                int layer = 0;

                if( selectedArray != null )
                    IdentifySelectedNameAndLayer(out oldArrayName,out layer);

                LoadFile(saf.Filename);

                if( selectedArray != null )
                    SelectByNameAndLayer(oldArrayName,layer);

                else
                    displayedSA = null;
            }
        }

        void ApplyChangeToAllDimensions(out int startLayer,out int endLayer)
        {
            if( displayedSA.Dimension == 3 && MessageBox.Show(String.Format("Apply to all {0} layers of {1}?",displayedSA.Lay,displayedSA.Name),
                    applicationName,MessageBoxButtons.YesNo) == DialogResult.No )
            {
                startLayer = displayedStartingLay;
                endLayer = displayedStartingLay + 1;
            }

            else
            {
                startLayer = 0;
                endLayer = displayedSA.Lay;
            }
        }

        void SetAllCellsToCurrentHelper(bool settingOnlyDefault)
        {
            if( arrayDataGridView.SelectedCells.Count != 1 )
            {
                MessageBox.Show("Select only one cell to specify the desired value!");
                return;
            }

            int startLayer,endLayer;
            ApplyChangeToAllDimensions(out startLayer,out endLayer);

            string cellVal = (string)arrayDataGridView.SelectedCells[0].Value;

            for( int layer = startLayer; layer < endLayer; layer++ )
            {
                for( int c = 0; c < displayedSA.Col; c++ )
                {
                    for( int r = 0; r < displayedSA.Row; r++ )
                    {
                        if( !settingOnlyDefault || GetCellByValuesViewing(displayedSA,r,c,layer).Equals("DEFAULT") )
                            SetCellByValuesViewing(cellVal,displayedSA,r,c,layer);
                    }
                }
            }

            ReselectTreeAndCellAfterOptionsUpdate(0);
            MarkChangesMade();
        }

        // Array Control -> Set All Cells -> Current Cell Value
        private void currentCellValueToolStripMenuItem_Click(object sender,EventArgs e)
        {
            SetAllCellsToCurrentHelper(false);
        }

        // Array Control -> Set All Cells -> Default
        private void defaultToolStripMenuItem_Click(object sender,EventArgs e)
        {
            int startLayer,endLayer;
            ApplyChangeToAllDimensions(out startLayer,out endLayer);

            for( int layer = startLayer; layer < endLayer; layer++ )
            {
                for( int c = 0; c < displayedSA.Col; c++ )
                {
                    for( int r = 0; r < displayedSA.Row; r++ )
                        SetCellByValuesViewing("DEFAULT",displayedSA,r,c,layer);
                }
            }

            ReselectTreeAndCellAfterOptionsUpdate(0);
            MarkChangesMade();
        }

        // Array Control -> Set Default Cells -> Current Cell Value
        private void defaultCurrentCellValueToolStripMenuItem_Click(object sender,EventArgs e)
        {
            SetAllCellsToCurrentHelper(true);
        }

        // Array Control -> Delete Array
        private void deleteArrayToolStripMenuItem_Click(object sender,EventArgs e)
        {
            saf.RemoveSaveArray(displayedSA);
            MarkChangesMade();
            UpdateTree();
        }

        // Statistics -> View Save Array Statistics
        private void viewStatisticsToolStripMenuItem_Click(object sender,EventArgs e)
        {
            if( statisticsForm == null || statisticsForm.IsDisposed )
                statisticsForm = new StatisticsForm(saf,showZeroIndicesToolStripMenuItem.Checked);

            statisticsForm.GenerateStatistics();
            statisticsForm.Show();
            statisticsForm.Activate();
        }

        void ResetGetPutStatistics(SaveArray sa)
        {
            for( int layer = 0; layer < sa.Lay; layer++ )
            {
                for( int c = 0; c < sa.Col; c++ )
                {
                    for( int r = 0; r < sa.Row; r++ )
                    {
                        sa.SetGetCell(0,r,c,layer);
                        sa.SetPutCell(0,r,c,layer);
                    }
                }
            }

            sa.Runs = 0;
            sa.Cases = 0;
        }

        // Statistics -> Reset Statistics -> All Arrays
        private void allTablesToolStripMenuItem_Click(object sender,EventArgs e)
        {
            foreach( SaveArray sa in saf.SaveArrays )
                ResetGetPutStatistics(sa);

            MarkChangesMade();

            if( valuesViewing != DisplayedValues.ArrayValues )
                ReselectTreeAndCellAfterOptionsUpdate(0);

        }

        // Statistics -> Reset Statistics -> This Array Only
        private void thisTableToolStripMenuItem_Click(object sender,EventArgs e)
        {
            ResetGetPutStatistics(displayedSA);
            MarkChangesMade();

            if( valuesViewing != DisplayedValues.ArrayValues )
                ReselectTreeAndCellAfterOptionsUpdate(0);
        }

        private void highlightCellsWhereGetPutToolStripMenuItem_Click(object sender,EventArgs e)
        {
            bool temp = highlightCellsWhereGetPutToolStripMenuItem.Checked;
            UncheckAllHighlights();
            highlightCellsWhereGetPutToolStripMenuItem.Checked = !temp;
            ReselectTreeAndCellAfterOptionsUpdate(0);
        }

        private void highlightCellsWhereGet0ToolStripMenuItem_Click(object sender,EventArgs e)
        {
            bool temp = highlightCellsWhereGet0ToolStripMenuItem.Checked;
            UncheckAllHighlights();
            highlightCellsWhereGet0ToolStripMenuItem.Checked = !temp;
            ReselectTreeAndCellAfterOptionsUpdate(0);
        }

        private void highlightCellsWherePut0ToolStripMenuItem_Click(object sender,EventArgs e)
        {
            bool temp = highlightCellsWherePut0ToolStripMenuItem.Checked;
            UncheckAllHighlights();
            highlightCellsWherePut0ToolStripMenuItem.Checked = !temp;
            ReselectTreeAndCellAfterOptionsUpdate(0);
        }

        private void highlightCellsWhereGet0AndPut0ToolStripMenuItem_Click(object sender,EventArgs e)
        {
            bool temp = highlightCellsWhereGet0AndPut0ToolStripMenuItem.Checked;
            UncheckAllHighlights();
            highlightCellsWhereGet0AndPut0ToolStripMenuItem.Checked = !temp;
            ReselectTreeAndCellAfterOptionsUpdate(0);
        }

        double CalculatePercentile(int[] vals,double factor)
        {
            if( vals.Length == 1 )
                return vals[0];

            if( factor == 0.5 )
            {
                int startPoint = vals.Length / 2;

                switch( vals.Length % 2 )
                {
                    case 0:
                        return ( vals[startPoint] + vals[startPoint - 1] ) * 0.5;

                    case 1:
                        return vals[startPoint];
                }
            }

            else
            {
                int startPoint = (int)( vals.Length * factor );
                int nextPoint = Math.Min(startPoint,vals.Length - 1);

                switch( ( vals.Length - startPoint ) % 4 )
                {
                    case 0:
                        return vals[startPoint - 1];

                    case 1:
                        return vals[startPoint] * 0.75 + vals[nextPoint] * 0.25;

                    case 2:
                        return ( vals[startPoint] + vals[nextPoint] ) * 0.5;

                    case 3:
                        return vals[startPoint] * 0.25 + vals[nextPoint] * 0.75;
                }

            }

            throw new Exception();
        }

        void CalculatePercentiles(int[] vals,out double percentile25,out double percentile50,out double percentile75)
        {
            percentile25 = CalculatePercentile(vals,.25);
            percentile50 = CalculatePercentile(vals,.5);
            percentile75 = CalculatePercentile(vals,.75);
        }

        void CalculateGetPercentiles(SaveArray sa)
        {
            CalculatePercentiles(sa.GetSortedGets(showZeroIndicesToolStripMenuItem.Checked),out getPercentile25,out getPercentile50,out getPercentile75);
        }

        void CalculatePutPercentiles(SaveArray sa)
        {
            CalculatePercentiles(sa.GetSortedPuts(showZeroIndicesToolStripMenuItem.Checked),out putPercentile25,out putPercentile50,out putPercentile75);
        }

        private void top25GetToolStripMenuItem_Click(object sender,EventArgs e)
        {
            bool temp = top25GetToolStripMenuItem.Checked;
            UncheckAllHighlights();
            top25GetToolStripMenuItem.Checked = !temp;
            ReselectTreeAndCellAfterOptionsUpdate(0);
        }

        private void top50GetToolStripMenuItem_Click(object sender,EventArgs e)
        {
            bool temp = top50GetToolStripMenuItem.Checked;
            UncheckAllHighlights();
            top50GetToolStripMenuItem.Checked = !temp;
            ReselectTreeAndCellAfterOptionsUpdate(0);
        }

        private void least50GetToolStripMenuItem_Click(object sender,EventArgs e)
        {
            bool temp = least50GetToolStripMenuItem.Checked;
            UncheckAllHighlights();
            least50GetToolStripMenuItem.Checked = !temp;
            ReselectTreeAndCellAfterOptionsUpdate(0);
        }

        private void least25GetToolStripMenuItem_Click(object sender,EventArgs e)
        {
            bool temp = least25GetToolStripMenuItem.Checked;
            UncheckAllHighlights();
            least25GetToolStripMenuItem.Checked = !temp;
            ReselectTreeAndCellAfterOptionsUpdate(0);
        }

        private void top25PutToolStripMenuItem_Click(object sender,EventArgs e)
        {
            bool temp = top25PutToolStripMenuItem.Checked;
            UncheckAllHighlights();
            top25PutToolStripMenuItem.Checked = !temp;
            ReselectTreeAndCellAfterOptionsUpdate(0);
        }

        private void top50PutToolStripMenuItem_Click(object sender,EventArgs e)
        {
            bool temp = top50PutToolStripMenuItem.Checked;
            UncheckAllHighlights();
            top50PutToolStripMenuItem.Checked = !temp;
            ReselectTreeAndCellAfterOptionsUpdate(0);
        }

        private void least50PutToolStripMenuItem_Click(object sender,EventArgs e)
        {
            bool temp = least50PutToolStripMenuItem.Checked;
            UncheckAllHighlights();
            least50PutToolStripMenuItem.Checked = !temp;
            ReselectTreeAndCellAfterOptionsUpdate(0);
        }

        private void least25PutToolStripMenuItem_Click(object sender,EventArgs e)
        {
            bool temp = least25PutToolStripMenuItem.Checked;
            UncheckAllHighlights();
            least25PutToolStripMenuItem.Checked = !temp;
            ReselectTreeAndCellAfterOptionsUpdate(0);
        }

        void CalculatePutGetRatio(SaveArray sa)
        {
            maxPutGetRatio = sa.GetMaxPutGetRatio(showZeroIndicesToolStripMenuItem.Checked);
        }

        private void shadeCellsByPutGetRatioToolStripMenuItem_Click(object sender,EventArgs e)
        {
            bool temp = shadeCellsByPutGetRatioToolStripMenuItem.Checked;
            UncheckAllHighlights();
            shadeCellsByPutGetRatioToolStripMenuItem.Checked = !temp;
            ReselectTreeAndCellAfterOptionsUpdate(0);
        }

        void SetChecksForViewMenu()
        {
            arrayValuesToolStripMenuItem.Checked = valuesViewing == DisplayedValues.ArrayValues;
            getValuesToolStripMenuItem.Checked = valuesViewing == DisplayedValues.GetValues;
            putValuesToolStripMenuItem.Checked = valuesViewing == DisplayedValues.PutValues;

            defaultToolStripMenuItem.Enabled = valuesViewing == DisplayedValues.ArrayValues;
            defaultCurrentCellValueToolStripMenuItem.Enabled = valuesViewing == DisplayedValues.ArrayValues;
        }

        // View -> Array Values
        private void arrayValuesToolStripMenuItem_Click(object sender,EventArgs e)
        {
            valuesViewing = DisplayedValues.ArrayValues;
            SetChecksForViewMenu();
            ReselectTreeAndCellAfterOptionsUpdate(0);
        }

        // View -> Get Values
        private void getValuesToolStripMenuItem_Click(object sender,EventArgs e)
        {
            valuesViewing = DisplayedValues.GetValues;
            SetChecksForViewMenu();
            ReselectTreeAndCellAfterOptionsUpdate(0);
        }

        // View -> Put Values
        private void putValuesToolStripMenuItem_Click(object sender,EventArgs e)
        {
            valuesViewing = DisplayedValues.PutValues;
            SetChecksForViewMenu();
            ReselectTreeAndCellAfterOptionsUpdate(0);
        }

        // Help -> About
        private void aboutSaveArrayViewerToolStripMenuItem_Click(object sender,EventArgs e)
        {
            new WinFormsShared.AboutForm("CSPro Save Array Viewer",
                "The Save Array Viewer is a tool used to view and modify save arrays. If batch editing a census or survey, this tool can be used to view and modify the hotdecks used in the edits.",
                Properties.Resources.MainIcon).ShowDialog();
        }
    }
}
