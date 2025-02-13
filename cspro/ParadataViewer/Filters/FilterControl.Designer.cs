namespace ParadataViewer
{
    partial class FilterControl
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
            this.labelGettingValues = new System.Windows.Forms.Label();
            this.linkLabelLoadAdditionalResults = new System.Windows.Forms.LinkLabel();
            this.listViewValues = new System.Windows.Forms.ListView();
            this.columnHeader1 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.SuspendLayout();
            //
            // labelGettingValues
            //
            this.labelGettingValues.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.labelGettingValues.AutoSize = true;
            this.labelGettingValues.Location = new System.Drawing.Point(3, 3);
            this.labelGettingValues.Name = "labelGettingValues";
            this.labelGettingValues.Size = new System.Drawing.Size(111, 13);
            this.labelGettingValues.TabIndex = 0;
            this.labelGettingValues.Text = "Getting list of values...";
            //
            // linkLabelLoadAdditionalResults
            //
            this.linkLabelLoadAdditionalResults.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.linkLabelLoadAdditionalResults.AutoSize = true;
            this.linkLabelLoadAdditionalResults.Location = new System.Drawing.Point(58, 35);
            this.linkLabelLoadAdditionalResults.Name = "linkLabelLoadAdditionalResults";
            this.linkLabelLoadAdditionalResults.Size = new System.Drawing.Size(112, 13);
            this.linkLabelLoadAdditionalResults.TabIndex = 1;
            this.linkLabelLoadAdditionalResults.TabStop = true;
            this.linkLabelLoadAdditionalResults.Text = "Load additional results";
            this.linkLabelLoadAdditionalResults.Visible = false;
            this.linkLabelLoadAdditionalResults.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabelLoadAdditionalResults_LinkClicked);
            //
            // listViewValues
            //
            this.listViewValues.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.listViewValues.CheckBoxes = true;
            this.listViewValues.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader1});
            this.listViewValues.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.None;
            this.listViewValues.Location = new System.Drawing.Point(6, 3);
            this.listViewValues.MultiSelect = false;
            this.listViewValues.Name = "listViewValues";
            this.listViewValues.Size = new System.Drawing.Size(160, 25);
            this.listViewValues.TabIndex = 0;
            this.listViewValues.UseCompatibleStateImageBehavior = false;
            this.listViewValues.View = System.Windows.Forms.View.Details;
            this.listViewValues.ItemChecked += new System.Windows.Forms.ItemCheckedEventHandler(this.listViewValues_ItemChecked);
            this.listViewValues.MouseUp += new System.Windows.Forms.MouseEventHandler(this.listViewValues_MouseUp);
            //
            // columnHeader1
            //
            this.columnHeader1.Text = "Value";
            //
            // FilterControl
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.listViewValues);
            this.Controls.Add(this.linkLabelLoadAdditionalResults);
            this.Controls.Add(this.labelGettingValues);
            this.Name = "FilterControl";
            this.Size = new System.Drawing.Size(173, 57);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label labelGettingValues;
        private System.Windows.Forms.LinkLabel linkLabelLoadAdditionalResults;
        private System.Windows.Forms.ListView listViewValues;
        private System.Windows.Forms.ColumnHeader columnHeader1;
    }
}
