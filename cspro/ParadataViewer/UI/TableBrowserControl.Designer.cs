namespace ParadataViewer
{
    partial class TableBrowserControl
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
            this.treeViewTables = new System.Windows.Forms.TreeView();
            this.checkBoxListTablesAlphabetically = new System.Windows.Forms.CheckBox();
            this.labelDisplayedTableName = new System.Windows.Forms.Label();
            this.checkBoxOptionIncludeBaseEventInstances = new System.Windows.Forms.CheckBox();
            this.SuspendLayout();
            //
            // treeViewTables
            //
            this.treeViewTables.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.treeViewTables.HideSelection = false;
            this.treeViewTables.Location = new System.Drawing.Point(9, 29);
            this.treeViewTables.Name = "treeViewTables";
            this.treeViewTables.Size = new System.Drawing.Size(226, 431);
            this.treeViewTables.TabIndex = 0;
            this.treeViewTables.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeViewTables_AfterSelect);
            //
            // checkBoxListTablesAlphabetically
            //
            this.checkBoxListTablesAlphabetically.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.checkBoxListTablesAlphabetically.AutoSize = true;
            this.checkBoxListTablesAlphabetically.Location = new System.Drawing.Point(9, 466);
            this.checkBoxListTablesAlphabetically.Name = "checkBoxListTablesAlphabetically";
            this.checkBoxListTablesAlphabetically.Size = new System.Drawing.Size(140, 17);
            this.checkBoxListTablesAlphabetically.TabIndex = 1;
            this.checkBoxListTablesAlphabetically.Text = "List tables alphabetically";
            this.checkBoxListTablesAlphabetically.UseVisualStyleBackColor = true;
            this.checkBoxListTablesAlphabetically.CheckedChanged += new System.EventHandler(this.checkBoxListTablesAlphabetically_CheckedChanged);
            //
            // labelDisplayedTableName
            //
            this.labelDisplayedTableName.AutoSize = true;
            this.labelDisplayedTableName.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labelDisplayedTableName.Location = new System.Drawing.Point(6, 5);
            this.labelDisplayedTableName.Name = "labelDisplayedTableName";
            this.labelDisplayedTableName.Size = new System.Drawing.Size(86, 13);
            this.labelDisplayedTableName.TabIndex = 2;
            this.labelDisplayedTableName.Text = "Select a table";
            //
            // checkBoxOptionIncludeBaseEventInstances
            //
            this.checkBoxOptionIncludeBaseEventInstances.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.checkBoxOptionIncludeBaseEventInstances.AutoSize = true;
            this.checkBoxOptionIncludeBaseEventInstances.Location = new System.Drawing.Point(9, 489);
            this.checkBoxOptionIncludeBaseEventInstances.Name = "checkBoxOptionIncludeBaseEventInstances";
            this.checkBoxOptionIncludeBaseEventInstances.Size = new System.Drawing.Size(209, 17);
            this.checkBoxOptionIncludeBaseEventInstances.TabIndex = 2;
            this.checkBoxOptionIncludeBaseEventInstances.Text = "Include the base event\'s instance links";
            this.checkBoxOptionIncludeBaseEventInstances.UseVisualStyleBackColor = true;
            this.checkBoxOptionIncludeBaseEventInstances.Click += new System.EventHandler(this.checkBoxOptionIncludeBaseEventInstances_Click);
            //
            // TableBrowserControl
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.checkBoxOptionIncludeBaseEventInstances);
            this.Controls.Add(this.labelDisplayedTableName);
            this.Controls.Add(this.checkBoxListTablesAlphabetically);
            this.Controls.Add(this.treeViewTables);
            this.Name = "TableBrowserControl";
            this.Size = new System.Drawing.Size(247, 515);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TreeView treeViewTables;
        private System.Windows.Forms.CheckBox checkBoxListTablesAlphabetically;
        private System.Windows.Forms.Label labelDisplayedTableName;
        private System.Windows.Forms.CheckBox checkBoxOptionIncludeBaseEventInstances;
    }
}
