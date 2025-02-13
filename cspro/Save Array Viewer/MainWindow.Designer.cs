namespace SaveArrayViewer
{
    partial class MainWindow
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if( disposing && ( components != null ) )
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle1 = new System.Windows.Forms.DataGridViewCellStyle();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainWindow));
            this.menuStrip1 = new System.Windows.Forms.MenuStrip();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.openToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.saveToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.saveAsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.exitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.optionsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.showZeroIndicesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.highlightDefaultValuesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.showValueSetLabelsForDeckArraysToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.arrayControlToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.reloadFileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.setAllCellsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.currentCellValueToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.defaultToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.setDEFAULTCellsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.defaultCurrentCellValueToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.deleteArrayToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.statisticsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.viewStatisticsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.resetGetAndPutStatisticsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.allTablesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.thisTableToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.highlightCellsWhereGetPutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.highlightCellsWhereGet0ToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.highlightCellsWherePut0ToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.highlightCellsWhereGet0AndPut0ToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.highlightCellsByGetFrequencyToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.top25GetToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.top50GetToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.least50GetToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.least25GetToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.highlightCellsByPutFrequencyToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.top25PutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.top50PutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.least50PutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.least25PutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.shadeCellsByPutGetRatioToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.viewToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.arrayValuesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.getValuesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.putValuesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.helpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.aboutSaveArrayViewerToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.usedInLabel = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.arraysTreeView = new System.Windows.Forms.TreeView();
            this.arrayDataGridView = new System.Windows.Forms.DataGridView();
            this.defLabel = new System.Windows.Forms.Label();
            this.definitionLabel = new System.Windows.Forms.Label();
            this.procsLabel = new System.Windows.Forms.Label();
            this.labelTypeValues = new System.Windows.Forms.Label();
            this.menuStrip1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.arrayDataGridView)).BeginInit();
            this.SuspendLayout();
            //
            // menuStrip1
            //
            this.menuStrip1.BackColor = System.Drawing.SystemColors.MenuBar;
            this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.optionsToolStripMenuItem,
            this.arrayControlToolStripMenuItem,
            this.statisticsToolStripMenuItem,
            this.viewToolStripMenuItem,
            this.helpToolStripMenuItem});
            this.menuStrip1.Location = new System.Drawing.Point(0, 0);
            this.menuStrip1.Name = "menuStrip1";
            this.menuStrip1.Size = new System.Drawing.Size(984, 24);
            this.menuStrip1.TabIndex = 0;
            this.menuStrip1.Text = "menuStrip1";
            //
            // fileToolStripMenuItem
            //
            this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.openToolStripMenuItem,
            this.saveToolStripMenuItem,
            this.saveAsToolStripMenuItem,
            this.toolStripSeparator1,
            this.exitToolStripMenuItem});
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size(37, 20);
            this.fileToolStripMenuItem.Text = "&File";
            //
            // openToolStripMenuItem
            //
            this.openToolStripMenuItem.Name = "openToolStripMenuItem";
            this.openToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.O)));
            this.openToolStripMenuItem.Size = new System.Drawing.Size(155, 22);
            this.openToolStripMenuItem.Text = "&Open...";
            this.openToolStripMenuItem.Click += new System.EventHandler(this.openToolStripMenuItem_Click);
            //
            // saveToolStripMenuItem
            //
            this.saveToolStripMenuItem.Name = "saveToolStripMenuItem";
            this.saveToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.S)));
            this.saveToolStripMenuItem.Size = new System.Drawing.Size(155, 22);
            this.saveToolStripMenuItem.Text = "&Save";
            this.saveToolStripMenuItem.Click += new System.EventHandler(this.saveToolStripMenuItem_Click);
            //
            // saveAsToolStripMenuItem
            //
            this.saveAsToolStripMenuItem.Name = "saveAsToolStripMenuItem";
            this.saveAsToolStripMenuItem.Size = new System.Drawing.Size(155, 22);
            this.saveAsToolStripMenuItem.Text = "Save &As...";
            this.saveAsToolStripMenuItem.Click += new System.EventHandler(this.saveAsToolStripMenuItem_Click);
            //
            // toolStripSeparator1
            //
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(152, 6);
            //
            // exitToolStripMenuItem
            //
            this.exitToolStripMenuItem.Name = "exitToolStripMenuItem";
            this.exitToolStripMenuItem.Size = new System.Drawing.Size(155, 22);
            this.exitToolStripMenuItem.Text = "E&xit";
            this.exitToolStripMenuItem.Click += new System.EventHandler(this.exitToolStripMenuItem_Click);
            //
            // optionsToolStripMenuItem
            //
            this.optionsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.showZeroIndicesToolStripMenuItem,
            this.highlightDefaultValuesToolStripMenuItem,
            this.showValueSetLabelsForDeckArraysToolStripMenuItem});
            this.optionsToolStripMenuItem.Name = "optionsToolStripMenuItem";
            this.optionsToolStripMenuItem.Size = new System.Drawing.Size(61, 20);
            this.optionsToolStripMenuItem.Text = "&Options";
            //
            // showZeroIndicesToolStripMenuItem
            //
            this.showZeroIndicesToolStripMenuItem.Name = "showZeroIndicesToolStripMenuItem";
            this.showZeroIndicesToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.D0)));
            this.showZeroIndicesToolStripMenuItem.Size = new System.Drawing.Size(269, 22);
            this.showZeroIndicesToolStripMenuItem.Text = "Show &Zero Indices";
            this.showZeroIndicesToolStripMenuItem.Click += new System.EventHandler(this.showZeroIndicesToolStripMenuItem_Click);
            //
            // highlightDefaultValuesToolStripMenuItem
            //
            this.highlightDefaultValuesToolStripMenuItem.Checked = true;
            this.highlightDefaultValuesToolStripMenuItem.CheckState = System.Windows.Forms.CheckState.Checked;
            this.highlightDefaultValuesToolStripMenuItem.Name = "highlightDefaultValuesToolStripMenuItem";
            this.highlightDefaultValuesToolStripMenuItem.Size = new System.Drawing.Size(269, 22);
            this.highlightDefaultValuesToolStripMenuItem.Text = "Highlight &Default Values";
            this.highlightDefaultValuesToolStripMenuItem.Click += new System.EventHandler(this.highlightDefaultValuesToolStripMenuItem_Click);
            //
            // showValueSetLabelsForDeckArraysToolStripMenuItem
            //
            this.showValueSetLabelsForDeckArraysToolStripMenuItem.Checked = true;
            this.showValueSetLabelsForDeckArraysToolStripMenuItem.CheckState = System.Windows.Forms.CheckState.Checked;
            this.showValueSetLabelsForDeckArraysToolStripMenuItem.Name = "showValueSetLabelsForDeckArraysToolStripMenuItem";
            this.showValueSetLabelsForDeckArraysToolStripMenuItem.Size = new System.Drawing.Size(269, 22);
            this.showValueSetLabelsForDeckArraysToolStripMenuItem.Text = "Show Value Set &Labels for DeckArrays";
            this.showValueSetLabelsForDeckArraysToolStripMenuItem.Click += new System.EventHandler(this.showValueSetLabelsForDeckArraysToolStripMenuItem_Click);
            //
            // arrayControlToolStripMenuItem
            //
            this.arrayControlToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.reloadFileToolStripMenuItem,
            this.setAllCellsToolStripMenuItem,
            this.setDEFAULTCellsToolStripMenuItem,
            this.deleteArrayToolStripMenuItem});
            this.arrayControlToolStripMenuItem.Name = "arrayControlToolStripMenuItem";
            this.arrayControlToolStripMenuItem.Size = new System.Drawing.Size(90, 20);
            this.arrayControlToolStripMenuItem.Text = "&Array Control";
            //
            // reloadFileToolStripMenuItem
            //
            this.reloadFileToolStripMenuItem.Name = "reloadFileToolStripMenuItem";
            this.reloadFileToolStripMenuItem.ShortcutKeys = System.Windows.Forms.Keys.F5;
            this.reloadFileToolStripMenuItem.Size = new System.Drawing.Size(189, 22);
            this.reloadFileToolStripMenuItem.Text = "&Reload Arrays";
            this.reloadFileToolStripMenuItem.Click += new System.EventHandler(this.reloadFileToolStripMenuItem_Click);
            //
            // setAllCellsToolStripMenuItem
            //
            this.setAllCellsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.currentCellValueToolStripMenuItem,
            this.defaultToolStripMenuItem});
            this.setAllCellsToolStripMenuItem.Name = "setAllCellsToolStripMenuItem";
            this.setAllCellsToolStripMenuItem.Size = new System.Drawing.Size(189, 22);
            this.setAllCellsToolStripMenuItem.Text = "Set &All Cells";
            //
            // currentCellValueToolStripMenuItem
            //
            this.currentCellValueToolStripMenuItem.Name = "currentCellValueToolStripMenuItem";
            this.currentCellValueToolStripMenuItem.Size = new System.Drawing.Size(168, 22);
            this.currentCellValueToolStripMenuItem.Text = "&Current Cell Value";
            this.currentCellValueToolStripMenuItem.Click += new System.EventHandler(this.currentCellValueToolStripMenuItem_Click);
            //
            // defaultToolStripMenuItem
            //
            this.defaultToolStripMenuItem.Name = "defaultToolStripMenuItem";
            this.defaultToolStripMenuItem.Size = new System.Drawing.Size(168, 22);
            this.defaultToolStripMenuItem.Text = "&Default";
            this.defaultToolStripMenuItem.Click += new System.EventHandler(this.defaultToolStripMenuItem_Click);
            //
            // setDEFAULTCellsToolStripMenuItem
            //
            this.setDEFAULTCellsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.defaultCurrentCellValueToolStripMenuItem});
            this.setDEFAULTCellsToolStripMenuItem.Name = "setDEFAULTCellsToolStripMenuItem";
            this.setDEFAULTCellsToolStripMenuItem.Size = new System.Drawing.Size(189, 22);
            this.setDEFAULTCellsToolStripMenuItem.Text = "Set Default &Cells";
            //
            // defaultCurrentCellValueToolStripMenuItem
            //
            this.defaultCurrentCellValueToolStripMenuItem.Name = "defaultCurrentCellValueToolStripMenuItem";
            this.defaultCurrentCellValueToolStripMenuItem.Size = new System.Drawing.Size(168, 22);
            this.defaultCurrentCellValueToolStripMenuItem.Text = "&Current Cell Value";
            this.defaultCurrentCellValueToolStripMenuItem.Click += new System.EventHandler(this.defaultCurrentCellValueToolStripMenuItem_Click);
            //
            // deleteArrayToolStripMenuItem
            //
            this.deleteArrayToolStripMenuItem.Name = "deleteArrayToolStripMenuItem";
            this.deleteArrayToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Delete)));
            this.deleteArrayToolStripMenuItem.Size = new System.Drawing.Size(189, 22);
            this.deleteArrayToolStripMenuItem.Text = "&Delete Array";
            this.deleteArrayToolStripMenuItem.Click += new System.EventHandler(this.deleteArrayToolStripMenuItem_Click);
            //
            // statisticsToolStripMenuItem
            //
            this.statisticsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.viewStatisticsToolStripMenuItem,
            this.resetGetAndPutStatisticsToolStripMenuItem,
            this.highlightCellsWhereGetPutToolStripMenuItem,
            this.highlightCellsWhereGet0ToolStripMenuItem,
            this.highlightCellsWherePut0ToolStripMenuItem,
            this.highlightCellsWhereGet0AndPut0ToolStripMenuItem,
            this.highlightCellsByGetFrequencyToolStripMenuItem,
            this.highlightCellsByPutFrequencyToolStripMenuItem,
            this.shadeCellsByPutGetRatioToolStripMenuItem});
            this.statisticsToolStripMenuItem.Name = "statisticsToolStripMenuItem";
            this.statisticsToolStripMenuItem.Size = new System.Drawing.Size(65, 20);
            this.statisticsToolStripMenuItem.Text = "&Statistics";
            //
            // viewStatisticsToolStripMenuItem
            //
            this.viewStatisticsToolStripMenuItem.Name = "viewStatisticsToolStripMenuItem";
            this.viewStatisticsToolStripMenuItem.ShortcutKeys = System.Windows.Forms.Keys.F8;
            this.viewStatisticsToolStripMenuItem.Size = new System.Drawing.Size(294, 22);
            this.viewStatisticsToolStripMenuItem.Text = "&View Save Array Statistics";
            this.viewStatisticsToolStripMenuItem.Click += new System.EventHandler(this.viewStatisticsToolStripMenuItem_Click);
            //
            // resetGetAndPutStatisticsToolStripMenuItem
            //
            this.resetGetAndPutStatisticsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.allTablesToolStripMenuItem,
            this.thisTableToolStripMenuItem});
            this.resetGetAndPutStatisticsToolStripMenuItem.Name = "resetGetAndPutStatisticsToolStripMenuItem";
            this.resetGetAndPutStatisticsToolStripMenuItem.Size = new System.Drawing.Size(294, 22);
            this.resetGetAndPutStatisticsToolStripMenuItem.Text = "Reset Statistics";
            //
            // allTablesToolStripMenuItem
            //
            this.allTablesToolStripMenuItem.Name = "allTablesToolStripMenuItem";
            this.allTablesToolStripMenuItem.Size = new System.Drawing.Size(155, 22);
            this.allTablesToolStripMenuItem.Text = "&All Arrays";
            this.allTablesToolStripMenuItem.Click += new System.EventHandler(this.allTablesToolStripMenuItem_Click);
            //
            // thisTableToolStripMenuItem
            //
            this.thisTableToolStripMenuItem.Name = "thisTableToolStripMenuItem";
            this.thisTableToolStripMenuItem.Size = new System.Drawing.Size(155, 22);
            this.thisTableToolStripMenuItem.Text = "&This Array Only";
            this.thisTableToolStripMenuItem.Click += new System.EventHandler(this.thisTableToolStripMenuItem_Click);
            //
            // highlightCellsWhereGetPutToolStripMenuItem
            //
            this.highlightCellsWhereGetPutToolStripMenuItem.Name = "highlightCellsWhereGetPutToolStripMenuItem";
            this.highlightCellsWhereGetPutToolStripMenuItem.Size = new System.Drawing.Size(294, 22);
            this.highlightCellsWhereGetPutToolStripMenuItem.Text = "Highlight Cells Where Get > Put";
            this.highlightCellsWhereGetPutToolStripMenuItem.Click += new System.EventHandler(this.highlightCellsWhereGetPutToolStripMenuItem_Click);
            //
            // highlightCellsWhereGet0ToolStripMenuItem
            //
            this.highlightCellsWhereGet0ToolStripMenuItem.Name = "highlightCellsWhereGet0ToolStripMenuItem";
            this.highlightCellsWhereGet0ToolStripMenuItem.Size = new System.Drawing.Size(294, 22);
            this.highlightCellsWhereGet0ToolStripMenuItem.Text = "Highlight Cells Where Get = 0";
            this.highlightCellsWhereGet0ToolStripMenuItem.Click += new System.EventHandler(this.highlightCellsWhereGet0ToolStripMenuItem_Click);
            //
            // highlightCellsWherePut0ToolStripMenuItem
            //
            this.highlightCellsWherePut0ToolStripMenuItem.Name = "highlightCellsWherePut0ToolStripMenuItem";
            this.highlightCellsWherePut0ToolStripMenuItem.Size = new System.Drawing.Size(294, 22);
            this.highlightCellsWherePut0ToolStripMenuItem.Text = "Highlight Cells Where Put = 0";
            this.highlightCellsWherePut0ToolStripMenuItem.Click += new System.EventHandler(this.highlightCellsWherePut0ToolStripMenuItem_Click);
            //
            // highlightCellsWhereGet0AndPut0ToolStripMenuItem
            //
            this.highlightCellsWhereGet0AndPut0ToolStripMenuItem.Name = "highlightCellsWhereGet0AndPut0ToolStripMenuItem";
            this.highlightCellsWhereGet0AndPut0ToolStripMenuItem.Size = new System.Drawing.Size(294, 22);
            this.highlightCellsWhereGet0AndPut0ToolStripMenuItem.Text = "Highlight Cells Where Get = 0 and Put = 0";
            this.highlightCellsWhereGet0AndPut0ToolStripMenuItem.Click += new System.EventHandler(this.highlightCellsWhereGet0AndPut0ToolStripMenuItem_Click);
            //
            // highlightCellsByGetFrequencyToolStripMenuItem
            //
            this.highlightCellsByGetFrequencyToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.top25GetToolStripMenuItem,
            this.top50GetToolStripMenuItem,
            this.least50GetToolStripMenuItem,
            this.least25GetToolStripMenuItem});
            this.highlightCellsByGetFrequencyToolStripMenuItem.Name = "highlightCellsByGetFrequencyToolStripMenuItem";
            this.highlightCellsByGetFrequencyToolStripMenuItem.Size = new System.Drawing.Size(294, 22);
            this.highlightCellsByGetFrequencyToolStripMenuItem.Text = "Highlight Cells by &Get Frequency";
            //
            // top25GetToolStripMenuItem
            //
            this.top25GetToolStripMenuItem.Name = "top25GetToolStripMenuItem";
            this.top25GetToolStripMenuItem.Size = new System.Drawing.Size(126, 22);
            this.top25GetToolStripMenuItem.Text = "Top 25%";
            this.top25GetToolStripMenuItem.Click += new System.EventHandler(this.top25GetToolStripMenuItem_Click);
            //
            // top50GetToolStripMenuItem
            //
            this.top50GetToolStripMenuItem.Name = "top50GetToolStripMenuItem";
            this.top50GetToolStripMenuItem.Size = new System.Drawing.Size(126, 22);
            this.top50GetToolStripMenuItem.Text = "Top 50%";
            this.top50GetToolStripMenuItem.Click += new System.EventHandler(this.top50GetToolStripMenuItem_Click);
            //
            // least50GetToolStripMenuItem
            //
            this.least50GetToolStripMenuItem.Name = "least50GetToolStripMenuItem";
            this.least50GetToolStripMenuItem.Size = new System.Drawing.Size(126, 22);
            this.least50GetToolStripMenuItem.Text = "Least 50%";
            this.least50GetToolStripMenuItem.Click += new System.EventHandler(this.least50GetToolStripMenuItem_Click);
            //
            // least25GetToolStripMenuItem
            //
            this.least25GetToolStripMenuItem.Name = "least25GetToolStripMenuItem";
            this.least25GetToolStripMenuItem.Size = new System.Drawing.Size(126, 22);
            this.least25GetToolStripMenuItem.Text = "Least 25%";
            this.least25GetToolStripMenuItem.Click += new System.EventHandler(this.least25GetToolStripMenuItem_Click);
            //
            // highlightCellsByPutFrequencyToolStripMenuItem
            //
            this.highlightCellsByPutFrequencyToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.top25PutToolStripMenuItem,
            this.top50PutToolStripMenuItem,
            this.least50PutToolStripMenuItem,
            this.least25PutToolStripMenuItem});
            this.highlightCellsByPutFrequencyToolStripMenuItem.Name = "highlightCellsByPutFrequencyToolStripMenuItem";
            this.highlightCellsByPutFrequencyToolStripMenuItem.Size = new System.Drawing.Size(294, 22);
            this.highlightCellsByPutFrequencyToolStripMenuItem.Text = "Highlight Cells by &Put Frequency";
            //
            // top25PutToolStripMenuItem
            //
            this.top25PutToolStripMenuItem.Name = "top25PutToolStripMenuItem";
            this.top25PutToolStripMenuItem.Size = new System.Drawing.Size(126, 22);
            this.top25PutToolStripMenuItem.Text = "Top 25%";
            this.top25PutToolStripMenuItem.Click += new System.EventHandler(this.top25PutToolStripMenuItem_Click);
            //
            // top50PutToolStripMenuItem
            //
            this.top50PutToolStripMenuItem.Name = "top50PutToolStripMenuItem";
            this.top50PutToolStripMenuItem.Size = new System.Drawing.Size(126, 22);
            this.top50PutToolStripMenuItem.Text = "Top 50%";
            this.top50PutToolStripMenuItem.Click += new System.EventHandler(this.top50PutToolStripMenuItem_Click);
            //
            // least50PutToolStripMenuItem
            //
            this.least50PutToolStripMenuItem.Name = "least50PutToolStripMenuItem";
            this.least50PutToolStripMenuItem.Size = new System.Drawing.Size(126, 22);
            this.least50PutToolStripMenuItem.Text = "Least 50%";
            this.least50PutToolStripMenuItem.Click += new System.EventHandler(this.least50PutToolStripMenuItem_Click);
            //
            // least25PutToolStripMenuItem
            //
            this.least25PutToolStripMenuItem.Name = "least25PutToolStripMenuItem";
            this.least25PutToolStripMenuItem.Size = new System.Drawing.Size(126, 22);
            this.least25PutToolStripMenuItem.Text = "Least 25%";
            this.least25PutToolStripMenuItem.Click += new System.EventHandler(this.least25PutToolStripMenuItem_Click);
            //
            // shadeCellsByPutGetRatioToolStripMenuItem
            //
            this.shadeCellsByPutGetRatioToolStripMenuItem.Name = "shadeCellsByPutGetRatioToolStripMenuItem";
            this.shadeCellsByPutGetRatioToolStripMenuItem.Size = new System.Drawing.Size(294, 22);
            this.shadeCellsByPutGetRatioToolStripMenuItem.Text = "&Shade Cells by Put : Get Ratio";
            this.shadeCellsByPutGetRatioToolStripMenuItem.Click += new System.EventHandler(this.shadeCellsByPutGetRatioToolStripMenuItem_Click);
            //
            // viewToolStripMenuItem
            //
            this.viewToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.arrayValuesToolStripMenuItem,
            this.getValuesToolStripMenuItem,
            this.putValuesToolStripMenuItem});
            this.viewToolStripMenuItem.Name = "viewToolStripMenuItem";
            this.viewToolStripMenuItem.Size = new System.Drawing.Size(44, 20);
            this.viewToolStripMenuItem.Text = "&View";
            //
            // arrayValuesToolStripMenuItem
            //
            this.arrayValuesToolStripMenuItem.Name = "arrayValuesToolStripMenuItem";
            this.arrayValuesToolStripMenuItem.ShortcutKeys = System.Windows.Forms.Keys.F10;
            this.arrayValuesToolStripMenuItem.Size = new System.Drawing.Size(163, 22);
            this.arrayValuesToolStripMenuItem.Text = "&Array Values";
            this.arrayValuesToolStripMenuItem.Click += new System.EventHandler(this.arrayValuesToolStripMenuItem_Click);
            //
            // getValuesToolStripMenuItem
            //
            this.getValuesToolStripMenuItem.Name = "getValuesToolStripMenuItem";
            this.getValuesToolStripMenuItem.ShortcutKeys = System.Windows.Forms.Keys.F11;
            this.getValuesToolStripMenuItem.Size = new System.Drawing.Size(163, 22);
            this.getValuesToolStripMenuItem.Text = "&Get Values";
            this.getValuesToolStripMenuItem.Click += new System.EventHandler(this.getValuesToolStripMenuItem_Click);
            //
            // putValuesToolStripMenuItem
            //
            this.putValuesToolStripMenuItem.Name = "putValuesToolStripMenuItem";
            this.putValuesToolStripMenuItem.ShortcutKeys = System.Windows.Forms.Keys.F12;
            this.putValuesToolStripMenuItem.Size = new System.Drawing.Size(163, 22);
            this.putValuesToolStripMenuItem.Text = "&Put Values";
            this.putValuesToolStripMenuItem.Click += new System.EventHandler(this.putValuesToolStripMenuItem_Click);
            //
            // helpToolStripMenuItem
            //
            this.helpToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.aboutSaveArrayViewerToolStripMenuItem});
            this.helpToolStripMenuItem.Name = "helpToolStripMenuItem";
            this.helpToolStripMenuItem.Size = new System.Drawing.Size(44, 20);
            this.helpToolStripMenuItem.Text = "&Help";
            //
            // aboutSaveArrayViewerToolStripMenuItem
            //
            this.aboutSaveArrayViewerToolStripMenuItem.Name = "aboutSaveArrayViewerToolStripMenuItem";
            this.aboutSaveArrayViewerToolStripMenuItem.Size = new System.Drawing.Size(203, 22);
            this.aboutSaveArrayViewerToolStripMenuItem.Text = "&About Save Array Viewer";
            this.aboutSaveArrayViewerToolStripMenuItem.Click += new System.EventHandler(this.aboutSaveArrayViewerToolStripMenuItem_Click);
            //
            // usedInLabel
            //
            this.usedInLabel.AutoSize = true;
            this.usedInLabel.Location = new System.Drawing.Point(265, 42);
            this.usedInLabel.Name = "usedInLabel";
            this.usedInLabel.Size = new System.Drawing.Size(47, 13);
            this.usedInLabel.TabIndex = 6;
            this.usedInLabel.Text = "Used In:";
            this.usedInLabel.Visible = false;
            //
            // label1
            //
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(9, 42);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(64, 13);
            this.label1.TabIndex = 5;
            this.label1.Text = "Save Arrays";
            //
            // arraysTreeView
            //
            this.arraysTreeView.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)));
            this.arraysTreeView.HideSelection = false;
            this.arraysTreeView.Location = new System.Drawing.Point(12, 58);
            this.arraysTreeView.Name = "arraysTreeView";
            this.arraysTreeView.Size = new System.Drawing.Size(250, 492);
            this.arraysTreeView.TabIndex = 7;
            this.arraysTreeView.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.arraysTreeView_AfterSelect);
            //
            // arrayDataGridView
            //
            this.arrayDataGridView.AllowUserToAddRows = false;
            this.arrayDataGridView.AllowUserToDeleteRows = false;
            dataGridViewCellStyle1.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(235)))), ((int)(((byte)(245)))), ((int)(((byte)(255)))));
            this.arrayDataGridView.AlternatingRowsDefaultCellStyle = dataGridViewCellStyle1;
            this.arrayDataGridView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.arrayDataGridView.AutoSizeColumnsMode = System.Windows.Forms.DataGridViewAutoSizeColumnsMode.AllCells;
            this.arrayDataGridView.BackgroundColor = System.Drawing.SystemColors.Window;
            this.arrayDataGridView.ClipboardCopyMode = System.Windows.Forms.DataGridViewClipboardCopyMode.EnableAlwaysIncludeHeaderText;
            this.arrayDataGridView.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.arrayDataGridView.Location = new System.Drawing.Point(268, 58);
            this.arrayDataGridView.Name = "arrayDataGridView";
            this.arrayDataGridView.RowHeadersWidthSizeMode = System.Windows.Forms.DataGridViewRowHeadersWidthSizeMode.AutoSizeToAllHeaders;
            this.arrayDataGridView.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.CellSelect;
            this.arrayDataGridView.ShowEditingIcon = false;
            this.arrayDataGridView.ShowRowErrors = false;
            this.arrayDataGridView.Size = new System.Drawing.Size(704, 492);
            this.arrayDataGridView.TabIndex = 8;
            this.arrayDataGridView.CellEndEdit += new System.Windows.Forms.DataGridViewCellEventHandler(this.arrayDataGridView_CellEndEdit);
            this.arrayDataGridView.CellValidating += new System.Windows.Forms.DataGridViewCellValidatingEventHandler(this.arrayDataGridView_CellValidating);
            this.arrayDataGridView.KeyDown += new System.Windows.Forms.KeyEventHandler(this.arrayDataGridView_KeyDown);
            //
            // defLabel
            //
            this.defLabel.AutoSize = true;
            this.defLabel.Location = new System.Drawing.Point(265, 26);
            this.defLabel.Name = "defLabel";
            this.defLabel.Size = new System.Drawing.Size(54, 13);
            this.defLabel.TabIndex = 9;
            this.defLabel.Text = "Definition:";
            this.defLabel.Visible = false;
            //
            // definitionLabel
            //
            this.definitionLabel.AutoSize = true;
            this.definitionLabel.Location = new System.Drawing.Point(328, 26);
            this.definitionLabel.Name = "definitionLabel";
            this.definitionLabel.Size = new System.Drawing.Size(13, 13);
            this.definitionLabel.TabIndex = 11;
            this.definitionLabel.Text = "  ";
            this.definitionLabel.Visible = false;
            //
            // procsLabel
            //
            this.procsLabel.AutoSize = true;
            this.procsLabel.Location = new System.Drawing.Point(328, 42);
            this.procsLabel.Name = "procsLabel";
            this.procsLabel.Size = new System.Drawing.Size(13, 13);
            this.procsLabel.TabIndex = 10;
            this.procsLabel.Text = "  ";
            this.procsLabel.Visible = false;
            //
            // labelTypeValues
            //
            this.labelTypeValues.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.labelTypeValues.AutoSize = true;
            this.labelTypeValues.Location = new System.Drawing.Point(951, 42);
            this.labelTypeValues.Name = "labelTypeValues";
            this.labelTypeValues.Size = new System.Drawing.Size(0, 13);
            this.labelTypeValues.TabIndex = 12;
            this.labelTypeValues.TextAlign = System.Drawing.ContentAlignment.TopRight;
            //
            // MainWindow
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(984, 562);
            this.Controls.Add(this.labelTypeValues);
            this.Controls.Add(this.definitionLabel);
            this.Controls.Add(this.procsLabel);
            this.Controls.Add(this.defLabel);
            this.Controls.Add(this.arrayDataGridView);
            this.Controls.Add(this.arraysTreeView);
            this.Controls.Add(this.usedInLabel);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.menuStrip1);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MainMenuStrip = this.menuStrip1;
            this.MinimumSize = new System.Drawing.Size(500, 300);
            this.Name = "MainWindow";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "CSPro Save Array Viewer";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.MainWindow_FormClosing);
            this.Shown += new System.EventHandler(this.MainWindow_Shown);
            this.menuStrip1.ResumeLayout(false);
            this.menuStrip1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.arrayDataGridView)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.MenuStrip menuStrip1;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem openToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem saveToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem saveAsToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripMenuItem exitToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem optionsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem showZeroIndicesToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem highlightDefaultValuesToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem showValueSetLabelsForDeckArraysToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem statisticsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem viewStatisticsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem highlightCellsWhereGetPutToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem highlightCellsWhereGet0ToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem highlightCellsWherePut0ToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem highlightCellsWhereGet0AndPut0ToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem highlightCellsByPutFrequencyToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem top25PutToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem top50PutToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem least50PutToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem least25PutToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem shadeCellsByPutGetRatioToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem highlightCellsByGetFrequencyToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem top25GetToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem top50GetToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem least50GetToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem least25GetToolStripMenuItem;
        private System.Windows.Forms.Label usedInLabel;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TreeView arraysTreeView;
        private System.Windows.Forms.DataGridView arrayDataGridView;
        private System.Windows.Forms.Label defLabel;
        private System.Windows.Forms.ToolStripMenuItem resetGetAndPutStatisticsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem allTablesToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem thisTableToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem arrayControlToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem setAllCellsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem currentCellValueToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem defaultToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem setDEFAULTCellsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem defaultCurrentCellValueToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem deleteArrayToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem reloadFileToolStripMenuItem;
        private System.Windows.Forms.Label definitionLabel;
        private System.Windows.Forms.Label procsLabel;
        private System.Windows.Forms.ToolStripMenuItem viewToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem arrayValuesToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem getValuesToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem putValuesToolStripMenuItem;
        private System.Windows.Forms.Label labelTypeValues;
        private System.Windows.Forms.ToolStripMenuItem helpToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem aboutSaveArrayViewerToolStripMenuItem;
    }
}

