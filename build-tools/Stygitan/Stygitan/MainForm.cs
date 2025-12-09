using System.Diagnostics;
using System.Text;
using LibGit2Sharp;

namespace Stygitan
{
    public partial class MainForm : Form
    {
        private struct ModifiedFile
        {
            public string FilePath;
            public string RepositoryPath;
            public bool Process;
            public bool WriteUtf8Bom;
            public bool RemoveLineFeeds;
        }

        private string _gitDirectory;
        private string? _comparisonBranchName;
        private List<ModifiedFile> _modifiedFiles = new List<ModifiedFile>();

        public MainForm()
        {
            InitializeComponent();

            string[] command_line_arguments = Environment.GetCommandLineArgs();

            _gitDirectory = ( command_line_arguments.Length > 1 ) ? Path.GetFullPath(command_line_arguments[1]) :
                                                                    Path.GetFullPath(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, @"..\..\..\..\..\..\.git"));

            if( command_line_arguments.Length > 2 )
                _comparisonBranchName = command_line_arguments[2];
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            try
            {
                if( !Path.Exists(_gitDirectory) )
                    throw new Exception("Could not find Git directory: " + _gitDirectory);

                labelGitDirectory.Text = _gitDirectory;

                using( Repository repository = new Repository(_gitDirectory) )
                    LoadData(repository);
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
                Close();
            }
        }

        private void LoadData(Repository repository)
        {
            Branch current_branch = repository.Head;
            labelLocalBranch.Text = current_branch.FriendlyName;

            // find the corresponding remote branch
            string remote_branch_name = $"refs/remotes/origin/{current_branch.FriendlyName}";
            Branch remote_branch = repository.Branches[remote_branch_name];

            if( remote_branch == null )
            {
                if( _comparisonBranchName != null )
                {
                    remote_branch = repository.Branches[_comparisonBranchName];
                }

                else
                {
                    const string DevBranchName = "refs/remotes/origin/dev";
                    const string MainBranchName = "refs/remotes/origin/main";

                    _comparisonBranchName = DevBranchName;
                    remote_branch = repository.Branches[_comparisonBranchName];

                    if( remote_branch == null )
                    {
                        _comparisonBranchName = MainBranchName;
                        remote_branch = repository.Branches[_comparisonBranchName];
                    }
                }

                if( remote_branch == null )
                    throw new Exception("Could not find remote branch: " + _comparisonBranchName);
            }

            labelRemoteBranch.Text = remote_branch.FriendlyName;

            // lists commits not in the remote branch
            CommitFilter filter = new CommitFilter
            {
                IncludeReachableFrom = current_branch,
                ExcludeReachableFrom = remote_branch
            };

            Commit most_recent_commit = null;
            Commit oldest_commit = null;

            foreach( Commit commit in repository.Commits.QueryBy(filter) )
            {
                var lvi = new ListViewItem(commit.Committer.When.ToLocalTime().DateTime.ToString());
                lvi.SubItems.Add(commit.Committer.Name);
                lvi.SubItems.Add(commit.Message);
                listViewCommits.Items.Add(lvi);

                if( most_recent_commit == null )
                    most_recent_commit = commit;

                oldest_commit = commit;
            }

            if( most_recent_commit != null )
            {
                // oldest_commit is the first temporary commit, but we want to compare against the last commit on the origin
                oldest_commit = oldest_commit.Parents.FirstOrDefault();

                // get a list of modified files
                foreach( TreeEntryChanges change in repository.Diff.Compare<TreeChanges>(oldest_commit.Tree, most_recent_commit.Tree) )
                {
                    if( change.Status == ChangeKind.Modified || change.Status == ChangeKind.Added )
                        ProcessModifiedFile(repository, change.Path);
                }
            }

            // add any currently staged files
            foreach( StatusEntry index_entry in repository.RetrieveStatus().Staged )
            {
                ProcessModifiedFile(repository, index_entry.FilePath);
            }

            // filter the list by file path and files that will be processed
            _modifiedFiles.Sort((x, y) =>
            {
                if( x.Process != y.Process )
                    return x.Process ? -1 : 1;

                return string.Compare(x.FilePath, y.FilePath, StringComparison.InvariantCultureIgnoreCase);
            });

            int not_processed_count = 0;

            foreach( var modified_file in _modifiedFiles )
            {
                var lvi = new ListViewItem(modified_file.RepositoryPath);

                if( !modified_file.Process )
                {
                    lvi.SubItems.Add("XXXXXX");
                    ++not_processed_count;
                }

                listViewModifiedFiles.Items.Add(lvi);
            }

            labelModifiedFiles.Text = $"Modified and Staged Files ({_modifiedFiles.Count})";

            if( not_processed_count != 0 )
                labelModifiedFiles.Text += $"  -- Files That Will Not Be Processed ({not_processed_count})";

            buttonStandardizeFiles.Enabled = ( _modifiedFiles.Count != not_processed_count );

            // adjust column widths
            listViewCommits.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);
            listViewCommits.AutoResizeColumns(ColumnHeaderAutoResizeStyle.HeaderSize);

