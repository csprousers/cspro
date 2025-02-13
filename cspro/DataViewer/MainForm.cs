using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Windows.Forms;
using Microsoft.Win32;
using WinFormsShared;

namespace DataViewer
{
    partial class MainForm : Form
    {
        private string _defaultWindowTitle;
        private int _keyPrefixPanelHeightAdjustment;

        private CSPro.Data.DataViewerSettings _settings;
        private List<ToolStripMenuItem> _settingMenuItems;

        private CSPro.Dictionary.DataDictionary _dictionary;
        private string _dictionarySourceFilename;
        private CSPro.Data.DataRepository _repository;
        private CSPro.Util.ConnectionString _connectionString;

        private CSPro.Data.DataViewerController _controller;
        private CaseListingProvider _caseListingProcessor;
        private CSPro.Util.PFF _executedPff;

        public delegate bool AcceleratorHandler(Keys keyData);
        public static AcceleratorHandler HandleAcceleratorKey { get; private set; }
        private Dictionary<Keys, ToolStripMenuItem> _acceleratorKeyMap;

        public MainForm()
        {
            HandleAcceleratorKey = HandleAcceleratorKeyWorker;

            InitializeComponent();

            menuItemWindowCascade.Tag = MdiLayout.Cascade;
            menuItemWindowTileTopToBottom.Tag = MdiLayout.TileHorizontal;
            menuItemWindowTileSideBySide.Tag = MdiLayout.TileVertical;

            _defaultWindowTitle = this.Text;
            _keyPrefixPanelHeightAdjustment = listViewCases.Location.Y - panelKeyPrefix.Location.Y;

            _settings = CSPro.Data.DataViewerSettings.Load();

            // restore the window to the state/size of its last use
            if( _settings.WindowState != FormWindowState.Minimized )
            {
                this.WindowState = _settings.WindowState;

                if( _settings.WindowSize.Width > 0 && _settings.WindowSize.Width <= Screen.PrimaryScreen.Bounds.Width &&
                    _settings.WindowSize.Height <= Screen.PrimaryScreen.Bounds.Height )
                {
                    this.Size = _settings.WindowSize;
                }
            }

            UpdateWindowUI();

            UpdateMenuChecks();

            UpdateKeyPrefixPanel();
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            // setup the HTML provider
            try
            {
                CSPro.Data.DataViewerHtmlProvider.Initialize();
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
                Close();
            }
        }

        private void MainForm_Shown(object sender, EventArgs e)
        {
            Array command_args = Environment.GetCommandLineArgs();

            if( command_args.Length < 2 )
                return;

            try
            {
                // command line arguments can come in a variety of ways
                // 1) [PFF filename]
                // 2) [data filename] [optional dictionary filename]
                // 3) [file=data filename] [dcf=optional dictionary filename] [key=case key to select] [uuid=optional case uuid to select]
                var string_args = command_args.Cast<string>().Skip(1).ToArray();

                string filename = null;
                string dictionary_filename = null;
                string case_key = null;
                string case_uuid = null;

                foreach( string arg in string_args )
                {
                    int equals_pos = arg.IndexOf('=');

                    if( equals_pos < 0 )
                        break;

                    string command = arg.Substring(0, equals_pos);
                    string value = arg.Substring(equals_pos + 1);

                    if( command == "file" )      filename = GetFullPathWithConnectionStringHandling(value);
                    else if( command == "dcf" )  dictionary_filename = Path.GetFullPath(value);
                    else if( command == "key" )  case_key = value;
                    else if( command == "uuid" ) case_uuid = value;
                    else                         break;
                }

                // if not using the command/value approach, the filename will be the first argument
                // and the dictionary will be the second optional argument
                if( filename == null )
                {
                    filename = GetFullPathWithConnectionStringHandling(string_args[0]);

                    if( dictionary_filename == null && string_args.Length > 1 )
                        dictionary_filename = Path.GetFullPath(string_args[1]);
                }

                OpenFile(filename, dictionary_filename);

                // load and show the specified case
                if( case_key != null && _repository != null )
                {
                    var data_case = _controller.ReadCase(case_key, case_uuid);
                    ShowCaseRelatedForm(data_case);
                }
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }
        }

