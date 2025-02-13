using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Windows.Forms;
using CSPro.ParadataViewer;

namespace ParadataViewer
{
    partial class MainForm : Form
    {
        private Controller _controller;
        private string _initialTitleText;
        private int? _numberBuiltInViewMenuItems;
        private List<FilterControl> _filterControls;

        public MainForm()
        {
            InitializeComponent();

            _controller = new Controller(this);
            _initialTitleText = this.Text;

            UpdateSplittersStateAndVisibility();

            UpdatePluginMenuItems();

            this.WindowState = FormWindowState.Maximized;
        }

        internal void UpdateTitle()
        {
            this.Text = _controller.IsLogOpen ?
                ( Path.GetFileName(_controller.OpenLogFilename) + " - " + _initialTitleText ) :
                _initialTitleText;
        }

        internal void UpdateStatusBarText(string text)
        {
            statusBarLabel.Text = text;
        }

        internal void InitializeFilters()
        {
            _filterControls = filterPanelControl.Initialize(_controller);
        }

        internal void HideFilters()
        {
            filterPanelControl.HideFilters();
        }

        internal List<FilterControl> FilterControls { get { return _filterControls; } }

        private void UpdateSplittersStateAndVisibility()
        {
            _controller.RestoreSplitterState(this,panelQuerySql,splitterQuerySql);
            _controller.RestoreSplitterState(this,panelFilter,splitterFilter);
            UpdateSqlStatementsPaneVisibility();
        }

        private void UpdateSqlStatementsPaneVisibility()
        {
            splitterQuerySql.Visible = _controller.Settings.ShowSqlStatementsPane;
            panelQuerySql.Visible = _controller.Settings.ShowSqlStatementsPane;
        }

        private void splitterQuerySql_SplitterMoved(object sender,SplitterEventArgs e)
        {
            _controller.SaveSplitterState(this,panelQuerySql,splitterQuerySql);
        }

        private void splitterFilter_SplitterMoved(object sender,SplitterEventArgs e)
        {
            _controller.SaveSplitterState(this,panelFilter,splitterFilter);
        }

        private void MainForm_Shown(object sender,EventArgs e)
        {
            // if a log is specified on the command line, open it
            Array commandArgs = Environment.GetCommandLineArgs();

            if( commandArgs.Length > 1 )
            {
                try
                {
                    var filenames = new List<string>();

                    for( int i = 1; i < commandArgs.Length; i++ )
                    {
                        string filename = Path.GetFullPath((string)commandArgs.GetValue(i));

                        if( File.Exists(filename) )
                            filenames.Add(filename);
                    }

                    LoadLogs(filenames);
                }
                catch { }
            }
        }

        private void MainForm_DragEnter(object sender,DragEventArgs e)
        {
            e.Effect = e.Data.GetDataPresent(DataFormats.FileDrop) ? DragDropEffects.Copy : DragDropEffects.None;
        }

        private void MainForm_DragDrop(object sender,DragEventArgs e)
        {
            var filenames = new List<string>();

            foreach( string filename in (Array)e.Data.GetData(DataFormats.FileDrop) )
            {
                if( Directory.Exists(filename) )
                {
                    var di = new DirectoryInfo(filename);

                    foreach( var fi in di.GetFiles(Controller.LogFilenameFilter,SearchOption.AllDirectories) )
                        filenames.Add(fi.FullName);
                }

                else
                {
                    var fi = new FileInfo(filename);

                    if( fi.Extension.Equals(Controller.LogFilenameExtension,StringComparison.CurrentCultureIgnoreCase) )
                        filenames.Add(fi.FullName);
                }
            }

            LoadLogs(filenames);
        }

