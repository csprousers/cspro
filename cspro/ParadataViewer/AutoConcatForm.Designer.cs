namespace ParadataViewer
{
    partial class AutoConcatForm
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
            this.labelText = new System.Windows.Forms.Label();
            this.checkBoxIncludeCurrentLog = new System.Windows.Forms.CheckBox();
            this.buttonConcatenateTemporaryResult = new System.Windows.Forms.Button();
            this.buttonConcatenateSaveResult = new System.Windows.Forms.Button();
            this.buttonCancel = new System.Windows.Forms.Button();
            this.SuspendLayout();
            //
            // labelText
            //
            this.labelText.AutoSize = true;
            this.labelText.Location = new System.Drawing.Point(12, 14);
            this.labelText.Name = "labelText";
            this.labelText.Size = new System.Drawing.Size(516, 13);
            this.labelText.TabIndex = 0;
            this.labelText.Text = "You can only view one log at a time. Would you like to concatenate these {0} logs" +
    " and then view the result?";
            //
            // checkBoxIncludeCurrentLog
            //
            this.checkBoxIncludeCurrentLog.AutoSize = true;
            this.checkBoxIncludeCurrentLog.Location = new System.Drawing.Point(15, 45);
            this.checkBoxIncludeCurrentLog.Name = "checkBoxIncludeCurrentLog";
            this.checkBoxIncludeCurrentLog.Size = new System.Drawing.Size(148, 17);
            this.checkBoxIncludeCurrentLog.TabIndex = 3;
            this.checkBoxIncludeCurrentLog.Text = "Include currently open log";
            this.checkBoxIncludeCurrentLog.UseVisualStyleBackColor = true;
            //
            // buttonConcatenateTemporaryResult
            //
            this.buttonConcatenateTemporaryResult.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.buttonConcatenateTemporaryResult.Location = new System.Drawing.Point(15, 75);
            this.buttonConcatenateTemporaryResult.Name = "buttonConcatenateTemporaryResult";
            this.buttonConcatenateTemporaryResult.Size = new System.Drawing.Size(185, 23);
            this.buttonConcatenateTemporaryResult.TabIndex = 0;
            this.buttonConcatenateTemporaryResult.Text = "Concatenate to Temporary Result";
            this.buttonConcatenateTemporaryResult.UseVisualStyleBackColor = true;
            //
            // buttonConcatenateSaveResult
            //
            this.buttonConcatenateSaveResult.Location = new System.Drawing.Point(234, 75);
            this.buttonConcatenateSaveResult.Name = "buttonConcatenateSaveResult";
            this.buttonConcatenateSaveResult.Size = new System.Drawing.Size(185, 23);
            this.buttonConcatenateSaveResult.TabIndex = 1;
            this.buttonConcatenateSaveResult.Text = "Concatenate and Save Result";
            this.buttonConcatenateSaveResult.UseVisualStyleBackColor = true;
            this.buttonConcatenateSaveResult.Click += new System.EventHandler(this.buttonConcatenateSaveResult_Click);
            //
            // buttonCancel
            //
            this.buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.buttonCancel.Location = new System.Drawing.Point(453, 75);
            this.buttonCancel.Name = "buttonCancel";
            this.buttonCancel.Size = new System.Drawing.Size(75, 23);
            this.buttonCancel.TabIndex = 2;
            this.buttonCancel.Text = "Cancel";
            this.buttonCancel.UseVisualStyleBackColor = true;
            //
            // AutoConcatForm
            //
            this.AcceptButton = this.buttonConcatenateTemporaryResult;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.buttonCancel;
            this.ClientSize = new System.Drawing.Size(541, 113);
            this.Controls.Add(this.buttonCancel);
            this.Controls.Add(this.buttonConcatenateSaveResult);
            this.Controls.Add(this.buttonConcatenateTemporaryResult);
            this.Controls.Add(this.checkBoxIncludeCurrentLog);
            this.Controls.Add(this.labelText);
            this.Name = "AutoConcatForm";
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Concatenate Logs?";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label labelText;
        private System.Windows.Forms.CheckBox checkBoxIncludeCurrentLog;
        private System.Windows.Forms.Button buttonConcatenateTemporaryResult;
        private System.Windows.Forms.Button buttonConcatenateSaveResult;
        private System.Windows.Forms.Button buttonCancel;
    }
}