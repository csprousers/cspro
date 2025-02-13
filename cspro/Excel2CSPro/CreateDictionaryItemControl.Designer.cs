namespace Excel2CSPro
{
    partial class CreateDictionaryItemControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(CreateDictionaryItemControl));
            this.checkBoxIncludeItem = new System.Windows.Forms.CheckBox();
            this.textBoxItemName = new System.Windows.Forms.TextBox();
            this.panelItem = new System.Windows.Forms.Panel();
            this.checkBoxItemCreateValueSet = new System.Windows.Forms.CheckBox();
            this.textBoxItemAfterDecLength = new System.Windows.Forms.TextBox();
            this.textBoxItemBeforeDecLength = new System.Windows.Forms.TextBox();
            this.textBoxItemLength = new System.Windows.Forms.TextBox();
            this.checkBoxItemNumeric = new System.Windows.Forms.CheckBox();
            this.checkBoxItemID = new System.Windows.Forms.CheckBox();
            this.panelItem.SuspendLayout();
            this.SuspendLayout();
            //
            // checkBoxIncludeItem
            //
            resources.ApplyResources(this.checkBoxIncludeItem, "checkBoxIncludeItem");
            this.checkBoxIncludeItem.Checked = true;
            this.checkBoxIncludeItem.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxIncludeItem.Name = "checkBoxIncludeItem";
            this.checkBoxIncludeItem.UseVisualStyleBackColor = true;
            this.checkBoxIncludeItem.CheckedChanged += new System.EventHandler(this.checkBoxIncludeItem_CheckedChanged);
            //
            // textBoxItemName
            //
            resources.ApplyResources(this.textBoxItemName, "textBoxItemName");
            this.textBoxItemName.Name = "textBoxItemName";
            //
            // panelItem
            //
            resources.ApplyResources(this.panelItem, "panelItem");
            this.panelItem.Controls.Add(this.checkBoxItemCreateValueSet);
            this.panelItem.Controls.Add(this.textBoxItemAfterDecLength);
            this.panelItem.Controls.Add(this.textBoxItemBeforeDecLength);
            this.panelItem.Controls.Add(this.textBoxItemLength);
            this.panelItem.Controls.Add(this.checkBoxItemNumeric);
            this.panelItem.Controls.Add(this.checkBoxItemID);
            this.panelItem.Controls.Add(this.textBoxItemName);
            this.panelItem.Name = "panelItem";
            //
            // checkBoxItemCreateValueSet
            //
            resources.ApplyResources(this.checkBoxItemCreateValueSet, "checkBoxItemCreateValueSet");
            this.checkBoxItemCreateValueSet.Name = "checkBoxItemCreateValueSet";
            this.checkBoxItemCreateValueSet.UseVisualStyleBackColor = true;
            //
            // textBoxItemAfterDecLength
            //
            resources.ApplyResources(this.textBoxItemAfterDecLength, "textBoxItemAfterDecLength");
            this.textBoxItemAfterDecLength.Name = "textBoxItemAfterDecLength";
            //
            // textBoxItemBeforeDecLength
            //
            resources.ApplyResources(this.textBoxItemBeforeDecLength, "textBoxItemBeforeDecLength");
            this.textBoxItemBeforeDecLength.Name = "textBoxItemBeforeDecLength";
            //
            // textBoxItemLength
            //
            resources.ApplyResources(this.textBoxItemLength, "textBoxItemLength");
            this.textBoxItemLength.Name = "textBoxItemLength";
            //
            // checkBoxItemNumeric
            //
            resources.ApplyResources(this.checkBoxItemNumeric, "checkBoxItemNumeric");
            this.checkBoxItemNumeric.Name = "checkBoxItemNumeric";
            this.checkBoxItemNumeric.UseVisualStyleBackColor = true;
            this.checkBoxItemNumeric.CheckedChanged += new System.EventHandler(this.checkBoxItemNumeric_CheckedChanged);
            //
            // checkBoxItemID
            //
            resources.ApplyResources(this.checkBoxItemID, "checkBoxItemID");
            this.checkBoxItemID.Name = "checkBoxItemID";
            this.checkBoxItemID.UseVisualStyleBackColor = true;
            //
            // CreateDictionaryItemControl
            //
            resources.ApplyResources(this, "$this");
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.panelItem);
            this.Controls.Add(this.checkBoxIncludeItem);
            this.Name = "CreateDictionaryItemControl";
            this.panelItem.ResumeLayout(false);
            this.panelItem.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.CheckBox checkBoxIncludeItem;
        private System.Windows.Forms.TextBox textBoxItemName;
        private System.Windows.Forms.Panel panelItem;
        private System.Windows.Forms.TextBox textBoxItemAfterDecLength;
        private System.Windows.Forms.TextBox textBoxItemBeforeDecLength;
        private System.Windows.Forms.TextBox textBoxItemLength;
        private System.Windows.Forms.CheckBox checkBoxItemNumeric;
        private System.Windows.Forms.CheckBox checkBoxItemID;
        private System.Windows.Forms.CheckBox checkBoxItemCreateValueSet;
    }
}
