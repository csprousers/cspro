namespace CSDeploy
{
    partial class FileTreeControl
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
            this.components = new System.ComponentModel.Container();
            this.treeViewFiles = new System.Windows.Forms.TreeView();
            this.fileIconsImageList = new System.Windows.Forms.ImageList(this.components);
            this.labelEmptyTreeMessage = new System.Windows.Forms.Label();
            this.buttonAddFolder = new System.Windows.Forms.Button();
            this.buttonAddFiles = new System.Windows.Forms.Button();
            this.fileContextMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.removeFileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.openContainingFolderToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.onlyOnFirstInstallToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.folderContextMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.removeFolderToolStripMenuItem_Click = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.addDataEntryApplicationsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.addAllFilesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.fileContextMenuStrip.SuspendLayout();
            this.folderContextMenuStrip.SuspendLayout();
            this.SuspendLayout();
            // 
            // treeViewFiles
            // 
            this.treeViewFiles.AllowDrop = true;
            this.treeViewFiles.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.treeViewFiles.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.treeViewFiles.ImageIndex = 0;
            this.treeViewFiles.ImageList = this.fileIconsImageList;
            this.treeViewFiles.Location = new System.Drawing.Point(4, 0);
            this.treeViewFiles.Margin = new System.Windows.Forms.Padding(4);
            this.treeViewFiles.Name = "treeViewFiles";
            this.treeViewFiles.SelectedImageIndex = 0;
            this.treeViewFiles.Size = new System.Drawing.Size(281, 141);
            this.treeViewFiles.TabIndex = 7;
            this.treeViewFiles.NodeMouseClick += new System.Windows.Forms.TreeNodeMouseClickEventHandler(this.treeViewFiles_NodeMouseClick);
            this.treeViewFiles.DragDrop += new System.Windows.Forms.DragEventHandler(this.dragDrop);
            this.treeViewFiles.DragEnter += new System.Windows.Forms.DragEventHandler(this.dragEnter);
            this.treeViewFiles.KeyDown += new System.Windows.Forms.KeyEventHandler(this.treeViewFiles_KeyDown);
            // 
            // fileIconsImageList
            // 
            this.fileIconsImageList.ColorDepth = System.Windows.Forms.ColorDepth.Depth32Bit;
            this.fileIconsImageList.ImageSize = new System.Drawing.Size(16, 16);
            this.fileIconsImageList.TransparentColor = System.Drawing.Color.Transparent;
            // 
            // labelEmptyTreeMessage
            // 
            this.labelEmptyTreeMessage.AllowDrop = true;
            this.labelEmptyTreeMessage.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.labelEmptyTreeMessage.BackColor = System.Drawing.SystemColors.Window;
            this.labelEmptyTreeMessage.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.labelEmptyTreeMessage.Location = new System.Drawing.Point(0, 0);
            this.labelEmptyTreeMessage.Margin = new System.Windows.Forms.Padding(0);
            this.labelEmptyTreeMessage.Name = "labelEmptyTreeMessage";
            this.labelEmptyTreeMessage.Size = new System.Drawing.Size(285, 141);
            this.labelEmptyTreeMessage.TabIndex = 20;
            this.labelEmptyTreeMessage.Text = "Drag and drop files here to add them to your application deployment package.";
            this.labelEmptyTreeMessage.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.labelEmptyTreeMessage.DragDrop += new System.Windows.Forms.DragEventHandler(this.dragDrop);
            this.labelEmptyTreeMessage.DragEnter += new System.Windows.Forms.DragEventHandler(this.dragEnter);
            // 
            // buttonAddFolder
            // 
            this.buttonAddFolder.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.buttonAddFolder.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.buttonAddFolder.Location = new System.Drawing.Point(293, 39);
            this.buttonAddFolder.Margin = new System.Windows.Forms.Padding(4);
            this.buttonAddFolder.Name = "buttonAddFolder";
            this.buttonAddFolder.Size = new System.Drawing.Size(105, 23);
            this.buttonAddFolder.TabIndex = 22;
            this.buttonAddFolder.Text = "Add folder...";
            this.buttonAddFolder.UseVisualStyleBackColor = true;
            this.buttonAddFolder.Click += new System.EventHandler(this.buttonAddFolder_Click);
            // 
            // buttonAddFiles
            // 
            this.buttonAddFiles.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.buttonAddFiles.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.buttonAddFiles.Location = new System.Drawing.Point(293, 0);
            this.buttonAddFiles.Margin = new System.Windows.Forms.Padding(4);
            this.buttonAddFiles.Name = "buttonAddFiles";
            this.buttonAddFiles.Size = new System.Drawing.Size(105, 23);
            this.buttonAddFiles.TabIndex = 21;
            this.buttonAddFiles.Text = "Add files...";
            this.buttonAddFiles.UseVisualStyleBackColor = true;
            this.buttonAddFiles.Click += new System.EventHandler(this.buttonAddFiles_Click);
            // 
            // fileContextMenuStrip
            // 
            this.fileContextMenuStrip.ImageScalingSize = new System.Drawing.Size(20, 20);
            this.fileContextMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.removeFileToolStripMenuItem,
            this.toolStripSeparator2,
            this.openContainingFolderToolStripMenuItem,
            this.onlyOnFirstInstallToolStripMenuItem});
            this.fileContextMenuStrip.Name = "contextMenuStrip";
            this.fileContextMenuStrip.Size = new System.Drawing.Size(198, 76);
            // 
            // removeFileToolStripMenuItem
            // 
            this.removeFileToolStripMenuItem.Name = "removeFileToolStripMenuItem";
            this.removeFileToolStripMenuItem.Size = new System.Drawing.Size(197, 22);
            this.removeFileToolStripMenuItem.Text = "Remove";
            this.removeFileToolStripMenuItem.Click += new System.EventHandler(this.removeToolStripMenuItem_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(194, 6);
            // 
            // openContainingFolderToolStripMenuItem
            // 
            this.openContainingFolderToolStripMenuItem.Name = "openContainingFolderToolStripMenuItem";
            this.openContainingFolderToolStripMenuItem.Size = new System.Drawing.Size(197, 22);
            this.openContainingFolderToolStripMenuItem.Text = "Open containing folder";
            this.openContainingFolderToolStripMenuItem.Click += new System.EventHandler(this.openContainingFolderToolStripMenuItem_Click);
            // 
            // onlyOnFirstInstallToolStripMenuItem
            // 
            this.onlyOnFirstInstallToolStripMenuItem.CheckOnClick = true;
            this.onlyOnFirstInstallToolStripMenuItem.Name = "onlyOnFirstInstallToolStripMenuItem";
            this.onlyOnFirstInstallToolStripMenuItem.Size = new System.Drawing.Size(197, 22);
            this.onlyOnFirstInstallToolStripMenuItem.Text = "Only on first install";
            this.onlyOnFirstInstallToolStripMenuItem.Click += new System.EventHandler(this.onlyOnFirstInstallToolStripMenuItem_Click);
            // 
            // folderContextMenuStrip
            // 
            this.folderContextMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.removeFolderToolStripMenuItem_Click,
            this.toolStripSeparator1,
            this.addDataEntryApplicationsToolStripMenuItem,
            this.addAllFilesToolStripMenuItem});
            this.folderContextMenuStrip.Name = "folderContextMenuStrip";
            this.folderContextMenuStrip.Size = new System.Drawing.Size(220, 98);
            // 
            // removeFolderToolStripMenuItem_Click
            // 
            this.removeFolderToolStripMenuItem_Click.Name = "removeFolderToolStripMenuItem_Click";
            this.removeFolderToolStripMenuItem_Click.Size = new System.Drawing.Size(219, 22);
            this.removeFolderToolStripMenuItem_Click.Text = "Remove";
            this.removeFolderToolStripMenuItem_Click.Click += new System.EventHandler(this.removeToolStripMenuItem_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(216, 6);
            // 
            // addDataEntryApplicationsToolStripMenuItem
            // 
            this.addDataEntryApplicationsToolStripMenuItem.Name = "addDataEntryApplicationsToolStripMenuItem";
            this.addDataEntryApplicationsToolStripMenuItem.Size = new System.Drawing.Size(219, 22);
            this.addDataEntryApplicationsToolStripMenuItem.Text = "Add data entry applications";
            this.addDataEntryApplicationsToolStripMenuItem.Click += new System.EventHandler(this.addFilesToolStripMenuItem_Click);
            // 
            // addAllFilesToolStripMenuItem
            // 
            this.addAllFilesToolStripMenuItem.Name = "addAllFilesToolStripMenuItem";
            this.addAllFilesToolStripMenuItem.Size = new System.Drawing.Size(219, 22);
            this.addAllFilesToolStripMenuItem.Text = "Add all files";
            this.addAllFilesToolStripMenuItem.Click += new System.EventHandler(this.addFilesToolStripMenuItem_Click);
            // 
            // FileTreeControl
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Inherit;
            this.BackColor = System.Drawing.SystemColors.Control;
            this.Controls.Add(this.buttonAddFolder);
            this.Controls.Add(this.buttonAddFiles);
            this.Controls.Add(this.labelEmptyTreeMessage);
            this.Controls.Add(this.treeViewFiles);
            this.Margin = new System.Windows.Forms.Padding(4);
            this.Name = "FileTreeControl";
            this.Size = new System.Drawing.Size(400, 145);
            this.DragEnter += new System.Windows.Forms.DragEventHandler(this.dragEnter);
            this.fileContextMenuStrip.ResumeLayout(false);
            this.folderContextMenuStrip.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TreeView treeViewFiles;
        private System.Windows.Forms.Label labelEmptyTreeMessage;
        private System.Windows.Forms.Button buttonAddFolder;
        private System.Windows.Forms.Button buttonAddFiles;
        private System.Windows.Forms.ImageList fileIconsImageList;
        private System.Windows.Forms.ContextMenuStrip fileContextMenuStrip;
        private System.Windows.Forms.ToolStripMenuItem removeFileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem openContainingFolderToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem onlyOnFirstInstallToolStripMenuItem;
        private System.Windows.Forms.ContextMenuStrip folderContextMenuStrip;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripMenuItem removeFolderToolStripMenuItem_Click;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripMenuItem addDataEntryApplicationsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem addAllFilesToolStripMenuItem;
    }
}