        private async void LoadLogs(List<string> filenames)
        {
            string filenameToLoad = null;

            if( filenames.Count == 0 )
                MessageBox.Show("No valid file selected");

            else if( filenames.Count == 1 )
                filenameToLoad = filenames[0];

            else if( filenames.Count > 1 )
            {
                var autoConcatForm = new AutoConcatForm(filenames.Count,_controller.IsLogOpen);

                if( autoConcatForm.ShowDialog() != DialogResult.OK )
                    return;

                // concatenate the logs
                if( autoConcatForm.IncludeCurrentLog )
                    filenames.Add(_controller.OpenLogFilename);

                string outputLogFilename = autoConcatForm.UseTemporaryLog ?
                    _controller.GetTemporaryFilename(Controller.LogFilenameExtension) : autoConcatForm.OutputLogFilename;

                // create and run a PFF with the concatenation parameters
                string pffFilename = _controller.GetTemporaryFilename(".pff");
                string lstFilename = _controller.GetTemporaryFilename(".lst");

                Helper.CreateParadataConcatPff(pffFilename,lstFilename,filenames,outputLogFilename);

                ExecuteParadataConcat('"' + pffFilename + '"');

                if( File.Exists(outputLogFilename) )
                    filenameToLoad = outputLogFilename;

                else
                    MessageBox.Show("There was an error creating: " + outputLogFilename);
            }

            if( filenameToLoad != null )
            {
                await _controller.LoadFileAsync(filenameToLoad);

                // open the Reports Viewer as the default view
                menuItemViewReports_Click(null,null);
            }
        }

        private void ExecuteParadataConcat(string argument = null)
        {
            try
            {
                string paradataConcatExeFilename = "ParadataConcat.exe";
                paradataConcatExeFilename = Path.Combine(Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location),paradataConcatExeFilename);

                if( !File.Exists(paradataConcatExeFilename) )
                    throw new Exception("The tool could not be located here: " + paradataConcatExeFilename);

                if( argument == null )
                    Process.Start(paradataConcatExeFilename);

                else
                {
                    var processStartInfo = new ProcessStartInfo(paradataConcatExeFilename,argument);
                    Process.Start(processStartInfo).WaitForExit();
                }
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }
        }

        internal async void RefreshWindows()
        {
            foreach( var form in this.MdiChildren )
            {
                if( form is ViewerForm )
                    await ((ViewerForm)form).RefreshFormAsync(form == this.ActiveMdiChild);
            }
        }

        internal void CloseWindows()
        {
            foreach( var form in this.MdiChildren )
                form.Close();
        }

        internal void UpdateQuerySql(string sql)
        {
            textBoxQuerySql.Text = sql;
        }

        private void linkLabelCopyQuerySql_LinkClicked(object sender,LinkLabelLinkClickedEventArgs e)
        {
            if( textBoxQuerySql.Text.Length > 0 )
            {
                Clipboard.SetText(textBoxQuerySql.Text);
                UpdateStatusBarText("Query copied to clipboard");
            }
        }

        private void linkLabelOpenInQueryConstructor_LinkClicked(object sender,LinkLabelLinkClickedEventArgs e)
        {
            if( textBoxQuerySql.Text.Length > 0 )
                OpenQueryConstructor(textBoxQuerySql.Text);
        }


        // --------------------------------
        // -------------------------------- FILE MENU
        // --------------------------------

        private void toolStripFile_DropDownOpening(object sender,System.EventArgs e)
        {
            menuItemFileClose.Enabled = _controller.IsLogOpen;
        }

        private void menuItemFileOpen_Click(object sender,System.EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Title = "Select Paradata Log";
            ofd.Filter = String.Format("Paradata Log ({0})|{0}",Controller.LogFilenameFilter);
            ofd.Multiselect = true;

            if( ofd.ShowDialog() != DialogResult.OK )
                return;

            LoadLogs(new List<string>(ofd.FileNames));
        }

        private void menuItemFileClose_Click(object sender,System.EventArgs e)
        {
            if( _controller.IsLogOpen )
                _controller.CloseFile();
        }

        private void menuItemFileExit_Click(object sender,System.EventArgs e)
        {
            Close();
        }


        // --------------------------------
        // -------------------------------- VIEW MENU
        // --------------------------------

