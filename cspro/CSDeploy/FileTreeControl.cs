using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
using Microsoft.WindowsAPICodePack.Dialogs;

namespace CSDeploy
{
    public partial class FileTreeControl : UserControl
    {
        // Filter function used to determine if a file in a folder
        // should be added to tree. Takes file path as argument
        // and returns true if file should be added.
        public Func<string, bool> AddFolderFilter { get; set; }

        public string RootPath { get { return m_rootPath; } }
        public IEnumerable<FileSpec> Files
        {
            get
            {
                return m_files;
            }
            set
            {
                m_files.Clear();
                m_files.UnionWith(value);
                updateTree();
            }
        }

        public event EventHandler OnFilesModified;

        private string m_rootPath;
        private Dictionary<String, int> m_fileExtensionImageIndexes = new Dictionary<string, int>();
        private int m_folderImageIndex;
        private int m_noFileAssocImage;
        private HashSet<FileSpec> m_files = new HashSet<FileSpec>(new FileSpecComparer());

        // For the purpose of the tree control, FileSpecs are equal if path is same i.e. ignore the onlyOnFirstInstall
        private class FileSpecComparer : IEqualityComparer<FileSpec>
        {
            public bool Equals(FileSpec x, FileSpec y)
            {
                return x.Path.Equals(y.Path);
            }

            public int GetHashCode(FileSpec obj)
            {
                return EqualityComparer<string>.Default.GetHashCode(obj.Path);
            }
        }

        public FileTreeControl()
        {
            InitializeComponent();

            fileIconsImageList.Images.Add(IconRetreiver.GetStockIcon(IconRetreiver.SHSTOCKICONID.SIID_FOLDER));
            m_folderImageIndex = 0;
            fileIconsImageList.Images.Add(IconRetreiver.GetStockIcon(IconRetreiver.SHSTOCKICONID.SIID_DOCNOASSOC));
            m_noFileAssocImage = 1;
        }

        private void dragEnter(object sender, DragEventArgs e)
        {
            e.Effect = e.Data.GetDataPresent(DataFormats.FileDrop) ? DragDropEffects.Copy : DragDropEffects.None;
        }

        private void dragDrop(object sender, DragEventArgs e)
        {
            var dropped = (string[])e.Data.GetData(DataFormats.FileDrop);
            var files = dropped.Where(f => (File.GetAttributes(f) & FileAttributes.Directory) != FileAttributes.Directory);
            addFiles(files);
            var directories = dropped.Where(f => (File.GetAttributes(f) & FileAttributes.Directory) == FileAttributes.Directory);
            foreach (var dirname in directories)
            {
                addFolder(dirname, AddFolderFilter);
            }
        }

        private void buttonAddFiles_Click(object sender, EventArgs e)
        {
            OpenFileDialog fdlg = new OpenFileDialog();
            fdlg.Title = Messages.AddFileOpenTitle;
            fdlg.Multiselect = true;
            if (fdlg.ShowDialog() == DialogResult.OK)
            {
                addFiles(fdlg.FileNames);
            }
        }

        private void buttonAddFolder_Click(object sender, EventArgs e)
        {
            CommonOpenFileDialog fdlg = new CommonOpenFileDialog();
            fdlg.Title = Messages.AddFolderFileOpenTitle;
            fdlg.IsFolderPicker = true;
            if (fdlg.ShowDialog() == CommonFileDialogResult.Ok)
            {
                addFolder(fdlg.FileName, AddFolderFilter);
            }
        }

        private void treeViewFiles_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                ContextMenuStrip contextMenuStrip;

                treeViewFiles.SelectedNode = e.Node;
                
                if( e.Node.Tag == null )
                    contextMenuStrip = folderContextMenuStrip;

                else
                {
                    contextMenuStrip = fileContextMenuStrip;

                    FileSpec file = e.Node.Tag as FileSpec;
                    ((ToolStripMenuItem)contextMenuStrip.Items[3]).Checked = ((FileSpec)e.Node.Tag).OnlyOnFirstInstall;
                }

