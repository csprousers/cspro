namespace Excel2CSPro
{
    partial class Excel2CSProItemControl
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
            this.groupBoxOptions = new System.Windows.Forms.GroupBox();
            this.label1 = new System.Windows.Forms.Label();
            this.comboBoxCaseManagement = new System.Windows.Forms.ComboBox();
            this.checkBoxRunOnlyIfNewer = new System.Windows.Forms.CheckBox();
            this.label3 = new System.Windows.Forms.Label();
            this.textBoxStartingRow = new System.Windows.Forms.TextBox();
            this.buttonCreateDataFile = new System.Windows.Forms.Button();
            this.groupBoxSelectFiles = new System.Windows.Forms.GroupBox();
            this.labelDataFile = new System.Windows.Forms.Label();
            this.labelCSProDictionary = new System.Windows.Forms.Label();
            this.labelExcelFile = new System.Windows.Forms.Label();
            this.buttonSelectOutputFile = new System.Windows.Forms.Button();
            this.buttonSelectDictionary = new System.Windows.Forms.Button();
            this.buttonSelectExcelFile = new System.Windows.Forms.Button();
            this.groupBoxRecordToWorksheetMapping = new System.Windows.Forms.GroupBox();
            this.panelRecordToWorksheetMapping = new System.Windows.Forms.Panel();
            this.groupBoxItemToColumnMapping = new System.Windows.Forms.GroupBox();
            this.panelItemToColumnMapping = new System.Windows.Forms.Panel();
            this.groupBoxOptions.SuspendLayout();
            this.groupBoxSelectFiles.SuspendLayout();
            this.groupBoxRecordToWorksheetMapping.SuspendLayout();
            this.groupBoxItemToColumnMapping.SuspendLayout();
            this.SuspendLayout();
            // 
            // groupBoxOptions
            // 
            this.groupBoxOptions.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBoxOptions.Controls.Add(this.label1);
            this.groupBoxOptions.Controls.Add(this.comboBoxCaseManagement);
            this.groupBoxOptions.Controls.Add(this.checkBoxRunOnlyIfNewer);
            this.groupBoxOptions.Controls.Add(this.label3);
            this.groupBoxOptions.Controls.Add(this.textBoxStartingRow);
            this.groupBoxOptions.Location = new System.Drawing.Point(651, 3);
            this.groupBoxOptions.Name = "groupBoxOptions";
            this.groupBoxOptions.Size = new System.Drawing.Size(281, 111);
            this.groupBoxOptions.TabIndex = 1;
            this.groupBoxOptions.TabStop = false;
            this.groupBoxOptions.Text = "Options";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(6, 51);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(98, 13);
            this.label1.TabIndex = 8;
            this.label1.Text = "Case management:";
            // 
            // comboBoxCaseManagement
            // 
            this.comboBoxCaseManagement.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxCaseManagement.FormattingEnabled = true;
            this.comboBoxCaseManagement.Items.AddRange(new object[] {
            "Create new file",
            "Modify, add cases",
            "Modify, add, delete cases"});
            this.comboBoxCaseManagement.Location = new System.Drawing.Point(110, 48);
            this.comboBoxCaseManagement.Name = "comboBoxCaseManagement";
            this.comboBoxCaseManagement.Size = new System.Drawing.Size(158, 21);
            this.comboBoxCaseManagement.TabIndex = 1;
            // 
            // checkBoxRunOnlyIfNewer
            // 
            this.checkBoxRunOnlyIfNewer.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.checkBoxRunOnlyIfNewer.Location = new System.Drawing.Point(9, 72);
            this.checkBoxRunOnlyIfNewer.Name = "checkBoxRunOnlyIfNewer";
            this.checkBoxRunOnlyIfNewer.Size = new System.Drawing.Size(259, 34);
            this.checkBoxRunOnlyIfNewer.TabIndex = 2;
            this.checkBoxRunOnlyIfNewer.Text = "Skip processing if the CSPro output data file is newer than the Excel file";
            this.checkBoxRunOnlyIfNewer.UseVisualStyleBackColor = true;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.label3.Location = new System.Drawing.Point(6, 21);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(66, 13);
            this.label3.TabIndex = 5;
            this.label3.Text = "Starting row:";
            // 
            // textBoxStartingRow
            // 
            this.textBoxStartingRow.Location = new System.Drawing.Point(110, 18);
            this.textBoxStartingRow.Name = "textBoxStartingRow";
            this.textBoxStartingRow.Size = new System.Drawing.Size(61, 20);
            this.textBoxStartingRow.TabIndex = 0;
            // 
            // buttonCreateDataFile
            // 
            this.buttonCreateDataFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.buttonCreateDataFile.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.buttonCreateDataFile.Location = new System.Drawing.Point(802, 122);
            this.buttonCreateDataFile.Name = "buttonCreateDataFile";
            this.buttonCreateDataFile.Size = new System.Drawing.Size(130, 23);
            this.buttonCreateDataFile.TabIndex = 2;
            this.buttonCreateDataFile.Text = "Create CSPro Data File";
            this.buttonCreateDataFile.UseVisualStyleBackColor = true;
            this.buttonCreateDataFile.Click += new System.EventHandler(this.buttonCreateDataFile_Click);
            // 
            // groupBoxSelectFiles
            // 
            this.groupBoxSelectFiles.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBoxSelectFiles.Controls.Add(this.labelDataFile);
            this.groupBoxSelectFiles.Controls.Add(this.labelCSProDictionary);
            this.groupBoxSelectFiles.Controls.Add(this.labelExcelFile);
            this.groupBoxSelectFiles.Controls.Add(this.buttonSelectOutputFile);
            this.groupBoxSelectFiles.Controls.Add(this.buttonSelectDictionary);
            this.groupBoxSelectFiles.Controls.Add(this.buttonSelectExcelFile);
            this.groupBoxSelectFiles.Location = new System.Drawing.Point(3, 3);
            this.groupBoxSelectFiles.Name = "groupBoxSelectFiles";
            this.groupBoxSelectFiles.Size = new System.Drawing.Size(642, 111);
            this.groupBoxSelectFiles.TabIndex = 0;
            this.groupBoxSelectFiles.TabStop = false;
            this.groupBoxSelectFiles.Text = "Select Files";
            // 
            // labelDataFile
            // 
            this.labelDataFile.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.labelDataFile.AutoEllipsis = true;
            this.labelDataFile.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.labelDataFile.Location = new System.Drawing.Point(12, 81);
            this.labelDataFile.Name = "labelDataFile";
            this.labelDataFile.Size = new System.Drawing.Size(480, 13);
            this.labelDataFile.TabIndex = 12;
            this.labelDataFile.Text = "Select a CSPro output data file...";
            // 
            // labelCSProDictionary
            // 
            this.labelCSProDictionary.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.labelCSProDictionary.AutoEllipsis = true;
            this.labelCSProDictionary.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.labelCSProDictionary.Location = new System.Drawing.Point(12, 51);
            this.labelCSProDictionary.Name = "labelCSProDictionary";
            this.labelCSProDictionary.Size = new System.Drawing.Size(480, 13);
            this.labelCSProDictionary.TabIndex = 11;
            this.labelCSProDictionary.Text = "Select a CSPro dictionary...";
            // 
            // labelExcelFile
            // 
            this.labelExcelFile.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.labelExcelFile.AutoEllipsis = true;
            this.labelExcelFile.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.labelExcelFile.Location = new System.Drawing.Point(12, 21);
            this.labelExcelFile.Name = "labelExcelFile";
            this.labelExcelFile.Size = new System.Drawing.Size(480, 13);
            this.labelExcelFile.TabIndex = 10;
            this.labelExcelFile.Text = "Select an Excel file...";
            // 
            // buttonSelectOutputFile
            // 
            this.buttonSelectOutputFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.buttonSelectOutputFile.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.buttonSelectOutputFile.Location = new System.Drawing.Point(498, 76);
            this.buttonSelectOutputFile.Name = "buttonSelectOutputFile";
            this.buttonSelectOutputFile.Size = new System.Drawing.Size(130, 23);
            this.buttonSelectOutputFile.TabIndex = 2;
            this.buttonSelectOutputFile.Text = "Select Output Data File";
            this.buttonSelectOutputFile.UseVisualStyleBackColor = true;
            this.buttonSelectOutputFile.Click += new System.EventHandler(this.buttonSelectOutputFile_Click);
            // 
            // buttonSelectDictionary
            // 
            this.buttonSelectDictionary.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.buttonSelectDictionary.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.buttonSelectDictionary.Location = new System.Drawing.Point(498, 46);
            this.buttonSelectDictionary.Name = "buttonSelectDictionary";
            this.buttonSelectDictionary.Size = new System.Drawing.Size(130, 23);
            this.buttonSelectDictionary.TabIndex = 1;
            this.buttonSelectDictionary.Text = "Select CSPro Dictionary";
            this.buttonSelectDictionary.UseVisualStyleBackColor = true;
            this.buttonSelectDictionary.Click += new System.EventHandler(this.buttonSelectDictionary_Click);
            // 
            // buttonSelectExcelFile
            // 
            this.buttonSelectExcelFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.buttonSelectExcelFile.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.buttonSelectExcelFile.Location = new System.Drawing.Point(498, 16);
            this.buttonSelectExcelFile.Name = "buttonSelectExcelFile";
            this.buttonSelectExcelFile.Size = new System.Drawing.Size(130, 23);
            this.buttonSelectExcelFile.TabIndex = 0;
            this.buttonSelectExcelFile.Text = "Select Excel File";
            this.buttonSelectExcelFile.UseVisualStyleBackColor = true;
            this.buttonSelectExcelFile.Click += new System.EventHandler(this.buttonSelectExcelFile_Click);
            // 
            // groupBoxRecordToWorksheetMapping
            // 
            this.groupBoxRecordToWorksheetMapping.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left)));
            this.groupBoxRecordToWorksheetMapping.Controls.Add(this.panelRecordToWorksheetMapping);
            this.groupBoxRecordToWorksheetMapping.Enabled = false;
            this.groupBoxRecordToWorksheetMapping.Location = new System.Drawing.Point(3, 149);
            this.groupBoxRecordToWorksheetMapping.Name = "groupBoxRecordToWorksheetMapping";
            this.groupBoxRecordToWorksheetMapping.Size = new System.Drawing.Size(450, 229);
            this.groupBoxRecordToWorksheetMapping.TabIndex = 4;
            this.groupBoxRecordToWorksheetMapping.TabStop = false;
            this.groupBoxRecordToWorksheetMapping.Text = "Record to Worksheet Mapping";
            // 
            // panelRecordToWorksheetMapping
            // 
            this.panelRecordToWorksheetMapping.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.panelRecordToWorksheetMapping.AutoScroll = true;
            this.panelRecordToWorksheetMapping.Location = new System.Drawing.Point(6, 19);
            this.panelRecordToWorksheetMapping.Name = "panelRecordToWorksheetMapping";
            this.panelRecordToWorksheetMapping.Size = new System.Drawing.Size(438, 204);
            this.panelRecordToWorksheetMapping.TabIndex = 0;
            // 
            // groupBoxItemToColumnMapping
            // 
            this.groupBoxItemToColumnMapping.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBoxItemToColumnMapping.Controls.Add(this.panelItemToColumnMapping);
            this.groupBoxItemToColumnMapping.Enabled = false;
            this.groupBoxItemToColumnMapping.Location = new System.Drawing.Point(459, 149);
            this.groupBoxItemToColumnMapping.Name = "groupBoxItemToColumnMapping";
            this.groupBoxItemToColumnMapping.Size = new System.Drawing.Size(473, 229);
            this.groupBoxItemToColumnMapping.TabIndex = 5;
            this.groupBoxItemToColumnMapping.TabStop = false;
            this.groupBoxItemToColumnMapping.Text = "Item to Column Mapping";
            // 
            // panelItemToColumnMapping
            // 
            this.panelItemToColumnMapping.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.panelItemToColumnMapping.AutoScroll = true;
            this.panelItemToColumnMapping.Location = new System.Drawing.Point(6, 19);
            this.panelItemToColumnMapping.Name = "panelItemToColumnMapping";
            this.panelItemToColumnMapping.Size = new System.Drawing.Size(461, 204);
            this.panelItemToColumnMapping.TabIndex = 0;
            // 
            // Excel2CSProItemControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.groupBoxItemToColumnMapping);
            this.Controls.Add(this.groupBoxRecordToWorksheetMapping);
            this.Controls.Add(this.groupBoxOptions);
            this.Controls.Add(this.buttonCreateDataFile);
            this.Controls.Add(this.groupBoxSelectFiles);
            this.Name = "Excel2CSProItemControl";
            this.Size = new System.Drawing.Size(940, 381);
            this.groupBoxOptions.ResumeLayout(false);
            this.groupBoxOptions.PerformLayout();
            this.groupBoxSelectFiles.ResumeLayout(false);
            this.groupBoxRecordToWorksheetMapping.ResumeLayout(false);
            this.groupBoxItemToColumnMapping.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBoxOptions;
        private System.Windows.Forms.Button buttonCreateDataFile;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TextBox textBoxStartingRow;
        private System.Windows.Forms.GroupBox groupBoxSelectFiles;
        private System.Windows.Forms.Label labelDataFile;
        private System.Windows.Forms.Label labelCSProDictionary;
        private System.Windows.Forms.Label labelExcelFile;
        private System.Windows.Forms.Button buttonSelectOutputFile;
        private System.Windows.Forms.Button buttonSelectDictionary;
        private System.Windows.Forms.Button buttonSelectExcelFile;
        private System.Windows.Forms.GroupBox groupBoxRecordToWorksheetMapping;
        private System.Windows.Forms.GroupBox groupBoxItemToColumnMapping;
        private System.Windows.Forms.Panel panelRecordToWorksheetMapping;
        private System.Windows.Forms.Panel panelItemToColumnMapping;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ComboBox comboBoxCaseManagement;
        private System.Windows.Forms.CheckBox checkBoxRunOnlyIfNewer;
    }
}