        private void toolStripView_DropDownOpening(object sender,EventArgs e)
        {
            foreach( var item in toolStripView.DropDownItems )
            {
                if( item is ToolStripMenuItem )
                    ((ToolStripMenuItem)item).Enabled = _controller.IsLogOpen;
            }
        }

        private void OpenMdiForm(Form form)
        {
            form.MdiParent = this;
            form.WindowState = FormWindowState.Maximized;
            form.Show();
        }

        private void menuItemViewQueryConstructor_Click(object sender,EventArgs e)
        {
            OpenQueryConstructor();
        }

        internal void OpenQueryConstructor(string initialSql = null)
        {
            if( _controller.IsLogOpen )
            {
                OpenMdiForm(new QueryConstructorForm(_controller,initialSql));
                UpdateStatusBarText("Opened query constructor");
            }
        }

        private void menuItemViewMetadata_Click(object sender,EventArgs e)
        {
            if( _controller.IsLogOpen )
            {
                OpenMdiForm(new MetadataForm(_controller));
                UpdateStatusBarText("Opened table metadata");
            }
        }

        private void menuItemViewTableBrowser_Click(object sender,EventArgs e)
        {
            if( _controller.IsLogOpen )
            {
                OpenMdiForm(new TableBrowserForm(_controller));
                UpdateStatusBarText("Opened table browser");
            }
        }

        private void menuItemViewReports_Click(object sender,EventArgs e)
        {
            if( _controller.IsLogOpen )
            {
                OpenMdiForm(new ReportViewerForm(_controller));
                UpdateStatusBarText("Opened report viewer");
            }
        }

        private void menuItemViewLocationMapper_Click(object sender, EventArgs e)
        {
            if( _controller.IsLogOpen )
            {
                OpenMdiForm(new LocationMapperForm(_controller));
                UpdateStatusBarText("Opened location mapper");
            }
        }

        private void UpdatePluginMenuItems()
        {
            // set the initial count of items
            if( _numberBuiltInViewMenuItems == null )
                _numberBuiltInViewMenuItems = toolStripView.DropDownItems.Count;

            // clear any displayed plugins
            for( int i = toolStripView.DropDownItems.Count - 1; i >= (int)_numberBuiltInViewMenuItems; i-- )
                toolStripView.DropDownItems.Remove(toolStripView.DropDownItems[i]);

            // if there are plugins, add a separator and then the plugins
            bool addedSeparator = false;

            foreach( var plugin in _controller.Settings.InstalledPlugins.OrderBy(x => x.DisplayName).Where(x => x.Enabled) )
            {
                // as of CSPro 7.7, the Location Mapper is built into Paradata Viewer
                // so do not show the plugin if it exists
                if( plugin.ClassName == "LocationMapper.LocationMapperControl" )
                    continue;

                if( !addedSeparator )
                {
                    toolStripView.DropDownItems.Add(new ToolStripSeparator());
                    addedSeparator = true;
                }

                var item = toolStripView.DropDownItems.Add(plugin.DisplayName);
                item.Click += Plugin_Click;
                item.Tag = plugin;
            }
        }

        private void Plugin_Click(object sender,EventArgs e)
        {
            if( _controller.IsLogOpen )
            {
                var item = (ToolStripMenuItem)sender;
                var plugin = (PluginMetadata)item.Tag;

                // download the plugin if necessary
                if( plugin is OnlinePluginMetadataLink )
                {
                    var onlinePluginMetadataLink = (OnlinePluginMetadataLink)plugin;
                    var uri = new Uri(onlinePluginMetadataLink.Url);

                    string message = $"This plugin is available from {uri.Host}. Do you want to download it now?";

                    if( MessageBox.Show(message,"Download Plugin?",MessageBoxButtons.YesNoCancel) == DialogResult.Yes )
                        LaunchPluginManager(onlinePluginMetadataLink);
                }

                // is already installed, open it
                else
                {
                    try
                    {
                        IPluggableQueryControl pluggableQueryControl = PluginManager.GetQueryControl(plugin);
                        OpenMdiForm(new PluggableQueryForm(_controller,pluggableQueryControl));
                        UpdateStatusBarText("Opened " + plugin.DisplayName);
                    }

                    catch( Exception exception )
                    {
                        MessageBox.Show("There was an error loading the plugin: " + exception.Message);
                    }
                }
            }
        }


