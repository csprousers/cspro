
namespace DataViewer
{
    partial class ExportDataForm
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ExportDataForm));
            this.label1 = new System.Windows.Forms.Label();
            this.checkBoxCsv = new System.Windows.Forms.CheckBox();
            this.groupBoxExportFormats = new System.Windows.Forms.GroupBox();
            this.checkBoxJson = new System.Windows.Forms.CheckBox();
            this.checkBoxSas = new System.Windows.Forms.CheckBox();
            this.checkBoxR = new System.Windows.Forms.CheckBox();
            this.checkBoxStata = new System.Windows.Forms.CheckBox();
            this.checkBoxSpss = new System.Windows.Forms.CheckBox();
            this.checkBoxExcel = new System.Windows.Forms.CheckBox();
            this.linkLabelExportDataTool = new System.Windows.Forms.LinkLabel();
            this.textBoxOutputFilenames = new System.Windows.Forms.TextBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.label2 = new System.Windows.Forms.Label();
            this.textBoxBaseFilename = new System.Windows.Forms.TextBox();
            this.buttonExportData = new System.Windows.Forms.Button();
            this.checkBoxOneFilePerRecord = new System.Windows.Forms.CheckBox();
            this.groupBoxExportFormats.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.Location = new System.Drawing.Point(15, 9);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(738, 34);
            this.label1.TabIndex = 0;
            this.label1.Text = resources.GetString("label1.Text");
            // 
            // checkBoxCsv
            // 
            this.checkBoxCsv.AutoSize = true;
            this.checkBoxCsv.Location = new System.Drawing.Point(17, 22);
            this.checkBoxCsv.Name = "checkBoxCsv";
            this.checkBoxCsv.Size = new System.Drawing.Size(47, 17);
            this.checkBoxCsv.TabIndex = 0;
            this.checkBoxCsv.Tag = CSPro.Data.ExporterExportType.CommaDelimited;
            this.checkBoxCsv.Text = "CSV";
            this.checkBoxCsv.UseVisualStyleBackColor = true;
            this.checkBoxCsv.Click += new System.EventHandler(this.UpdateUI);
            // 
            // groupBoxExportFormats
            // 
            this.groupBoxExportFormats.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left)));
            this.groupBoxExportFormats.Controls.Add(this.checkBoxJson);
            this.groupBoxExportFormats.Controls.Add(this.checkBoxSas);
            this.groupBoxExportFormats.Controls.Add(this.checkBoxR);
            this.groupBoxExportFormats.Controls.Add(this.checkBoxStata);
            this.groupBoxExportFormats.Controls.Add(this.checkBoxSpss);
            this.groupBoxExportFormats.Controls.Add(this.checkBoxExcel);
            this.groupBoxExportFormats.Controls.Add(this.checkBoxCsv);
            this.groupBoxExportFormats.Location = new System.Drawing.Point(15, 47);
            this.groupBoxExportFormats.Name = "groupBoxExportFormats";
            this.groupBoxExportFormats.Size = new System.Drawing.Size(113, 188);
            this.groupBoxExportFormats.TabIndex = 2;
            this.groupBoxExportFormats.TabStop = false;
            this.groupBoxExportFormats.Text = "Export Formats";
            // 
            // checkBoxJson
            // 
            this.checkBoxJson.AutoSize = true;
            this.checkBoxJson.Location = new System.Drawing.Point(17, 68);
            this.checkBoxJson.Name = "checkBoxJson";
            this.checkBoxJson.Size = new System.Drawing.Size(54, 17);
            this.checkBoxJson.TabIndex = 2;
            this.checkBoxJson.Tag = CSPro.Data.ExporterExportType.Json;
            this.checkBoxJson.Text = "JSON";
            this.checkBoxJson.UseVisualStyleBackColor = true;
            this.checkBoxJson.Click += new System.EventHandler(this.UpdateUI);
            // 
            // checkBoxSas
            // 
            this.checkBoxSas.AutoSize = true;
            this.checkBoxSas.Location = new System.Drawing.Point(17, 114);
            this.checkBoxSas.Name = "checkBoxSas";
            this.checkBoxSas.Size = new System.Drawing.Size(47, 17);
            this.checkBoxSas.TabIndex = 4;
            this.checkBoxSas.Tag = CSPro.Data.ExporterExportType.SAS;
            this.checkBoxSas.Text = "SAS";
            this.checkBoxSas.UseVisualStyleBackColor = true;
            this.checkBoxSas.Click += new System.EventHandler(this.UpdateUI);
            // 
            // checkBoxR
            // 
            this.checkBoxR.AutoSize = true;
            this.checkBoxR.Location = new System.Drawing.Point(17, 91);
            this.checkBoxR.Name = "checkBoxR";
            this.checkBoxR.Size = new System.Drawing.Size(34, 17);
            this.checkBoxR.TabIndex = 3;
            this.checkBoxR.Tag = CSPro.Data.ExporterExportType.R;
            this.checkBoxR.Text = "R";
            this.checkBoxR.UseVisualStyleBackColor = true;
            this.checkBoxR.Click += new System.EventHandler(this.UpdateUI);
            // 
            // checkBoxStata
            // 
            this.checkBoxStata.AutoSize = true;
            this.checkBoxStata.Location = new System.Drawing.Point(17, 160);
            this.checkBoxStata.Name = "checkBoxStata";
            this.checkBoxStata.Size = new System.Drawing.Size(51, 17);
            this.checkBoxStata.TabIndex = 6;
            this.checkBoxStata.Tag = CSPro.Data.ExporterExportType.Stata;
            this.checkBoxStata.Text = "Stata";
            this.checkBoxStata.UseVisualStyleBackColor = true;
            this.checkBoxStata.Click += new System.EventHandler(this.UpdateUI);
            // 
            // checkBoxSpss
            // 
            this.checkBoxSpss.AutoSize = true;
            this.checkBoxSpss.Location = new System.Drawing.Point(17, 137);
            this.checkBoxSpss.Name = "checkBoxSpss";
            this.checkBoxSpss.Size = new System.Drawing.Size(54, 17);
            this.checkBoxSpss.TabIndex = 5;
            this.checkBoxSpss.Tag = CSPro.Data.ExporterExportType.SPSS;
            this.checkBoxSpss.Text = "SPSS";
            this.checkBoxSpss.UseVisualStyleBackColor = true;
            this.checkBoxSpss.Click += new System.EventHandler(this.UpdateUI);
            // 
            // checkBoxExcel
            // 
            this.checkBoxExcel.AutoSize = true;
            this.checkBoxExcel.Location = new System.Drawing.Point(17, 45);
            this.checkBoxExcel.Name = "checkBoxExcel";
            this.checkBoxExcel.Size = new System.Drawing.Size(52, 17);
            this.checkBoxExcel.TabIndex = 1;
            this.checkBoxExcel.Tag = CSPro.Data.ExporterExportType.Excel;
            this.checkBoxExcel.Text = "Excel";
            this.checkBoxExcel.UseVisualStyleBackColor = true;
            this.checkBoxExcel.Click += new System.EventHandler(this.UpdateUI);
            // 
            // linkLabelExportDataTool
            // 
            this.linkLabelExportDataTool.AutoSize = true;
            this.linkLabelExportDataTool.Location = new System.Drawing.Point(307, 22);
            this.linkLabelExportDataTool.Name = "linkLabelExportDataTool";
            this.linkLabelExportDataTool.Size = new System.Drawing.Size(208, 13);
            this.linkLabelExportDataTool.TabIndex = 1;
            this.linkLabelExportDataTool.TabStop = true;
            this.linkLabelExportDataTool.Text = "For more options, use the Export Data tool.";
            this.linkLabelExportDataTool.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabelExportDataTool_LinkClicked);
            // 
            // textBoxOutputFilenames
            // 
            this.textBoxOutputFilenames.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.textBoxOutputFilenames.BackColor = System.Drawing.Color.White;
            this.textBoxOutputFilenames.Location = new System.Drawing.Point(14, 53);
            this.textBoxOutputFilenames.Multiline = true;
            this.textBoxOutputFilenames.Name = "textBoxOutputFilenames";
            this.textBoxOutputFilenames.ReadOnly = true;
            this.textBoxOutputFilenames.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.textBoxOutputFilenames.Size = new System.Drawing.Size(583, 124);
            this.textBoxOutputFilenames.TabIndex = 2;
            // 
            // groupBox2
            // 
            this.groupBox2.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBox2.Controls.Add(this.label2);
            this.groupBox2.Controls.Add(this.textBoxBaseFilename);
            this.groupBox2.Controls.Add(this.textBoxOutputFilenames);
            this.groupBox2.Location = new System.Drawing.Point(140, 47);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(613, 188);
            this.groupBox2.TabIndex = 3;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Output Filenames";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(11, 27);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(76, 13);
            this.label2.TabIndex = 0;
            this.label2.Text = "Base Filename";
            // 
            // textBoxBaseFilename
            // 
            this.textBoxBaseFilename.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.textBoxBaseFilename.Location = new System.Drawing.Point(105, 24);
            this.textBoxBaseFilename.Name = "textBoxBaseFilename";
            this.textBoxBaseFilename.Size = new System.Drawing.Size(492, 20);
            this.textBoxBaseFilename.TabIndex = 1;
            this.textBoxBaseFilename.TextChanged += new System.EventHandler(this.UpdateUI);
            // 
            // buttonExportData
            // 
            this.buttonExportData.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.buttonExportData.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.buttonExportData.Enabled = false;
            this.buttonExportData.Location = new System.Drawing.Point(678, 242);
            this.buttonExportData.Name = "buttonExportData";
            this.buttonExportData.Size = new System.Drawing.Size(75, 23);
            this.buttonExportData.TabIndex = 0;
            this.buttonExportData.Text = "Export Data";
            this.buttonExportData.UseVisualStyleBackColor = true;
            this.buttonExportData.Click += new System.EventHandler(this.buttonExportData_Click);
            // 
            // checkBoxOneFilePerRecord
            // 
            this.checkBoxOneFilePerRecord.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.checkBoxOneFilePerRecord.AutoSize = true;
            this.checkBoxOneFilePerRecord.Location = new System.Drawing.Point(154, 246);
            this.checkBoxOneFilePerRecord.Name = "checkBoxOneFilePerRecord";
            this.checkBoxOneFilePerRecord.Size = new System.Drawing.Size(297, 17);
            this.checkBoxOneFilePerRecord.TabIndex = 4;
            this.checkBoxOneFilePerRecord.Text = "Output one file for each record regardless of export format";
            this.checkBoxOneFilePerRecord.UseVisualStyleBackColor = true;
            this.checkBoxOneFilePerRecord.CheckedChanged += new System.EventHandler(this.UpdateUI);
            // 
            // ExportDataForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(766, 276);
            this.Controls.Add(this.checkBoxOneFilePerRecord);
            this.Controls.Add(this.buttonExportData);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.linkLabelExportDataTool);
            this.Controls.Add(this.groupBoxExportFormats);
            this.Controls.Add(this.label1);
            this.MinimumSize = new System.Drawing.Size(782, 315);
            this.Name = "ExportDataForm";
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Export Data";
            this.groupBoxExportFormats.ResumeLayout(false);
            this.groupBoxExportFormats.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.CheckBox checkBoxCsv;
        private System.Windows.Forms.GroupBox groupBoxExportFormats;
        private System.Windows.Forms.CheckBox checkBoxSas;
        private System.Windows.Forms.CheckBox checkBoxR;
        private System.Windows.Forms.CheckBox checkBoxStata;
        private System.Windows.Forms.CheckBox checkBoxSpss;
        private System.Windows.Forms.CheckBox checkBoxExcel;
        private System.Windows.Forms.LinkLabel linkLabelExportDataTool;
        private System.Windows.Forms.TextBox textBoxOutputFilenames;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Button buttonExportData;
        private System.Windows.Forms.CheckBox checkBoxOneFilePerRecord;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox textBoxBaseFilename;
        private System.Windows.Forms.CheckBox checkBoxJson;
    }
}
