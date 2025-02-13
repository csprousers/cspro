namespace PFF_Editor
{
    partial class RepositoryUserControl
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
            this.repositoryComboBox = new System.Windows.Forms.ComboBox();
            this.SuspendLayout();
            // 
            // repositoryComboBox
            // 
            this.repositoryComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.repositoryComboBox.FormattingEnabled = true;
            this.repositoryComboBox.Location = new System.Drawing.Point(0, 0);
            this.repositoryComboBox.Margin = new System.Windows.Forms.Padding(0);
            this.repositoryComboBox.Name = "repositoryComboBox";
            this.repositoryComboBox.Size = new System.Drawing.Size(140, 21);
            this.repositoryComboBox.Sorted = true;
            this.repositoryComboBox.TabIndex = 0;
            this.repositoryComboBox.SelectionChangeCommitted += new System.EventHandler(this.repositoryComboBox_SelectionChangeCommitted);
            // 
            // RepositoryUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.repositoryComboBox);
            this.Margin = new System.Windows.Forms.Padding(0);
            this.Name = "RepositoryUserControl";
            this.Size = new System.Drawing.Size(140, 21);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.ComboBox repositoryComboBox;
    }
}