                contextMenuStrip.Show(treeViewFiles, e.Location);
            }
        }

        private void removeToolStripMenuItem_Click(object sender, EventArgs e)
        {
            var nodeToRemove = treeViewFiles.SelectedNode;
            removeTreeNodeFromFiles(nodeToRemove);
            updateTree();
        }

        private void openContainingFolderToolStripMenuItem_Click(object sender, EventArgs e)
        {
            var node = treeViewFiles.SelectedNode;
            if (node != null)
            {
                FileSpec file = node.Tag as FileSpec;
                if (file != null)
                    System.Diagnostics.Process.Start("explorer.exe", "/select, \"" + file.Path + "\"");
            }
        }

        private void onlyOnFirstInstallToolStripMenuItem_Click(object sender, EventArgs e)
        {
            var node = treeViewFiles.SelectedNode;
            if (node != null)
            {
                FileSpec file = node.Tag as FileSpec;
                if (file != null)
                {
                   file.OnlyOnFirstInstall = !file.OnlyOnFirstInstall;
                }
            }
        }

        private void addFilesToolStripMenuItem_Click(object sender, EventArgs e)
        {
            // get a list of all files in order to find the root path associated with this folder node
            var files = new List<String>();

            if( treeViewFiles.SelectedNode != null )
            {
                Action<TreeNode> traverse_nodes_to_get_files = null;
                traverse_nodes_to_get_files = tree_node =>
                {
                    foreach( TreeNode node in tree_node.Nodes )
                    {
                        if( node.Tag != null )
                            files.Add(((FileSpec)node.Tag).Path);

                        traverse_nodes_to_get_files(node);
                    }
                };

                traverse_nodes_to_get_files(treeViewFiles.SelectedNode);
            }

            if( files.Count > 0 )
            {
                var add_folder_filter = ( sender == addAllFilesToolStripMenuItem ) ? null : AddFolderFilter;
                addFolder(findCommonPath(files), add_folder_filter);
            }
        }

        private void addFolder(string path, Func<string, bool> add_folder_filter)
        {
            var selectedFiles = Directory.EnumerateFiles(path, "*.*", SearchOption.AllDirectories);
            addFiles(selectedFiles.Where(f => add_folder_filter == null || add_folder_filter(f)));
        }

        private void addFiles(IEnumerable<string> newFiles)
        {
            if (m_files.Count > 0 && newFiles.Any(f => findCommonPath(m_rootPath, f) == null))
            {
                MessageBox.Show(Messages.ErrorDifferentVolumes);
                return;
            }
            m_files.UnionWith(newFiles.Select(p => new FileSpec { Path = p, OnlyOnFirstInstall = false }));
            updateTree();
        }

        private void removeTreeNodeFromFiles(TreeNode n)
        {
            if (n.Tag != null)
                m_files.Remove(n.Tag as FileSpec);
            foreach (TreeNode c in n.Nodes)
                removeTreeNodeFromFiles(c);
        }

        private static string findCommonPath(string path1, string path2)
        {
            int lastSep;
            int nextSep = 0;
            do
            {
                lastSep = nextSep;
                nextSep = path1.IndexOf(Path.DirectorySeparatorChar, lastSep + 1);
            } while (nextSep >= 0 && nextSep < path2.Length && path2[nextSep] == Path.DirectorySeparatorChar && String.Equals(path1.Substring(lastSep, nextSep - lastSep), path2.Substring(lastSep, nextSep - lastSep), StringComparison.OrdinalIgnoreCase));

            if (lastSep > 0)
            {
                return path1.Substring(0, lastSep + 1);
            }
            else if (path1 == path2)
            {
                return path1;
            }
            else
            {
                return null;
            }
        }

        private static string findCommonPath(IEnumerable<string> files)
        {
            string common = null;
            foreach (var f in files)
            {
                var dir = Path.GetDirectoryName(f);
                if (!dir.EndsWith(Path.DirectorySeparatorChar.ToString()))
                    dir += Path.DirectorySeparatorChar;

                if (common == null)
                    common = dir;
                else
                    common = findCommonPath(common, dir);
            }

            return common;
        }

        private void updateTree()
        {
            treeViewFiles.Nodes.Clear();

            if (m_files.Count > 0)
            {
                m_rootPath = findCommonPath(m_files.Select(s => s.Path));

                TreeNode rootNode = new TreeNode(Path.GetFileName(Path.GetDirectoryName(m_rootPath)));
                rootNode.ImageIndex = m_folderImageIndex;
                rootNode.SelectedImageIndex = m_folderImageIndex;

                foreach (var file in m_files)
                {
                    var pathFromRoot = file.Path.Substring(m_rootPath.Length);
                    var segments = pathFromRoot.Split(Path.DirectorySeparatorChar);
                    int currentSegment = 0;
                    // Add parent folders from root
                    TreeNode parentNode = rootNode;

                    while (currentSegment < segments.Length - 1)
                    {
                        var segment = segments[currentSegment];
                        TreeNode segmentNode = parentNode.Nodes.Cast<TreeNode>().FirstOrDefault(n => n.Name == segment);
                        if (segmentNode == null)
                        {
                            segmentNode = new TreeNode(segment);
                            segmentNode.ImageIndex = m_folderImageIndex;
                            segmentNode.SelectedImageIndex = m_folderImageIndex;
                            segmentNode.Name = segment;
                            parentNode.Nodes.Add(segmentNode);
                        }
                        parentNode = segmentNode;
                        ++currentSegment;
                    }

                    var fileNode = new TreeNode(segments.Last());
                    fileNode.Tag = file;

                    string extension = Path.GetExtension(file.Path);
                    if (m_fileExtensionImageIndexes.ContainsKey(extension))
                    {
                        fileNode.ImageIndex = m_fileExtensionImageIndexes[extension];
                        fileNode.SelectedImageIndex = m_fileExtensionImageIndexes[extension];
                    }
                    else
                    {
                        var icon = IconRetreiver.GetIconForFile(file.Path);
                        if (icon != null)
                        {
                            fileIconsImageList.Images.Add(icon);
                            int newImageIndex = fileIconsImageList.Images.Count - 1;
                            fileNode.ImageIndex = newImageIndex;
                            fileNode.SelectedImageIndex = newImageIndex;

                            if (extension != "")
                                m_fileExtensionImageIndexes.Add(extension, newImageIndex);
                        }
                        else
                        {
                            fileNode.ImageIndex = m_noFileAssocImage;
                            fileNode.SelectedImageIndex = m_noFileAssocImage;
                        }
                    }

                    fileNode.Name = segments.Last();
                    parentNode.Nodes.Add(fileNode);
                }

                labelEmptyTreeMessage.Visible = false;
                treeViewFiles.Nodes.Add(rootNode);
                treeViewFiles.ExpandAll();
                rootNode.EnsureVisible();
            }
            else
            {
                labelEmptyTreeMessage.Visible = true;
            }

            RaiseOnFilesModified();
        }

        private void RaiseOnFilesModified()
        {
            var handler = OnFilesModified;
            if (handler != null)
            {
                EventArgs args = new EventArgs();
                handler(this, args);
            }
        }

        private void treeViewFiles_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Delete)
            {
                var nodeToRemove = treeViewFiles.SelectedNode;
                if (nodeToRemove != null)
                {
                    removeTreeNodeFromFiles(nodeToRemove);
                    updateTree();
                }
            }
        }
    }
}
