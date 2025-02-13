namespace ParadataViewer
{
    partial class MetadataForm
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
            this.treeViewMetadata = new System.Windows.Forms.TreeView();
            this.comboBoxTableFilter = new System.Windows.Forms.ComboBox();
            this.label1 = new System.Windows.Forms.Label();
            this.listViewColumns = new System.Windows.Forms.ListView();
            this.columnHeader1 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader2 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader3 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader6 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.labelColumnsHeader = new System.Windows.Forms.Label();
            this.labelCodesHeader = new System.Windows.Forms.Label();
            this.listViewCodes = new System.Windows.Forms.ListView();
            this.columnHeader4 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader5 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.linkLabelExpandAllNodes = new System.Windows.Forms.LinkLabel();
            this.panelTableMetadata = new System.Windows.Forms.Panel();
            this.checkBoxOptionIncludeBaseEventInstances = new System.Windows.Forms.CheckBox();
            this.checkBoxOptionExplicitlySelectColumns = new System.Windows.Forms.CheckBox();
            this.checkBoxOptionShowLinkedValue = new System.Windows.Forms.CheckBox();
            this.checkBoxOptionFullyLinkTables = new System.Windows.Forms.CheckBox();
            this.checkBoxOptionLinkFromBaseEvent = new System.Windows.Forms.CheckBox();
            this.label2 = new System.Windows.Forms.Label();
            this.linkLabelOpenInQueryConstructor = new System.Windows.Forms.LinkLabel();
            this.textBoxQuerySql = new System.Windows.Forms.TextBox();
            this.panelTableMetadata.SuspendLayout();
            this.SuspendLayout();
            //
            // treeViewMetadata
            //
            this.treeViewMetadata.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)));
            this.treeViewMetadata.HideSelection = false;
            this.treeViewMetadata.Location = new System.Drawing.Point(15, 41);
            this.treeViewMetadata.Name = "treeViewMetadata";
            this.treeViewMetadata.Size = new System.Drawing.Size(239, 509);
            this.treeViewMetadata.TabIndex = 0;
            this.treeViewMetadata.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeViewMetadata_AfterSelect);
            //
            // comboBoxTableFilter
            //
            this.comboBoxTableFilter.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxTableFilter.FormattingEnabled = true;
            this.comboBoxTableFilter.Items.AddRange(new object[] {
            "All",
            "Base Event",
            "Event",
            "Instance",
            "Information"});
            this.comboBoxTableFilter.Location = new System.Drawing.Point(144, 9);
            this.comboBoxTableFilter.Name = "comboBoxTableFilter";
            this.comboBoxTableFilter.Size = new System.Drawing.Size(110, 21);
            this.comboBoxTableFilter.TabIndex = 11;
            this.comboBoxTableFilter.SelectedIndexChanged += new System.EventHandler(this.comboBoxTableFilter_SelectedIndexChanged);
            //
            // label1
            //
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(12, 12);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(45, 13);
            this.label1.TabIndex = 2;
            this.label1.Text = "Tables";
            //
            // listViewColumns
            //
            this.listViewColumns.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader1,
            this.columnHeader2,
            this.columnHeader3,
            this.columnHeader6});
            this.listViewColumns.FullRowSelect = true;
            this.listViewColumns.Location = new System.Drawing.Point(272, 41);
            this.listViewColumns.Name = "listViewColumns";
            this.listViewColumns.Size = new System.Drawing.Size(486, 320);
            this.listViewColumns.TabIndex = 2;
            this.listViewColumns.UseCompatibleStateImageBehavior = false;
            this.listViewColumns.View = System.Windows.Forms.View.Details;
            this.listViewColumns.SelectedIndexChanged += new System.EventHandler(this.listViewColumns_SelectedIndexChanged);
            //
            // columnHeader1
            //
            this.columnHeader1.Text = "Name";
            this.columnHeader1.Width = 142;
            //
            // columnHeader2
            //
            this.columnHeader2.Text = "Type";
            this.columnHeader2.Width = 59;
            //
            // columnHeader3
            //
            this.columnHeader3.Text = "SQLite Type";
            this.columnHeader3.Width = 101;
            //
            // columnHeader6
            //
            this.columnHeader6.Text = "Linked Table";
            this.columnHeader6.Width = 167;
            //
            // labelColumnsHeader
            //
            this.labelColumnsHeader.AutoSize = true;
            this.labelColumnsHeader.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labelColumnsHeader.Location = new System.Drawing.Point(269, 12);
            this.labelColumnsHeader.Name = "labelColumnsHeader";
            this.labelColumnsHeader.Size = new System.Drawing.Size(102, 13);
            this.labelColumnsHeader.TabIndex = 4;
            this.labelColumnsHeader.Text = "Columns for `{0}`";
            //
            // labelCodesHeader
            //
            this.labelCodesHeader.AutoSize = true;
            this.labelCodesHeader.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labelCodesHeader.Location = new System.Drawing.Point(773, 17);
            this.labelCodesHeader.Name = "labelCodesHeader";
            this.labelCodesHeader.Size = new System.Drawing.Size(90, 13);
            this.labelCodesHeader.TabIndex = 5;
            this.labelCodesHeader.Text = "Codes for `{0}`";
            //
            // listViewCodes
            //
            this.listViewCodes.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader4,
            this.columnHeader5});
            this.listViewCodes.FullRowSelect = true;
            this.listViewCodes.Location = new System.Drawing.Point(776, 41);
            this.listViewCodes.Name = "listViewCodes";
            this.listViewCodes.Size = new System.Drawing.Size(181, 320);
            this.listViewCodes.TabIndex = 3;
            this.listViewCodes.UseCompatibleStateImageBehavior = false;
            this.listViewCodes.View = System.Windows.Forms.View.Details;
            //
            // columnHeader4
            //
            this.columnHeader4.Text = "Code";
            this.columnHeader4.Width = 59;
            //
            // columnHeader5
            //
            this.columnHeader5.Text = "Value";
            this.columnHeader5.Width = 110;
            //
            // linkLabelExpandAllNodes
            //
            this.linkLabelExpandAllNodes.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.linkLabelExpandAllNodes.AutoSize = true;
            this.linkLabelExpandAllNodes.Location = new System.Drawing.Point(12, 558);
            this.linkLabelExpandAllNodes.Name = "linkLabelExpandAllNodes";
            this.linkLabelExpandAllNodes.Size = new System.Drawing.Size(145, 13);
            this.linkLabelExpandAllNodes.TabIndex = 1;
            this.linkLabelExpandAllNodes.TabStop = true;
            this.linkLabelExpandAllNodes.Text = "Expand all table and columns";
            this.linkLabelExpandAllNodes.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabelExpandAllNodes_LinkClicked);
            //
            // panelTableMetadata
            //
            this.panelTableMetadata.AutoScroll = true;
            this.panelTableMetadata.Controls.Add(this.checkBoxOptionIncludeBaseEventInstances);
            this.panelTableMetadata.Controls.Add(this.checkBoxOptionExplicitlySelectColumns);
            this.panelTableMetadata.Controls.Add(this.checkBoxOptionShowLinkedValue);
            this.panelTableMetadata.Controls.Add(this.checkBoxOptionFullyLinkTables);
            this.panelTableMetadata.Controls.Add(this.checkBoxOptionLinkFromBaseEvent);
            this.panelTableMetadata.Controls.Add(this.label2);
            this.panelTableMetadata.Controls.Add(this.linkLabelOpenInQueryConstructor);
            this.panelTableMetadata.Controls.Add(this.textBoxQuerySql);
            this.panelTableMetadata.Controls.Add(this.label1);
            this.panelTableMetadata.Controls.Add(this.linkLabelExpandAllNodes);
            this.panelTableMetadata.Controls.Add(this.treeViewMetadata);
            this.panelTableMetadata.Controls.Add(this.listViewColumns);
            this.panelTableMetadata.Controls.Add(this.comboBoxTableFilter);
            this.panelTableMetadata.Controls.Add(this.listViewCodes);
            this.panelTableMetadata.Controls.Add(this.labelColumnsHeader);
            this.panelTableMetadata.Controls.Add(this.labelCodesHeader);
            this.panelTableMetadata.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panelTableMetadata.Location = new System.Drawing.Point(0, 0);
            this.panelTableMetadata.Name = "panelTableMetadata";
            this.panelTableMetadata.Size = new System.Drawing.Size(972, 582);
            this.panelTableMetadata.TabIndex = 8;
            //
            // checkBoxOptionIncludeBaseEventInstances
            //
            this.checkBoxOptionIncludeBaseEventInstances.AutoSize = true;
            this.checkBoxOptionIncludeBaseEventInstances.Location = new System.Drawing.Point(629, 393);
            this.checkBoxOptionIncludeBaseEventInstances.Name = "checkBoxOptionIncludeBaseEventInstances";
            this.checkBoxOptionIncludeBaseEventInstances.Size = new System.Drawing.Size(209, 17);
            this.checkBoxOptionIncludeBaseEventInstances.TabIndex = 8;
            this.checkBoxOptionIncludeBaseEventInstances.Text = "Include the base event\'s instance links";
            this.checkBoxOptionIncludeBaseEventInstances.UseVisualStyleBackColor = true;
            this.checkBoxOptionIncludeBaseEventInstances.Click += new System.EventHandler(this.checkBoxOption_Click);
            //
            // checkBoxOptionExplicitlySelectColumns
            //
            this.checkBoxOptionExplicitlySelectColumns.AutoSize = true;
            this.checkBoxOptionExplicitlySelectColumns.Checked = true;
            this.checkBoxOptionExplicitlySelectColumns.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxOptionExplicitlySelectColumns.Location = new System.Drawing.Point(402, 372);
            this.checkBoxOptionExplicitlySelectColumns.Name = "checkBoxOptionExplicitlySelectColumns";
            this.checkBoxOptionExplicitlySelectColumns.Size = new System.Drawing.Size(155, 17);
            this.checkBoxOptionExplicitlySelectColumns.TabIndex = 4;
            this.checkBoxOptionExplicitlySelectColumns.Text = "Explictly select the columns";
            this.checkBoxOptionExplicitlySelectColumns.UseVisualStyleBackColor = true;
            this.checkBoxOptionExplicitlySelectColumns.Click += new System.EventHandler(this.checkBoxOption_Click);
            //
            // checkBoxOptionShowLinkedValue
            //
            this.checkBoxOptionShowLinkedValue.AutoSize = true;
            this.checkBoxOptionShowLinkedValue.Location = new System.Drawing.Point(402, 414);
            this.checkBoxOptionShowLinkedValue.Name = "checkBoxOptionShowLinkedValue";
            this.checkBoxOptionShowLinkedValue.Size = new System.Drawing.Size(224, 17);
            this.checkBoxOptionShowLinkedValue.TabIndex = 6;
            this.checkBoxOptionShowLinkedValue.Text = "Show the linked value when linking tables";
            this.checkBoxOptionShowLinkedValue.UseVisualStyleBackColor = true;
            this.checkBoxOptionShowLinkedValue.Click += new System.EventHandler(this.checkBoxOption_Click);
            //
            // checkBoxOptionFullyLinkTables
            //
            this.checkBoxOptionFullyLinkTables.AutoSize = true;
            this.checkBoxOptionFullyLinkTables.Checked = true;
            this.checkBoxOptionFullyLinkTables.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxOptionFullyLinkTables.Location = new System.Drawing.Point(402, 393);
            this.checkBoxOptionFullyLinkTables.Name = "checkBoxOptionFullyLinkTables";
            this.checkBoxOptionFullyLinkTables.Size = new System.Drawing.Size(152, 17);
            this.checkBoxOptionFullyLinkTables.TabIndex = 5;
            this.checkBoxOptionFullyLinkTables.Text = "Use joins to fully link tables";
            this.checkBoxOptionFullyLinkTables.UseVisualStyleBackColor = true;
            this.checkBoxOptionFullyLinkTables.Click += new System.EventHandler(this.checkBoxOption_Click);
            //
            // checkBoxOptionLinkFromBaseEvent
            //
            this.checkBoxOptionLinkFromBaseEvent.AutoSize = true;
            this.checkBoxOptionLinkFromBaseEvent.Checked = true;
            this.checkBoxOptionLinkFromBaseEvent.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxOptionLinkFromBaseEvent.Location = new System.Drawing.Point(629, 372);
            this.checkBoxOptionLinkFromBaseEvent.Name = "checkBoxOptionLinkFromBaseEvent";
            this.checkBoxOptionLinkFromBaseEvent.Size = new System.Drawing.Size(203, 17);
            this.checkBoxOptionLinkFromBaseEvent.TabIndex = 7;
            this.checkBoxOptionLinkFromBaseEvent.Text = "Link event tables with the base event";
            this.checkBoxOptionLinkFromBaseEvent.UseVisualStyleBackColor = true;
            this.checkBoxOptionLinkFromBaseEvent.Click += new System.EventHandler(this.checkBoxOption_Click);
            //
            // label2
            //
            this.label2.AutoSize = true;
            this.label2.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label2.Location = new System.Drawing.Point(269, 373);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(111, 13);
            this.label2.TabIndex = 8;
            this.label2.Text = "SQL Query Builder";
            //
            // linkLabelOpenInQueryConstructor
            //
            this.linkLabelOpenInQueryConstructor.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.linkLabelOpenInQueryConstructor.AutoSize = true;
            this.linkLabelOpenInQueryConstructor.Location = new System.Drawing.Point(812, 558);
            this.linkLabelOpenInQueryConstructor.Name = "linkLabelOpenInQueryConstructor";
            this.linkLabelOpenInQueryConstructor.Size = new System.Drawing.Size(145, 13);
            this.linkLabelOpenInQueryConstructor.TabIndex = 10;
            this.linkLabelOpenInQueryConstructor.TabStop = true;
            this.linkLabelOpenInQueryConstructor.Text = "Execute in Query Constructor";
            this.linkLabelOpenInQueryConstructor.TextAlign = System.Drawing.ContentAlignment.TopRight;
            this.linkLabelOpenInQueryConstructor.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabelOpenInQueryConstructor_LinkClicked);
            //
            // textBoxQuerySql
            //
            this.textBoxQuerySql.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.textBoxQuerySql.BackColor = System.Drawing.SystemColors.Window;
            this.textBoxQuerySql.Font = new System.Drawing.Font("Consolas", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.textBoxQuerySql.Location = new System.Drawing.Point(272, 437);
            this.textBoxQuerySql.Multiline = true;
            this.textBoxQuerySql.Name = "textBoxQuerySql";
            this.textBoxQuerySql.ReadOnly = true;
            this.textBoxQuerySql.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.textBoxQuerySql.Size = new System.Drawing.Size(685, 113);
            this.textBoxQuerySql.TabIndex = 9;
            //
            // MetadataForm
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(972, 582);
            this.Controls.Add(this.panelTableMetadata);
            this.Name = "MetadataForm";
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.Text = "Table Metadata";
            this.panelTableMetadata.ResumeLayout(false);
            this.panelTableMetadata.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TreeView treeViewMetadata;
        private System.Windows.Forms.ComboBox comboBoxTableFilter;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ListView listViewColumns;
        private System.Windows.Forms.Label labelColumnsHeader;
        private System.Windows.Forms.ColumnHeader columnHeader1;
        private System.Windows.Forms.ColumnHeader columnHeader2;
        private System.Windows.Forms.ColumnHeader columnHeader3;
        private System.Windows.Forms.Label labelCodesHeader;
        private System.Windows.Forms.ListView listViewCodes;
        private System.Windows.Forms.ColumnHeader columnHeader4;
        private System.Windows.Forms.ColumnHeader columnHeader5;
        private System.Windows.Forms.ColumnHeader columnHeader6;
        private System.Windows.Forms.LinkLabel linkLabelExpandAllNodes;
        private System.Windows.Forms.Panel panelTableMetadata;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.LinkLabel linkLabelOpenInQueryConstructor;
        private System.Windows.Forms.TextBox textBoxQuerySql;
        private System.Windows.Forms.CheckBox checkBoxOptionExplicitlySelectColumns;
        private System.Windows.Forms.CheckBox checkBoxOptionShowLinkedValue;
        private System.Windows.Forms.CheckBox checkBoxOptionFullyLinkTables;
        private System.Windows.Forms.CheckBox checkBoxOptionLinkFromBaseEvent;
        private System.Windows.Forms.CheckBox checkBoxOptionIncludeBaseEventInstances;
    }
}