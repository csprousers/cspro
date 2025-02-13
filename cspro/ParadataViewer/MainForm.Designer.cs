namespace ParadataViewer
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
            this.menuStrip = new System.Windows.Forms.MenuStrip();
            this.toolStripFile = new System.Windows.Forms.ToolStripMenuItem();
            this.menuItemFileOpen = new System.Windows.Forms.ToolStripMenuItem();
            this.menuItemFileClose = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            this.menuItemFileExit = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripView = new System.Windows.Forms.ToolStripMenuItem();
            this.menuItemViewReports = new System.Windows.Forms.ToolStripMenuItem();
            this.menuItemViewTableBrowser = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator5 = new System.Windows.Forms.ToolStripSeparator();
            this.menuItemViewQueryConstructor = new System.Windows.Forms.ToolStripMenuItem();
            this.menuItemViewMetadata = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator7 = new System.Windows.Forms.ToolStripSeparator();
            this.menuItemViewLocationMapper = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripOptions = new System.Windows.Forms.ToolStripMenuItem();
            this.menuItemOptionsValueLabels = new System.Windows.Forms.ToolStripMenuItem();
            this.menuItemOptionsConvertTimestamps = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator4 = new System.Windows.Forms.ToolStripSeparator();
            this.menuItemOptionsLinkedTables = new System.Windows.Forms.ToolStripMenuItem();
            this.menuItemOptionsLinkingValues = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.menuItemOptionsSqlStatements = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator6 = new System.Windows.Forms.ToolStripSeparator();
            this.menuItemOptionsSettings = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripTools = new System.Windows.Forms.ToolStripMenuItem();
            this.menuItemToolsConcatenator = new System.Windows.Forms.ToolStripMenuItem();
            this.menuItemToolsPluginManager = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripWindow = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripHelp = new System.Windows.Forms.ToolStripMenuItem();
            this.menuItemHelpHelp = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.menuItemHelpAbout = new System.Windows.Forms.ToolStripMenuItem();
            this.statusStrip = new System.Windows.Forms.StatusStrip();
            this.statusBarLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this.panelFilter = new System.Windows.Forms.Panel();
            this.filterPanelControl = new ParadataViewer.FilterPanelControl();
            this.panelQuerySql = new System.Windows.Forms.Panel();
            this.label1 = new System.Windows.Forms.Label();
            this.linkLabelOpenInQueryConstructor = new System.Windows.Forms.LinkLabel();
            this.linkLabelCopyQuerySql = new System.Windows.Forms.LinkLabel();
            this.textBoxQuerySql = new System.Windows.Forms.TextBox();
            this.splitterFilter = new System.Windows.Forms.Splitter();
            this.splitterQuerySql = new System.Windows.Forms.Splitter();
            this.menuStrip.SuspendLayout();
            this.statusStrip.SuspendLayout();
            this.panelFilter.SuspendLayout();
            this.panelQuerySql.SuspendLayout();
            this.SuspendLayout();
            // 
            // menuStrip
            // 
            this.menuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripFile,
            this.toolStripView,
            this.toolStripOptions,
            this.toolStripTools,
            this.toolStripWindow,
            this.toolStripHelp});
            this.menuStrip.Location = new System.Drawing.Point(0, 0);
            this.menuStrip.MdiWindowListItem = this.toolStripWindow;
            this.menuStrip.Name = "menuStrip";
            this.menuStrip.Size = new System.Drawing.Size(1084, 24);
            this.menuStrip.TabIndex = 1;
            this.menuStrip.Text = "menuStrip";
            // 
            // toolStripFile
            // 
            this.toolStripFile.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.menuItemFileOpen,
            this.menuItemFileClose,
            this.toolStripSeparator3,
            this.menuItemFileExit});
            this.toolStripFile.Name = "toolStripFile";
            this.toolStripFile.Size = new System.Drawing.Size(37, 20);
            this.toolStripFile.Text = "&File";
            this.toolStripFile.DropDownOpening += new System.EventHandler(this.toolStripFile_DropDownOpening);
            // 
            // menuItemFileOpen
            // 
            this.menuItemFileOpen.Name = "menuItemFileOpen";
            this.menuItemFileOpen.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.O)));
            this.menuItemFileOpen.Size = new System.Drawing.Size(171, 22);
            this.menuItemFileOpen.Text = "&Open Log";
            this.menuItemFileOpen.Click += new System.EventHandler(this.menuItemFileOpen_Click);
            // 
            // menuItemFileClose
            // 
            this.menuItemFileClose.Name = "menuItemFileClose";
            this.menuItemFileClose.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.W)));
            this.menuItemFileClose.Size = new System.Drawing.Size(171, 22);
            this.menuItemFileClose.Text = "&Close Log";
            this.menuItemFileClose.Click += new System.EventHandler(this.menuItemFileClose_Click);
            // 
            // toolStripSeparator3
            // 
            this.toolStripSeparator3.Name = "toolStripSeparator3";
            this.toolStripSeparator3.Size = new System.Drawing.Size(168, 6);
            // 
            // menuItemFileExit
            // 
            this.menuItemFileExit.Name = "menuItemFileExit";
            this.menuItemFileExit.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Alt | System.Windows.Forms.Keys.F4)));
            this.menuItemFileExit.Size = new System.Drawing.Size(171, 22);
            this.menuItemFileExit.Text = "E&xit";
            this.menuItemFileExit.Click += new System.EventHandler(this.menuItemFileExit_Click);
            // 
            // toolStripView
            // 
            this.toolStripView.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.menuItemViewReports,
            this.menuItemViewTableBrowser,
            this.toolStripSeparator5,
            this.menuItemViewQueryConstructor,
            this.menuItemViewMetadata,
            this.toolStripSeparator7,
            this.menuItemViewLocationMapper});
            this.toolStripView.Name = "toolStripView";
            this.toolStripView.Size = new System.Drawing.Size(44, 20);
            this.toolStripView.Text = "&View";
            this.toolStripView.DropDownOpening += new System.EventHandler(this.toolStripView_DropDownOpening);
            // 
            // menuItemViewReports
            // 
            this.menuItemViewReports.Name = "menuItemViewReports";
            this.menuItemViewReports.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.R)));
            this.menuItemViewReports.Size = new System.Drawing.Size(215, 22);
            this.menuItemViewReports.Text = "&Reports";
            this.menuItemViewReports.Click += new System.EventHandler(this.menuItemViewReports_Click);
            // 
            // menuItemViewTableBrowser
            // 
            this.menuItemViewTableBrowser.Name = "menuItemViewTableBrowser";
            this.menuItemViewTableBrowser.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.B)));
            this.menuItemViewTableBrowser.Size = new System.Drawing.Size(215, 22);
            this.menuItemViewTableBrowser.Text = "Table &Browser";
            this.menuItemViewTableBrowser.Click += new System.EventHandler(this.menuItemViewTableBrowser_Click);
            // 
            // toolStripSeparator5
            // 
            this.toolStripSeparator5.Name = "toolStripSeparator5";
            this.toolStripSeparator5.Size = new System.Drawing.Size(212, 6);
            // 
            // menuItemViewQueryConstructor
            // 
            this.menuItemViewQueryConstructor.Name = "menuItemViewQueryConstructor";
            this.menuItemViewQueryConstructor.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Q)));
            this.menuItemViewQueryConstructor.Size = new System.Drawing.Size(215, 22);
            this.menuItemViewQueryConstructor.Text = "&Query Constructor";
            this.menuItemViewQueryConstructor.Click += new System.EventHandler(this.menuItemViewQueryConstructor_Click);
            // 
            // menuItemViewMetadata
            // 
            this.menuItemViewMetadata.Name = "menuItemViewMetadata";
            this.menuItemViewMetadata.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.H)));
            this.menuItemViewMetadata.Size = new System.Drawing.Size(215, 22);
            this.menuItemViewMetadata.Text = "Table &Metadata";
            this.menuItemViewMetadata.Click += new System.EventHandler(this.menuItemViewMetadata_Click);
            // 
            // toolStripSeparator7
            // 
            this.toolStripSeparator7.Name = "toolStripSeparator7";
            this.toolStripSeparator7.Size = new System.Drawing.Size(212, 6);
            // 
            // menuItemViewLocationMapper
            // 
            this.menuItemViewLocationMapper.Name = "menuItemViewLocationMapper";
            this.menuItemViewLocationMapper.Size = new System.Drawing.Size(215, 22);
            this.menuItemViewLocationMapper.Text = "&Location Mapper";
            this.menuItemViewLocationMapper.Click += new System.EventHandler(this.menuItemViewLocationMapper_Click);
            // 
            // toolStripOptions
            // 
            this.toolStripOptions.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.menuItemOptionsValueLabels,
            this.menuItemOptionsConvertTimestamps,
            this.toolStripSeparator4,
            this.menuItemOptionsLinkedTables,
            this.menuItemOptionsLinkingValues,
            this.toolStripSeparator2,
            this.menuItemOptionsSqlStatements,
            this.toolStripSeparator6,
            this.menuItemOptionsSettings});
            this.toolStripOptions.Name = "toolStripOptions";
            this.toolStripOptions.Size = new System.Drawing.Size(61, 20);
            this.toolStripOptions.Text = "&Options";
            this.toolStripOptions.DropDownOpening += new System.EventHandler(this.toolStripOptions_DropDownOpening);
            // 
            // menuItemOptionsValueLabels
            // 
            this.menuItemOptionsValueLabels.Name = "menuItemOptionsValueLabels";
            this.menuItemOptionsValueLabels.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.T)));
            this.menuItemOptionsValueLabels.Size = new System.Drawing.Size(298, 22);
            this.menuItemOptionsValueLabels.Text = "View Value &Labels";
            this.menuItemOptionsValueLabels.Click += new System.EventHandler(this.menuItemOptionsValueLabels_Click);
            // 
            // menuItemOptionsConvertTimestamps
            // 
            this.menuItemOptionsConvertTimestamps.Name = "menuItemOptionsConvertTimestamps";
            this.menuItemOptionsConvertTimestamps.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.X)));
            this.menuItemOptionsConvertTimestamps.Size = new System.Drawing.Size(298, 22);
            this.menuItemOptionsConvertTimestamps.Text = "Convert &Timestamps to Local Time";
            this.menuItemOptionsConvertTimestamps.Click += new System.EventHandler(this.menuItemOptionsConvertTimestamps_Click);
            // 
            // toolStripSeparator4
            // 
            this.toolStripSeparator4.Name = "toolStripSeparator4";
            this.toolStripSeparator4.Size = new System.Drawing.Size(295, 6);
            // 
            // menuItemOptionsLinkedTables
            // 
            this.menuItemOptionsLinkedTables.Name = "menuItemOptionsLinkedTables";
            this.menuItemOptionsLinkedTables.Size = new System.Drawing.Size(298, 22);
            this.menuItemOptionsLinkedTables.Text = "Show Fully Linked Tables";
            this.menuItemOptionsLinkedTables.Click += new System.EventHandler(this.menuItemOptionsLinkedTables_Click);
            // 
            // menuItemOptionsLinkingValues
            // 
            this.menuItemOptionsLinkingValues.Name = "menuItemOptionsLinkingValues";
            this.menuItemOptionsLinkingValues.Size = new System.Drawing.Size(298, 22);
            this.menuItemOptionsLinkingValues.Text = "Show Linking Values";
            this.menuItemOptionsLinkingValues.Click += new System.EventHandler(this.menuItemOptionsLinkingValues_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(295, 6);
            // 
            // menuItemOptionsSqlStatements
            // 
            this.menuItemOptionsSqlStatements.Name = "menuItemOptionsSqlStatements";
            this.menuItemOptionsSqlStatements.Size = new System.Drawing.Size(298, 22);
            this.menuItemOptionsSqlStatements.Text = "Show S&QL Statements Pane";
            this.menuItemOptionsSqlStatements.Click += new System.EventHandler(this.menuItemOptionsSqlStatements_Click);
            // 
            // toolStripSeparator6
            // 
            this.toolStripSeparator6.Name = "toolStripSeparator6";
            this.toolStripSeparator6.Size = new System.Drawing.Size(295, 6);
            // 
            // menuItemOptionsSettings
            // 
            this.menuItemOptionsSettings.Name = "menuItemOptionsSettings";
            this.menuItemOptionsSettings.Size = new System.Drawing.Size(298, 22);
            this.menuItemOptionsSettings.Text = "&Settings";
            this.menuItemOptionsSettings.Click += new System.EventHandler(this.menuItemOptionsSettings_Click);
            // 
            // toolStripTools
            // 
            this.toolStripTools.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.menuItemToolsConcatenator,
            this.menuItemToolsPluginManager});
            this.toolStripTools.Name = "toolStripTools";
            this.toolStripTools.Size = new System.Drawing.Size(46, 20);
            this.toolStripTools.Text = "&Tools";
            // 
            // menuItemToolsConcatenator
            // 
            this.menuItemToolsConcatenator.Name = "menuItemToolsConcatenator";
            this.menuItemToolsConcatenator.Size = new System.Drawing.Size(158, 22);
            this.menuItemToolsConcatenator.Text = "&Concatenator";
            this.menuItemToolsConcatenator.Click += new System.EventHandler(this.menuItemToolsConcatenator_Click);
            // 
            // menuItemToolsPluginManager
            // 
            this.menuItemToolsPluginManager.Name = "menuItemToolsPluginManager";
            this.menuItemToolsPluginManager.Size = new System.Drawing.Size(158, 22);
            this.menuItemToolsPluginManager.Text = "&Plugin Manager";
            this.menuItemToolsPluginManager.Click += new System.EventHandler(this.menuItemToolsPluginManager_Click);
            // 
            // toolStripWindow
            // 
            this.toolStripWindow.Name = "toolStripWindow";
            this.toolStripWindow.Size = new System.Drawing.Size(63, 20);
            this.toolStripWindow.Text = "&Window";
            // 
            // toolStripHelp
            // 
            this.toolStripHelp.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.menuItemHelpHelp,
            this.toolStripSeparator1,
            this.menuItemHelpAbout});
            this.toolStripHelp.Name = "toolStripHelp";
            this.toolStripHelp.Size = new System.Drawing.Size(44, 20);
            this.toolStripHelp.Text = "&Help";
            // 
            // menuItemHelpHelp
            // 
            this.menuItemHelpHelp.Name = "menuItemHelpHelp";
            this.menuItemHelpHelp.ShortcutKeys = System.Windows.Forms.Keys.F1;
            this.menuItemHelpHelp.Size = new System.Drawing.Size(194, 22);
            this.menuItemHelpHelp.Text = "&Help Topics";
            this.menuItemHelpHelp.Click += new System.EventHandler(this.menuItemHelpHelp_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(191, 6);
            // 
            // menuItemHelpAbout
            // 
            this.menuItemHelpAbout.Name = "menuItemHelpAbout";
            this.menuItemHelpAbout.Size = new System.Drawing.Size(194, 22);
            this.menuItemHelpAbout.Text = "&About Paradata Viewer";
            this.menuItemHelpAbout.Click += new System.EventHandler(this.menuItemAbout_Click);
            // 
            // statusStrip
            // 
            this.statusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.statusBarLabel});
            this.statusStrip.LayoutStyle = System.Windows.Forms.ToolStripLayoutStyle.Flow;
            this.statusStrip.Location = new System.Drawing.Point(0, 641);
            this.statusStrip.Name = "statusStrip";
            this.statusStrip.Size = new System.Drawing.Size(1084, 20);
            this.statusStrip.TabIndex = 3;
            this.statusStrip.Text = "statusStrip";
            // 
            // statusBarLabel
            // 
            this.statusBarLabel.Name = "statusBarLabel";
            this.statusBarLabel.Size = new System.Drawing.Size(136, 15);
            this.statusBarLabel.Text = "Opened Paradata Viewer";
            // 
            // panelFilter
            // 
            this.panelFilter.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.panelFilter.Controls.Add(this.filterPanelControl);
            this.panelFilter.Dock = System.Windows.Forms.DockStyle.Left;
            this.panelFilter.Location = new System.Drawing.Point(0, 24);
            this.panelFilter.Name = "panelFilter";
            this.panelFilter.Size = new System.Drawing.Size(268, 617);
            this.panelFilter.TabIndex = 5;
            // 
            // filterPanelControl
            // 
            this.filterPanelControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this.filterPanelControl.Location = new System.Drawing.Point(0, 0);
            this.filterPanelControl.Name = "filterPanelControl";
            this.filterPanelControl.Size = new System.Drawing.Size(264, 613);
            this.filterPanelControl.TabIndex = 0;
            // 
            // panelQuerySql
            // 
            this.panelQuerySql.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.panelQuerySql.Controls.Add(this.label1);
            this.panelQuerySql.Controls.Add(this.linkLabelOpenInQueryConstructor);
            this.panelQuerySql.Controls.Add(this.linkLabelCopyQuerySql);
            this.panelQuerySql.Controls.Add(this.textBoxQuerySql);
            this.panelQuerySql.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panelQuerySql.Location = new System.Drawing.Point(268, 455);
            this.panelQuerySql.Name = "panelQuerySql";
            this.panelQuerySql.Size = new System.Drawing.Size(816, 186);
            this.panelQuerySql.TabIndex = 6;
            this.panelQuerySql.Visible = false;
            // 
            // label1
            // 
            this.label1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(660, 165);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(12, 13);
            this.label1.TabIndex = 11;
            this.label1.Text = "–";
            this.label1.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // linkLabelOpenInQueryConstructor
            // 
            this.linkLabelOpenInQueryConstructor.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.linkLabelOpenInQueryConstructor.AutoSize = true;
            this.linkLabelOpenInQueryConstructor.Location = new System.Drawing.Point(502, 165);
            this.linkLabelOpenInQueryConstructor.Name = "linkLabelOpenInQueryConstructor";
            this.linkLabelOpenInQueryConstructor.Size = new System.Drawing.Size(145, 13);
            this.linkLabelOpenInQueryConstructor.TabIndex = 1;
            this.linkLabelOpenInQueryConstructor.TabStop = true;
            this.linkLabelOpenInQueryConstructor.Text = "Execute in Query Constructor";
            this.linkLabelOpenInQueryConstructor.TextAlign = System.Drawing.ContentAlignment.TopRight;
            this.linkLabelOpenInQueryConstructor.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabelOpenInQueryConstructor_LinkClicked);
            // 
            // linkLabelCopyQuerySql
            // 
            this.linkLabelCopyQuerySql.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.linkLabelCopyQuerySql.AutoSize = true;
            this.linkLabelCopyQuerySql.Location = new System.Drawing.Point(685, 165);
            this.linkLabelCopyQuerySql.Name = "linkLabelCopyQuerySql";
            this.linkLabelCopyQuerySql.Size = new System.Drawing.Size(118, 13);
            this.linkLabelCopyQuerySql.TabIndex = 2;
            this.linkLabelCopyQuerySql.TabStop = true;
            this.linkLabelCopyQuerySql.Text = "Copy query to clipboard";
            this.linkLabelCopyQuerySql.TextAlign = System.Drawing.ContentAlignment.TopRight;
            this.linkLabelCopyQuerySql.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabelCopyQuerySql_LinkClicked);
            // 
            // textBoxQuerySql
            // 
            this.textBoxQuerySql.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.textBoxQuerySql.BackColor = System.Drawing.SystemColors.Window;
            this.textBoxQuerySql.Font = new System.Drawing.Font("Consolas", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.textBoxQuerySql.Location = new System.Drawing.Point(13, 11);
            this.textBoxQuerySql.Multiline = true;
            this.textBoxQuerySql.Name = "textBoxQuerySql";
            this.textBoxQuerySql.ReadOnly = true;
            this.textBoxQuerySql.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.textBoxQuerySql.Size = new System.Drawing.Size(786, 147);
            this.textBoxQuerySql.TabIndex = 0;
            // 
            // splitterFilter
            // 
            this.splitterFilter.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.splitterFilter.Location = new System.Drawing.Point(268, 24);
            this.splitterFilter.Name = "splitterFilter";
            this.splitterFilter.Size = new System.Drawing.Size(3, 431);
            this.splitterFilter.TabIndex = 7;
            this.splitterFilter.TabStop = false;
            this.splitterFilter.SplitterMoved += new System.Windows.Forms.SplitterEventHandler(this.splitterFilter_SplitterMoved);
            // 
            // splitterQuerySql
            // 
            this.splitterQuerySql.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.splitterQuerySql.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.splitterQuerySql.Location = new System.Drawing.Point(271, 452);
            this.splitterQuerySql.Name = "splitterQuerySql";
            this.splitterQuerySql.Size = new System.Drawing.Size(813, 3);
            this.splitterQuerySql.TabIndex = 8;
            this.splitterQuerySql.TabStop = false;
            this.splitterQuerySql.Visible = false;
            this.splitterQuerySql.SplitterMoved += new System.Windows.Forms.SplitterEventHandler(this.splitterQuerySql_SplitterMoved);
            // 
            // MainForm
            // 
            this.AllowDrop = true;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1084, 661);
            this.Controls.Add(this.splitterQuerySql);
            this.Controls.Add(this.splitterFilter);
            this.Controls.Add(this.panelQuerySql);
            this.Controls.Add(this.panelFilter);
            this.Controls.Add(this.statusStrip);
            this.Controls.Add(this.menuStrip);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.IsMdiContainer = true;
            this.MainMenuStrip = this.menuStrip;
            this.Name = "MainForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "CSPro Paradata Viewer";
            this.Shown += new System.EventHandler(this.MainForm_Shown);
            this.DragDrop += new System.Windows.Forms.DragEventHandler(this.MainForm_DragDrop);
            this.DragEnter += new System.Windows.Forms.DragEventHandler(this.MainForm_DragEnter);
            this.menuStrip.ResumeLayout(false);
            this.menuStrip.PerformLayout();
            this.statusStrip.ResumeLayout(false);
            this.statusStrip.PerformLayout();
            this.panelFilter.ResumeLayout(false);
            this.panelQuerySql.ResumeLayout(false);
            this.panelQuerySql.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.MenuStrip menuStrip;
        private System.Windows.Forms.ToolStripMenuItem toolStripHelp;
        private System.Windows.Forms.ToolStripMenuItem menuItemHelpHelp;
        private System.Windows.Forms.ToolStripMenuItem menuItemHelpAbout;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripMenuItem toolStripOptions;
        private System.Windows.Forms.ToolStripMenuItem menuItemOptionsValueLabels;
        private System.Windows.Forms.ToolStripMenuItem menuItemOptionsSqlStatements;
        private System.Windows.Forms.ToolStripMenuItem toolStripFile;
        private System.Windows.Forms.ToolStripMenuItem toolStripTools;
        private System.Windows.Forms.ToolStripMenuItem menuItemToolsConcatenator;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripMenuItem menuItemFileOpen;
        private System.Windows.Forms.ToolStripMenuItem menuItemFileClose;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator3;
        private System.Windows.Forms.ToolStripMenuItem menuItemFileExit;
        private System.Windows.Forms.ToolStripMenuItem toolStripView;
        private System.Windows.Forms.ToolStripMenuItem menuItemOptionsConvertTimestamps;
        private System.Windows.Forms.ToolStripMenuItem menuItemOptionsLinkedTables;
        private System.Windows.Forms.StatusStrip statusStrip;
        private System.Windows.Forms.ToolStripStatusLabel statusBarLabel;
        private System.Windows.Forms.Panel panelFilter;
        private System.Windows.Forms.Panel panelQuerySql;
        private System.Windows.Forms.Splitter splitterFilter;
        private System.Windows.Forms.Splitter splitterQuerySql;
        private System.Windows.Forms.LinkLabel linkLabelCopyQuerySql;
        private System.Windows.Forms.TextBox textBoxQuerySql;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator4;
        private System.Windows.Forms.ToolStripMenuItem menuItemOptionsLinkingValues;
        private System.Windows.Forms.ToolStripMenuItem menuItemViewQueryConstructor;
        private System.Windows.Forms.ToolStripMenuItem toolStripWindow;
        private System.Windows.Forms.ToolStripMenuItem menuItemViewMetadata;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator5;
        private System.Windows.Forms.ToolStripMenuItem menuItemViewTableBrowser;
        private System.Windows.Forms.ToolStripMenuItem menuItemToolsPluginManager;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator6;
        private System.Windows.Forms.ToolStripMenuItem menuItemOptionsSettings;
        private FilterPanelControl filterPanelControl;
        private System.Windows.Forms.LinkLabel linkLabelOpenInQueryConstructor;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ToolStripMenuItem menuItemViewReports;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator7;
        private System.Windows.Forms.ToolStripMenuItem menuItemViewLocationMapper;
    }
}

