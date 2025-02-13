namespace CSDeploy
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
            this.menuStrip = new System.Windows.Forms.MenuStrip();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.openToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.saveToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.saveAsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.recentFilesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.exitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.helpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.helpTopicsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.aboutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.label1 = new System.Windows.Forms.Label();
            this.textBoxPackageName = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.textBoxDescription = new System.Windows.Forms.TextBox();
            this.textBoxOutputFolder = new System.Windows.Forms.TextBox();
            this.buttonBrowseOutputDirectory = new System.Windows.Forms.Button();
            this.buttonDeploy = new System.Windows.Forms.Button();
            this.radioButtonDeployCSWeb = new System.Windows.Forms.RadioButton();
            this.radioButtonDeployDropbox = new System.Windows.Forms.RadioButton();
            this.radioButtonDeployFTP = new System.Windows.Forms.RadioButton();
            this.radioButtonDeployFile = new System.Windows.Forms.RadioButton();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.buttonBarcode = new System.Windows.Forms.Button();
            this.textBoxFtpServerUrl = new System.Windows.Forms.TextBox();
            this.buttonBrowseZipFile = new System.Windows.Forms.Button();
            this.textBoxZipFile = new System.Windows.Forms.TextBox();
            this.radioButtonDeployFolder = new System.Windows.Forms.RadioButton();
            this.textBoxCSWebURL = new System.Windows.Forms.TextBox();
            this.errorProvider = new System.Windows.Forms.ErrorProvider(this.components);
            this.groupBoxFiles = new System.Windows.Forms.GroupBox();
            this.fileTreeControl = new CSDeploy.FileTreeControl();
            this.groupBoxDictionaries = new System.Windows.Forms.GroupBox();
            this.checkedListBoxDictionaries = new System.Windows.Forms.CheckedListBox();
            this.menuStrip.SuspendLayout();
            this.groupBox1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.errorProvider)).BeginInit();
            this.groupBoxFiles.SuspendLayout();
            this.groupBoxDictionaries.SuspendLayout();
            this.SuspendLayout();
            // 
            // menuStrip
            // 
            this.menuStrip.ImageScalingSize = new System.Drawing.Size(20, 20);
            this.menuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.helpToolStripMenuItem});
            resources.ApplyResources(this.menuStrip, "menuStrip");
            this.menuStrip.Name = "menuStrip";
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.openToolStripMenuItem,
            this.saveToolStripMenuItem,
            this.saveAsToolStripMenuItem,
            this.recentFilesToolStripMenuItem,
            this.toolStripSeparator2,
            this.exitToolStripMenuItem});
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            resources.ApplyResources(this.fileToolStripMenuItem, "fileToolStripMenuItem");
            // 
            // openToolStripMenuItem
            // 
            this.openToolStripMenuItem.Name = "openToolStripMenuItem";
            resources.ApplyResources(this.openToolStripMenuItem, "openToolStripMenuItem");
            this.openToolStripMenuItem.Click += new System.EventHandler(this.openToolStripMenuItem_Click);
            // 
            // saveToolStripMenuItem
            // 
            this.saveToolStripMenuItem.Name = "saveToolStripMenuItem";
            resources.ApplyResources(this.saveToolStripMenuItem, "saveToolStripMenuItem");
            this.saveToolStripMenuItem.Click += new System.EventHandler(this.saveToolStripMenuItem_Click);
            // 
            // saveAsToolStripMenuItem
            // 
            this.saveAsToolStripMenuItem.Name = "saveAsToolStripMenuItem";
            resources.ApplyResources(this.saveAsToolStripMenuItem, "saveAsToolStripMenuItem");
            this.saveAsToolStripMenuItem.Click += new System.EventHandler(this.saveAsToolStripMenuItem_Click);
            // 
            // recentFilesToolStripMenuItem
            // 
            this.recentFilesToolStripMenuItem.Name = "recentFilesToolStripMenuItem";
            resources.ApplyResources(this.recentFilesToolStripMenuItem, "recentFilesToolStripMenuItem");
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            resources.ApplyResources(this.toolStripSeparator2, "toolStripSeparator2");
            // 
            // exitToolStripMenuItem
            // 
            this.exitToolStripMenuItem.Name = "exitToolStripMenuItem";
            resources.ApplyResources(this.exitToolStripMenuItem, "exitToolStripMenuItem");
            this.exitToolStripMenuItem.Click += new System.EventHandler(this.exitToolStripMenuItem_Click);
            // 
            // helpToolStripMenuItem
            // 
            this.helpToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.helpTopicsToolStripMenuItem,
            this.toolStripSeparator1,
            this.aboutToolStripMenuItem});
            this.helpToolStripMenuItem.Name = "helpToolStripMenuItem";
            resources.ApplyResources(this.helpToolStripMenuItem, "helpToolStripMenuItem");
            // 
            // helpTopicsToolStripMenuItem
            // 
            this.helpTopicsToolStripMenuItem.Name = "helpTopicsToolStripMenuItem";
            resources.ApplyResources(this.helpTopicsToolStripMenuItem, "helpTopicsToolStripMenuItem");
            this.helpTopicsToolStripMenuItem.Click += new System.EventHandler(this.helpTopicsToolStripMenuItem_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            resources.ApplyResources(this.toolStripSeparator1, "toolStripSeparator1");
            // 
            // aboutToolStripMenuItem
            // 
            this.aboutToolStripMenuItem.Name = "aboutToolStripMenuItem";
            resources.ApplyResources(this.aboutToolStripMenuItem, "aboutToolStripMenuItem");
            this.aboutToolStripMenuItem.Click += new System.EventHandler(this.aboutToolStripMenuItem_Click);
            // 
            // label1
            // 
            resources.ApplyResources(this.label1, "label1");
            this.label1.Name = "label1";
            // 
            // textBoxPackageName
            // 
            resources.ApplyResources(this.textBoxPackageName, "textBoxPackageName");
            this.textBoxPackageName.Name = "textBoxPackageName";
            this.textBoxPackageName.TextChanged += new System.EventHandler(this.textBoxPackageName_TextChanged);
            // 
            // label2
            // 
            resources.ApplyResources(this.label2, "label2");
            this.label2.Name = "label2";
            // 
            // textBoxDescription
            // 
            resources.ApplyResources(this.textBoxDescription, "textBoxDescription");
            this.textBoxDescription.Name = "textBoxDescription";
            // 
            // textBoxOutputFolder
            // 
            resources.ApplyResources(this.textBoxOutputFolder, "textBoxOutputFolder");
            this.textBoxOutputFolder.Name = "textBoxOutputFolder";
            this.textBoxOutputFolder.TextChanged += new System.EventHandler(this.textBoxOutputFile_TextChanged);
            // 
            // buttonBrowseOutputDirectory
            // 
            resources.ApplyResources(this.buttonBrowseOutputDirectory, "buttonBrowseOutputDirectory");
            this.buttonBrowseOutputDirectory.Name = "buttonBrowseOutputDirectory";
            this.buttonBrowseOutputDirectory.UseVisualStyleBackColor = true;
            this.buttonBrowseOutputDirectory.Click += new System.EventHandler(this.buttonBrowseOutputDirectory_Click);
            // 
            // buttonDeploy
            // 
            resources.ApplyResources(this.buttonDeploy, "buttonDeploy");
            this.buttonDeploy.Name = "buttonDeploy";
            this.buttonDeploy.UseVisualStyleBackColor = true;
            this.buttonDeploy.Click += new System.EventHandler(this.buttonDeployPackage_ClickAsync);
            // 
            // radioButtonDeployCSWeb
            // 
            resources.ApplyResources(this.radioButtonDeployCSWeb, "radioButtonDeployCSWeb");
            this.radioButtonDeployCSWeb.Name = "radioButtonDeployCSWeb";
            this.radioButtonDeployCSWeb.TabStop = true;
            this.radioButtonDeployCSWeb.UseVisualStyleBackColor = true;
            this.radioButtonDeployCSWeb.CheckedChanged += new System.EventHandler(this.radioButtonDeployType_CheckedChanged);
            // 
            // radioButtonDeployDropbox
            // 
            resources.ApplyResources(this.radioButtonDeployDropbox, "radioButtonDeployDropbox");
            this.radioButtonDeployDropbox.Name = "radioButtonDeployDropbox";
            this.radioButtonDeployDropbox.TabStop = true;
            this.radioButtonDeployDropbox.UseVisualStyleBackColor = true;
            this.radioButtonDeployDropbox.CheckedChanged += new System.EventHandler(this.radioButtonDeployType_CheckedChanged);
            // 
            // radioButtonDeployFTP
            // 
            resources.ApplyResources(this.radioButtonDeployFTP, "radioButtonDeployFTP");
            this.radioButtonDeployFTP.Name = "radioButtonDeployFTP";
            this.radioButtonDeployFTP.TabStop = true;
            this.radioButtonDeployFTP.UseVisualStyleBackColor = true;
            this.radioButtonDeployFTP.CheckedChanged += new System.EventHandler(this.radioButtonDeployType_CheckedChanged);
            // 
            // radioButtonDeployFile
            // 
            resources.ApplyResources(this.radioButtonDeployFile, "radioButtonDeployFile");
            this.radioButtonDeployFile.Name = "radioButtonDeployFile";
            this.radioButtonDeployFile.TabStop = true;
            this.radioButtonDeployFile.UseVisualStyleBackColor = true;
            this.radioButtonDeployFile.CheckedChanged += new System.EventHandler(this.radioButtonDeployType_CheckedChanged);
            // 
            // groupBox1
            // 
            resources.ApplyResources(this.groupBox1, "groupBox1");
            this.groupBox1.Controls.Add(this.buttonBarcode);
            this.groupBox1.Controls.Add(this.textBoxFtpServerUrl);
            this.groupBox1.Controls.Add(this.buttonBrowseZipFile);
            this.groupBox1.Controls.Add(this.textBoxZipFile);
            this.groupBox1.Controls.Add(this.radioButtonDeployFolder);
            this.groupBox1.Controls.Add(this.textBoxCSWebURL);
            this.groupBox1.Controls.Add(this.radioButtonDeployCSWeb);
            this.groupBox1.Controls.Add(this.radioButtonDeployFile);
            this.groupBox1.Controls.Add(this.radioButtonDeployDropbox);
            this.groupBox1.Controls.Add(this.buttonBrowseOutputDirectory);
            this.groupBox1.Controls.Add(this.radioButtonDeployFTP);
            this.groupBox1.Controls.Add(this.textBoxOutputFolder);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.TabStop = false;
            // 
            // buttonBarcode
            // 
            this.buttonBarcode.BackgroundImage = global::CSDeploy.Properties.Resources.QRCode;
            resources.ApplyResources(this.buttonBarcode, "buttonBarcode");
            this.buttonBarcode.Name = "buttonBarcode";
            this.buttonBarcode.UseVisualStyleBackColor = true;
            this.buttonBarcode.Click += new System.EventHandler(this.buttonBarcode_Click);
            // 
            // textBoxFtpServerUrl
            // 
            resources.ApplyResources(this.textBoxFtpServerUrl, "textBoxFtpServerUrl");
            this.textBoxFtpServerUrl.Name = "textBoxFtpServerUrl";
            this.textBoxFtpServerUrl.TextChanged += new System.EventHandler(this.textBoxFtpServerUrl_TextChanged);
            // 
            // buttonBrowseZipFile
            // 
            resources.ApplyResources(this.buttonBrowseZipFile, "buttonBrowseZipFile");
            this.buttonBrowseZipFile.Name = "buttonBrowseZipFile";
            this.buttonBrowseZipFile.UseVisualStyleBackColor = true;
            this.buttonBrowseZipFile.Click += new System.EventHandler(this.buttonBrowseZipFile_Click);
            // 
            // textBoxZipFile
            // 
            resources.ApplyResources(this.textBoxZipFile, "textBoxZipFile");
            this.textBoxZipFile.Name = "textBoxZipFile";
            this.textBoxZipFile.TextChanged += new System.EventHandler(this.textBoxZipFile_TextChanged);
            // 
            // radioButtonDeployFolder
            // 
            resources.ApplyResources(this.radioButtonDeployFolder, "radioButtonDeployFolder");
            this.radioButtonDeployFolder.Name = "radioButtonDeployFolder";
            this.radioButtonDeployFolder.TabStop = true;
            this.radioButtonDeployFolder.UseVisualStyleBackColor = true;
            this.radioButtonDeployFolder.CheckedChanged += new System.EventHandler(this.radioButtonDeployType_CheckedChanged);
            // 
            // textBoxCSWebURL
            // 
            resources.ApplyResources(this.textBoxCSWebURL, "textBoxCSWebURL");
            this.textBoxCSWebURL.Name = "textBoxCSWebURL";
            this.textBoxCSWebURL.TextChanged += new System.EventHandler(this.textBoxCSWebURL_TextChanged);
            // 
            // errorProvider
            // 
            this.errorProvider.ContainerControl = this;
            // 
            // groupBoxFiles
            // 
            resources.ApplyResources(this.groupBoxFiles, "groupBoxFiles");
            this.groupBoxFiles.Controls.Add(this.fileTreeControl);
            this.groupBoxFiles.Name = "groupBoxFiles";
            this.groupBoxFiles.TabStop = false;
            // 
            // fileTreeControl
            // 
            this.fileTreeControl.AddFolderFilter = null;
            resources.ApplyResources(this.fileTreeControl, "fileTreeControl");
            this.fileTreeControl.BackColor = System.Drawing.SystemColors.Control;
            this.fileTreeControl.Name = "fileTreeControl";
            // 
            // groupBoxDictionaries
            // 
            resources.ApplyResources(this.groupBoxDictionaries, "groupBoxDictionaries");
            this.groupBoxDictionaries.Controls.Add(this.checkedListBoxDictionaries);
            this.groupBoxDictionaries.Name = "groupBoxDictionaries";
            this.groupBoxDictionaries.TabStop = false;
            // 
            // checkedListBoxDictionaries
            // 
            resources.ApplyResources(this.checkedListBoxDictionaries, "checkedListBoxDictionaries");
            this.checkedListBoxDictionaries.CheckOnClick = true;
            this.checkedListBoxDictionaries.FormattingEnabled = true;
            this.checkedListBoxDictionaries.Name = "checkedListBoxDictionaries";
            // 
            // MainForm
            // 
            resources.ApplyResources(this, "$this");
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.groupBoxDictionaries);
            this.Controls.Add(this.groupBoxFiles);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.buttonDeploy);
            this.Controls.Add(this.textBoxDescription);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.textBoxPackageName);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.menuStrip);
            this.MainMenuStrip = this.menuStrip;
            this.Name = "MainForm";
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.MainForm_FormClosed);
            this.Shown += new System.EventHandler(this.MainForm_Shown);
            this.menuStrip.ResumeLayout(false);
            this.menuStrip.PerformLayout();
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.errorProvider)).EndInit();
            this.groupBoxFiles.ResumeLayout(false);
            this.groupBoxDictionaries.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.MenuStrip menuStrip;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem exitToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem helpToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem helpTopicsToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripMenuItem aboutToolStripMenuItem;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox textBoxPackageName;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox textBoxDescription;
        private System.Windows.Forms.TextBox textBoxOutputFolder;
        private System.Windows.Forms.Button buttonBrowseOutputDirectory;
        private System.Windows.Forms.Button buttonDeploy;
        private System.Windows.Forms.RadioButton radioButtonDeployCSWeb;
        private System.Windows.Forms.RadioButton radioButtonDeployDropbox;
        private System.Windows.Forms.RadioButton radioButtonDeployFTP;
        private System.Windows.Forms.RadioButton radioButtonDeployFile;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.TextBox textBoxCSWebURL;
        private System.Windows.Forms.ToolStripMenuItem openToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem saveToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem saveAsToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ErrorProvider errorProvider;
        private System.Windows.Forms.GroupBox groupBoxFiles;
        private FileTreeControl fileTreeControl;
        private System.Windows.Forms.ToolStripMenuItem recentFilesToolStripMenuItem;
        private System.Windows.Forms.GroupBox groupBoxDictionaries;
        private System.Windows.Forms.CheckedListBox checkedListBoxDictionaries;
        private System.Windows.Forms.RadioButton radioButtonDeployFolder;
        private System.Windows.Forms.TextBox textBoxFtpServerUrl;
        private System.Windows.Forms.Button buttonBrowseZipFile;
        private System.Windows.Forms.TextBox textBoxZipFile;
        private System.Windows.Forms.Button buttonBarcode;
    }
}

