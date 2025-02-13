using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net;
using System.Windows.Forms;
using Microsoft.VisualBasic.FileIO;

namespace ParadataViewer
{
    partial class PluginManagerForm : Form
    {
        private const string PluginTitle = "Paradata Viewer Plugin";
        private const string PluginExtension = ".cspvp";
        private const string PluginWildcardExtension = "*" + PluginExtension;

        private Controller _controller;
        private OnlinePluginMetadataLink _onlinePluginMetadataLinkToDownload;
        private List<PluginMetadata> _installedPlugins;
        private WebClient _downloadWebClient;
        private string _temporaryDownloadFilename;

        public PluginManagerForm(Controller controller,OnlinePluginMetadataLink onlinePluginMetadataLinkToDownload = null)
        {
            InitializeComponent();

            _controller = controller;
            _onlinePluginMetadataLinkToDownload = onlinePluginMetadataLinkToDownload;
            _installedPlugins = _controller.Settings.InstalledPlugins;

            UpdatePluginList();
        }

        private void PluginManagerForm_Load(object sender,EventArgs e)
        {
            if( _onlinePluginMetadataLinkToDownload != null )
                DownloadPlugin(_onlinePluginMetadataLinkToDownload.Url);
        }

        private void UpdatePluginList()
        {
            listViewPlugins.Items.Clear();

            foreach( var plugin in _installedPlugins.Where(x => !( x is OnlinePluginMetadataLink )).OrderBy(x => x.DisplayName) )
            {
                var lvi = listViewPlugins.Items.Add(plugin.DisplayName);
                lvi.SubItems.Add(plugin.DllFilename);
                lvi.Checked = plugin.Enabled;
                lvi.Tag = plugin;
            }
        }

        private void listViewPlugins_ItemCheck(object sender,ItemCheckEventArgs e)
        {
            if( listViewPlugins.FocusedItem != null )
            {
                var plugin = (PluginMetadata)listViewPlugins.FocusedItem.Tag;
                plugin.Enabled = ( e.NewValue == CheckState.Checked );
            }
        }

        private void buttonRemovePlugin_Click(object sender,EventArgs e)
        {
            try
            {
                if( listViewPlugins.SelectedItems.Count == 0 )
                    throw new Exception("You must select a plugin");

                string dllToDelete = ((PluginMetadata)listViewPlugins.SelectedItems[0].Tag).DllFilename;

                var pluginsToDelete = _installedPlugins.Where(x => ( x.DllFilename == dllToDelete ));
                var pluginsToKeep = _installedPlugins.Where(x => ( x.DllFilename != dllToDelete ));

                string confirmationMessage = "";

                foreach( var plugin in pluginsToDelete )
                {
                    confirmationMessage = confirmationMessage + String.Format("{0} {1}",
                        ( confirmationMessage.Length == 0 ) ? "Are you sure that you want to remove" : " and",
                        plugin.DisplayName);
                }

                if( MessageBox.Show(confirmationMessage + "?",this.Text,MessageBoxButtons.YesNoCancel) != DialogResult.Yes )
                    return;

                // get a list of files to delete
                var filesToDelete = new HashSet<string>();

                foreach( var plugin in pluginsToDelete )
                {
                    foreach( var filename in plugin.Filenames )
                        filesToDelete.Add(filename);
                }

                // and then remove the files used by other plugins
                foreach( var plugin in pluginsToKeep )
                {
                    foreach( var filename in plugin.Filenames )
                        filesToDelete.Remove(filename);
                }

                // delete the files
                foreach( var filename in filesToDelete )
                {
                    if( File.Exists(filename) )
                        FileSystem.DeleteFile(filename,UIOption.OnlyErrorDialogs,RecycleOption.SendToRecycleBin);
                }

                // remove the plugins from the metadata
                _installedPlugins.RemoveAll(x => ( x.DllFilename == dllToDelete ));

            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }

            UpdatePluginList();
        }

        private void buttonDownloadPlugin_Click(object sender,EventArgs e)
        {
            DownloadPlugin(textBoxUrl.Text);
        }

        private void DownloadPlugin(string url)
        {
            try
            {
                if( _downloadWebClient != null )
                    throw new Exception("You can only download one file at a time");

                if( String.IsNullOrWhiteSpace(url) )
                    throw new Exception("You must specify a URL");

                Uri uri = new Uri(url);

                _downloadWebClient = new WebClient();
                _downloadWebClient.DownloadProgressChanged += DownloadWebClient_DownloadProgressChanged;
                _downloadWebClient.DownloadFileCompleted += DownloadWebClient_DownloadFileCompleted;

                _temporaryDownloadFilename = _controller.GetTemporaryFilename(PluginExtension);
                _downloadWebClient.DownloadFileAsync(uri,_temporaryDownloadFilename);

                progressBarDownload.Value = 0;
                progressBarDownload.Visible = true;
                buttonCancelDownload.Visible = true;
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
                _downloadWebClient = null;
            }
        }

        private void DownloadWebClient_DownloadProgressChanged(object sender,DownloadProgressChangedEventArgs e)
        {
            progressBarDownload.Value = e.ProgressPercentage;
        }

        private void DownloadWebClient_DownloadFileCompleted(object sender,AsyncCompletedEventArgs e)
        {
            _downloadWebClient = null;
            progressBarDownload.Visible = false;
            buttonCancelDownload.Visible = false;

            if( !e.Cancelled )
                ApplyPlugin(_temporaryDownloadFilename);

            _onlinePluginMetadataLinkToDownload = null;
        }

