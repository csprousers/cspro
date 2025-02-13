namespace ParadataViewer
{
    partial class FilterPanelControl
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
            this.flowLayoutPanelFilters = new System.Windows.Forms.FlowLayoutPanel();
            this.labelFilters = new System.Windows.Forms.Label();
            this.linkLabelResetFilters = new System.Windows.Forms.LinkLabel();
            this.SuspendLayout();
            //
            // flowLayoutPanelFilters
            //
            this.flowLayoutPanelFilters.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.flowLayoutPanelFilters.AutoScroll = true;
            this.flowLayoutPanelFilters.FlowDirection = System.Windows.Forms.FlowDirection.TopDown;
            this.flowLayoutPanelFilters.Location = new System.Drawing.Point(3, 28);
            this.flowLayoutPanelFilters.Name = "flowLayoutPanelFilters";
            this.flowLayoutPanelFilters.Size = new System.Drawing.Size(242, 503);
            this.flowLayoutPanelFilters.TabIndex = 0;
            this.flowLayoutPanelFilters.Visible = false;
            this.flowLayoutPanelFilters.WrapContents = false;
            this.flowLayoutPanelFilters.SizeChanged += new System.EventHandler(this.flowLayoutPanelFilters_SizeChanged);
            //
            // labelFilters
            //
            this.labelFilters.AutoSize = true;
            this.labelFilters.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labelFilters.Location = new System.Drawing.Point(3, 5);
            this.labelFilters.Name = "labelFilters";
            this.labelFilters.Size = new System.Drawing.Size(41, 13);
            this.labelFilters.TabIndex = 1;
            this.labelFilters.Text = "Filters";
            this.labelFilters.Visible = false;
            //
            // linkLabelResetFilters
            //
            this.linkLabelResetFilters.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.linkLabelResetFilters.AutoSize = true;
            this.linkLabelResetFilters.Location = new System.Drawing.Point(3, 536);
            this.linkLabelResetFilters.Name = "linkLabelResetFilters";
            this.linkLabelResetFilters.Size = new System.Drawing.Size(65, 13);
            this.linkLabelResetFilters.TabIndex = 2;
            this.linkLabelResetFilters.TabStop = true;
            this.linkLabelResetFilters.Text = "Reset Filters";
            this.linkLabelResetFilters.Visible = false;
            this.linkLabelResetFilters.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabelResetFilters_LinkClicked);
            //
            // FilterPanelControl
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.linkLabelResetFilters);
            this.Controls.Add(this.labelFilters);
            this.Controls.Add(this.flowLayoutPanelFilters);
            this.Name = "FilterPanelControl";
            this.Size = new System.Drawing.Size(248, 549);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanelFilters;
        private System.Windows.Forms.Label labelFilters;
        private System.Windows.Forms.LinkLabel linkLabelResetFilters;
    }
}
