namespace Excel2CSPro
{
    partial class MainForm
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
            this.menuStrip = new System.Windows.Forms.MenuStrip();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.menuItemNew = new System.Windows.Forms.ToolStripMenuItem();
            this.menuItemOpen = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.menuItemSave = new System.Windows.Forms.ToolStripMenuItem();
            this.menuItemSaveAs = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.menuItemExit = new System.Windows.Forms.ToolStripMenuItem();
            this.helpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.menuItemHelp = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            this.menuItemAbout = new System.Windows.Forms.ToolStripMenuItem();
            this.tabControlMain = new System.Windows.Forms.TabControl();
            this.tabPageExcel2CSPro = new System.Windows.Forms.TabPage();
            this.excel2CSProItemControl = new Excel2CSPro.Excel2CSProItemControl();
            this.tabPageCreateDictionary = new System.Windows.Forms.TabPage();
            this.createDictionaryControl = new Excel2CSPro.CreateDictionaryControl();
            this.menuStrip.SuspendLayout();
            this.tabControlMain.SuspendLayout();
            this.tabPageExcel2CSPro.SuspendLayout();
            this.tabPageCreateDictionary.SuspendLayout();
            this.SuspendLayout();
            //
            // menuStrip
            //
            this.menuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.helpToolStripMenuItem});
            resources.ApplyResources(this.menuStrip, "menuStrip");
            this.menuStrip.Name = "menuStrip";
            //
            // fileToolStripMenuItem
            //
            this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.menuItemNew,
            this.menuItemOpen,
            this.toolStripSeparator1,
            this.menuItemSave,
            this.menuItemSaveAs,
            this.toolStripSeparator2,
            this.menuItemExit});
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            resources.ApplyResources(this.fileToolStripMenuItem, "fileToolStripMenuItem");
            //
            // menuItemNew
            //
            this.menuItemNew.Name = "menuItemNew";
            resources.ApplyResources(this.menuItemNew, "menuItemNew");
            this.menuItemNew.Click += new System.EventHandler(this.menuItemNew_Click);
            //
            // menuItemOpen
            //
            this.menuItemOpen.Name = "menuItemOpen";
            resources.ApplyResources(this.menuItemOpen, "menuItemOpen");
            this.menuItemOpen.Click += new System.EventHandler(this.menuItemOpen_Click);
            //
            // toolStripSeparator1
            //
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            resources.ApplyResources(this.toolStripSeparator1, "toolStripSeparator1");
            //
            // menuItemSave
            //
            this.menuItemSave.Name = "menuItemSave";
            resources.ApplyResources(this.menuItemSave, "menuItemSave");
            this.menuItemSave.Click += new System.EventHandler(this.menuItemSave_Click);
            //
            // menuItemSaveAs
            //
            this.menuItemSaveAs.Name = "menuItemSaveAs";
            resources.ApplyResources(this.menuItemSaveAs, "menuItemSaveAs");
            this.menuItemSaveAs.Click += new System.EventHandler(this.menuItemSaveAs_Click);
            //
            // toolStripSeparator2
            //
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            resources.ApplyResources(this.toolStripSeparator2, "toolStripSeparator2");
            //
            // menuItemExit
            //
            this.menuItemExit.Name = "menuItemExit";
            resources.ApplyResources(this.menuItemExit, "menuItemExit");
            this.menuItemExit.Click += new System.EventHandler(this.menuItemExit_Click);
            //
            // helpToolStripMenuItem
            //
            this.helpToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.menuItemHelp,
            this.toolStripSeparator3,
            this.menuItemAbout});
            this.helpToolStripMenuItem.Name = "helpToolStripMenuItem";
            resources.ApplyResources(this.helpToolStripMenuItem, "helpToolStripMenuItem");
            //
            // menuItemHelp
            //
            this.menuItemHelp.Name = "menuItemHelp";
            resources.ApplyResources(this.menuItemHelp, "menuItemHelp");
            this.menuItemHelp.Click += new System.EventHandler(this.menuItemHelp_Click);
            //
            // toolStripSeparator3
            //
            this.toolStripSeparator3.Name = "toolStripSeparator3";
            resources.ApplyResources(this.toolStripSeparator3, "toolStripSeparator3");
            //
            // menuItemAbout
            //
            this.menuItemAbout.Name = "menuItemAbout";
            resources.ApplyResources(this.menuItemAbout, "menuItemAbout");
            this.menuItemAbout.Click += new System.EventHandler(this.menuItemAbout_Click);
            //
            // tabControlMain
            //
            resources.ApplyResources(this.tabControlMain, "tabControlMain");
            this.tabControlMain.Controls.Add(this.tabPageExcel2CSPro);
            this.tabControlMain.Controls.Add(this.tabPageCreateDictionary);
            this.tabControlMain.Name = "tabControlMain";
            this.tabControlMain.SelectedIndex = 0;
            this.tabControlMain.SelectedIndexChanged += new System.EventHandler(this.tabControlMain_SelectedIndexChanged);
            //
            // tabPageExcel2CSPro
            //
            this.tabPageExcel2CSPro.Controls.Add(this.excel2CSProItemControl);
            resources.ApplyResources(this.tabPageExcel2CSPro, "tabPageExcel2CSPro");
            this.tabPageExcel2CSPro.Name = "tabPageExcel2CSPro";
            this.tabPageExcel2CSPro.UseVisualStyleBackColor = true;
            //
            // excel2CSProItemControl
            //
            resources.ApplyResources(this.excel2CSProItemControl, "excel2CSProItemControl");
            this.excel2CSProItemControl.Name = "excel2CSProItemControl";
            //
            // tabPageCreateDictionary
            //
            this.tabPageCreateDictionary.Controls.Add(this.createDictionaryControl);
            resources.ApplyResources(this.tabPageCreateDictionary, "tabPageCreateDictionary");
            this.tabPageCreateDictionary.Name = "tabPageCreateDictionary";
            this.tabPageCreateDictionary.UseVisualStyleBackColor = true;
            //
            // createDictionaryControl
            //
            resources.ApplyResources(this.createDictionaryControl, "createDictionaryControl");
            this.createDictionaryControl.Name = "createDictionaryControl";
            //
            // MainForm
            //
            resources.ApplyResources(this, "$this");
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.tabControlMain);
            this.Controls.Add(this.menuStrip);
            this.MainMenuStrip = this.menuStrip;
            this.Name = "MainForm";
            this.Shown += new System.EventHandler(this.MainForm_Shown);
            this.menuStrip.ResumeLayout(false);
            this.menuStrip.PerformLayout();
            this.tabControlMain.ResumeLayout(false);
            this.tabPageExcel2CSPro.ResumeLayout(false);
            this.tabPageCreateDictionary.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.MenuStrip menuStrip;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem menuItemNew;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripMenuItem menuItemSave;
        private System.Windows.Forms.ToolStripMenuItem menuItemSaveAs;
        private System.Windows.Forms.ToolStripMenuItem menuItemOpen;
        private System.Windows.Forms.ToolStripMenuItem helpToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem menuItemAbout;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripMenuItem menuItemExit;
        private System.Windows.Forms.TabControl tabControlMain;
        private System.Windows.Forms.TabPage tabPageExcel2CSPro;
        private System.Windows.Forms.TabPage tabPageCreateDictionary;
        private CreateDictionaryControl createDictionaryControl;
        private Excel2CSProItemControl excel2CSProItemControl;
        private System.Windows.Forms.ToolStripMenuItem menuItemHelp;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator3;
    }
}

