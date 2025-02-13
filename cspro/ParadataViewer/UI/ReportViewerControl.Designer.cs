namespace ParadataViewer
{
    partial class ReportViewerControl
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

        #region Component Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.treeViewReports = new System.Windows.Forms.TreeView();
            this.contextMenuReports = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.menuItemReportsCopyQueryName = new System.Windows.Forms.ToolStripMenuItem();
            this.checkBoxListReportsAlphabetically = new System.Windows.Forms.CheckBox();
            this.labelDisplayedReportName = new System.Windows.Forms.Label();
            this.linkLabelViewAsChart = new System.Windows.Forms.LinkLabel();
            this.linkLabelViewAsTable = new System.Windows.Forms.LinkLabel();
            this.label1 = new System.Windows.Forms.Label();
            this.contextMenuReports.SuspendLayout();
            this.SuspendLayout();
            //
            // treeViewReports
            //
            this.treeViewReports.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.treeViewReports.HideSelection = false;
            this.treeViewReports.Location = new System.Drawing.Point(9, 29);
            this.treeViewReports.Name = "treeViewReports";
            this.treeViewReports.Size = new System.Drawing.Size(228, 431);
            this.treeViewReports.TabIndex = 0;
            this.treeViewReports.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeViewReports_AfterSelect);
            this.treeViewReports.NodeMouseClick += new System.Windows.Forms.TreeNodeMouseClickEventHandler(this.treeViewReports_NodeMouseClick);
            //
            // contextMenuReports
            //
            this.contextMenuReports.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.menuItemReportsCopyQueryName});
            this.contextMenuReports.Name = "contextMenuReports";
            this.contextMenuReports.Size = new System.Drawing.Size(173, 26);
            //
            // menuItemReportsCopyQueryName
            //
            this.menuItemReportsCopyQueryName.Name = "menuItemReportsCopyQueryName";
            this.menuItemReportsCopyQueryName.Size = new System.Drawing.Size(172, 22);
            this.menuItemReportsCopyQueryName.Text = "Copy Query Name";
            this.menuItemReportsCopyQueryName.Click += new System.EventHandler(this.menuItemReportsCopyQueryName_Click);
            //
            // checkBoxListReportsAlphabetically
            //
            this.checkBoxListReportsAlphabetically.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.checkBoxListReportsAlphabetically.AutoSize = true;
            this.checkBoxListReportsAlphabetically.Location = new System.Drawing.Point(9, 466);
            this.checkBoxListReportsAlphabetically.Name = "checkBoxListReportsAlphabetically";
            this.checkBoxListReportsAlphabetically.Size = new System.Drawing.Size(144, 17);
            this.checkBoxListReportsAlphabetically.TabIndex = 1;
            this.checkBoxListReportsAlphabetically.Text = "List reports alphabetically";
            this.checkBoxListReportsAlphabetically.UseVisualStyleBackColor = true;
            this.checkBoxListReportsAlphabetically.CheckedChanged += new System.EventHandler(this.checkBoxListReportsAlphabetically_CheckedChanged);
            //
            // labelDisplayedReportName
            //
            this.labelDisplayedReportName.AutoSize = true;
            this.labelDisplayedReportName.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labelDisplayedReportName.Location = new System.Drawing.Point(6, 7);
            this.labelDisplayedReportName.Name = "labelDisplayedReportName";
            this.labelDisplayedReportName.Size = new System.Drawing.Size(91, 13);
            this.labelDisplayedReportName.TabIndex = 2;
            this.labelDisplayedReportName.Text = "Select a report";
            //
            // linkLabelViewAsChart
            //
            this.linkLabelViewAsChart.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.linkLabelViewAsChart.AutoSize = true;
            this.linkLabelViewAsChart.Enabled = false;
            this.linkLabelViewAsChart.Location = new System.Drawing.Point(121, 495);
            this.linkLabelViewAsChart.Name = "linkLabelViewAsChart";
            this.linkLabelViewAsChart.Size = new System.Drawing.Size(32, 13);
            this.linkLabelViewAsChart.TabIndex = 11;
            this.linkLabelViewAsChart.TabStop = true;
            this.linkLabelViewAsChart.Text = "Chart";
            this.linkLabelViewAsChart.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabelViewAsChart_LinkClicked);
            //
            // linkLabelViewAsTable
            //
            this.linkLabelViewAsTable.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.linkLabelViewAsTable.AutoSize = true;
            this.linkLabelViewAsTable.Enabled = false;
            this.linkLabelViewAsTable.Location = new System.Drawing.Point(63, 495);
            this.linkLabelViewAsTable.Name = "linkLabelViewAsTable";
            this.linkLabelViewAsTable.Size = new System.Drawing.Size(34, 13);
            this.linkLabelViewAsTable.TabIndex = 10;
            this.linkLabelViewAsTable.TabStop = true;
            this.linkLabelViewAsTable.Text = "Table";
            this.linkLabelViewAsTable.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabelViewAsTable_LinkClicked);
            //
            // label1
            //
            this.label1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(6, 495);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(47, 13);
            this.label1.TabIndex = 9;
            this.label1.Text = "View as:";
            //
            // ReportViewerControl
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.linkLabelViewAsChart);
            this.Controls.Add(this.linkLabelViewAsTable);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.labelDisplayedReportName);
            this.Controls.Add(this.checkBoxListReportsAlphabetically);
            this.Controls.Add(this.treeViewReports);
            this.Name = "ReportViewerControl";
            this.Size = new System.Drawing.Size(247, 515);
            this.contextMenuReports.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TreeView treeViewReports;
        private System.Windows.Forms.CheckBox checkBoxListReportsAlphabetically;
        private System.Windows.Forms.Label labelDisplayedReportName;
        private System.Windows.Forms.LinkLabel linkLabelViewAsChart;
        private System.Windows.Forms.LinkLabel linkLabelViewAsTable;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ContextMenuStrip contextMenuReports;
        private System.Windows.Forms.ToolStripMenuItem menuItemReportsCopyQueryName;
    }
}
