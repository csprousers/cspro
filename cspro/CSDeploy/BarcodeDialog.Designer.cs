namespace CSDeploy
{
    partial class BarcodeDialog
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
            System.Windows.Forms.Label label3;
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(BarcodeDialog));
            System.Windows.Forms.Label label4;
            this.labelInstructions = new System.Windows.Forms.Label();
            this.pictureBoxQRCode = new System.Windows.Forms.PictureBox();
            this.checkBoxUsernamePassword = new System.Windows.Forms.CheckBox();
            this.textBoxUsername = new System.Windows.Forms.TextBox();
            this.textBoxPassword = new System.Windows.Forms.TextBox();
            this.buttonOK = new System.Windows.Forms.Button();
            this.textBoxHyperlink = new System.Windows.Forms.TextBox();
            this.buttonPrint = new System.Windows.Forms.Button();
            this.printDialog = new System.Windows.Forms.PrintDialog();
            this.printDocument = new System.Drawing.Printing.PrintDocument();
            this.buttonSave = new System.Windows.Forms.Button();
            label3 = new System.Windows.Forms.Label();
            label4 = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxQRCode)).BeginInit();
            this.SuspendLayout();
            // 
            // label3
            // 
            resources.ApplyResources(label3, "label3");
            label3.Name = "label3";
            // 
            // label4
            // 
            resources.ApplyResources(label4, "label4");
            label4.Name = "label4";
            // 
            // labelInstructions
            // 
            resources.ApplyResources(this.labelInstructions, "labelInstructions");
            this.labelInstructions.Name = "labelInstructions";
            // 
            // pictureBoxQRCode
            // 
            resources.ApplyResources(this.pictureBoxQRCode, "pictureBoxQRCode");
            this.pictureBoxQRCode.Name = "pictureBoxQRCode";
            this.pictureBoxQRCode.TabStop = false;
            // 
            // checkBoxUsernamePassword
            // 
            resources.ApplyResources(this.checkBoxUsernamePassword, "checkBoxUsernamePassword");
            this.checkBoxUsernamePassword.Name = "checkBoxUsernamePassword";
            this.checkBoxUsernamePassword.UseVisualStyleBackColor = true;
            this.checkBoxUsernamePassword.CheckedChanged += new System.EventHandler(this.checkBoxUsernamePassword_CheckedChanged);
            // 
            // textBoxUsername
            // 
            resources.ApplyResources(this.textBoxUsername, "textBoxUsername");
            this.textBoxUsername.Name = "textBoxUsername";
            this.textBoxUsername.TextChanged += new System.EventHandler(this.textBoxUsername_TextChanged);
            // 
            // textBoxPassword
            // 
            resources.ApplyResources(this.textBoxPassword, "textBoxPassword");
            this.textBoxPassword.Name = "textBoxPassword";
            this.textBoxPassword.TextChanged += new System.EventHandler(this.textBoxPassword_TextChanged);
            // 
            // buttonOK
            // 
            resources.ApplyResources(this.buttonOK, "buttonOK");
            this.buttonOK.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.buttonOK.Name = "buttonOK";
            this.buttonOK.UseVisualStyleBackColor = true;
            this.buttonOK.Click += new System.EventHandler(this.buttonOK_Click);
            // 
            // textBoxHyperlink
            // 
            resources.ApplyResources(this.textBoxHyperlink, "textBoxHyperlink");
            this.textBoxHyperlink.BackColor = System.Drawing.SystemColors.Control;
            this.textBoxHyperlink.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.textBoxHyperlink.ForeColor = System.Drawing.SystemColors.HotTrack;
            this.textBoxHyperlink.Name = "textBoxHyperlink";
            this.textBoxHyperlink.ReadOnly = true;
            this.textBoxHyperlink.TabStop = false;
            // 
            // buttonPrint
            // 
            resources.ApplyResources(this.buttonPrint, "buttonPrint");
            this.buttonPrint.Name = "buttonPrint";
            this.buttonPrint.UseVisualStyleBackColor = true;
            this.buttonPrint.Click += new System.EventHandler(this.buttonPrint_Click);
            // 
            // printDialog
            // 
            this.printDialog.Document = this.printDocument;
            this.printDialog.UseEXDialog = true;
            // 
            // printDocument
            // 
            this.printDocument.PrintPage += new System.Drawing.Printing.PrintPageEventHandler(this.printDocument_PrintPage);
            // 
            // buttonSave
            // 
            resources.ApplyResources(this.buttonSave, "buttonSave");
            this.buttonSave.Name = "buttonSave";
            this.buttonSave.UseVisualStyleBackColor = true;
            this.buttonSave.Click += new System.EventHandler(this.buttonSave_Click);
            // 
            // BarcodeDialog
            // 
            this.AcceptButton = this.buttonOK;
            resources.ApplyResources(this, "$this");
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.buttonOK;
            this.Controls.Add(this.buttonSave);
            this.Controls.Add(this.buttonPrint);
            this.Controls.Add(this.textBoxHyperlink);
            this.Controls.Add(this.buttonOK);
            this.Controls.Add(this.textBoxPassword);
            this.Controls.Add(this.textBoxUsername);
            this.Controls.Add(label4);
            this.Controls.Add(label3);
            this.Controls.Add(this.checkBoxUsernamePassword);
            this.Controls.Add(this.pictureBoxQRCode);
            this.Controls.Add(this.labelInstructions);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "BarcodeDialog";
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.Load += new System.EventHandler(this.BarcodeDialog_Load);
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxQRCode)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.Label labelInstructions;
        private System.Windows.Forms.PictureBox pictureBoxQRCode;
        private System.Windows.Forms.CheckBox checkBoxUsernamePassword;
        private System.Windows.Forms.TextBox textBoxUsername;
        private System.Windows.Forms.TextBox textBoxPassword;
        private System.Windows.Forms.Button buttonOK;
        private System.Windows.Forms.TextBox textBoxHyperlink;
        private System.Windows.Forms.Button buttonPrint;
        private System.Windows.Forms.PrintDialog printDialog;
        private System.Drawing.Printing.PrintDocument printDocument;
        private System.Windows.Forms.Button buttonSave;
    }
}
