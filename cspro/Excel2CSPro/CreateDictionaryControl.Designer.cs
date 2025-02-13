namespace Excel2CSPro
{
    partial class CreateDictionaryControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(CreateDictionaryControl));
            this.groupBoxExcelOptions = new System.Windows.Forms.GroupBox();
            this.buttonSelectExcelFile = new System.Windows.Forms.Button();
            this.buttonAnalyzeWorksheet = new System.Windows.Forms.Button();
            this.label4 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.comboBoxWorksheet = new System.Windows.Forms.ComboBox();
            this.textBoxStartingRow = new System.Windows.Forms.TextBox();
            this.groupBoxCreateDictionary = new System.Windows.Forms.GroupBox();
            this.label1 = new System.Windows.Forms.Label();
            this.textBoxNamePrefix = new System.Windows.Forms.TextBox();
            this.checkBoxZeroFill = new System.Windows.Forms.CheckBox();
            this.checkBoxDecChar = new System.Windows.Forms.CheckBox();
            this.buttonCreateDictionary = new System.Windows.Forms.Button();
            this.groupBoxDictionaryContents = new System.Windows.Forms.GroupBox();
            this.label11 = new System.Windows.Forms.Label();
            this.label10 = new System.Windows.Forms.Label();
            this.label9 = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.label7 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.panelDictionaryContents = new System.Windows.Forms.Panel();
            this.groupBoxExcelOptions.SuspendLayout();
            this.groupBoxCreateDictionary.SuspendLayout();
            this.groupBoxDictionaryContents.SuspendLayout();
            this.SuspendLayout();
            //
            // groupBoxExcelOptions
            //
            resources.ApplyResources(this.groupBoxExcelOptions, "groupBoxExcelOptions");
            this.groupBoxExcelOptions.Controls.Add(this.buttonSelectExcelFile);
            this.groupBoxExcelOptions.Controls.Add(this.buttonAnalyzeWorksheet);
            this.groupBoxExcelOptions.Controls.Add(this.label4);
            this.groupBoxExcelOptions.Controls.Add(this.label3);
            this.groupBoxExcelOptions.Controls.Add(this.comboBoxWorksheet);
            this.groupBoxExcelOptions.Controls.Add(this.textBoxStartingRow);
            this.groupBoxExcelOptions.Name = "groupBoxExcelOptions";
            this.groupBoxExcelOptions.TabStop = false;
            //
            // buttonSelectExcelFile
            //
            resources.ApplyResources(this.buttonSelectExcelFile, "buttonSelectExcelFile");
            this.buttonSelectExcelFile.Name = "buttonSelectExcelFile";
            this.buttonSelectExcelFile.UseVisualStyleBackColor = true;
            this.buttonSelectExcelFile.Click += new System.EventHandler(this.buttonSelectExcelFile_Click);
            //
            // buttonAnalyzeWorksheet
            //
            resources.ApplyResources(this.buttonAnalyzeWorksheet, "buttonAnalyzeWorksheet");
            this.buttonAnalyzeWorksheet.Name = "buttonAnalyzeWorksheet";
            this.buttonAnalyzeWorksheet.UseVisualStyleBackColor = true;
            this.buttonAnalyzeWorksheet.Click += new System.EventHandler(this.buttonAnalyzeWorksheet_Click);
            //
            // label4
            //
            resources.ApplyResources(this.label4, "label4");
            this.label4.Name = "label4";
            //
            // label3
            //
            resources.ApplyResources(this.label3, "label3");
            this.label3.Name = "label3";
            //
            // comboBoxWorksheet
            //
            this.comboBoxWorksheet.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxWorksheet.FormattingEnabled = true;
            resources.ApplyResources(this.comboBoxWorksheet, "comboBoxWorksheet");
            this.comboBoxWorksheet.Name = "comboBoxWorksheet";
            //
            // textBoxStartingRow
            //
            resources.ApplyResources(this.textBoxStartingRow, "textBoxStartingRow");
            this.textBoxStartingRow.Name = "textBoxStartingRow";
            //
            // groupBoxCreateDictionary
            //
            resources.ApplyResources(this.groupBoxCreateDictionary, "groupBoxCreateDictionary");
            this.groupBoxCreateDictionary.Controls.Add(this.label1);
            this.groupBoxCreateDictionary.Controls.Add(this.textBoxNamePrefix);
            this.groupBoxCreateDictionary.Controls.Add(this.checkBoxZeroFill);
            this.groupBoxCreateDictionary.Controls.Add(this.checkBoxDecChar);
            this.groupBoxCreateDictionary.Controls.Add(this.buttonCreateDictionary);
            this.groupBoxCreateDictionary.Name = "groupBoxCreateDictionary";
            this.groupBoxCreateDictionary.TabStop = false;
            //
            // label1
            //
            resources.ApplyResources(this.label1, "label1");
            this.label1.Name = "label1";
            //
            // textBoxNamePrefix
            //
            resources.ApplyResources(this.textBoxNamePrefix, "textBoxNamePrefix");
            this.textBoxNamePrefix.Name = "textBoxNamePrefix";
            //
            // checkBoxZeroFill
            //
            resources.ApplyResources(this.checkBoxZeroFill, "checkBoxZeroFill");
            this.checkBoxZeroFill.Checked = true;
            this.checkBoxZeroFill.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxZeroFill.Name = "checkBoxZeroFill";
            this.checkBoxZeroFill.UseVisualStyleBackColor = true;
            //
            // checkBoxDecChar
            //
            resources.ApplyResources(this.checkBoxDecChar, "checkBoxDecChar");
            this.checkBoxDecChar.Checked = true;
            this.checkBoxDecChar.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxDecChar.Name = "checkBoxDecChar";
            this.checkBoxDecChar.UseVisualStyleBackColor = true;
            //
            // buttonCreateDictionary
            //
            resources.ApplyResources(this.buttonCreateDictionary, "buttonCreateDictionary");
            this.buttonCreateDictionary.Name = "buttonCreateDictionary";
            this.buttonCreateDictionary.UseVisualStyleBackColor = true;
            this.buttonCreateDictionary.Click += new System.EventHandler(this.buttonCreateDictionary_Click);
            //
            // groupBoxDictionaryContents
            //
            resources.ApplyResources(this.groupBoxDictionaryContents, "groupBoxDictionaryContents");
            this.groupBoxDictionaryContents.Controls.Add(this.label11);
            this.groupBoxDictionaryContents.Controls.Add(this.label10);
            this.groupBoxDictionaryContents.Controls.Add(this.label9);
            this.groupBoxDictionaryContents.Controls.Add(this.label8);
            this.groupBoxDictionaryContents.Controls.Add(this.label7);
            this.groupBoxDictionaryContents.Controls.Add(this.label6);
            this.groupBoxDictionaryContents.Controls.Add(this.label5);
            this.groupBoxDictionaryContents.Controls.Add(this.label2);
            this.groupBoxDictionaryContents.Controls.Add(this.panelDictionaryContents);
            this.groupBoxDictionaryContents.Name = "groupBoxDictionaryContents";
            this.groupBoxDictionaryContents.TabStop = false;
            //
            // label11
            //
            resources.ApplyResources(this.label11, "label11");
            this.label11.Name = "label11";
            //
            // label10
            //
            resources.ApplyResources(this.label10, "label10");
            this.label10.Name = "label10";
            //
            // label9
            //
            resources.ApplyResources(this.label9, "label9");
            this.label9.Name = "label9";
            //
            // label8
            //
            resources.ApplyResources(this.label8, "label8");
            this.label8.Name = "label8";
            //
            // label7
            //
            resources.ApplyResources(this.label7, "label7");
            this.label7.Name = "label7";
            //
            // label6
            //
            resources.ApplyResources(this.label6, "label6");
            this.label6.Name = "label6";
            //
            // label5
            //
            resources.ApplyResources(this.label5, "label5");
            this.label5.Name = "label5";
            //
            // label2
            //
            resources.ApplyResources(this.label2, "label2");
            this.label2.Name = "label2";
            //
            // panelDictionaryContents
            //
            resources.ApplyResources(this.panelDictionaryContents, "panelDictionaryContents");
            this.panelDictionaryContents.Name = "panelDictionaryContents";
            //
            // CreateDictionaryControl
            //
            resources.ApplyResources(this, "$this");
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.groupBoxDictionaryContents);
            this.Controls.Add(this.groupBoxCreateDictionary);
            this.Controls.Add(this.groupBoxExcelOptions);
            this.Name = "CreateDictionaryControl";
            this.groupBoxExcelOptions.ResumeLayout(false);
            this.groupBoxExcelOptions.PerformLayout();
            this.groupBoxCreateDictionary.ResumeLayout(false);
            this.groupBoxCreateDictionary.PerformLayout();
            this.groupBoxDictionaryContents.ResumeLayout(false);
            this.groupBoxDictionaryContents.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBoxExcelOptions;
        private System.Windows.Forms.Button buttonSelectExcelFile;
        private System.Windows.Forms.Button buttonAnalyzeWorksheet;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.ComboBox comboBoxWorksheet;
        private System.Windows.Forms.TextBox textBoxStartingRow;
        private System.Windows.Forms.GroupBox groupBoxCreateDictionary;
        private System.Windows.Forms.Button buttonCreateDictionary;
        private System.Windows.Forms.CheckBox checkBoxZeroFill;
        private System.Windows.Forms.CheckBox checkBoxDecChar;
        private System.Windows.Forms.TextBox textBoxNamePrefix;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.GroupBox groupBoxDictionaryContents;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label10;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.Label label11;
        private System.Windows.Forms.Panel panelDictionaryContents;
    }
}
