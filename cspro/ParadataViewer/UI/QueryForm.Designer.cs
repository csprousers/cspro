namespace ParadataViewer
{
    partial class QueryForm
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
            this.linkLabelSave = new System.Windows.Forms.LinkLabel();
            this.linkLabelLoadCompleteResults = new System.Windows.Forms.LinkLabel();
            this.labelResultsIgnoreFilters = new System.Windows.Forms.Label();
            this.panelLinks = new System.Windows.Forms.Panel();
            this.panelResults = new System.Windows.Forms.Panel();
            this.panelControls = new System.Windows.Forms.Panel();
            this.panelResultsAndControls = new System.Windows.Forms.Panel();
            this.splitterControls = new System.Windows.Forms.Splitter();
            this.panelLinks.SuspendLayout();
            this.panelResultsAndControls.SuspendLayout();
            this.SuspendLayout();
            //
            // linkLabelSave
            //
            this.linkLabelSave.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.linkLabelSave.Location = new System.Drawing.Point(562, 7);
            this.linkLabelSave.Name = "linkLabelSave";
            this.linkLabelSave.Size = new System.Drawing.Size(200, 13);
            this.linkLabelSave.TabIndex = 0;
            this.linkLabelSave.TabStop = true;
            this.linkLabelSave.Text = "Save results";
            this.linkLabelSave.TextAlign = System.Drawing.ContentAlignment.TopRight;
            this.linkLabelSave.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabelSave_LinkClicked);
            //
            // linkLabelLoadCompleteResults
            //
            this.linkLabelLoadCompleteResults.AutoSize = true;
            this.linkLabelLoadCompleteResults.Location = new System.Drawing.Point(12, 7);
            this.linkLabelLoadCompleteResults.Name = "linkLabelLoadCompleteResults";
            this.linkLabelLoadCompleteResults.Size = new System.Drawing.Size(110, 13);
            this.linkLabelLoadCompleteResults.TabIndex = 1;
            this.linkLabelLoadCompleteResults.TabStop = true;
            this.linkLabelLoadCompleteResults.Text = "Load complete results";
            this.linkLabelLoadCompleteResults.Visible = false;
            this.linkLabelLoadCompleteResults.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabelLoadCompleteResults_LinkClicked);
            //
            // labelResultsIgnoreFilters
            //
            this.labelResultsIgnoreFilters.Anchor = System.Windows.Forms.AnchorStyles.Top;
            this.labelResultsIgnoreFilters.AutoSize = true;
            this.labelResultsIgnoreFilters.Location = new System.Drawing.Point(296, 7);
            this.labelResultsIgnoreFilters.Name = "labelResultsIgnoreFilters";
            this.labelResultsIgnoreFilters.Size = new System.Drawing.Size(183, 13);
            this.labelResultsIgnoreFilters.TabIndex = 3;
            this.labelResultsIgnoreFilters.Text = "These results do not respond to filters";
            this.labelResultsIgnoreFilters.Visible = false;
            //
            // panelLinks
            //
            this.panelLinks.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.panelLinks.Controls.Add(this.labelResultsIgnoreFilters);
            this.panelLinks.Controls.Add(this.linkLabelLoadCompleteResults);
            this.panelLinks.Controls.Add(this.linkLabelSave);
            this.panelLinks.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panelLinks.Location = new System.Drawing.Point(0, 383);
            this.panelLinks.Name = "panelLinks";
            this.panelLinks.Size = new System.Drawing.Size(778, 28);
            this.panelLinks.TabIndex = 4;
            //
            // panelResults
            //
            this.panelResults.AutoScroll = true;
            this.panelResults.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.panelResults.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panelResults.Location = new System.Drawing.Point(0, 0);
            this.panelResults.Name = "panelResults";
            this.panelResults.Size = new System.Drawing.Size(778, 263);
            this.panelResults.TabIndex = 5;
            //
            // panelControls
            //
            this.panelControls.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.panelControls.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panelControls.Location = new System.Drawing.Point(0, 263);
            this.panelControls.Name = "panelControls";
            this.panelControls.Size = new System.Drawing.Size(778, 120);
            this.panelControls.TabIndex = 6;
            //
            // panelResultsAndControls
            //
            this.panelResultsAndControls.Controls.Add(this.splitterControls);
            this.panelResultsAndControls.Controls.Add(this.panelResults);
            this.panelResultsAndControls.Controls.Add(this.panelControls);
            this.panelResultsAndControls.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panelResultsAndControls.Location = new System.Drawing.Point(0, 0);
            this.panelResultsAndControls.Name = "panelResultsAndControls";
            this.panelResultsAndControls.Size = new System.Drawing.Size(778, 383);
            this.panelResultsAndControls.TabIndex = 7;
            //
            // splitterControls
            //
            this.splitterControls.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.splitterControls.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.splitterControls.Location = new System.Drawing.Point(0, 260);
            this.splitterControls.Name = "splitterControls";
            this.splitterControls.Size = new System.Drawing.Size(778, 3);
            this.splitterControls.TabIndex = 7;
            this.splitterControls.TabStop = false;
            this.splitterControls.SplitterMoved += new System.Windows.Forms.SplitterEventHandler(this.splitterControls_SplitterMoved);
            //
            // QueryForm
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(778, 411);
            this.Controls.Add(this.panelResultsAndControls);
            this.Controls.Add(this.panelLinks);
            this.Name = "QueryForm";
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.Text = "Paradata Query";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.QueryForm_FormClosing);
            this.Load += new System.EventHandler(this.QueryForm_Load);
            this.panelLinks.ResumeLayout(false);
            this.panelLinks.PerformLayout();
            this.panelResultsAndControls.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        protected System.Windows.Forms.LinkLabel linkLabelSave;
        private System.Windows.Forms.LinkLabel linkLabelLoadCompleteResults;
        private System.Windows.Forms.Label labelResultsIgnoreFilters;
        private System.Windows.Forms.Panel panelLinks;
        protected System.Windows.Forms.Panel panelResults;
        protected System.Windows.Forms.Panel panelControls;
        private System.Windows.Forms.Panel panelResultsAndControls;
        protected System.Windows.Forms.Splitter splitterControls;
    }
}