        // --------------------------------
        // -------------------------------- OPTIONS MENU
        // --------------------------------

        private void toolStripOptions_DropDownOpening(object sender,EventArgs e)
        {
            menuItemOptionsValueLabels.Checked = _controller.Settings.QueryOptions.ViewValueLabels;
            menuItemOptionsConvertTimestamps.Checked = _controller.Settings.QueryOptions.ConvertTimestamps;
            menuItemOptionsLinkedTables.Checked = _controller.Settings.QueryOptions.ShowFullyLinkedTables;
            menuItemOptionsLinkingValues.Checked = _controller.Settings.QueryOptions.ShowLinkingValues;
            menuItemOptionsSqlStatements.Checked = _controller.Settings.ShowSqlStatementsPane;
        }

        private void menuItemOptionsValueLabels_Click(object sender,EventArgs e)
        {
            _controller.Settings.QueryOptions.ViewValueLabels = !_controller.Settings.QueryOptions.ViewValueLabels;
            RefreshWindows();
        }

        private void menuItemOptionsConvertTimestamps_Click(object sender,EventArgs e)
        {
            _controller.Settings.QueryOptions.ConvertTimestamps = !_controller.Settings.QueryOptions.ConvertTimestamps;
            RefreshWindows();
        }

        private void menuItemOptionsLinkedTables_Click(object sender,EventArgs e)
        {
            _controller.Settings.QueryOptions.ShowFullyLinkedTables = !_controller.Settings.QueryOptions.ShowFullyLinkedTables;
            RefreshWindows();
        }

        private void menuItemOptionsLinkingValues_Click(object sender,EventArgs e)
        {
            _controller.Settings.QueryOptions.ShowLinkingValues = !_controller.Settings.QueryOptions.ShowLinkingValues;
            RefreshWindows();
        }

        private void menuItemOptionsSqlStatements_Click(object sender,EventArgs e)
        {
            _controller.Settings.ShowSqlStatementsPane = !_controller.Settings.ShowSqlStatementsPane;
            UpdateSqlStatementsPaneVisibility();
        }

        private void menuItemOptionsSettings_Click(object sender,EventArgs e)
        {
            var settingsForm = new SettingsForm(_controller);
            settingsForm.ShowDialog();
        }


        // --------------------------------
        // -------------------------------- TOOLS MENU
        // --------------------------------

        private void menuItemToolsConcatenator_Click(object sender,System.EventArgs e)
        {
            ExecuteParadataConcat();
        }

        private void menuItemToolsPluginManager_Click(object sender,EventArgs e)
        {
            LaunchPluginManager();
        }

        private void LaunchPluginManager(OnlinePluginMetadataLink onlinePluginMetadataLink = null)
        {
            var pluginManagerForm = new PluginManagerForm(_controller,onlinePluginMetadataLink);
            pluginManagerForm.ShowDialog();

            // save the settings immediately so that the plugin settings are saved (in case the
            // program crashes before closing normally)
            _controller.Settings.Save();

            UpdatePluginMenuItems();
        }

        // --------------------------------
        // -------------------------------- HELP MENU
        // --------------------------------

        private void menuItemHelpHelp_Click(object sender,System.EventArgs e)
        {
            Help.ShowHelp(null,"ParadataViewer.chm");
        }

        private void menuItemAbout_Click(object sender,System.EventArgs e)
        {
            new WinFormsShared.AboutForm("CSPro Paradata Viewer",
                "The Paradata Viewer tool is used to view reports from the events stored in paradata logs created during data collection.",
                Properties.Resources.MainIcon).ShowDialog();
        }
    }
}