            listViewModifiedFiles.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);
            listViewModifiedFiles.AutoResizeColumns(ColumnHeaderAutoResizeStyle.HeaderSize);
        }

        private void ProcessModifiedFile(Repository repository, string path_in_repository)
        {
            string file_path = Path.GetFullPath(Path.Combine(repository.Info.WorkingDirectory, path_in_repository));
            string extension = Path.GetExtension(file_path).ToLower();

            bool process_and_write_utf8_bom =
                ( extension == ".h" || extension == ".cpp" ||
                  extension == ".cs" ||
                  extension == ".csdocset" || extension == ".csdoc" || extension == ".hgi" || extension == ".index" || extension == ".toc" ||
                  extension == ".apc" || extension == ".mgf" );

            bool process_and_remove_line_feeds =
                ( extension == ".php" );

            bool process =
                ( process_and_write_utf8_bom ||
                  process_and_remove_line_feeds ||
                  extension == ".cxx" ||
                  extension == ".kt" || extension == ".java" ||
                  extension == ".json" ||
                  extension == ".html" || extension == ".js" || extension == ".css" ||
                  extension == ".mk" || extension == ".gradle" ||
                  extension == ".txt" || extension == ".md" ||
                  extension == ".nsh" || extension == ".nsi" );

            if( process )
            {
                process = ( Path.GetFileName(file_path).Equals("resource.h", StringComparison.InvariantCultureIgnoreCase) == false ) &&
                          ( Path.GetFileName(file_path).IndexOf(".Designer.cs", StringComparison.InvariantCultureIgnoreCase) < 0 );
            }

            _modifiedFiles.Add(new ModifiedFile
            {
                FilePath = file_path,
                RepositoryPath = path_in_repository,
                Process = process,
                WriteUtf8Bom = process_and_write_utf8_bom,
                RemoveLineFeeds = process_and_remove_line_feeds
            });
        }

        private void buttonStandardizeFiles_Click(object sender, EventArgs e)
        {
            try
            {
                int files_modified = 0;

                foreach( var modified_file in _modifiedFiles )
                {
                    if( modified_file.Process && StandardizeFile(modified_file) )
                        ++files_modified;
                }

                MessageBox.Show($"{files_modified} files modified.");
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }
        }

        private bool StandardizeFile(ModifiedFile modified_file)
        {
            // read the text, which will be processed with only \n newline characters
            string original_file_text = File.ReadAllText(modified_file.FilePath);
            bool file_had_line_feeds = ( original_file_text.IndexOf('\r') >= 0 );

            original_file_text = original_file_text.Replace("\r\n", "\n").Replace('\r', '\n');

            // make sure that there are no tabs
            if( original_file_text.IndexOf('\t') >= 0 )
                throw new Exception($"Rework tabs in {modified_file.FilePath}");

            string file_text = "";

            // right trim lines
            foreach( string line in original_file_text.Split([ '\n' ]) )
                file_text = file_text + line.TrimEnd() + '\n';

            // make sure there is only one final newline
            file_text = file_text.TrimEnd() + '\n';

            if( ( original_file_text == file_text ) &&
                ( !modified_file.RemoveLineFeeds || !file_had_line_feeds ) )
            {
                return false;
            }

            // write the modified text, potentially with \r\n newline characters and a UTF-8 BOM
            if( !modified_file.RemoveLineFeeds )
                file_text = file_text.Replace("\n", "\r\n");

            File.WriteAllText(modified_file.FilePath, file_text, new UTF8Encoding(modified_file.WriteUtf8Bom));

            return true;
        }

        private void listViewModifiedFiles_DoubleClick(object sender, EventArgs e)
        {
            if( listViewModifiedFiles.SelectedIndices.Count > 0 )
                OpenFile(listViewModifiedFiles.SelectedItems[0]);
        }

        private void listViewModifiedFiles_MouseUp(object sender, MouseEventArgs e)
        {
            // show a context menu when right-clicking on items
            if( e.Button == MouseButtons.Right )
            {
                ListViewHitTestInfo hit_test_info = listViewModifiedFiles.HitTest(e.Location);;

                if( hit_test_info.Item != null && hit_test_info.SubItem == hit_test_info.Item.SubItems[0] )
                {
                    ContextMenuStrip menu = new ContextMenuStrip();

                    ToolStripMenuItem menu_item = new ToolStripMenuItem("Copy Path");
                    menu_item.Click += (s, args) => { CopyPath(hit_test_info.Item); };
                    menu.Items.Add(menu_item);

                    menu_item = new ToolStripMenuItem("Open File");
                    menu_item.Click += (s, args) => { OpenFile(hit_test_info.Item); };
                    menu.Items.Add(menu_item);

                    menu_item = new ToolStripMenuItem("Open Containing Folder");
                    menu_item.Click += (s, args) => { OpenContainingFolder(hit_test_info.Item); };
                    menu.Items.Add(menu_item);

                    menu.Show(listViewModifiedFiles, e.Location);
                }
            }
        }

        private void CopyPath(ListViewItem lvi)
        {
            Clipboard.SetText(_modifiedFiles[lvi.Index].FilePath);
        }

        private void OpenFile(ListViewItem lvi)
        {
            Process.Start(new ProcessStartInfo()
            {
                FileName = _modifiedFiles[lvi.Index].FilePath,
                UseShellExecute = true
            });
        }

        private void OpenContainingFolder(ListViewItem lvi)
        {
            Process.Start("explorer.exe", $"/select,\"{_modifiedFiles[lvi.Index].FilePath}\"");
        }
    }
}
