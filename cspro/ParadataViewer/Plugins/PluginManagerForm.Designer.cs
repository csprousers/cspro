namespace ParadataViewer
{
    partial class PluginManagerForm
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
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.buttonDownloadPlugin = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.buttonBrowsePlugin = new System.Windows.Forms.Button();
            this.label2 = new System.Windows.Forms.Label();
            this.textBoxUrl = new System.Windows.Forms.TextBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.buttonRemovePlugin = new System.Windows.Forms.Button();
            this.listViewPlugins = new System.Windows.Forms.ListView();
            this.columnHeader1 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader2 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.buttonClose = new System.Windows.Forms.Button();
            this.buttonCancelDownload = new System.Windows.Forms.Button();
            this.progressBarDownload = new System.Windows.Forms.ProgressBar();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.SuspendLayout();
            //
            // groupBox1
            //
            this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBox1.Controls.Add(this.buttonDownloadPlugin);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Controls.Add(this.buttonBrowsePlugin);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Controls.Add(this.textBoxUrl);
            this.groupBox1.Location = new System.Drawing.Point(12, 251);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(624, 131);
            this.groupBox1.TabIndex = 1;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Add Plugin";
            //
            // buttonDownloadPlugin
            //
            this.buttonDownloadPlugin.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.buttonDownloadPlugin.Location = new System.Drawing.Point(497, 49);
            this.buttonDownloadPlugin.Name = "buttonDownloadPlugin";
            this.buttonDownloadPlugin.Size = new System.Drawing.Size(116, 23);
            this.buttonDownloadPlugin.TabIndex = 1;
            this.buttonDownloadPlugin.Text = "Download Plugin";
            this.buttonDownloadPlugin.UseVisualStyleBackColor = true;
            this.buttonDownloadPlugin.Click += new System.EventHandler(this.buttonDownloadPlugin_Click);
            //
            // label1
            //
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(8, 54);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(32, 13);
            this.label1.TabIndex = 3;
            this.label1.Text = "URL:";
            //
            // buttonBrowsePlugin
            //
            this.buttonBrowsePlugin.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.buttonBrowsePlugin.Location = new System.Drawing.Point(497, 91);
            this.buttonBrowsePlugin.Name = "buttonBrowsePlugin";
            this.buttonBrowsePlugin.Size = new System.Drawing.Size(116, 23);
            this.buttonBrowsePlugin.TabIndex = 2;
            this.buttonBrowsePlugin.Text = "Browse for Plugin";
            this.buttonBrowsePlugin.UseVisualStyleBackColor = true;
            this.buttonBrowsePlugin.Click += new System.EventHandler(this.buttonBrowsePlugin_Click);
            //
            // label2
            //
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(8, 24);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(479, 13);
            this.label2.TabIndex = 2;
            this.label2.Text = "You can browse for and select a plugin located on your computer or download one f" +
    "rom the Internet.";
            //
            // textBoxUrl
            //
            this.textBoxUrl.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.textBoxUrl.Location = new System.Drawing.Point(65, 51);
            this.textBoxUrl.Name = "textBoxUrl";
            this.textBoxUrl.Size = new System.Drawing.Size(422, 20);
            this.textBoxUrl.TabIndex = 0;
            //
            // groupBox2
            //
            this.groupBox2.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBox2.Controls.Add(this.buttonRemovePlugin);
            this.groupBox2.Controls.Add(this.listViewPlugins);
            this.groupBox2.Location = new System.Drawing.Point(12, 12);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(624, 230);
            this.groupBox2.TabIndex = 3;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Manage Installed Plugins";
            //
            // buttonRemovePlugin
            //
            this.buttonRemovePlugin.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.buttonRemovePlugin.Location = new System.Drawing.Point(497, 193);
            this.buttonRemovePlugin.Name = "buttonRemovePlugin";
            this.buttonRemovePlugin.Size = new System.Drawing.Size(116, 23);
            this.buttonRemovePlugin.TabIndex = 1;
            this.buttonRemovePlugin.Text = "Remove Plugin";
            this.buttonRemovePlugin.UseVisualStyleBackColor = true;
            this.buttonRemovePlugin.Click += new System.EventHandler(this.buttonRemovePlugin_Click);
            //
            // listViewPlugins
            //
            this.listViewPlugins.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.listViewPlugins.CheckBoxes = true;
            this.listViewPlugins.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader1,
            this.columnHeader2});
            this.listViewPlugins.FullRowSelect = true;
            this.listViewPlugins.Location = new System.Drawing.Point(11, 26);
            this.listViewPlugins.Name = "listViewPlugins";
            this.listViewPlugins.Size = new System.Drawing.Size(602, 156);
            this.listViewPlugins.TabIndex = 0;
            this.listViewPlugins.UseCompatibleStateImageBehavior = false;
            this.listViewPlugins.View = System.Windows.Forms.View.Details;
            this.listViewPlugins.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.listViewPlugins_ItemCheck);
            //
            // columnHeader1
            //
            this.columnHeader1.Text = "Plugin";
            this.columnHeader1.Width = 155;
            //
            // columnHeader2
            //
            this.columnHeader2.Text = "Location";
            this.columnHeader2.Width = 431;
            //
            // buttonClose
            //
            this.buttonClose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.buttonClose.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.buttonClose.Location = new System.Drawing.Point(509, 398);
            this.buttonClose.Name = "buttonClose";
            this.buttonClose.Size = new System.Drawing.Size(116, 23);
            this.buttonClose.TabIndex = 0;
            this.buttonClose.Text = "Close";
            this.buttonClose.UseVisualStyleBackColor = true;
            //
            // buttonCancelDownload
            //
            this.buttonCancelDownload.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.buttonCancelDownload.Location = new System.Drawing.Point(383, 398);
            this.buttonCancelDownload.Name = "buttonCancelDownload";
            this.buttonCancelDownload.Size = new System.Drawing.Size(116, 23);
            this.buttonCancelDownload.TabIndex = 4;
            this.buttonCancelDownload.Text = "Cancel Download";
            this.buttonCancelDownload.UseVisualStyleBackColor = true;
            this.buttonCancelDownload.Visible = false;
            this.buttonCancelDownload.Click += new System.EventHandler(this.buttonCancelDownload_Click);
            //
            // progressBarDownload
            //
            this.progressBarDownload.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.progressBarDownload.Location = new System.Drawing.Point(12, 398);
            this.progressBarDownload.Name = "progressBarDownload";
            this.progressBarDownload.Size = new System.Drawing.Size(360, 23);
            this.progressBarDownload.TabIndex = 5;
            this.progressBarDownload.Visible = false;
            //
            // PluginManagerForm
            //
            this.AcceptButton = this.buttonClose;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.buttonClose;
            this.ClientSize = new System.Drawing.Size(652, 433);
            this.Controls.Add(this.progressBarDownload);
            this.Controls.Add(this.buttonCancelDownload);
            this.Controls.Add(this.buttonClose);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.groupBox1);
            this.Name = "PluginManagerForm";
            this.ShowIcon = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Plugin Manager";
            this.Load += new System.EventHandler(this.PluginManagerForm_Load);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Button buttonDownloadPlugin;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox textBoxUrl;
        private System.Windows.Forms.Button buttonBrowsePlugin;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Button buttonRemovePlugin;
        private System.Windows.Forms.ListView listViewPlugins;
        private System.Windows.Forms.ColumnHeader columnHeader1;
        private System.Windows.Forms.ColumnHeader columnHeader2;
        private System.Windows.Forms.Button buttonClose;
        private System.Windows.Forms.Button buttonCancelDownload;
        private System.Windows.Forms.ProgressBar progressBarDownload;
    }
}