namespace Excel2CSPro
{
    partial class Excel2CSProMappingControl
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
            this.comboBoxMapping = new System.Windows.Forms.ComboBox();
            this.labelLabel = new System.Windows.Forms.Label();
            this.labelName = new System.Windows.Forms.Label();
            this.buttonFillDefaultItemMappings = new System.Windows.Forms.Button();
            this.buttonResetMappings = new System.Windows.Forms.Button();
            this.SuspendLayout();
            //
            // comboBoxMapping
            //
            this.comboBoxMapping.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxMapping.FormattingEnabled = true;
            this.comboBoxMapping.Location = new System.Drawing.Point(224, 22);
            this.comboBoxMapping.Name = "comboBoxMapping";
            this.comboBoxMapping.Size = new System.Drawing.Size(191, 21);
            this.comboBoxMapping.TabIndex = 5;
            this.comboBoxMapping.SelectedIndexChanged += new System.EventHandler(this.comboBoxMapping_SelectedIndexChanged);
            //
            // labelLabel
            //
            this.labelLabel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.labelLabel.AutoEllipsis = true;
            this.labelLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Italic);
            this.labelLabel.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.labelLabel.Location = new System.Drawing.Point(3, 0);
            this.labelLabel.Name = "labelLabel";
            this.labelLabel.Size = new System.Drawing.Size(406, 13);
            this.labelLabel.TabIndex = 4;
            this.labelLabel.Text = "...";
            //
            // labelName
            //
            this.labelName.AutoEllipsis = true;
            this.labelName.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold);
            this.labelName.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.labelName.Location = new System.Drawing.Point(3, 25);
            this.labelName.Name = "labelName";
            this.labelName.Size = new System.Drawing.Size(215, 13);
            this.labelName.TabIndex = 3;
            this.labelName.Text = "...";
            //
            // buttonFillDefaultItemMappings
            //
            this.buttonFillDefaultItemMappings.Enabled = false;
            this.buttonFillDefaultItemMappings.Location = new System.Drawing.Point(282, 54);
            this.buttonFillDefaultItemMappings.Name = "buttonFillDefaultItemMappings";
            this.buttonFillDefaultItemMappings.Size = new System.Drawing.Size(135, 23);
            this.buttonFillDefaultItemMappings.TabIndex = 6;
            this.buttonFillDefaultItemMappings.Text = "Assign Default Mappings";
            this.buttonFillDefaultItemMappings.UseVisualStyleBackColor = true;
            this.buttonFillDefaultItemMappings.Click += new System.EventHandler(this.buttonFillDefaultItemMappings_Click);
            //
            // buttonResetMappings
            //
            this.buttonResetMappings.Enabled = false;
            this.buttonResetMappings.Location = new System.Drawing.Point(141, 54);
            this.buttonResetMappings.Name = "buttonResetMappings";
            this.buttonResetMappings.Size = new System.Drawing.Size(135, 23);
            this.buttonResetMappings.TabIndex = 7;
            this.buttonResetMappings.Text = "Reset Mappings";
            this.buttonResetMappings.UseVisualStyleBackColor = true;
            this.buttonResetMappings.Click += new System.EventHandler(this.buttonResetMappings_Click);
            //
            // Excel2CSProMappingControl
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.buttonResetMappings);
            this.Controls.Add(this.buttonFillDefaultItemMappings);
            this.Controls.Add(this.comboBoxMapping);
            this.Controls.Add(this.labelLabel);
            this.Controls.Add(this.labelName);
            this.Name = "Excel2CSProMappingControl";
            this.Size = new System.Drawing.Size(420, 88);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.ComboBox comboBoxMapping;
        private System.Windows.Forms.Label labelLabel;
        private System.Windows.Forms.Label labelName;
        private System.Windows.Forms.Button buttonFillDefaultItemMappings;
        private System.Windows.Forms.Button buttonResetMappings;
    }
}
