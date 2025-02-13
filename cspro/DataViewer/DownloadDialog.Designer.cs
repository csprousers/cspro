namespace DataViewer
{
    partial class DownloadDialog
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

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.label1 = new System.Windows.Forms.Label();
            this.buttonConnect = new System.Windows.Forms.Button();
            this.label2 = new System.Windows.Forms.Label();
            this.comboBoxDictionaries = new System.Windows.Forms.ComboBox();
            this.label3 = new System.Windows.Forms.Label();
            this.textBoxSaveAsFilename = new System.Windows.Forms.TextBox();
            this.buttonSaveAsBrowse = new System.Windows.Forms.Button();
            this.buttonDownload = new System.Windows.Forms.Button();
            this.buttonCancel = new System.Windows.Forms.Button();
            this.serverSelector = new DataViewer.SyncServerSelector();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(3, 80);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(41, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Server:";
            // 
            // buttonConnect
            // 
            this.buttonConnect.Location = new System.Drawing.Point(368, 70);
            this.buttonConnect.Name = "buttonConnect";
            this.buttonConnect.Size = new System.Drawing.Size(75, 23);
            this.buttonConnect.TabIndex = 2;
            this.buttonConnect.Text = "Connect";
            this.buttonConnect.UseVisualStyleBackColor = true;
            this.buttonConnect.Click += new System.EventHandler(this.buttonConnect_Click);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(9, 193);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(33, 13);
            this.label2.TabIndex = 3;
            this.label2.Text = "Data:";
            // 
            // comboBoxDictionaries
            // 
            this.comboBoxDictionaries.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxDictionaries.FormattingEnabled = true;
            this.comboBoxDictionaries.Location = new System.Drawing.Point(68, 190);
            this.comboBoxDictionaries.Name = "comboBoxDictionaries";
            this.comboBoxDictionaries.Size = new System.Drawing.Size(247, 21);
            this.comboBoxDictionaries.TabIndex = 4;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(9, 238);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(49, 13);
            this.label3.TabIndex = 5;
            this.label3.Text = "Save as:";
            // 
            // textBoxSaveAsFilename
            // 
            this.textBoxSaveAsFilename.Location = new System.Drawing.Point(68, 235);
            this.textBoxSaveAsFilename.Name = "textBoxSaveAsFilename";
            this.textBoxSaveAsFilename.Size = new System.Drawing.Size(247, 20);
            this.textBoxSaveAsFilename.TabIndex = 6;
            this.textBoxSaveAsFilename.TextChanged += new System.EventHandler(this.textBoxSaveAsFilename_TextChanged);
            // 
            // buttonSaveAsBrowse
            // 
            this.buttonSaveAsBrowse.Location = new System.Drawing.Point(328, 233);
            this.buttonSaveAsBrowse.Name = "buttonSaveAsBrowse";
            this.buttonSaveAsBrowse.Size = new System.Drawing.Size(34, 23);
            this.buttonSaveAsBrowse.TabIndex = 7;
            this.buttonSaveAsBrowse.Text = "...";
            this.buttonSaveAsBrowse.UseVisualStyleBackColor = true;
            this.buttonSaveAsBrowse.Click += new System.EventHandler(this.buttonSaveAsBrowse_Click);
            // 
            // buttonDownload
            // 
            this.buttonDownload.Location = new System.Drawing.Point(287, 278);
            this.buttonDownload.Name = "buttonDownload";
            this.buttonDownload.Size = new System.Drawing.Size(75, 23);
            this.buttonDownload.TabIndex = 8;
            this.buttonDownload.Text = "Download";
            this.buttonDownload.UseVisualStyleBackColor = true;
            this.buttonDownload.Click += new System.EventHandler(this.buttonDownload_Click);
            // 
            // buttonCancel
            // 
            this.buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.buttonCancel.Location = new System.Drawing.Point(368, 278);
            this.buttonCancel.Name = "buttonCancel";
            this.buttonCancel.Size = new System.Drawing.Size(75, 23);
            this.buttonCancel.TabIndex = 9;
            this.buttonCancel.Text = "Cancel";
            this.buttonCancel.UseVisualStyleBackColor = true;
            // 
            // serverSelector
            // 
            this.serverSelector.Location = new System.Drawing.Point(68, 12);
            this.serverSelector.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.serverSelector.Name = "serverSelector";
            this.serverSelector.Size = new System.Drawing.Size(298, 156);
            this.serverSelector.TabIndex = 1;

            // 
            // DownloadDialog
            // 
            this.AcceptButton = this.buttonDownload;
            this.CancelButton = this.buttonCancel;
            this.ClientSize = new System.Drawing.Size(455, 313);
            this.Controls.Add(this.buttonCancel);
            this.Controls.Add(this.buttonDownload);
            this.Controls.Add(this.buttonSaveAsBrowse);
            this.Controls.Add(this.textBoxSaveAsFilename);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.comboBoxDictionaries);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.buttonConnect);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.serverSelector);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "DownloadDialog";
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Download";
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.DownloadDialog_FormClosed);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private SyncServerSelector serverSelector;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button buttonConnect;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.ComboBox comboBoxDictionaries;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TextBox textBoxSaveAsFilename;
        private System.Windows.Forms.Button buttonSaveAsBrowse;
        private System.Windows.Forms.Button buttonDownload;
        private System.Windows.Forms.Button buttonCancel;
    }
}