        private void buttonCancelDownload_Click(object sender,EventArgs e)
        {
            _downloadWebClient.CancelAsync();
        }

        private void buttonBrowsePlugin_Click(object sender,EventArgs e)
        {
            var ofd = new OpenFileDialog();
            ofd.Title = "Select " + PluginTitle;
            ofd.Filter = String.Format("{0} ({1})|{1}|All Files (*.*)|*.*",PluginTitle,PluginWildcardExtension);

            if( ofd.ShowDialog() == DialogResult.OK )
            {
                bool movePluginFile = ( MessageBox.Show("Do you want to move this plugin into the CSPro plugins folder?",this.Text,MessageBoxButtons.YesNo) == DialogResult.Yes );
                ApplyPlugin(ofd.FileName,movePluginFile);
            }
        }

        void ApplyPlugin(string filename,bool movePluginFile = true)
        {
            string temporaryExtractedDirectory = null;
            bool zipFile = false;

            try
            {
                var potentialPluginFilenames = new List<string>();

                try
                {
                    using( var zip = ZipFile.OpenRead(filename) )
                    {
                        temporaryExtractedDirectory = Path.Combine(Path.GetTempPath(),Path.GetRandomFileName());
                        zip.ExtractToDirectory(temporaryExtractedDirectory);

                        foreach( var fi in new DirectoryInfo(temporaryExtractedDirectory).GetFiles() )
                            potentialPluginFilenames.Add(fi.FullName);

                        zipFile = true;
                    }
                }

                catch
                {
                    // not a zip file
                    potentialPluginFilenames.Add(filename);
                }

                // load each file to create a list of plugins
                if( potentialPluginFilenames.Count == 0 )
                    throw new Exception("No plugin file was specified");

                // load all of the files and generate a list of plugins
                var plugins = new List<PluginMetadata>();

                foreach( string pluginFilename in potentialPluginFilenames )
                {
                    try
                    {
                        plugins.AddRange(PluginManager.LoadPluginsFromFile(pluginFilename));
                    }
                    catch { }
                }

                if( plugins.Count == 0 )
                    throw new Exception("No valid plugins were found");

                HashSet<string> movedFilenames = new HashSet<string>();

                foreach( var plugin in plugins )
                {
                    if( _installedPlugins.FirstOrDefault(x => ( x.ClassName == plugin.ClassName )) != null )
                    {
                        MessageBox.Show(String.Format("{0} will not be added because it is already installed",plugin.DisplayName));
                        continue;
                    }

                    // add any associated files
                    foreach( string pluginFilename in potentialPluginFilenames )
                    {
                        if( pluginFilename != plugin.DllFilename )
                            plugin.Filenames.Add(pluginFilename);
                    }

                    if( movePluginFile )
                    {
                        var newFilenames = new List<Tuple<string,string>>();
                        bool moveCanProceed = true;

                        for( int i = 0; i < plugin.Filenames.Count; i++ )
                        {
                            var pluginFilename = plugin.Filenames[i];
                            string newFilename = Path.Combine(Settings.PluginsDirectory,Path.GetFileName(pluginFilename));

                            // no need to move a file more than once
                            if( movedFilenames.Contains(newFilename) )
                            {
                                plugin.Filenames[i] = newFilename;
                                continue;
                            }

                            if( File.Exists(newFilename) )
                            {
                                if( MessageBox.Show(String.Format("The file {0} already exists. Do you want to overwrite it to install {1}?",
                                    newFilename,plugin.DisplayName),this.Text,MessageBoxButtons.YesNoCancel) != DialogResult.Yes )
                                {
                                    moveCanProceed = false;
                                    break;
                                }
                            }

                            newFilenames.Add(new Tuple<string,string>(pluginFilename,newFilename));
                        }

                        if( !moveCanProceed )
                            continue;

                        // move the files
                        for( int i = 0; i < newFilenames.Count; i++ )
                        {
                            plugin.Filenames[i] = newFilenames[i].Item2;

                            // delete files that the user said should be overwriten
                            if( File.Exists(newFilenames[i].Item2) )
                                File.Delete(newFilenames[i].Item2);

                            File.Move(newFilenames[i].Item1,newFilenames[i].Item2);

                            movedFilenames.Add(newFilenames[i].Item2);
                        }

                        // delete the zip file that housed the extracted plugin files
                        if( zipFile && File.Exists(filename) )
                            FileSystem.DeleteFile(filename,UIOption.OnlyErrorDialogs,RecycleOption.SendToRecycleBin);
                    }

                    _installedPlugins.Add(plugin);

                    // remove the link from which this plugin was downloaded
                    if( _onlinePluginMetadataLinkToDownload != null )
                        _installedPlugins.Remove(_onlinePluginMetadataLinkToDownload);
                }
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }

            finally
            {
                // delete any temporary files still remaining
                if( temporaryExtractedDirectory != null )
                {
                    foreach( var fi in new DirectoryInfo(temporaryExtractedDirectory).GetFiles() )
                    {
                        try
                        {
                            File.Delete(fi.FullName);
                        }
                        catch { }
                    }

                    try
                    {
                        Directory.Delete(temporaryExtractedDirectory);
                    }
                    catch { }
                }
            }

            UpdatePluginList();
        }
    }
}
