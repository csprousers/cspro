namespace Stygitan
{
    partial class MainForm
    {
        /// <summary>
        ///  Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        ///  Clean up any resources being used.
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
        ///  Required method for Designer support - do not modify
        ///  the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
            label1 = new Label();
            label4 = new Label();
            labelGitDirectory = new Label();
            labelLocalBranch = new Label();
            label2 = new Label();
            labelRemoteBranch = new Label();
            listViewCommits = new ListView();
            columnHeaderCommitter = new ColumnHeader();
            columnHeaderCommitDate = new ColumnHeader();
            columnHeaderCommitMessage = new ColumnHeader();
            label3 = new Label();
            labelModifiedFiles = new Label();
            listViewModifiedFiles = new ListView();
            columnHeaderPath = new ColumnHeader();
            columnHeaderStatus = new ColumnHeader();
            buttonStandardizeFiles = new Button();
            SuspendLayout();
            //
            // label1
            //
            label1.AutoSize = true;
            label1.Location = new Point(12, 43);
            label1.Name = "label1";
            label1.Size = new Size(78, 15);
            label1.TabIndex = 1;
            label1.Text = "Local Branch:";
            //
            // label4
            //
            label4.AutoSize = true;
            label4.Location = new Point(12, 15);
            label4.Name = "label4";
            label4.Size = new Size(76, 15);
            label4.TabIndex = 4;
            label4.Text = "Git Directory:";
            //
            // labelGitDirectory
            //
            labelGitDirectory.AutoSize = true;
            labelGitDirectory.Font = new Font("Segoe UI", 9F, FontStyle.Bold);
            labelGitDirectory.Location = new Point(125, 15);
            labelGitDirectory.Name = "labelGitDirectory";
            labelGitDirectory.Size = new Size(16, 15);
            labelGitDirectory.TabIndex = 5;
            labelGitDirectory.Text = "...";
            //
            // labelLocalBranch
            //
            labelLocalBranch.AutoSize = true;
            labelLocalBranch.Font = new Font("Segoe UI", 9F, FontStyle.Bold);
            labelLocalBranch.Location = new Point(125, 43);
            labelLocalBranch.Name = "labelLocalBranch";
            labelLocalBranch.Size = new Size(16, 15);
            labelLocalBranch.TabIndex = 6;
            labelLocalBranch.Text = "...";
            //
            // label2
            //
            label2.AutoSize = true;
            label2.Location = new Point(12, 71);
            label2.Name = "label2";
            label2.Size = new Size(91, 15);
            label2.TabIndex = 7;
            label2.Text = "Remote Branch:";
            //
            // labelRemoteBranch
            //
            labelRemoteBranch.AutoSize = true;
            labelRemoteBranch.Font = new Font("Segoe UI", 9F, FontStyle.Bold);
            labelRemoteBranch.Location = new Point(125, 71);
            labelRemoteBranch.Name = "labelRemoteBranch";
            labelRemoteBranch.Size = new Size(16, 15);
            labelRemoteBranch.TabIndex = 8;
            labelRemoteBranch.Text = "...";
            //
            // listViewCommits
            //
            listViewCommits.Anchor =   AnchorStyles.Top  |  AnchorStyles.Bottom   |  AnchorStyles.Left ;
            listViewCommits.Columns.AddRange(new ColumnHeader[] { columnHeaderCommitter, columnHeaderCommitDate, columnHeaderCommitMessage });
            listViewCommits.Location = new Point(12, 126);
            listViewCommits.Name = "listViewCommits";
            listViewCommits.Size = new Size(525, 737);
            listViewCommits.TabIndex = 1;
            listViewCommits.UseCompatibleStateImageBehavior = false;
            listViewCommits.View = View.Details;
            //
            // columnHeaderCommitter
            //
            columnHeaderCommitter.Text = "Committer";
            //
            // columnHeaderCommitDate
            //
            columnHeaderCommitDate.Text = "Date";
            //
            // columnHeaderCommitMessage
            //
            columnHeaderCommitMessage.Text = "Message";
            //
            // label3
            //
            label3.AutoSize = true;
            label3.Location = new Point(12, 108);
            label3.Name = "label3";
            label3.Size = new Size(118, 15);
            label3.TabIndex = 10;
            label3.Text = "Temporary Commits:";
            //
            // labelModifiedFiles
            //
            labelModifiedFiles.AutoSize = true;
            labelModifiedFiles.Location = new Point(553, 15);
            labelModifiedFiles.Name = "labelModifiedFiles";
            labelModifiedFiles.Size = new Size(146, 15);
            labelModifiedFiles.TabIndex = 12;
            labelModifiedFiles.Text = "Modified and Staged Files:";
            //
            // listViewModifiedFiles
            //
            listViewModifiedFiles.Anchor =    AnchorStyles.Top  |  AnchorStyles.Bottom   |  AnchorStyles.Left   |  AnchorStyles.Right ;
            listViewModifiedFiles.Columns.AddRange(new ColumnHeader[] { columnHeaderPath, columnHeaderStatus });
            listViewModifiedFiles.Location = new Point(553, 43);
            listViewModifiedFiles.Name = "listViewModifiedFiles";
            listViewModifiedFiles.Size = new Size(853, 820);
            listViewModifiedFiles.TabIndex = 2;
            listViewModifiedFiles.UseCompatibleStateImageBehavior = false;
            listViewModifiedFiles.View = View.Details;
            listViewModifiedFiles.DoubleClick +=  listViewModifiedFiles_DoubleClick ;
            listViewModifiedFiles.MouseUp +=  listViewModifiedFiles_MouseUp ;
            //
            // columnHeaderPath
            //
            columnHeaderPath.Text = "Path";
            columnHeaderPath.Width = 200;
            //
            // columnHeaderStatus
            //
            columnHeaderStatus.Text = "Skip Standardizing?";
            //
            // buttonStandardizeFiles
            //
            buttonStandardizeFiles.Anchor =  AnchorStyles.Top  |  AnchorStyles.Right ;
            buttonStandardizeFiles.Enabled = false;
            buttonStandardizeFiles.Location = new Point(1279, 15);
            buttonStandardizeFiles.Name = "buttonStandardizeFiles";
            buttonStandardizeFiles.Size = new Size(127, 23);
            buttonStandardizeFiles.TabIndex = 0;
            buttonStandardizeFiles.Text = "Standardize Files";
            buttonStandardizeFiles.UseVisualStyleBackColor = true;
            buttonStandardizeFiles.Click +=  buttonStandardizeFiles_Click ;
            //
            // MainForm
            //
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(1418, 875);
            Controls.Add(buttonStandardizeFiles);
            Controls.Add(labelModifiedFiles);
            Controls.Add(listViewModifiedFiles);
            Controls.Add(label3);
            Controls.Add(listViewCommits);
            Controls.Add(labelRemoteBranch);
            Controls.Add(label2);
            Controls.Add(labelLocalBranch);
            Controls.Add(labelGitDirectory);
            Controls.Add(label4);
            Controls.Add(label1);
            Icon = (Icon)resources.GetObject("$this.Icon");
            Name = "MainForm";
            StartPosition = FormStartPosition.CenterScreen;
            Text = "Stygitan";
            Load +=  Form1_Load ;
            ResumeLayout(false);
            PerformLayout();
        }

        #endregion
        private Label label1;
        private Label label4;
        private Label labelGitDirectory;
        private Label labelLocalBranch;
        private Label label2;
        private Label labelRemoteBranch;
        private ListView listViewCommits;
        private ColumnHeader columnHeaderCommitDate;
        private ColumnHeader columnHeaderCommitMessage;
        private ColumnHeader columnHeaderCommitter;
        private Label label3;
        private Label labelModifiedFiles;
        private ListView listViewModifiedFiles;
        private ColumnHeader columnHeaderPath;
        private ColumnHeader columnHeaderStatus;
        private Button buttonStandardizeFiles;
    }
}
