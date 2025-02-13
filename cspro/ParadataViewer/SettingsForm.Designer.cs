namespace ParadataViewer
{
    partial class SettingsForm
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
            this.label1 = new System.Windows.Forms.Label();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.textBoxTabularQueryInitialRows = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.textBoxTabularQueryMaximumRows = new System.Windows.Forms.TextBox();
            this.linkLabelTimestampFormatters = new System.Windows.Forms.LinkLabel();
            this.label3 = new System.Windows.Forms.Label();
            this.textBoxTimestampConversionFormat = new System.Windows.Forms.TextBox();
            this.buttonOK = new System.Windows.Forms.Button();
            this.buttonCancel = new System.Windows.Forms.Button();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.labelTimestampConversionFormatForFilters = new System.Windows.Forms.Label();
            this.labelTimestampConversionFormat = new System.Windows.Forms.Label();
            this.textBoxTimestampConversionFormatForFilters = new System.Windows.Forms.TextBox();
            this.label4 = new System.Windows.Forms.Label();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.textBoxFilterQueryRows = new System.Windows.Forms.TextBox();
            this.label5 = new System.Windows.Forms.Label();
            this.groupBox4 = new System.Windows.Forms.GroupBox();
            this.textBoxGeneralQueryInitialRows = new System.Windows.Forms.TextBox();
            this.label6 = new System.Windows.Forms.Label();
            this.textBoxGeneralQueryMaximumRows = new System.Windows.Forms.TextBox();
            this.label7 = new System.Windows.Forms.Label();
            this.buttonRestoreDefaults = new System.Windows.Forms.Button();
            this.label8 = new System.Windows.Forms.Label();
            this.label9 = new System.Windows.Forms.Label();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.groupBox3.SuspendLayout();
            this.groupBox4.SuspendLayout();
            this.SuspendLayout();
            //
            // label1
            //
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 30);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(153, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Initial number of rows to display";
            //
            // groupBox1
            //
            this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBox1.Controls.Add(this.label9);
            this.groupBox1.Controls.Add(this.textBoxTabularQueryInitialRows);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Controls.Add(this.textBoxTabularQueryMaximumRows);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Location = new System.Drawing.Point(12, 183);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(546, 93);
            this.groupBox1.TabIndex = 5;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Tabular Queries";
            //
            // textBoxTabularQueryInitialRows
            //
            this.textBoxTabularQueryInitialRows.Location = new System.Drawing.Point(237, 27);
            this.textBoxTabularQueryInitialRows.Name = "textBoxTabularQueryInitialRows";
            this.textBoxTabularQueryInitialRows.Size = new System.Drawing.Size(71, 20);
            this.textBoxTabularQueryInitialRows.TabIndex = 0;
            //
            // label2
            //
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(12, 61);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(173, 13);
            this.label2.TabIndex = 2;
            this.label2.Text = "Maximum number of rows to display";
            //
            // textBoxTabularQueryMaximumRows
            //
            this.textBoxTabularQueryMaximumRows.Location = new System.Drawing.Point(237, 58);
            this.textBoxTabularQueryMaximumRows.Name = "textBoxTabularQueryMaximumRows";
            this.textBoxTabularQueryMaximumRows.Size = new System.Drawing.Size(71, 20);
            this.textBoxTabularQueryMaximumRows.TabIndex = 1;
            //
            // linkLabelTimestampFormatters
            //
            this.linkLabelTimestampFormatters.AutoSize = true;
            this.linkLabelTimestampFormatters.Location = new System.Drawing.Point(12, 84);
            this.linkLabelTimestampFormatters.Name = "linkLabelTimestampFormatters";
            this.linkLabelTimestampFormatters.Size = new System.Drawing.Size(116, 13);
            this.linkLabelTimestampFormatters.TabIndex = 2;
            this.linkLabelTimestampFormatters.TabStop = true;
            this.linkLabelTimestampFormatters.Text = "View formatting options";
            this.linkLabelTimestampFormatters.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabelTimestampFormatters_LinkClicked);
            //
            // label3
            //
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(12, 28);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(141, 13);
            this.label3.TabIndex = 4;
            this.label3.Text = "Format for reports and tables";
            //
            // textBoxTimestampConversionFormat
            //
            this.textBoxTimestampConversionFormat.Location = new System.Drawing.Point(237, 25);
            this.textBoxTimestampConversionFormat.Name = "textBoxTimestampConversionFormat";
            this.textBoxTimestampConversionFormat.Size = new System.Drawing.Size(129, 20);
            this.textBoxTimestampConversionFormat.TabIndex = 0;
            this.textBoxTimestampConversionFormat.TextChanged += new System.EventHandler(this.textBoxTimestampConversionFormat_TextChanged);
            //
            // buttonOK
            //
            this.buttonOK.Anchor = System.Windows.Forms.AnchorStyles.Bottom;
            this.buttonOK.Location = new System.Drawing.Point(186, 416);
            this.buttonOK.Name = "buttonOK";
            this.buttonOK.Size = new System.Drawing.Size(75, 23);
            this.buttonOK.TabIndex = 0;
            this.buttonOK.Text = "OK";
            this.buttonOK.UseVisualStyleBackColor = true;
            this.buttonOK.Click += new System.EventHandler(this.buttonOK_Click);
            //
            // buttonCancel
            //
            this.buttonCancel.Anchor = System.Windows.Forms.AnchorStyles.Bottom;
            this.buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.buttonCancel.Location = new System.Drawing.Point(309, 416);
            this.buttonCancel.Name = "buttonCancel";
            this.buttonCancel.Size = new System.Drawing.Size(75, 23);
            this.buttonCancel.TabIndex = 1;
            this.buttonCancel.Text = "Cancel";
            this.buttonCancel.UseVisualStyleBackColor = true;
            //
            // groupBox2
            //
            this.groupBox2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBox2.Controls.Add(this.labelTimestampConversionFormatForFilters);
            this.groupBox2.Controls.Add(this.labelTimestampConversionFormat);
            this.groupBox2.Controls.Add(this.textBoxTimestampConversionFormatForFilters);
            this.groupBox2.Controls.Add(this.label4);
            this.groupBox2.Controls.Add(this.textBoxTimestampConversionFormat);
            this.groupBox2.Controls.Add(this.linkLabelTimestampFormatters);
            this.groupBox2.Controls.Add(this.label3);
            this.groupBox2.Location = new System.Drawing.Point(12, 285);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(546, 115);
            this.groupBox2.TabIndex = 6;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Timestamp Conversions";
            //
            // labelTimestampConversionFormatForFilters
            //
            this.labelTimestampConversionFormatForFilters.AutoSize = true;
            this.labelTimestampConversionFormatForFilters.Location = new System.Drawing.Point(381, 56);
            this.labelTimestampConversionFormatForFilters.Name = "labelTimestampConversionFormatForFilters";
            this.labelTimestampConversionFormatForFilters.Size = new System.Drawing.Size(16, 13);
            this.labelTimestampConversionFormatForFilters.TabIndex = 8;
            this.labelTimestampConversionFormatForFilters.Text = "...";
            //
            // labelTimestampConversionFormat
            //
            this.labelTimestampConversionFormat.AutoSize = true;
            this.labelTimestampConversionFormat.Location = new System.Drawing.Point(381, 28);
            this.labelTimestampConversionFormat.Name = "labelTimestampConversionFormat";
            this.labelTimestampConversionFormat.Size = new System.Drawing.Size(16, 13);
            this.labelTimestampConversionFormat.TabIndex = 7;
            this.labelTimestampConversionFormat.Text = "...";
            //
            // textBoxTimestampConversionFormatForFilters
            //
            this.textBoxTimestampConversionFormatForFilters.Location = new System.Drawing.Point(237, 53);
            this.textBoxTimestampConversionFormatForFilters.Name = "textBoxTimestampConversionFormatForFilters";
            this.textBoxTimestampConversionFormatForFilters.Size = new System.Drawing.Size(129, 20);
            this.textBoxTimestampConversionFormatForFilters.TabIndex = 1;
            this.textBoxTimestampConversionFormatForFilters.TextChanged += new System.EventHandler(this.textBoxTimestampConversionFormatForFilters_TextChanged);
            //
            // label4
            //
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(12, 56);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(81, 13);
            this.label4.TabIndex = 5;
            this.label4.Text = "Format for filters";
            //
            // groupBox3
            //
            this.groupBox3.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBox3.Controls.Add(this.textBoxFilterQueryRows);
            this.groupBox3.Controls.Add(this.label5);
            this.groupBox3.Location = new System.Drawing.Point(12, 13);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(546, 59);
            this.groupBox3.TabIndex = 3;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "Filter Options";
            //
            // textBoxFilterQueryRows
            //
            this.textBoxFilterQueryRows.Location = new System.Drawing.Point(237, 24);
            this.textBoxFilterQueryRows.Name = "textBoxFilterQueryRows";
            this.textBoxFilterQueryRows.Size = new System.Drawing.Size(71, 20);
            this.textBoxFilterQueryRows.TabIndex = 3;
            //
            // label5
            //
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(12, 27);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(178, 13);
            this.label5.TabIndex = 4;
            this.label5.Text = "Number of rows to retrieve per query";
            //
            // groupBox4
            //
            this.groupBox4.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBox4.Controls.Add(this.label8);
            this.groupBox4.Controls.Add(this.textBoxGeneralQueryInitialRows);
            this.groupBox4.Controls.Add(this.label6);
            this.groupBox4.Controls.Add(this.textBoxGeneralQueryMaximumRows);
            this.groupBox4.Controls.Add(this.label7);
            this.groupBox4.Location = new System.Drawing.Point(12, 81);
            this.groupBox4.Name = "groupBox4";
            this.groupBox4.Size = new System.Drawing.Size(546, 93);
            this.groupBox4.TabIndex = 4;
            this.groupBox4.TabStop = false;
            this.groupBox4.Text = "General Queries";
            //
            // textBoxGeneralQueryInitialRows
            //
            this.textBoxGeneralQueryInitialRows.Location = new System.Drawing.Point(237, 27);
            this.textBoxGeneralQueryInitialRows.Name = "textBoxGeneralQueryInitialRows";
            this.textBoxGeneralQueryInitialRows.Size = new System.Drawing.Size(71, 20);
            this.textBoxGeneralQueryInitialRows.TabIndex = 0;
            //
            // label6
            //
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(12, 61);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(178, 13);
            this.label6.TabIndex = 2;
            this.label6.Text = "Maximum number of rows to process";
            //
            // textBoxGeneralQueryMaximumRows
            //
            this.textBoxGeneralQueryMaximumRows.Location = new System.Drawing.Point(237, 58);
            this.textBoxGeneralQueryMaximumRows.Name = "textBoxGeneralQueryMaximumRows";
            this.textBoxGeneralQueryMaximumRows.Size = new System.Drawing.Size(71, 20);
            this.textBoxGeneralQueryMaximumRows.TabIndex = 1;
            //
            // label7
            //
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(12, 30);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(158, 13);
            this.label7.TabIndex = 0;
            this.label7.Text = "Initial number of rows to process";
            //
            // buttonRestoreDefaults
            //
            this.buttonRestoreDefaults.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.buttonRestoreDefaults.Location = new System.Drawing.Point(447, 416);
            this.buttonRestoreDefaults.Name = "buttonRestoreDefaults";
            this.buttonRestoreDefaults.Size = new System.Drawing.Size(111, 23);
            this.buttonRestoreDefaults.TabIndex = 2;
            this.buttonRestoreDefaults.Text = "Restore Defaults";
            this.buttonRestoreDefaults.UseVisualStyleBackColor = true;
            this.buttonRestoreDefaults.Click += new System.EventHandler(this.buttonRestoreDefaults_Click);
            //
            // label8
            //
            this.label8.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Italic, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label8.Location = new System.Drawing.Point(330, 27);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(201, 63);
            this.label8.TabIndex = 3;
            this.label8.Text = "These values control how many rows are retrieved for information displayed in sum" +
    "mary tables, charts, maps, and other queries";
            //
            // label9
            //
            this.label9.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Italic, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label9.Location = new System.Drawing.Point(330, 27);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(201, 63);
            this.label9.TabIndex = 4;
            this.label9.Text = "These values control how many rows are retrieved for information displayed in tab" +
    "les that are not summary tables.";
            //
            // SettingsForm
            //
            this.AcceptButton = this.buttonOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.buttonCancel;
            this.ClientSize = new System.Drawing.Size(570, 451);
            this.Controls.Add(this.buttonRestoreDefaults);
            this.Controls.Add(this.groupBox4);
            this.Controls.Add(this.groupBox3);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.buttonCancel);
            this.Controls.Add(this.buttonOK);
            this.Controls.Add(this.groupBox1);
            this.Name = "SettingsForm";
            this.ShowIcon = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Paradata Viewer Settings";
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.groupBox3.ResumeLayout(false);
            this.groupBox3.PerformLayout();
            this.groupBox4.ResumeLayout(false);
            this.groupBox4.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox textBoxTabularQueryMaximumRows;
        private System.Windows.Forms.TextBox textBoxTabularQueryInitialRows;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TextBox textBoxTimestampConversionFormat;
        private System.Windows.Forms.LinkLabel linkLabelTimestampFormatters;
        private System.Windows.Forms.Button buttonOK;
        private System.Windows.Forms.Button buttonCancel;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Label labelTimestampConversionFormatForFilters;
        private System.Windows.Forms.Label labelTimestampConversionFormat;
        private System.Windows.Forms.TextBox textBoxTimestampConversionFormatForFilters;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.GroupBox groupBox3;
        private System.Windows.Forms.TextBox textBoxFilterQueryRows;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.GroupBox groupBox4;
        private System.Windows.Forms.TextBox textBoxGeneralQueryInitialRows;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.TextBox textBoxGeneralQueryMaximumRows;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Button buttonRestoreDefaults;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.Label label8;
    }
}