        protected override void WndProc(ref Message message)
        {
            if( message.Msg == InterProcessCommunication.WM_COPYDATA )
            {
                message.Result = (IntPtr)0;

                try
                {
                    // the parameters passed by the UriHandler are: filename, case key, case UUID
                    var parameters = InterProcessCommunication.GetCopyDataStrings(ref message);

                    // return if no file is open or if a different file is open
                    if( _repository == null || _connectionString.ToString() != parameters[0] )
                        return;

                    // only load the case if one is specified; otherwise the UriHandler is only trying
                    // to open the data file
                    if( !string.IsNullOrEmpty(parameters[1]) || !string.IsNullOrEmpty(parameters[2]) )
                    {
                        var data_case = _controller.ReadCase(parameters[1], parameters[2]);
                        ShowCaseRelatedForm(data_case);
                    }

                    message.Result = (IntPtr)1;
                }

                catch { }
            }

            else
                base.WndProc(ref message);
        }


        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            CloseFile();

            _settings.WindowState = this.WindowState;
            _settings.WindowSize = this.Size;
        }

        private void MainForm_FormClosed(object sender, FormClosedEventArgs e)
        {
            if( _executedPff != null )
                _executedPff.ExecuteOnExitPff();
        }


        private void UpdateWindowTitle()
        {
            this.Text = ( _repository == null ) ? _defaultWindowTitle :
                $"{_repository.ConciseName} ({_dictionary.Name}: {_repository.NumberCases} cases) - {_defaultWindowTitle}";
        }

        private void UpdateWindowUI()
        {
            bool repository_open = ( _repository != null );
            bool allow_export = ( repository_open && _repository.Dictionary.AllowExport );

            UpdateWindowTitle();

            panelCaseListing.Visible = repository_open;

            // disable all options on some menus
            foreach( var menu_item in new ToolStripMenuItem[] { menuItemListing, menuItemCase, menuItemView, menuItemTools } )
            {
                foreach( ToolStripItem sub_menu_item in menu_item.DropDownItems )
                    sub_menu_item.Enabled = repository_open;
            }

            // and some options on other menus
            foreach( var menu_item in new ToolStripMenuItem[] { menuItemFileClose } )
                menu_item.Enabled = repository_open;

            foreach( var menu_item in new ToolStripMenuItem[] { menuItemFileExport, menuItemFileSaveAs, menuItemToolsExport } )
                menu_item.Enabled = allow_export;

            menuItemSynchronize.Enabled = ( repository_open && _repository.Syncable );

            // update the languages
            menuItemCaseLanguage.DropDownItems.Clear();

            if( repository_open )
            {
                foreach( var languages in _dictionary.Languages )
                {
                    var menu_item = menuItemCaseLanguage.DropDownItems.Add(languages.Item2);
                    menu_item.Tag = languages.Item1;
                    menu_item.Click += LanguageChange_Click;
                }
            }

            // update the recent files list
            menuItemFileRecent.DropDownItems.Clear();

            foreach( string filename in _settings.RecentFiles.Where(x => File.Exists(x)) )
            {
                // add the shortcut and escape any ampersands in the filename
                int shortcut_number = menuItemFileRecent.DropDownItems.Count + 1;
                string shortcut = ( shortcut_number < 10 )  ? "&" + shortcut_number.ToString() :
                                  ( shortcut_number == 10 ) ? "1&0" :
                                                              shortcut_number.ToString();
                string escaped_filename = Path.GetFileName(filename).Replace("&", "&&");

                var menu_item = menuItemFileRecent.DropDownItems.Add($"{shortcut}: {escaped_filename}");
                menu_item.Tag = filename;
                menu_item.Click += RecentFiles_Click;
            }
        }


        private void MainForm_DragEnter(object sender, DragEventArgs e)
        {
            e.Effect = e.Data.GetDataPresent(DataFormats.FileDrop) ? DragDropEffects.Copy : DragDropEffects.None;
        }

        private void MainForm_DragDrop(object sender, DragEventArgs e)
        {
            foreach( string filename in (string[])e.Data.GetData(DataFormats.FileDrop) )
            {
                if( File.Exists(filename) )
                {
                    OpenFile(filename, null);
                    return;
                }
            }
        }

        private void menuItemFileOpen_Click(object sender, EventArgs e)
        {
            var open_file_dialog = new OpenFileDialog();
            open_file_dialog.Filter = Properties.Resources.DataAndPffFileFilter;

            if( open_file_dialog.ShowDialog() != DialogResult.OK )
                return;

            OpenFile(open_file_dialog.FileName, null);
        }

        private static string GetFullPathWithConnectionStringHandling(string path)
        {
            int pipe_pos = path.IndexOf('|');

            if( pipe_pos < 0 )
                return Path.GetFullPath(path);

            return Path.GetFullPath(path.Substring(0, pipe_pos)) + path.Substring(pipe_pos);
        }

