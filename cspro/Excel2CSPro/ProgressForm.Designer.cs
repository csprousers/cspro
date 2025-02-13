namespace Excel2CSPro
{
    partial class ProgressForm
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ProgressForm));
            this.progressBarReading = new System.Windows.Forms.ProgressBar();
            this.buttonCancel = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.progressBarWriting = new System.Windows.Forms.ProgressBar();
            this.labelWriting = new System.Windows.Forms.Label();
            this.labelCases = new System.Windows.Forms.Label();
            this.panelWriting = new System.Windows.Forms.Panel();
            this.panelWriting.SuspendLayout();
            this.SuspendLayout();
            //
            // progressBarReading
            //
            resources.ApplyResources(this.progressBarReading, "progressBarReading");
            this.progressBarReading.Name = "progressBarReading";
            //
            // buttonCancel
            //
            resources.ApplyResources(this.buttonCancel, "buttonCancel");
            this.buttonCancel.Name = "buttonCancel";
            this.buttonCancel.UseVisualStyleBackColor = true;
            this.buttonCancel.Click += new System.EventHandler(this.buttonCancel_Click);
            //
            // label1
            //
            resources.ApplyResources(this.label1, "label1");
            this.label1.Name = "label1";
            //
            // progressBarWriting
            //
            resources.ApplyResources(this.progressBarWriting, "progressBarWriting");
            this.progressBarWriting.Name = "progressBarWriting";
            //
            // labelWriting
            //
            resources.ApplyResources(this.labelWriting, "labelWriting");
            this.labelWriting.Name = "labelWriting";
            //
            // labelCases
            //
            resources.ApplyResources(this.labelCases, "labelCases");
            this.labelCases.Name = "labelCases";
            //
            // panelWriting
            //
            resources.ApplyResources(this.panelWriting, "panelWriting");
            this.panelWriting.Controls.Add(this.progressBarWriting);
            this.panelWriting.Controls.Add(this.labelCases);
            this.panelWriting.Controls.Add(this.labelWriting);
            this.panelWriting.Name = "panelWriting";
            //
            // ProgressForm
            //
            resources.ApplyResources(this, "$this");
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.panelWriting);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.buttonCancel);
            this.Controls.Add(this.progressBarReading);
            this.Name = "ProgressForm";
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.ProgressForm_FormClosing);
            this.Load += new System.EventHandler(this.ProgressForm_Load);
            this.panelWriting.ResumeLayout(false);
            this.panelWriting.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ProgressBar progressBarReading;
        private System.Windows.Forms.Button buttonCancel;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ProgressBar progressBarWriting;
        private System.Windows.Forms.Label labelWriting;
        private System.Windows.Forms.Label labelCases;
        private System.Windows.Forms.Panel panelWriting;
    }
}