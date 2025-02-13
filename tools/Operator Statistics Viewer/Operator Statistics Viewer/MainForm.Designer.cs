namespace Operator_Statistics_Viewer
{
    partial class MainForm
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
            this.menuStrip1 = new System.Windows.Forms.MenuStrip();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.addLogToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.bulkAddLogsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.closeAllToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.openViewingConfigurationToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.saveViewingConfigurationToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.exitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.specifyToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.operationStatisticsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.operatorNamesFileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.viewToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.displayTimeInDecimalFormatToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.displayOperatorIDsInsteadOfNameToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.visualizeToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.numberOfKeyersByDayToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.keyingTimeByDayToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.keyingRateByDayToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            this.casesByDayToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.cumulativeCasesByDayToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.cumulativePercentOfCasesVerifiedByDayToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator4 = new System.Windows.Forms.ToolStripSeparator();
            this.cumulativeCasesByKeyerToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.keyingTimeByKeyerToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.keyingRateByKeyerToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.operatorsTreeView = new System.Windows.Forms.TreeView();
            this.label1 = new System.Windows.Forms.Label();
            this.checkBoxAdd = new System.Windows.Forms.CheckBox();
            this.checkBoxModify = new System.Windows.Forms.CheckBox();
            this.checkBoxVerify = new System.Windows.Forms.CheckBox();
            this.labelEntry = new System.Windows.Forms.Label();
            this.comboBoxTimePeriod = new System.Windows.Forms.ComboBox();
            this.labelTimePeriod = new System.Windows.Forms.Label();
            this.tabControl = new System.Windows.Forms.TabControl();
            this.tabPageSummary = new System.Windows.Forms.TabPage();
            this.label20 = new System.Windows.Forms.Label();
            this.label19 = new System.Windows.Forms.Label();
            this.label18 = new System.Windows.Forms.Label();
            this.label17 = new System.Windows.Forms.Label();
            this.label16 = new System.Windows.Forms.Label();
            this.label15 = new System.Windows.Forms.Label();
            this.label14 = new System.Windows.Forms.Label();
            this.label13 = new System.Windows.Forms.Label();
            this.label12 = new System.Windows.Forms.Label();
            this.label11 = new System.Windows.Forms.Label();
            this.label10 = new System.Windows.Forms.Label();
            this.label9 = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.label7 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.labelCasesRemaining = new System.Windows.Forms.Label();
            this.labelExpectedCases = new System.Windows.Forms.Label();
            this.labelCasesEntered = new System.Windows.Forms.Label();
            this.progressBarSummary = new System.Windows.Forms.ProgressBar();
            this.tabPageLogs = new System.Windows.Forms.TabPage();
            this.tabPageDailyReport = new System.Windows.Forms.TabPage();
            this.tabPageWeeklyReport = new System.Windows.Forms.TabPage();
            this.tabPageMonthlyReport = new System.Windows.Forms.TabPage();
            this.tabPageOperatorReport = new System.Windows.Forms.TabPage();
            this.tabPageOperatorPayroll = new System.Windows.Forms.TabPage();
            this.tabPageVisualize = new System.Windows.Forms.TabPage();
            this.checkBoxHideZeroRows = new System.Windows.Forms.CheckBox();
            this.dateTimePickerStart = new System.Windows.Forms.DateTimePicker();
            this.dateTimePickerEnd = new System.Windows.Forms.DateTimePicker();
            this.labelInterval = new System.Windows.Forms.Label();
            this.comboBoxInterval = new System.Windows.Forms.ComboBox();
            this.checkBoxVisualizeShowLabels = new System.Windows.Forms.CheckBox();
            this.checkBoxVisualizeSortXAxis = new System.Windows.Forms.CheckBox();
            this.linkLabelCopyChart = new System.Windows.Forms.LinkLabel();
            this.menuStrip1.SuspendLayout();
            this.tabControl.SuspendLayout();
            this.tabPageSummary.SuspendLayout();
            this.SuspendLayout();
            // 
            // menuStrip1
            // 
            this.menuStrip1.BackColor = System.Drawing.SystemColors.MenuBar;
            this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.specifyToolStripMenuItem,
            this.viewToolStripMenuItem,
            this.visualizeToolStripMenuItem});
            this.menuStrip1.Location = new System.Drawing.Point(0, 0);
            this.menuStrip1.Name = "menuStrip1";
            this.menuStrip1.Size = new System.Drawing.Size(942, 24);
            this.menuStrip1.TabIndex = 0;
            this.menuStrip1.Text = "menuStrip1";
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.addLogToolStripMenuItem,
            this.bulkAddLogsToolStripMenuItem,
            this.closeAllToolStripMenuItem,
            this.toolStripSeparator1,
            this.openViewingConfigurationToolStripMenuItem,
            this.saveViewingConfigurationToolStripMenuItem,
            this.toolStripSeparator2,
            this.exitToolStripMenuItem});
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size(37, 20);
            this.fileToolStripMenuItem.Text = "&File";
            // 
            // addLogToolStripMenuItem
            // 
            this.addLogToolStripMenuItem.Name = "addLogToolStripMenuItem";
            this.addLogToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.L)));
            this.addLogToolStripMenuItem.Size = new System.Drawing.Size(277, 22);
            this.addLogToolStripMenuItem.Text = "&Add Log...";
            this.addLogToolStripMenuItem.Click += new System.EventHandler(this.addLogToolStripMenuItem_Click);
            // 
            // bulkAddLogsToolStripMenuItem
            // 
            this.bulkAddLogsToolStripMenuItem.Name = "bulkAddLogsToolStripMenuItem";
            this.bulkAddLogsToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.B)));
            this.bulkAddLogsToolStripMenuItem.Size = new System.Drawing.Size(277, 22);
            this.bulkAddLogsToolStripMenuItem.Text = "&Bulk Add Logs...";
            this.bulkAddLogsToolStripMenuItem.Click += new System.EventHandler(this.bulkAddLogsToolStripMenuItem_Click);
            // 
            // closeAllToolStripMenuItem
            // 
            this.closeAllToolStripMenuItem.Name = "closeAllToolStripMenuItem";
            this.closeAllToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.W)));
            this.closeAllToolStripMenuItem.Size = new System.Drawing.Size(277, 22);
            this.closeAllToolStripMenuItem.Text = "Clos&e All";
            this.closeAllToolStripMenuItem.Click += new System.EventHandler(this.closeAllToolStripMenuItem_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(274, 6);
            // 
            // openViewingConfigurationToolStripMenuItem
            // 
            this.openViewingConfigurationToolStripMenuItem.Name = "openViewingConfigurationToolStripMenuItem";
            this.openViewingConfigurationToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.O)));
            this.openViewingConfigurationToolStripMenuItem.Size = new System.Drawing.Size(277, 22);
            this.openViewingConfigurationToolStripMenuItem.Text = "&Open Viewing Configuration...";
            this.openViewingConfigurationToolStripMenuItem.Click += new System.EventHandler(this.openViewingConfigurationToolStripMenuItem_Click);
            // 
            // saveViewingConfigurationToolStripMenuItem
            // 
            this.saveViewingConfigurationToolStripMenuItem.Name = "saveViewingConfigurationToolStripMenuItem";
            this.saveViewingConfigurationToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.S)));
            this.saveViewingConfigurationToolStripMenuItem.Size = new System.Drawing.Size(277, 22);
            this.saveViewingConfigurationToolStripMenuItem.Text = "&Save Viewing Configuration...";
            this.saveViewingConfigurationToolStripMenuItem.Click += new System.EventHandler(this.saveViewingConfigurationToolStripMenuItem_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(274, 6);
            // 
            // exitToolStripMenuItem
            // 
            this.exitToolStripMenuItem.Name = "exitToolStripMenuItem";
            this.exitToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Alt | System.Windows.Forms.Keys.F4)));
            this.exitToolStripMenuItem.Size = new System.Drawing.Size(277, 22);
            this.exitToolStripMenuItem.Text = "E&xit";
            this.exitToolStripMenuItem.Click += new System.EventHandler(this.exitToolStripMenuItem_Click);
            // 
            // specifyToolStripMenuItem
            // 
            this.specifyToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.operationStatisticsToolStripMenuItem,
            this.operatorNamesFileToolStripMenuItem});
            this.specifyToolStripMenuItem.Name = "specifyToolStripMenuItem";
            this.specifyToolStripMenuItem.Size = new System.Drawing.Size(57, 20);
            this.specifyToolStripMenuItem.Text = "&Specify";
            // 
            // operationStatisticsToolStripMenuItem
            // 
            this.operationStatisticsToolStripMenuItem.Name = "operationStatisticsToolStripMenuItem";
            this.operationStatisticsToolStripMenuItem.Size = new System.Drawing.Size(189, 22);
            this.operationStatisticsToolStripMenuItem.Text = "Operation &Parameters";
            this.operationStatisticsToolStripMenuItem.Click += new System.EventHandler(this.operationStatisticsToolStripMenuItem_Click);
            // 
            // operatorNamesFileToolStripMenuItem
            // 
            this.operatorNamesFileToolStripMenuItem.Name = "operatorNamesFileToolStripMenuItem";
            this.operatorNamesFileToolStripMenuItem.Size = new System.Drawing.Size(189, 22);
            this.operatorNamesFileToolStripMenuItem.Text = "Operator &Names File";
            this.operatorNamesFileToolStripMenuItem.Click += new System.EventHandler(this.operatorNamesFileToolStripMenuItem_Click);
            // 
            // viewToolStripMenuItem
            // 
            this.viewToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.displayTimeInDecimalFormatToolStripMenuItem,
            this.displayOperatorIDsInsteadOfNameToolStripMenuItem});
            this.viewToolStripMenuItem.Name = "viewToolStripMenuItem";
            this.viewToolStripMenuItem.Size = new System.Drawing.Size(61, 20);
            this.viewToolStripMenuItem.Text = "&Options";
            // 
            // displayTimeInDecimalFormatToolStripMenuItem
            // 
            this.displayTimeInDecimalFormatToolStripMenuItem.Name = "displayTimeInDecimalFormatToolStripMenuItem";
            this.displayTimeInDecimalFormatToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.T)));
            this.displayTimeInDecimalFormatToolStripMenuItem.Size = new System.Drawing.Size(319, 22);
            this.displayTimeInDecimalFormatToolStripMenuItem.Text = "Display &Time in Decimal Format";
            this.displayTimeInDecimalFormatToolStripMenuItem.Click += new System.EventHandler(this.displayTimeInDecimalFormatToolStripMenuItem_Click);
            // 
            // displayOperatorIDsInsteadOfNameToolStripMenuItem
            // 
            this.displayOperatorIDsInsteadOfNameToolStripMenuItem.Checked = true;
            this.displayOperatorIDsInsteadOfNameToolStripMenuItem.CheckState = System.Windows.Forms.CheckState.Checked;
            this.displayOperatorIDsInsteadOfNameToolStripMenuItem.Name = "displayOperatorIDsInsteadOfNameToolStripMenuItem";
            this.displayOperatorIDsInsteadOfNameToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.N)));
            this.displayOperatorIDsInsteadOfNameToolStripMenuItem.Size = new System.Drawing.Size(319, 22);
            this.displayOperatorIDsInsteadOfNameToolStripMenuItem.Text = "Display Operator IDs Instead of Names";
            this.displayOperatorIDsInsteadOfNameToolStripMenuItem.Click += new System.EventHandler(this.displayOperatorIDsInsteadOfNameToolStripMenuItem_Click);
            // 
            // visualizeToolStripMenuItem
            // 
            this.visualizeToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.numberOfKeyersByDayToolStripMenuItem,
            this.keyingTimeByDayToolStripMenuItem,
            this.keyingRateByDayToolStripMenuItem,
            this.toolStripSeparator3,
            this.casesByDayToolStripMenuItem,
            this.cumulativeCasesByDayToolStripMenuItem,
            this.cumulativePercentOfCasesVerifiedByDayToolStripMenuItem,
            this.toolStripSeparator4,
            this.cumulativeCasesByKeyerToolStripMenuItem,
            this.keyingTimeByKeyerToolStripMenuItem,
            this.keyingRateByKeyerToolStripMenuItem});
            this.visualizeToolStripMenuItem.Name = "visualizeToolStripMenuItem";
            this.visualizeToolStripMenuItem.Size = new System.Drawing.Size(64, 20);
            this.visualizeToolStripMenuItem.Text = "&Visualize";
            // 
            // numberOfKeyersByDayToolStripMenuItem
            // 
            this.numberOfKeyersByDayToolStripMenuItem.Name = "numberOfKeyersByDayToolStripMenuItem";
            this.numberOfKeyersByDayToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.D1)));
            this.numberOfKeyersByDayToolStripMenuItem.Size = new System.Drawing.Size(352, 22);
            this.numberOfKeyersByDayToolStripMenuItem.Text = "Number of Keyers by Time";
            this.numberOfKeyersByDayToolStripMenuItem.Click += new System.EventHandler(this.numberOfKeyersByDayToolStripMenuItem_Click);
            // 
            // keyingTimeByDayToolStripMenuItem
            // 
            this.keyingTimeByDayToolStripMenuItem.Name = "keyingTimeByDayToolStripMenuItem";
            this.keyingTimeByDayToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.D2)));
            this.keyingTimeByDayToolStripMenuItem.Size = new System.Drawing.Size(352, 22);
            this.keyingTimeByDayToolStripMenuItem.Text = "Keying Time by Time";
            this.keyingTimeByDayToolStripMenuItem.Click += new System.EventHandler(this.keyingTimeByDayToolStripMenuItem_Click);
            // 
            // keyingRateByDayToolStripMenuItem
            // 
            this.keyingRateByDayToolStripMenuItem.Name = "keyingRateByDayToolStripMenuItem";
            this.keyingRateByDayToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.D3)));
            this.keyingRateByDayToolStripMenuItem.Size = new System.Drawing.Size(352, 22);
            this.keyingRateByDayToolStripMenuItem.Text = "Keying Rate by Time";
            this.keyingRateByDayToolStripMenuItem.Click += new System.EventHandler(this.keyingRateByDayToolStripMenuItem_Click);
            // 
            // toolStripSeparator3
            // 
            this.toolStripSeparator3.Name = "toolStripSeparator3";
            this.toolStripSeparator3.Size = new System.Drawing.Size(349, 6);
            // 
            // casesByDayToolStripMenuItem
            // 
            this.casesByDayToolStripMenuItem.Name = "casesByDayToolStripMenuItem";
            this.casesByDayToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.D4)));
            this.casesByDayToolStripMenuItem.Size = new System.Drawing.Size(352, 22);
            this.casesByDayToolStripMenuItem.Text = "Cases by Time";
            this.casesByDayToolStripMenuItem.Click += new System.EventHandler(this.casesByDayToolStripMenuItem_Click);
            // 
            // cumulativeCasesByDayToolStripMenuItem
            // 
            this.cumulativeCasesByDayToolStripMenuItem.Name = "cumulativeCasesByDayToolStripMenuItem";
            this.cumulativeCasesByDayToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.D5)));
            this.cumulativeCasesByDayToolStripMenuItem.Size = new System.Drawing.Size(352, 22);
            this.cumulativeCasesByDayToolStripMenuItem.Text = "Cumulative Cases by Time";
            this.cumulativeCasesByDayToolStripMenuItem.Click += new System.EventHandler(this.cumulativeCasesByDayToolStripMenuItem_Click);
            // 
            // cumulativePercentOfCasesVerifiedByDayToolStripMenuItem
            // 
            this.cumulativePercentOfCasesVerifiedByDayToolStripMenuItem.Name = "cumulativePercentOfCasesVerifiedByDayToolStripMenuItem";
            this.cumulativePercentOfCasesVerifiedByDayToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.D6)));
            this.cumulativePercentOfCasesVerifiedByDayToolStripMenuItem.Size = new System.Drawing.Size(352, 22);
            this.cumulativePercentOfCasesVerifiedByDayToolStripMenuItem.Text = "Cumulative Percent of Cases Verified by Time";
            this.cumulativePercentOfCasesVerifiedByDayToolStripMenuItem.Click += new System.EventHandler(this.cumulativePercentOfCasesVerifiedByDayToolStripMenuItem_Click);
            // 
            // toolStripSeparator4
            // 
            this.toolStripSeparator4.Name = "toolStripSeparator4";
            this.toolStripSeparator4.Size = new System.Drawing.Size(349, 6);
            // 
            // cumulativeCasesByKeyerToolStripMenuItem
            // 
            this.cumulativeCasesByKeyerToolStripMenuItem.Name = "cumulativeCasesByKeyerToolStripMenuItem";
            this.cumulativeCasesByKeyerToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.D7)));
            this.cumulativeCasesByKeyerToolStripMenuItem.Size = new System.Drawing.Size(352, 22);
            this.cumulativeCasesByKeyerToolStripMenuItem.Text = "Total Cases by Keyer";
            this.cumulativeCasesByKeyerToolStripMenuItem.Click += new System.EventHandler(this.cumulativeCasesByKeyerToolStripMenuItem_Click);
            // 
            // keyingTimeByKeyerToolStripMenuItem
            // 
            this.keyingTimeByKeyerToolStripMenuItem.Name = "keyingTimeByKeyerToolStripMenuItem";
            this.keyingTimeByKeyerToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.D8)));
            this.keyingTimeByKeyerToolStripMenuItem.Size = new System.Drawing.Size(352, 22);
            this.keyingTimeByKeyerToolStripMenuItem.Text = "Total Time by Keyer";
            this.keyingTimeByKeyerToolStripMenuItem.Click += new System.EventHandler(this.keyingTimeByKeyerToolStripMenuItem_Click);
            // 
            // keyingRateByKeyerToolStripMenuItem
            // 
            this.keyingRateByKeyerToolStripMenuItem.Name = "keyingRateByKeyerToolStripMenuItem";
            this.keyingRateByKeyerToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.D9)));
            this.keyingRateByKeyerToolStripMenuItem.Size = new System.Drawing.Size(352, 22);
            this.keyingRateByKeyerToolStripMenuItem.Text = "Total Keying Rate by Keyer";
            this.keyingRateByKeyerToolStripMenuItem.Click += new System.EventHandler(this.keyingRateByKeyerToolStripMenuItem_Click);
            // 
            // operatorsTreeView
            // 
            this.operatorsTreeView.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left)));
            this.operatorsTreeView.HideSelection = false;
            this.operatorsTreeView.Location = new System.Drawing.Point(15, 55);
            this.operatorsTreeView.Name = "operatorsTreeView";
            this.operatorsTreeView.Size = new System.Drawing.Size(208, 495);
            this.operatorsTreeView.TabIndex = 1;
            this.operatorsTreeView.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.operatorsTreeView_AfterSelect);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 33);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(67, 13);
            this.label1.TabIndex = 2;
            this.label1.Text = "Operator IDs";
            // 
            // checkBoxAdd
            // 
            this.checkBoxAdd.AutoSize = true;
            this.checkBoxAdd.Checked = true;
            this.checkBoxAdd.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxAdd.Location = new System.Drawing.Point(302, 31);
            this.checkBoxAdd.Name = "checkBoxAdd";
            this.checkBoxAdd.Size = new System.Drawing.Size(45, 17);
            this.checkBoxAdd.TabIndex = 3;
            this.checkBoxAdd.Text = "Add";
            this.checkBoxAdd.UseVisualStyleBackColor = true;
            this.checkBoxAdd.CheckedChanged += new System.EventHandler(this.checkBoxAdd_CheckedChanged);
            // 
            // checkBoxModify
            // 
            this.checkBoxModify.AutoSize = true;
            this.checkBoxModify.Checked = true;
            this.checkBoxModify.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxModify.Location = new System.Drawing.Point(353, 31);
            this.checkBoxModify.Name = "checkBoxModify";
            this.checkBoxModify.Size = new System.Drawing.Size(57, 17);
            this.checkBoxModify.TabIndex = 4;
            this.checkBoxModify.Text = "Modify";
            this.checkBoxModify.UseVisualStyleBackColor = true;
            this.checkBoxModify.CheckedChanged += new System.EventHandler(this.checkBoxModify_CheckedChanged);
            // 
            // checkBoxVerify
            // 
            this.checkBoxVerify.AutoSize = true;
            this.checkBoxVerify.Checked = true;
            this.checkBoxVerify.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxVerify.Location = new System.Drawing.Point(410, 31);
            this.checkBoxVerify.Name = "checkBoxVerify";
            this.checkBoxVerify.Size = new System.Drawing.Size(52, 17);
            this.checkBoxVerify.TabIndex = 5;
            this.checkBoxVerify.Text = "Verify";
            this.checkBoxVerify.UseVisualStyleBackColor = true;
            this.checkBoxVerify.CheckedChanged += new System.EventHandler(this.checkBoxVerify_CheckedChanged);
            // 
            // labelEntry
            // 
            this.labelEntry.AutoSize = true;
            this.labelEntry.Location = new System.Drawing.Point(232, 32);
            this.labelEntry.Name = "labelEntry";
            this.labelEntry.Size = new System.Drawing.Size(64, 13);
            this.labelEntry.TabIndex = 6;
            this.labelEntry.Text = "Entry Mode:";
            // 
            // comboBoxTimePeriod
            // 
            this.comboBoxTimePeriod.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.comboBoxTimePeriod.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxTimePeriod.FormattingEnabled = true;
            this.comboBoxTimePeriod.Items.AddRange(new object[] {
            "All Time",
            "Last Month",
            "This Month",
            "Last Week",
            "This Week",
            "Last 7 Days",
            "Yesterday",
            "Today",
            "Custom"});
            this.comboBoxTimePeriod.Location = new System.Drawing.Point(637, 29);
            this.comboBoxTimePeriod.Name = "comboBoxTimePeriod";
            this.comboBoxTimePeriod.Size = new System.Drawing.Size(101, 21);
            this.comboBoxTimePeriod.TabIndex = 7;
            this.comboBoxTimePeriod.SelectedIndexChanged += new System.EventHandler(this.timePeriodComboBox_SelectedIndexChanged);
            // 
            // labelTimePeriod
            // 
            this.labelTimePeriod.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.labelTimePeriod.AutoSize = true;
            this.labelTimePeriod.Location = new System.Drawing.Point(565, 32);
            this.labelTimePeriod.Name = "labelTimePeriod";
            this.labelTimePeriod.Size = new System.Drawing.Size(66, 13);
            this.labelTimePeriod.TabIndex = 8;
            this.labelTimePeriod.Text = "Time Period:";
            // 
            // tabControl
            // 
            this.tabControl.AllowDrop = true;
            this.tabControl.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tabControl.Controls.Add(this.tabPageSummary);
            this.tabControl.Controls.Add(this.tabPageLogs);
            this.tabControl.Controls.Add(this.tabPageDailyReport);
            this.tabControl.Controls.Add(this.tabPageWeeklyReport);
            this.tabControl.Controls.Add(this.tabPageMonthlyReport);
            this.tabControl.Controls.Add(this.tabPageOperatorReport);
            this.tabControl.Controls.Add(this.tabPageOperatorPayroll);
            this.tabControl.Controls.Add(this.tabPageVisualize);
            this.tabControl.Location = new System.Drawing.Point(235, 55);
            this.tabControl.Name = "tabControl";
            this.tabControl.SelectedIndex = 0;
            this.tabControl.Size = new System.Drawing.Size(687, 495);
            this.tabControl.TabIndex = 9;
            this.tabControl.SelectedIndexChanged += new System.EventHandler(this.tabControl_SelectedIndexChanged);
            this.tabControl.DragDrop += new System.Windows.Forms.DragEventHandler(this.tabControl_DragDrop);
            this.tabControl.DragEnter += new System.Windows.Forms.DragEventHandler(this.tabControl_DragEnter);
            // 
            // tabPageSummary
            // 
            this.tabPageSummary.Controls.Add(this.label20);
            this.tabPageSummary.Controls.Add(this.label19);
            this.tabPageSummary.Controls.Add(this.label18);
            this.tabPageSummary.Controls.Add(this.label17);
            this.tabPageSummary.Controls.Add(this.label16);
            this.tabPageSummary.Controls.Add(this.label15);
            this.tabPageSummary.Controls.Add(this.label14);
            this.tabPageSummary.Controls.Add(this.label13);
            this.tabPageSummary.Controls.Add(this.label12);
            this.tabPageSummary.Controls.Add(this.label11);
            this.tabPageSummary.Controls.Add(this.label10);
            this.tabPageSummary.Controls.Add(this.label9);
            this.tabPageSummary.Controls.Add(this.label8);
            this.tabPageSummary.Controls.Add(this.label7);
            this.tabPageSummary.Controls.Add(this.label6);
            this.tabPageSummary.Controls.Add(this.label5);
            this.tabPageSummary.Controls.Add(this.label4);
            this.tabPageSummary.Controls.Add(this.label3);
            this.tabPageSummary.Controls.Add(this.label2);
            this.tabPageSummary.Controls.Add(this.labelCasesRemaining);
            this.tabPageSummary.Controls.Add(this.labelExpectedCases);
            this.tabPageSummary.Controls.Add(this.labelCasesEntered);
            this.tabPageSummary.Controls.Add(this.progressBarSummary);
            this.tabPageSummary.Location = new System.Drawing.Point(4, 22);
            this.tabPageSummary.Name = "tabPageSummary";
            this.tabPageSummary.Padding = new System.Windows.Forms.Padding(3);
            this.tabPageSummary.Size = new System.Drawing.Size(679, 469);
            this.tabPageSummary.TabIndex = 0;
            this.tabPageSummary.Text = "Summary";
            this.tabPageSummary.UseVisualStyleBackColor = true;
            // 
            // label20
            // 
            this.label20.AutoSize = true;
            this.label20.Location = new System.Drawing.Point(236, 131);
            this.label20.Name = "label20";
            this.label20.Size = new System.Drawing.Size(116, 13);
            this.label20.TabIndex = 22;
            this.label20.Text = "Last date of data entry:";
            this.label20.Visible = false;
            // 
            // label19
            // 
            this.label19.AutoSize = true;
            this.label19.Location = new System.Drawing.Point(560, 84);
            this.label19.Name = "label19";
            this.label19.Size = new System.Drawing.Size(104, 13);
            this.label19.TabIndex = 21;
            this.label19.Text = "Selected Operator(s)";
            this.label19.Visible = false;
            // 
            // label18
            // 
            this.label18.AutoSize = true;
            this.label18.Location = new System.Drawing.Point(460, 84);
            this.label18.Name = "label18";
            this.label18.Size = new System.Drawing.Size(59, 13);
            this.label18.TabIndex = 20;
            this.label18.Text = "Last Week";
            this.label18.Visible = false;
            // 
            // label17
            // 
            this.label17.AutoSize = true;
            this.label17.Location = new System.Drawing.Point(276, 167);
            this.label17.Name = "label17";
            this.label17.Size = new System.Drawing.Size(78, 13);
            this.label17.TabIndex = 19;
            this.label17.Text = "Cases entered:";
            this.label17.Visible = false;
            // 
            // label16
            // 
            this.label16.AutoSize = true;
            this.label16.Location = new System.Drawing.Point(9, 433);
            this.label16.Name = "label16";
            this.label16.Size = new System.Drawing.Size(345, 13);
            this.label16.TabIndex = 18;
            this.label16.Text = "Keying rate needed to complete on time using current number of keyers:";
            this.label16.Visible = false;
            // 
            // label15
            // 
            this.label15.AutoSize = true;
            this.label15.Location = new System.Drawing.Point(59, 415);
            this.label15.Name = "label15";
            this.label15.Size = new System.Drawing.Size(295, 13);
            this.label15.TabIndex = 17;
            this.label15.Text = "Keyers needed to complete on time using current keying rate:";
            this.label15.Visible = false;
            // 
            // label14
            // 
            this.label14.AutoSize = true;
            this.label14.Location = new System.Drawing.Point(147, 387);
            this.label14.Name = "label14";
            this.label14.Size = new System.Drawing.Size(207, 13);
            this.label14.TabIndex = 16;
            this.label14.Text = "Expected completion date at current rates:";
            this.label14.Visible = false;
            // 
            // label13
            // 
            this.label13.AutoSize = true;
            this.label13.Location = new System.Drawing.Point(142, 369);
            this.label13.Name = "label13";
            this.label13.Size = new System.Drawing.Size(212, 13);
            this.label13.TabIndex = 15;
            this.label13.Text = "Expected number of keying days remaining:";
            this.label13.Visible = false;
            // 
            // label12
            // 
            this.label12.AutoSize = true;
            this.label12.Location = new System.Drawing.Point(147, 341);
            this.label12.Name = "label12";
            this.label12.Size = new System.Drawing.Size(207, 13);
            this.label12.TabIndex = 14;
            this.label12.Text = "Average keying rate (keystrokes per hour):";
            this.label12.Visible = false;
            // 
            // label11
            // 
            this.label11.AutoSize = true;
            this.label11.Location = new System.Drawing.Point(113, 323);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(241, 13);
            this.label11.TabIndex = 13;
            this.label11.Text = "Average time needed to enter a case (in minutes):";
            this.label11.Visible = false;
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.Location = new System.Drawing.Point(204, 259);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(150, 13);
            this.label10.TabIndex = 12;
            this.label10.Text = "Weeks that data was entered:";
            this.label10.Visible = false;
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Location = new System.Drawing.Point(119, 231);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(235, 13);
            this.label9.TabIndex = 11;
            this.label9.Text = "Average number of keyers working during a day:";
            this.label9.Visible = false;
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(86, 277);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(268, 13);
            this.label8.TabIndex = 10;
            this.label8.Text = "Average number of hours spent in add mode in a week:";
            this.label8.Visible = false;
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(110, 295);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(244, 13);
            this.label7.TabIndex = 9;
            this.label7.Text = "Average number of keyers working during a week:";
            this.label7.Visible = false;
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(237, 113);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(115, 13);
            this.label6.TabIndex = 8;
            this.label6.Text = "First date of data entry:";
            this.label6.Visible = false;
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(95, 213);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(259, 13);
            this.label5.TabIndex = 7;
            this.label5.Text = "Average number of hours spent in add mode in a day:";
            this.label5.Visible = false;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(236, 149);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(118, 13);
            this.label4.TabIndex = 6;
            this.label4.Text = "Total number of keyers:";
            this.label4.Visible = false;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(214, 195);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(140, 13);
            this.label3.TabIndex = 5;
            this.label3.Text = "Days that data was entered:";
            this.label3.Visible = false;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(360, 84);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(44, 13);
            this.label2.TabIndex = 4;
            this.label2.Text = "All Time";
            this.label2.Visible = false;
            // 
            // labelCasesRemaining
            // 
            this.labelCasesRemaining.Anchor = System.Windows.Forms.AnchorStyles.Top;
            this.labelCasesRemaining.AutoSize = true;
            this.labelCasesRemaining.Location = new System.Drawing.Point(322, 37);
            this.labelCasesRemaining.Name = "labelCasesRemaining";
            this.labelCasesRemaining.Size = new System.Drawing.Size(92, 13);
            this.labelCasesRemaining.TabIndex = 3;
            this.labelCasesRemaining.Text = "Cases Remaining:";
            this.labelCasesRemaining.Visible = false;
            // 
            // labelExpectedCases
            // 
            this.labelExpectedCases.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.labelExpectedCases.AutoSize = true;
            this.labelExpectedCases.Location = new System.Drawing.Point(534, 37);
            this.labelExpectedCases.Name = "labelExpectedCases";
            this.labelExpectedCases.Size = new System.Drawing.Size(139, 13);
            this.labelExpectedCases.TabIndex = 2;
            this.labelExpectedCases.Text = "Expected Number of Cases:";
            this.labelExpectedCases.Visible = false;
            // 
            // labelCasesEntered
            // 
            this.labelCasesEntered.AutoSize = true;
            this.labelCasesEntered.Location = new System.Drawing.Point(4, 37);
            this.labelCasesEntered.Name = "labelCasesEntered";
            this.labelCasesEntered.Size = new System.Drawing.Size(79, 13);
            this.labelCasesEntered.TabIndex = 1;
            this.labelCasesEntered.Text = "Cases Entered:";
            this.labelCasesEntered.Visible = false;
            // 
            // progressBarSummary
            // 
            this.progressBarSummary.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.progressBarSummary.Location = new System.Drawing.Point(7, 7);
            this.progressBarSummary.Name = "progressBarSummary";
            this.progressBarSummary.Size = new System.Drawing.Size(666, 23);
            this.progressBarSummary.TabIndex = 0;
            this.progressBarSummary.Visible = false;
            // 
            // tabPageLogs
            // 
            this.tabPageLogs.Location = new System.Drawing.Point(4, 22);
            this.tabPageLogs.Name = "tabPageLogs";
            this.tabPageLogs.Padding = new System.Windows.Forms.Padding(3);
            this.tabPageLogs.Size = new System.Drawing.Size(679, 469);
            this.tabPageLogs.TabIndex = 1;
            this.tabPageLogs.Text = "Logs";
            this.tabPageLogs.UseVisualStyleBackColor = true;
            // 
            // tabPageDailyReport
            // 
            this.tabPageDailyReport.Location = new System.Drawing.Point(4, 22);
            this.tabPageDailyReport.Name = "tabPageDailyReport";
            this.tabPageDailyReport.Size = new System.Drawing.Size(679, 469);
            this.tabPageDailyReport.TabIndex = 2;
            this.tabPageDailyReport.Text = "Daily Report";
            this.tabPageDailyReport.UseVisualStyleBackColor = true;
            // 
            // tabPageWeeklyReport
            // 
            this.tabPageWeeklyReport.Location = new System.Drawing.Point(4, 22);
            this.tabPageWeeklyReport.Name = "tabPageWeeklyReport";
            this.tabPageWeeklyReport.Size = new System.Drawing.Size(679, 469);
            this.tabPageWeeklyReport.TabIndex = 3;
            this.tabPageWeeklyReport.Text = "Weekly Report";
            this.tabPageWeeklyReport.UseVisualStyleBackColor = true;
            // 
            // tabPageMonthlyReport
            // 
            this.tabPageMonthlyReport.Location = new System.Drawing.Point(4, 22);
            this.tabPageMonthlyReport.Name = "tabPageMonthlyReport";
            this.tabPageMonthlyReport.Size = new System.Drawing.Size(679, 469);
            this.tabPageMonthlyReport.TabIndex = 4;
            this.tabPageMonthlyReport.Text = "Monthly Report";
            this.tabPageMonthlyReport.UseVisualStyleBackColor = true;
            // 
            // tabPageOperatorReport
            // 
            this.tabPageOperatorReport.Location = new System.Drawing.Point(4, 22);
            this.tabPageOperatorReport.Name = "tabPageOperatorReport";
            this.tabPageOperatorReport.Padding = new System.Windows.Forms.Padding(3);
            this.tabPageOperatorReport.Size = new System.Drawing.Size(679, 469);
            this.tabPageOperatorReport.TabIndex = 5;
            this.tabPageOperatorReport.Text = "Operator Report";
            this.tabPageOperatorReport.UseVisualStyleBackColor = true;
            // 
            // tabPageOperatorPayroll
            // 
            this.tabPageOperatorPayroll.Location = new System.Drawing.Point(4, 22);
            this.tabPageOperatorPayroll.Name = "tabPageOperatorPayroll";
            this.tabPageOperatorPayroll.Size = new System.Drawing.Size(679, 469);
            this.tabPageOperatorPayroll.TabIndex = 6;
            this.tabPageOperatorPayroll.Text = "Operator Payroll";
            this.tabPageOperatorPayroll.UseVisualStyleBackColor = true;
            // 
            // tabPageVisualize
            // 
            this.tabPageVisualize.Location = new System.Drawing.Point(4, 22);
            this.tabPageVisualize.Name = "tabPageVisualize";
            this.tabPageVisualize.Padding = new System.Windows.Forms.Padding(3);
            this.tabPageVisualize.Size = new System.Drawing.Size(679, 469);
            this.tabPageVisualize.TabIndex = 7;
            this.tabPageVisualize.Text = "Visualize";
            this.tabPageVisualize.UseVisualStyleBackColor = true;
            // 
            // checkBoxHideZeroRows
            // 
            this.checkBoxHideZeroRows.AutoSize = true;
            this.checkBoxHideZeroRows.Checked = true;
            this.checkBoxHideZeroRows.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxHideZeroRows.Location = new System.Drawing.Point(235, 31);
            this.checkBoxHideZeroRows.Name = "checkBoxHideZeroRows";
            this.checkBoxHideZeroRows.Size = new System.Drawing.Size(103, 17);
            this.checkBoxHideZeroRows.TabIndex = 10;
            this.checkBoxHideZeroRows.Text = "Hide Zero Rows";
            this.checkBoxHideZeroRows.UseVisualStyleBackColor = true;
            this.checkBoxHideZeroRows.CheckedChanged += new System.EventHandler(this.checkBoxHideZeroRows_CheckedChanged);
            // 
            // dateTimePickerStart
            // 
            this.dateTimePickerStart.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.dateTimePickerStart.CustomFormat = "yyyy-MM-dd";
            this.dateTimePickerStart.Format = System.Windows.Forms.DateTimePickerFormat.Custom;
            this.dateTimePickerStart.Location = new System.Drawing.Point(751, 30);
            this.dateTimePickerStart.Name = "dateTimePickerStart";
            this.dateTimePickerStart.Size = new System.Drawing.Size(78, 20);
            this.dateTimePickerStart.TabIndex = 11;
            this.dateTimePickerStart.ValueChanged += new System.EventHandler(this.dateTimePickerStart_ValueChanged);
            // 
            // dateTimePickerEnd
            // 
            this.dateTimePickerEnd.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.dateTimePickerEnd.CustomFormat = "yyyy-MM-dd";
            this.dateTimePickerEnd.Format = System.Windows.Forms.DateTimePickerFormat.Custom;
            this.dateTimePickerEnd.Location = new System.Drawing.Point(840, 30);
            this.dateTimePickerEnd.Name = "dateTimePickerEnd";
            this.dateTimePickerEnd.Size = new System.Drawing.Size(78, 20);
            this.dateTimePickerEnd.TabIndex = 12;
            this.dateTimePickerEnd.ValueChanged += new System.EventHandler(this.dateTimePickerEnd_ValueChanged);
            // 
            // labelInterval
            // 
            this.labelInterval.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.labelInterval.AutoSize = true;
            this.labelInterval.Location = new System.Drawing.Point(377, 32);
            this.labelInterval.Name = "labelInterval";
            this.labelInterval.Size = new System.Drawing.Size(45, 13);
            this.labelInterval.TabIndex = 13;
            this.labelInterval.Text = "Interval:";
            // 
            // comboBoxInterval
            // 
            this.comboBoxInterval.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.comboBoxInterval.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxInterval.FormattingEnabled = true;
            this.comboBoxInterval.Items.AddRange(new object[] {
            "Monthly",
            "Weekly",
            "Daily"});
            this.comboBoxInterval.Location = new System.Drawing.Point(429, 29);
            this.comboBoxInterval.Name = "comboBoxInterval";
            this.comboBoxInterval.Size = new System.Drawing.Size(121, 21);
            this.comboBoxInterval.TabIndex = 14;
            this.comboBoxInterval.SelectedIndexChanged += new System.EventHandler(this.comboBoxInterval_SelectedIndexChanged);
            // 
            // checkBoxVisualizeShowLabels
            // 
            this.checkBoxVisualizeShowLabels.AutoSize = true;
            this.checkBoxVisualizeShowLabels.Location = new System.Drawing.Point(312, 31);
            this.checkBoxVisualizeShowLabels.Name = "checkBoxVisualizeShowLabels";
            this.checkBoxVisualizeShowLabels.Size = new System.Drawing.Size(117, 17);
            this.checkBoxVisualizeShowLabels.TabIndex = 15;
            this.checkBoxVisualizeShowLabels.Text = "Show Value Labels";
            this.checkBoxVisualizeShowLabels.UseVisualStyleBackColor = true;
            this.checkBoxVisualizeShowLabels.CheckedChanged += new System.EventHandler(this.checkBoxVisualizeShowLabels_CheckedChanged);
            // 
            // checkBoxVisualizeSortXAxis
            // 
            this.checkBoxVisualizeSortXAxis.AutoSize = true;
            this.checkBoxVisualizeSortXAxis.Location = new System.Drawing.Point(436, 31);
            this.checkBoxVisualizeSortXAxis.Name = "checkBoxVisualizeSortXAxis";
            this.checkBoxVisualizeSortXAxis.Size = new System.Drawing.Size(138, 17);
            this.checkBoxVisualizeSortXAxis.TabIndex = 16;
            this.checkBoxVisualizeSortXAxis.Text = "Sort in Ascending Order";
            this.checkBoxVisualizeSortXAxis.UseVisualStyleBackColor = true;
            this.checkBoxVisualizeSortXAxis.CheckedChanged += new System.EventHandler(this.checkBoxVisualizeSortXAxis_CheckedChanged);
            // 
            // linkLabelCopyChart
            // 
            this.linkLabelCopyChart.AutoSize = true;
            this.linkLabelCopyChart.Location = new System.Drawing.Point(232, 32);
            this.linkLabelCopyChart.Name = "linkLabelCopyChart";
            this.linkLabelCopyChart.Size = new System.Drawing.Size(59, 13);
            this.linkLabelCopyChart.TabIndex = 17;
            this.linkLabelCopyChart.TabStop = true;
            this.linkLabelCopyChart.Text = "Copy Chart";
            this.linkLabelCopyChart.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabelCopyChart_LinkClicked);
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(942, 566);
            this.Controls.Add(this.linkLabelCopyChart);
            this.Controls.Add(this.checkBoxVisualizeSortXAxis);
            this.Controls.Add(this.checkBoxVisualizeShowLabels);
            this.Controls.Add(this.comboBoxInterval);
            this.Controls.Add(this.labelInterval);
            this.Controls.Add(this.dateTimePickerEnd);
            this.Controls.Add(this.dateTimePickerStart);
            this.Controls.Add(this.checkBoxHideZeroRows);
            this.Controls.Add(this.tabControl);
            this.Controls.Add(this.labelTimePeriod);
            this.Controls.Add(this.comboBoxTimePeriod);
            this.Controls.Add(this.labelEntry);
            this.Controls.Add(this.checkBoxVerify);
            this.Controls.Add(this.checkBoxModify);
            this.Controls.Add(this.checkBoxAdd);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.operatorsTreeView);
            this.Controls.Add(this.menuStrip1);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MainMenuStrip = this.menuStrip1;
            this.MinimumSize = new System.Drawing.Size(950, 600);
            this.Name = "MainForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "CSPro Operator Statistics Viewer";
            this.Shown += new System.EventHandler(this.MainForm_Shown);
            this.menuStrip1.ResumeLayout(false);
            this.menuStrip1.PerformLayout();
            this.tabControl.ResumeLayout(false);
            this.tabPageSummary.ResumeLayout(false);
            this.tabPageSummary.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.MenuStrip menuStrip1;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem addLogToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem bulkAddLogsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem closeAllToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripMenuItem exitToolStripMenuItem;
        private System.Windows.Forms.TreeView operatorsTreeView;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ToolStripMenuItem viewToolStripMenuItem;
        private System.Windows.Forms.CheckBox checkBoxAdd;
        private System.Windows.Forms.CheckBox checkBoxModify;
        private System.Windows.Forms.CheckBox checkBoxVerify;
        private System.Windows.Forms.Label labelEntry;
        private System.Windows.Forms.ComboBox comboBoxTimePeriod;
        private System.Windows.Forms.Label labelTimePeriod;
        private System.Windows.Forms.TabControl tabControl;
        private System.Windows.Forms.TabPage tabPageSummary;
        private System.Windows.Forms.TabPage tabPageLogs;
        private System.Windows.Forms.TabPage tabPageDailyReport;
        private System.Windows.Forms.TabPage tabPageWeeklyReport;
        private System.Windows.Forms.TabPage tabPageMonthlyReport;
        private System.Windows.Forms.CheckBox checkBoxHideZeroRows;
        private System.Windows.Forms.ToolStripMenuItem displayTimeInDecimalFormatToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem displayOperatorIDsInsteadOfNameToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem specifyToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem operationStatisticsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem operatorNamesFileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem openViewingConfigurationToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem saveViewingConfigurationToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.DateTimePicker dateTimePickerStart;
        private System.Windows.Forms.DateTimePicker dateTimePickerEnd;
        private System.Windows.Forms.TabPage tabPageOperatorReport;
        private System.Windows.Forms.TabPage tabPageOperatorPayroll;
        private System.Windows.Forms.Label labelInterval;
        private System.Windows.Forms.ComboBox comboBoxInterval;
        private System.Windows.Forms.Label labelCasesEntered;
        private System.Windows.Forms.ProgressBar progressBarSummary;
        private System.Windows.Forms.Label labelExpectedCases;
        private System.Windows.Forms.Label labelCasesRemaining;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label16;
        private System.Windows.Forms.Label label15;
        private System.Windows.Forms.Label label14;
        private System.Windows.Forms.Label label13;
        private System.Windows.Forms.Label label12;
        private System.Windows.Forms.Label label11;
        private System.Windows.Forms.Label label10;
        private System.Windows.Forms.Label label17;
        private System.Windows.Forms.Label label19;
        private System.Windows.Forms.Label label18;
        private System.Windows.Forms.Label label20;
        private System.Windows.Forms.ToolStripMenuItem visualizeToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem numberOfKeyersByDayToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem keyingTimeByDayToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem keyingRateByDayToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator3;
        private System.Windows.Forms.ToolStripMenuItem casesByDayToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem cumulativeCasesByDayToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem cumulativePercentOfCasesVerifiedByDayToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator4;
        private System.Windows.Forms.ToolStripMenuItem cumulativeCasesByKeyerToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem keyingTimeByKeyerToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem keyingRateByKeyerToolStripMenuItem;
        private System.Windows.Forms.TabPage tabPageVisualize;
        private System.Windows.Forms.CheckBox checkBoxVisualizeShowLabels;
        private System.Windows.Forms.CheckBox checkBoxVisualizeSortXAxis;
        private System.Windows.Forms.LinkLabel linkLabelCopyChart;
    }
}

