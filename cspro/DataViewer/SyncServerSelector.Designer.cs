namespace DataViewer
{
    partial class SyncServerSelector
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
            if (disposing && (components != null))
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
            this.textBoxServerUrl = new System.Windows.Forms.TextBox();
            this.radioButtonFTP = new System.Windows.Forms.RadioButton();
            this.radioButtonDropbox = new System.Windows.Forms.RadioButton();
            this.radioButtonCSWeb = new System.Windows.Forms.RadioButton();
            this.radioButtonDropboxFolder = new System.Windows.Forms.RadioButton();
            this.radioButtonFTPFolder = new System.Windows.Forms.RadioButton();
            this.buttonFolderBrowse = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // textBoxServerUrl
            // 
            this.textBoxServerUrl.Location = new System.Drawing.Point(0, 132);
            this.textBoxServerUrl.Margin = new System.Windows.Forms.Padding(2);
            this.textBoxServerUrl.Name = "textBoxServerUrl";
            this.textBoxServerUrl.Size = new System.Drawing.Size(247, 20);
            this.textBoxServerUrl.TabIndex = 10;
            this.textBoxServerUrl.TextChanged += new System.EventHandler(this.textBoxServerUrl_TextChanged);
            // 
            // radioButtonFTP
            // 
            this.radioButtonFTP.AutoSize = true;
            this.radioButtonFTP.Location = new System.Drawing.Point(2, 54);
            this.radioButtonFTP.Margin = new System.Windows.Forms.Padding(2);
            this.radioButtonFTP.Name = "radioButtonFTP";
            this.radioButtonFTP.Size = new System.Drawing.Size(79, 17);
            this.radioButtonFTP.TabIndex = 9;
            this.radioButtonFTP.TabStop = true;
            this.radioButtonFTP.Text = "FTP Server";
            this.radioButtonFTP.UseVisualStyleBackColor = true;
            this.radioButtonFTP.CheckedChanged += new System.EventHandler(this.radioButtonFTP_CheckedChanged);
            // 
            // radioButtonDropbox
            // 
            this.radioButtonDropbox.AutoSize = true;
            this.radioButtonDropbox.Location = new System.Drawing.Point(2, 28);
            this.radioButtonDropbox.Margin = new System.Windows.Forms.Padding(2);
            this.radioButtonDropbox.Name = "radioButtonDropbox";
            this.radioButtonDropbox.Size = new System.Drawing.Size(104, 17);
            this.radioButtonDropbox.TabIndex = 8;
            this.radioButtonDropbox.TabStop = true;
            this.radioButtonDropbox.Text = "Dropbox (Online)";
            this.radioButtonDropbox.UseVisualStyleBackColor = true;
            this.radioButtonDropbox.CheckedChanged += new System.EventHandler(this.radioButtonDropbox_CheckedChanged);
            // 
            // radioButtonCSWeb
            // 
            this.radioButtonCSWeb.AutoSize = true;
            this.radioButtonCSWeb.Location = new System.Drawing.Point(2, 2);
            this.radioButtonCSWeb.Margin = new System.Windows.Forms.Padding(2);
            this.radioButtonCSWeb.Name = "radioButtonCSWeb";
            this.radioButtonCSWeb.Size = new System.Drawing.Size(96, 17);
            this.radioButtonCSWeb.TabIndex = 7;
            this.radioButtonCSWeb.TabStop = true;
            this.radioButtonCSWeb.Text = "CSWeb Server";
            this.radioButtonCSWeb.UseVisualStyleBackColor = true;
            this.radioButtonCSWeb.CheckedChanged += new System.EventHandler(this.radioButtonCSWeb_CheckedChanged);
            // 
            // radioButtonDropboxFolder
            // 
            this.radioButtonDropboxFolder.AutoSize = true;
            this.radioButtonDropboxFolder.Location = new System.Drawing.Point(2, 80);
            this.radioButtonDropboxFolder.Name = "radioButtonDropboxFolder";
            this.radioButtonDropboxFolder.Size = new System.Drawing.Size(179, 17);
            this.radioButtonDropboxFolder.TabIndex = 11;
            this.radioButtonDropboxFolder.TabStop = true;
            this.radioButtonDropboxFolder.Text = "Dropbox Folder on this Computer";
            this.radioButtonDropboxFolder.UseVisualStyleBackColor = true;
            this.radioButtonDropboxFolder.CheckedChanged += new System.EventHandler(this.radioButtonDropboxFolder_CheckedChanged);
            // 
            // radioButtonFTPFolder
            // 
            this.radioButtonFTPFolder.AutoSize = true;
            this.radioButtonFTPFolder.Location = new System.Drawing.Point(2, 106);
            this.radioButtonFTPFolder.Name = "radioButtonFTPFolder";
            this.radioButtonFTPFolder.Size = new System.Drawing.Size(159, 17);
            this.radioButtonFTPFolder.TabIndex = 12;
            this.radioButtonFTPFolder.TabStop = true;
            this.radioButtonFTPFolder.Text = "FTP Folder on this Computer";
            this.radioButtonFTPFolder.UseVisualStyleBackColor = true;
            this.radioButtonFTPFolder.CheckedChanged += new System.EventHandler(this.radioButtonFTPFolder_CheckedChanged);
            // 
            // buttonFolderBrowse
            // 
            this.buttonFolderBrowse.Location = new System.Drawing.Point(260, 130);
            this.buttonFolderBrowse.Name = "buttonFolderBrowse";
            this.buttonFolderBrowse.Size = new System.Drawing.Size(34, 23);
            this.buttonFolderBrowse.TabIndex = 13;
            this.buttonFolderBrowse.Text = "...";
            this.buttonFolderBrowse.UseVisualStyleBackColor = true;
            this.buttonFolderBrowse.Click += new System.EventHandler(this.buttonFolderBrowse_Click);
            // 
            // SyncServerSelector
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.buttonFolderBrowse);
            this.Controls.Add(this.radioButtonFTPFolder);
            this.Controls.Add(this.radioButtonDropboxFolder);
            this.Controls.Add(this.textBoxServerUrl);
            this.Controls.Add(this.radioButtonFTP);
            this.Controls.Add(this.radioButtonDropbox);
            this.Controls.Add(this.radioButtonCSWeb);
            this.Name = "SyncServerSelector";
            this.Size = new System.Drawing.Size(298, 156);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.TextBox textBoxServerUrl;
        private System.Windows.Forms.RadioButton radioButtonFTP;
        private System.Windows.Forms.RadioButton radioButtonDropbox;
        private System.Windows.Forms.RadioButton radioButtonCSWeb;
        private System.Windows.Forms.RadioButton radioButtonDropboxFolder;
        private System.Windows.Forms.RadioButton radioButtonFTPFolder;
        private System.Windows.Forms.Button buttonFolderBrowse;
    }
}