        private void OpenFile(string filename, string dictionary_filename)
        {
            // in case a connection string is passed in with a |, Path.GetExtension is
            // wrapped in a try block to avoid the 'Illegal characters in path.' exception
            try
            {
                if( Path.GetExtension(filename).ToLower() == CSPro.Util.PFF.Extension )
                {
                    OpenPffFile(filename);
                    return;
                }
            }
            catch { }

            OpenDataFile(new CSPro.Util.ConnectionString(filename), dictionary_filename);
        }

        private void OpenDataFile(CSPro.Util.ConnectionString connection_string, string dictionary_filename)
        {
            // if the dictionary isn't supplied, see if there is an embedded dictionary that can be used
            CSPro.Dictionary.DataDictionary this_dictionary = null;
            string this_dictionary_source_filename = dictionary_filename;

            if( dictionary_filename == null )
            {
                this_dictionary = CSPro.Data.DataRepository.GetEmbeddedDictionary(connection_string);

                if( this_dictionary != null )
                    this_dictionary_source_filename = connection_string.Filename;

                else
                {
                    // don't query about a dictionary for a data file that doesn't exist
                    if( !string.IsNullOrEmpty(connection_string.Filename) && !File.Exists(connection_string.Filename) )
                        return;

                    // when trying to open an Encrypted CSPro DB file, don't query about a dictionary if
                    // the embedded one couldn't be opened (because the person didn't know the password)
                    if( connection_string.Type == CSPro.Util.DataRepositoryType.EncryptedSQLite )
                        return;

                    // if there is none, but if a dictionary has previously been specified, see if they want to keep using it
                    if( _dictionary != null )
                    {
                        if( MessageBox.Show($"To open {System.IO.Path.GetFileName(connection_string.Filename)} you must specify a dictionary. Do you want to keep using {_dictionary.Name}?",
                            "Change Dictionary?", MessageBoxButtons.YesNo) == DialogResult.Yes )
                        {
                            this_dictionary = _dictionary;
                            this_dictionary_source_filename = _dictionarySourceFilename;
                        }
                    }

                    if( this_dictionary == null )
                    {
                        var open_dictionary_dialog = new OpenFileDialog();
                        open_dictionary_dialog.Title = $"Select the dictionary that describes {Path.GetFileName(connection_string.Filename)}";
                        open_dictionary_dialog.Filter = Properties.Resources.DataDictionaryFileFilter;
                        open_dictionary_dialog.InitialDirectory = Path.GetDirectoryName(connection_string.Filename);

                        if( open_dictionary_dialog.ShowDialog() != DialogResult.OK )
                            return;

                        this_dictionary_source_filename = open_dictionary_dialog.FileName;
                    }
                }
            }

            // load the dictionary if necessary
            if( this_dictionary == null )
            {
                try
                {
                    this_dictionary = new CSPro.Dictionary.DataDictionary(this_dictionary_source_filename);
                }

                catch( Exception )
                {
                    MessageBox.Show("There was an error reading the dictionary.");
                    return;
                }
            }

            // open the data file
            CloseFile();

            try
            {
                var this_repository = new CSPro.Data.DataRepository();
                this_repository.OpenForReading(this_dictionary, connection_string);

                _dictionary = this_dictionary;
                _dictionarySourceFilename = this_dictionary_source_filename;
                _repository = this_repository;
                _connectionString = connection_string;
                _controller = new CSPro.Data.DataViewerController(_settings, _repository);

                _settings.AddToRecentFilesList(connection_string.Filename);

                UpdateWindowUI();

                UpdateKeyPrefixPanel();

                _caseListingProcessor = new CaseListingProvider(_repository, listViewCases, panelCaseListing, _settings);
                UpdateCaseListing();

                ShowDataSummaryForm();
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message, "Error Opening Data File");
            }
        }


        private async void OpenPffFile(string pff_filename)
        {
            CSPro.Util.PFF pff = new CSPro.Util.PFF(pff_filename);

            if (!pff.Load())
            {
                MessageBox.Show("Unable to read file %s. Check that this is a valid file from the correct version of CSPro.", pff_filename);
                return;
            }

            if (pff.AppType != CSPro.Util.AppType.SYNC_TYPE)
            {
                MessageBox.Show("Incorrect type of PFF file. Only SYNC files are supported.");
                return;
            }

            var dictNames = pff.ExternalDictionaryNames;
            if (dictNames.Length == 0)
            {
                MessageBox.Show("PFF file missing dictionary name.");
                return;
            }

            var dictionaryName = dictNames[0];

            var downloadConnectionString = pff.GetExternalDataConnectionString(dictionaryName);
            if (downloadConnectionString == null || String.IsNullOrWhiteSpace(downloadConnectionString.Filename))
            {
                var saveFileDialog = new SaveFileDialog();
                saveFileDialog.Filter = FileFilters.CsdbFileFilter;
                saveFileDialog.OverwritePrompt = false;
                saveFileDialog.Title = "Choose data file to download to";
                if (saveFileDialog.ShowDialog() != DialogResult.OK)
                {
                    return;
                }
                downloadConnectionString = new CSPro.Util.ConnectionString(saveFileDialog.FileName);
            }

            bool sync_result = false;
            var progress_dialog = new ProgressDialog();
            progress_dialog.Show(this);
            try
            {
                var synchronizer = new Synchronizer();
                sync_result = await synchronizer.SyncAsync(null, downloadConnectionString,
                    new SyncServerParameters(pff.SyncServerType, pff.SyncUrl),
                    dictionaryName, pff.SyncDirection, progress_dialog.ProgressPercent, progress_dialog.ProgressMessage,
                    progress_dialog.CancellationToken, this);
            } finally
            {
                progress_dialog.Hide();
            }

            if (sync_result)
            {
                _executedPff = pff;

                if (pff.Silent)
                    Close();
                else
                    OpenDataFile(downloadConnectionString, null);
            }
        }


        private void listViewCases_RetrieveVirtualItem(object sender, RetrieveVirtualItemEventArgs e)
        {
            _caseListingProcessor?.RetrieveVirtualItem(sender, e);
        }

        private void UpdateCaseListing()
        {
            _caseListingProcessor?.Refresh();
        }


        private List<ToolStripMenuItem> GetAllMenuItems()
        {
            var all_menu_items = new List<ToolStripMenuItem>();

            Action<ToolStripItemCollection> populate_menu_items = null;
            populate_menu_items = new Action<ToolStripItemCollection>((menu_items) =>
            {
                foreach( var menu_item in menu_items )
                {
                    if( menu_item is ToolStripMenuItem )
                    {
                        var tool_strip_menu_item = (ToolStripMenuItem)menu_item;
                        all_menu_items.Add(tool_strip_menu_item);
                        populate_menu_items(tool_strip_menu_item.DropDownItems);
                    }
                }
            });

            populate_menu_items(menuStrip.Items);
            
            return all_menu_items;
        }

        private void UpdateMenuChecks()
        {
            menuItemListingCaseLabel.Checked = _settings.ShowCaseLabels;
            menuItemListingKey.Checked = !_settings.ShowCaseLabels;

            // get all of the menus
            if( _settingMenuItems == null )
                _settingMenuItems = GetAllMenuItems();

            _settings.UpdateMenuChecks(_settingMenuItems);
        }


        public bool HandleAcceleratorKeyWorker(Keys keyData)
        {
            // populate the accelerator key map if necessary
            if( _acceleratorKeyMap == null )
            {
                _acceleratorKeyMap = new Dictionary<Keys, ToolStripMenuItem>();

                foreach( var menu_item in GetAllMenuItems() )
                {
                    if( menu_item.ShortcutKeys != Keys.None )
                        _acceleratorKeyMap.Add(menu_item.ShortcutKeys, menu_item);
                }
            }

            ToolStripMenuItem triggered_menu_item;

            if( _acceleratorKeyMap.TryGetValue(keyData, out triggered_menu_item) )
            {
                if( triggered_menu_item.Enabled )
                {
                    BeginInvoke(new Action(() => { triggered_menu_item.PerformClick(); }));
                    return true;
                }
            }

            return false;
        }


        private void CaseIterationChange(object sender, EventArgs e)
        {
            _controller.ChangeSetting((ToolStripMenuItem)sender, ModifierKeys.HasFlag(Keys.Control));

            UpdateMenuChecks();

            foreach( var form in Application.OpenForms )
            {
                if( form is LogicHelperForm )
                    ((LogicHelperForm)form).RefreshSettings();
            }

            UpdateCaseListing();
        }

        private void CaseLabelChange(object sender, EventArgs e)
        {
            // if using the shortcut, Ctrl+L, allow toggling the value
            if( ModifierKeys.HasFlag(Keys.Control) )
                _settings.ShowCaseLabels = !_settings.ShowCaseLabels;

            else
                _settings.ShowCaseLabels = ( sender == menuItemListingCaseLabel );

            UpdateMenuChecks();
            _caseListingProcessor?.ToggleShowCaseLabels();
        }


        private void pictureBoxSearch_Click(object sender, EventArgs e)
        {
            if( _settings.ShowKeyPrefixPanel )
            {
                // clear any search
                if( _settings.KeyPrefix != null )
                {
                    _settings.KeyPrefix = null;
                    textBoxKeyPrefix.Text = string.Empty;
                    UpdateCaseListing();
                }
            }

            // toggle the visibility of the key prefix panel
            _settings.ShowKeyPrefixPanel = !_settings.ShowKeyPrefixPanel;

            UpdateKeyPrefixPanel();
        }

        private void UpdateKeyPrefixPanel()
        {
            int desired_list_view_cases_y = panelKeyPrefix.Location.Y + ( _settings.ShowKeyPrefixPanel ? _keyPrefixPanelHeightAdjustment : 0 );

            if( listViewCases.Location.Y != desired_list_view_cases_y )
            {
                listViewCases.Height += listViewCases.Location.Y - desired_list_view_cases_y;
                listViewCases.Location = new Point(listViewCases.Location.X, desired_list_view_cases_y);

                panelKeyPrefix.Visible = _settings.ShowKeyPrefixPanel;
            }
        }

        private void textBoxKeyPrefix_KeyPress(object sender, KeyPressEventArgs e)
        {
            if( e.KeyChar == (char)Keys.Return )
            {
                _settings.KeyPrefix = ( textBoxKeyPrefix.Text.Length == 0 ) ? null : textBoxKeyPrefix.Text;
                UpdateCaseListing();
            }
        }


        private void CaseToHtmlSettingChange(object sender, EventArgs e)
        {
            _controller.ChangeSetting((ToolStripMenuItem)sender, ModifierKeys.HasFlag(Keys.Control));

            UpdateMenuChecks();

            foreach( var form in Application.OpenForms )
            {
                if( form is CaseViewForm )
                    ((CaseViewForm)form).RefreshSettings();
            }
        }

        private void menuItemCaseLanguage_DropDownOpening(object sender, EventArgs e)
        {
            // place a check next to the current language, or change the language
            // to the first one if the current language doesn't exist in the list
            bool language_found = false;

            foreach( ToolStripMenuItem menu_item in menuItemCaseLanguage.DropDownItems )
            {
                if( ((string)menu_item.Tag) == _settings.LanguageName )
                {
                    menu_item.Checked = true;
                    language_found = true;
                }

                else
                    menu_item.Checked = false;
            }

            if( !language_found )
            {
                var menu_item = (ToolStripMenuItem)menuItemCaseLanguage.DropDownItems[0];
                menu_item.Checked = true;
                _settings.LanguageName = (string)menu_item.Tag;
            }
        }

        private void LanguageChange_Click(object sender, EventArgs e)
        {
            _settings.LanguageName = (string)((ToolStripMenuItem)sender).Tag;
            CaseToHtmlSettingChange(null, null);
        }


        private void menuItemFileClose_Click(object sender, EventArgs e)
        {
            CloseFile();
        }

        private void CloseFile()
        {
            CloseAllOpenForms();

            _settings.Reset();
            textBoxKeyPrefix.Text = string.Empty;

            listViewCases.VirtualListSize = 0;
            _caseListingProcessor = null;

            _controller = null;

            if( _repository != null )
                _repository.Close();

            _repository = null;

            UpdateWindowUI();
        }


        private void menuItemFileExport_Click(object sender, EventArgs e)
        {
            var export_data_dialog = new ExportDataForm(this, _settings, _connectionString, _repository);
            export_data_dialog.ShowDialog();
        }

        private void menuItemFileSaveData_Click(object sender, EventArgs e)
        {
            var save_file_dialog = new SaveFileDialog();
            save_file_dialog.Title = "Save Data As";
            save_file_dialog.Filter = Properties.Resources.DataFileFilter;
            save_file_dialog.InitialDirectory = Path.GetDirectoryName(_dictionarySourceFilename);

            if( save_file_dialog.ShowDialog() != DialogResult.OK )
                return;

            var save_data_form = new SaveDataForm(_repository, new CSPro.Util.ConnectionString(save_file_dialog.FileName));
            save_data_form.ShowDialog();
        }


        private void menuItemFileSaveDictionary_Click(object sender, EventArgs e)
        {
            // allow a convenient way to save an embedded dictionary
            var save_dictionary_dialog = new SaveFileDialog();
            save_dictionary_dialog.Title = $"Save Dictionary {_dictionary.Name} As";
            save_dictionary_dialog.Filter = Properties.Resources.DataDictionaryFileFilter;
            save_dictionary_dialog.InitialDirectory = Path.GetDirectoryName(_dictionarySourceFilename);
            save_dictionary_dialog.FileName = _dictionary.Name + ".dcf";

            if( save_dictionary_dialog.ShowDialog() != DialogResult.OK )
                return;

            try
            {
                _dictionary.Save(save_dictionary_dialog.FileName);
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }
        }


        private void menuItemFileSaveNotes_Click(object sender, EventArgs e)
        {
            // allow a way to extract the notes from a data file
            var save_notes_dialog = new SaveFileDialog();
            save_notes_dialog.Title = $"Save {_dictionary.Name} Notes As";
            save_notes_dialog.Filter = Properties.Resources.DataFileFilter;
            save_notes_dialog.InitialDirectory = Path.GetDirectoryName(_dictionarySourceFilename);
            save_notes_dialog.FileName = $"{Path.GetFileNameWithoutExtension(_connectionString.Filename)} Notes{Path.GetExtension(_connectionString.Filename)}";

            if( save_notes_dialog.ShowDialog() != DialogResult.OK )
                return;

            var save_notes_form = new SaveNotesForm(_repository, new CSPro.Util.ConnectionString(save_notes_dialog.FileName));
            save_notes_form.ShowDialog();
        }


        private void RecentFiles_Click(object sender, EventArgs e)
        {
            string filename = (string)((ToolStripMenuItem)sender).Tag;
            OpenFile(filename, null);
        }


        private void menuItemFileExit_Click(object sender, EventArgs e)
        {
            Close();
        }


        private void menuItemToolsExport_Click(object sender, EventArgs e)
        {
            OpenDataInCSExport();
        }

        private void menuItemToolsFrequencies_Click(object sender, EventArgs e)
        {
            OpenDataInCSProTool("CSFreq.exe");            
        }

        public void OpenDataInCSExport()
        {
            OpenDataInCSProTool("CSExport.exe");
        }

        private void OpenDataInCSProTool(string exe_filename)
        {
            try
            {
                string dataviewer_exe_filename = System.Reflection.Assembly.GetExecutingAssembly().Location;
                exe_filename = Path.Combine(Path.GetDirectoryName(dataviewer_exe_filename), exe_filename);

                if( !File.Exists(exe_filename) )
                    throw new Exception("The tool could not be located here: " + exe_filename);

                string argument = _connectionString.TypeContainsEmbeddedDictionary ? _connectionString.ToString() : _dictionarySourceFilename;

                Process process = new Process();
                process.StartInfo = new ProcessStartInfo(exe_filename, '"' + argument + '"');
                process.Start();
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }
        }


        private void menuItemHelpHelp_Click(object sender, EventArgs e)
        {
            Help.ShowHelp(null, "DataViewer.chm");
        }

        private void menuItemHelpAbout_Click(object sender, EventArgs e)
        {
            new WinFormsShared.AboutForm("CSPro Data Viewer",
                "The Data Viewer tool is used to view CSPro data files.",
                Properties.Resources.MainIcon).ShowDialog();
        }


        private List<CSPro.Data.Case> ReadSelectedCases()
        {
            var selected_cases = new List<CSPro.Data.Case>();

            foreach( var index in listViewCases.SelectedIndices )
            {
                var case_summary = (CSPro.Data.CaseSummary)listViewCases.Items[(int)index].Tag;
                selected_cases.Add(_controller.ReadCase(case_summary));
            }

            return selected_cases;
        }


        private void listViewCases_SelectedIndexChanged(object sender, EventArgs e)
        {
            // if only selecting a single case, read the case
            if( listViewCases.SelectedIndices.Count == 1 )
            {
                try
                {
                    var data_case = ReadSelectedCases().First();
                    ShowCaseRelatedForm(data_case);
                }

                catch( Exception exception )
                {
                    MessageBox.Show(exception.Message, "Error Reading Case");
                }
            }
        }

        private void ShowCaseRelatedForm(CSPro.Data.Case data_case)
        {
            // update all forms based on the case that was just read
            Form form_to_be_activated = null;

            foreach( Form form in Application.OpenForms )
            {
                if( form is ICaseProcessingForm )
                {
                    ((ICaseProcessingForm)form).RefreshCase(data_case);

                    if( ( form is CaseViewForm && ((CaseViewForm)form).DefaultViewer ) ||
                        ( form is LogicHelperForm ) )
                    {
                        if( form_to_be_activated == null || form == ActiveMdiChild )
                            form_to_be_activated = form;
                    }
                }
            }

            if( form_to_be_activated != null )
            {
                form_to_be_activated.Activate();
                return;
            }

            // otherwise, open a new default case viewer
            ShowForm(new CaseViewForm(_controller, true, data_case), true);
        }


        private void ShowForm(Form form, bool mdi)
        {
            if( mdi )
            {
                // maximize the window if nothing else is open
                if( this.MdiChildren.Length == 0 )
                    form.WindowState = FormWindowState.Maximized;

                form.MdiParent = this;
            }

            form.Show();
        }


        private void listViewCases_MouseClick(object sender, MouseEventArgs e)
        {
            if( e.Button == MouseButtons.Right && listViewCases.FocusedItem.Bounds.Contains(e.Location) )
            {
                var menu_items = new List<MenuItem>();

                if( listViewCases.SelectedIndices.Count > 1 )
                {
                    menu_items.Add(new MenuItem($"View {listViewCases.SelectedIndices.Count} Cases", ViewCases));
                    menu_items.Add(new MenuItem($"View {listViewCases.SelectedIndices.Count} Cases in Popout Windows", PopoutViewCases));
                }

                else
                {
                    var case_summary = (CSPro.Data.CaseSummary)listViewCases.Items[listViewCases.SelectedIndices[0]].Tag;

                    menu_items.Add(new MenuItem(case_summary.KeyForSingleLineDisplay));
                    menu_items.Add(new MenuItem("-"));

                    menu_items.Add(new MenuItem("Copy Key", CopyKey));

                    // if no case viewer is open, open the default one
                    bool default_viewer_is_open = false;

                    foreach( var form in Application.OpenForms )
                    {
                        if( form is CaseViewForm )
                        {
                            menu_items.Add(new MenuItem("View Case in New Window", ViewCases));
                            default_viewer_is_open = true;
                            break;
                        }
                    }

                    if( !default_viewer_is_open )
                        menu_items.Add(new MenuItem("View Case", ViewCaseInDefaultViewer));
                    
                    menu_items.Add(new MenuItem("View Case in Popout Window", PopoutViewCases));
                }

                var menu = new ContextMenu(menu_items.ToArray());
                menu.Show((Control)sender, new Point(e.X, e.Y));
            }
        }


        private void CopyKey(object sender, EventArgs e)
        {
            if( listViewCases.SelectedIndices.Count == 1 )
            {
                var data_case = ReadSelectedCases().First();
                Clipboard.SetText(data_case.Key);
            }
        }


        private void ViewCases(bool default_viewer, bool mdi)
        {
            try
            {
                foreach( var data_case in ReadSelectedCases() )
                    ShowForm(new CaseViewForm(_controller, default_viewer, data_case), mdi);
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message, "Error Reading Case");
            }
        }

        private void ViewCaseInDefaultViewer(object sender, EventArgs e)
        {
            ViewCases(true, true);
        }

        private void ViewCases(object sender, EventArgs e)
        {
            ViewCases(false, true);
        }

        private void PopoutViewCases(object sender, EventArgs e)
        {
            ViewCases(false, false);
        }


        private void menuItemViewRefresh_Click(object sender, EventArgs e)
        {
            _controller.Refresh();

            UpdateWindowTitle();
            UpdateCaseListing();

            // a case may have been modified or deleted, so refresh every form
            foreach( var form in Application.OpenForms )
            {
                if( form is ICaseProcessingForm )
                {
                    var case_processing_form = (ICaseProcessingForm)form;
                    var current_case = case_processing_form.CurrentCase;

                    if( current_case != null )
                    {
                        var refreshed_case = _controller.ReadRefreshedCase(current_case);
                        ((ICaseProcessingForm)form).UpdateCaseFromRepository(refreshed_case);
                    }
                }
            }
        }


        private bool ActivateForm(Type type)
        {
            foreach( var form in Application.OpenForms )
            {
                if( form.GetType() == type )
                {
                    ((Form)form).Activate();
                    return true;
                }
            }

            return false;
        }


        private void ShowDataSummaryForm()
        {
            if( ActivateForm(typeof(DataSummaryForm)) )
                return;

            try
            {
                ShowForm(new DataSummaryForm(_controller), true);
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message, "Error Getting Data Summary");
            }
        }

        private void menuItemViewDataSummary_Click(object sender, EventArgs e)
        {
            ShowDataSummaryForm();
        }


        private void menuItemViewLogicHelper_Click(object sender, EventArgs e)
        {
            if( ActivateForm(typeof(LogicHelperForm)) )
                return;

            try
            {
                ShowForm(new LogicHelperForm(_controller, ReadSelectedCases().FirstOrDefault()), true);
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message, "Error Reading Case");
            }
        }


        private void menuStrip_ItemAdded(object sender, ToolStripItemEventArgs e)
        {
            // prevent the addition of an icon by the MDI children
            if( e?.Item?.GetType().Name == "SystemMenuItem" )
                menuStrip.Items.RemoveAt(0);
        }

        private void MdiLayoutChange(object sender, EventArgs e)
        {
            LayoutMdi((MdiLayout)((ToolStripMenuItem)sender).Tag);
        }

        private void menuItemWindowCloseAll_Click(object sender, EventArgs e)
        {
            CloseAllOpenForms();
        }

        private void CloseAllOpenForms()
        {
            for( int i = Application.OpenForms.Count - 1; i >= 0; i-- )
            {
                if( Application.OpenForms[i] != this )
                    Application.OpenForms[i].Close();
            }
        }


        private void menuItemFileDownload_Click(object sender, EventArgs e)
        {
            var download_dialog = new DownloadDialog(LoadLastServerSettings());
            if (download_dialog.ShowDialog() == DialogResult.OK)
            {
                SaveLastServerSettings(download_dialog.ServerParameters);
                OpenDataFile(new CSPro.Util.ConnectionString(download_dialog.FilePath), null);
            }
        }

        private async void menuItemSynchronize_Click(object sender, EventArgs e)
        {
            // Default to server from last sync if there was one
            var sync_history = _repository.GetSyncHistory();

            SyncServerParameters server_params = new SyncServerParameters(CSPro.Util.SyncServerType.CSWeb, "");
            CSPro.Util.SyncDirection direction = CSPro.Util.SyncDirection.PUT;

            if (sync_history != null && sync_history.Length > 0)
            {
                var last_sync = sync_history.Last();
                server_params = new SyncServerParameters(last_sync.DeviceName);
                direction = last_sync.Direction;

                // because BOTH is equivalent to a GET and then a PUT, determine if the last sync
                // was likely a BOTH (which will be assumed if the syncs happened within five minutes of each other)
                if (direction == CSPro.Util.SyncDirection.PUT && sync_history.Length > 1)
                {
                    var second_to_last_sync = sync_history[sync_history.Length - 2];
                    if (second_to_last_sync.Direction == CSPro.Util.SyncDirection.GET && second_to_last_sync.DeviceName == last_sync.DeviceName)
                    {
                        if (last_sync.Time.Subtract(second_to_last_sync.Time).TotalMinutes < 5)
                            direction = CSPro.Util.SyncDirection.BOTH;
                    }
                }
            }

            var sync_dialog = new SyncDialog(server_params, direction);
            if (sync_dialog.ShowDialog() == DialogResult.OK)
            {
                var progress_dialog = new ProgressDialog();
                progress_dialog.Show(this);
                try
                {
                    var synchronizer = new Synchronizer();
                    await synchronizer.SyncAsync(_repository, _connectionString, sync_dialog.ServerParameters,
                        _repository.Dictionary.Name, sync_dialog.Direction, progress_dialog.ProgressPercent,
                        progress_dialog.ProgressMessage,
                        progress_dialog.CancellationToken, this);
                }
                finally
                {
                    progress_dialog.Hide();

                    // Reopen in read-only mode so that we don't have sharing
                    // violation when launching tools
                    OpenDataFile(_connectionString, null);
                    // CR_TODO: standardize moving from read-only to writable and back; the above
                    // call isn't ideal because the dictionary is already open and null may trigger
                    // asking for a dictionary again
                }
            }
        }

        private const string InterAppKey = @"Software\U.S. Census Bureau\InterApp";
        private const string LastSyncUrlKey = "Last Sync URL";

        private void SaveLastServerSettings(SyncServerParameters serverParameters)
        {
            RegistryKey key = Registry.CurrentUser.OpenSubKey(InterAppKey, true);
            if (key == null)
            {
                key = Registry.CurrentUser.CreateSubKey(InterAppKey);
            }

            switch (serverParameters.Type)
            {
                case CSPro.Util.SyncServerType.CSWeb:
                case CSPro.Util.SyncServerType.FTP:
                    key.SetValue(LastSyncUrlKey, serverParameters.Url);
                    break;
                case CSPro.Util.SyncServerType.Dropbox:
                    key.SetValue(LastSyncUrlKey, "Dropbox");
                    break;
            }
        }

        private SyncServerParameters LoadLastServerSettings()
        {
            try
            {
                RegistryKey key = Registry.CurrentUser.OpenSubKey(InterAppKey, false);
                if (key != null)
                {
                    var keyVal = key.GetValue(LastSyncUrlKey);
                    if (keyVal != null && keyVal is string)
                    {
                        string lastUrl = (string)keyVal;
                        return new SyncServerParameters(lastUrl);
                    }
                }
            }
            catch (Exception)
            {
            }
            return null;
        }
    }
}
