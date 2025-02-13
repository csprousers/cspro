namespace ParadataViewer
{
    partial class QueryConstructorControl
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
            this.label1 = new System.Windows.Forms.Label();
            this.textBoxQuerySql = new System.Windows.Forms.TextBox();
            this.SuspendLayout();
            //
            // label1
            //
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(11, 8);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(358, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Enter the SQL query and press Control + Enter (or F5) to execute the query";
            //
            // textBoxQuerySql
            //
            this.textBoxQuerySql.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.textBoxQuerySql.Font = new System.Drawing.Font("Consolas", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.textBoxQuerySql.Location = new System.Drawing.Point(14, 30);
            this.textBoxQuerySql.Multiline = true;
            this.textBoxQuerySql.Name = "textBoxQuerySql";
            this.textBoxQuerySql.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.textBoxQuerySql.Size = new System.Drawing.Size(347, 28);
            this.textBoxQuerySql.TabIndex = 1;
            this.textBoxQuerySql.KeyDown += new System.Windows.Forms.KeyEventHandler(this.textBoxQuerySql_KeyDown);
            //
            // QueryConstructorControl
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.textBoxQuerySql);
            this.Controls.Add(this.label1);
            this.Name = "QueryConstructorControl";
            this.Size = new System.Drawing.Size(374, 72);
            this.Load += new System.EventHandler(this.QueryConstructorControl_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox textBoxQuerySql;
    }
}
