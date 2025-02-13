using System;
using System.Data;
using System.IO;
using System.Linq;
using System.Windows.Forms;
using CSPro.Util;
using System.Threading.Tasks;
using System.IO.Compression;
using Microsoft.WindowsAPICodePack.Dialogs;
using Community.Windows.Forms;
using WinFormsShared;

namespace CSDeploy
{
    public partial class MainForm : Form
    {
        private string m_specFilename;
        private PFF m_executedPff;
        private bool RunningPff { get { return m_executedPff != null; } }

        private string m_savedTitle;
        private MenuStripMRUManager menuStripMRUManager = new MenuStripMRUManager();

        public MainForm()
        {
            InitializeComponent();

            menuStripMRUManager.RecentFileMenuItem = recentFilesToolStripMenuItem;
            menuStripMRUManager.RecentFileMenuItemClick += new System.Action<string>(menuStripMRUManager_RecentFileMenuItemClick);

            // This fixes the artifacts when resizing window introduced by making application DPI aware
            ResizeRedraw = true;
            DoubleBuffered = true;

            fileTreeControl.AddFolderFilter = IsPffOrEntFile;
            fileTreeControl.OnFilesModified += OnFilesModified;

            loadLastServerSettings();

            m_savedTitle = this.Text;

        }

        private static bool IsPffOrEntFile(string f)
        {
            var extension = Path.GetExtension(f).ToLower();
            bool validExtension = false;

            if( extension == ".ent" )
                validExtension = true;

            else if( extension == PFF.Extension )
            {
                try
                {
                    // Only include entry pff files
                    var pff = new PFF(f);
                    validExtension = ( pff.Load() && pff.AppType == AppType.ENTRY_TYPE );
                }

                catch (Exception)
                {
                }
            }

            return validExtension;
        }

        private void OpenSpecFile(string specFilename)
        {
            DeploymentSpecFile spec = DeploymentSpecFile.Load(specFilename);


            textBoxPackageName.Text = spec.Name;
            textBoxDescription.Text = spec.Description;
            var specFilePath = Path.GetDirectoryName(specFilename);

            checkedListBoxDictionaries.Items.Clear();
            if (spec.Dictionaries != null)
            {
                foreach (var dict in spec.Dictionaries)
                {
                    var dictRef = new CSPro.Dictionary.DictionaryDescription { Path = Path.GetFullPath(Path.Combine(specFilePath, dict.Path)) };
                    checkedListBoxDictionaries.Items.Add(dictRef);
                    checkedListBoxDictionaries.SetItemChecked(checkedListBoxDictionaries.Items.IndexOf(dictRef),
                        dict.UploadForSync);
                }
            }

            fileTreeControl.Files = spec.Files.Select(f => new FileSpec { Path = Path.GetFullPath(Path.Combine(specFilePath, f.Path)), OnlyOnFirstInstall = f.OnlyOnFirstInstall });

            switch (spec.Deployment.Type)
            {
                case "CSWeb":
                    radioButtonDeployCSWeb.Checked = true;
                    break;
                case "Dropbox":
                    radioButtonDeployDropbox.Checked = true;
                    break;
                case "FTP":
                    radioButtonDeployFTP.Checked = true;
                    break;
                case "LocalFile":
                    radioButtonDeployFile.Checked = true;
                    break;
                case "LocalFolder":
                    radioButtonDeployFolder.Checked = true;
                    break;
            }

            textBoxCSWebURL.Text = spec.Deployment.CSWebUrl;
            textBoxFtpServerUrl.Text = spec.Deployment.FtpUrl;
            textBoxZipFile.Text = !String.IsNullOrEmpty(spec.Deployment.LocalFilePath) ? Path.GetFullPath(Path.Combine(specFilePath, spec.Deployment.LocalFilePath)) : null;
            textBoxOutputFolder.Text = !String.IsNullOrEmpty(spec.Deployment.LocalFolderPath) ? Path.GetFullPath(Path.Combine(specFilePath, spec.Deployment.LocalFolderPath)) : null;

            m_specFilename = specFilename;
            ModifyTitle();

            menuStripMRUManager.AddRecentFile(specFilename);
            Properties.Settings.Default.Save();
        }

        private void openToolStripMenuItem_Click(object sender, EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Title = Messages.SpecFileOpenTitle;
            ofd.Filter = Messages.SpecFileFilter;

            if (ofd.ShowDialog() == DialogResult.OK)
            {
                try
                {
                    OpenSpecFile(ofd.FileName);
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Error opening file: " + ex.Message);
                }
            }
        }

        private void menuStripMRUManager_RecentFileMenuItemClick(string filename)
        {
            try
            {
                OpenSpecFile(filename);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error opening file: " + ex.Message);
                menuStripMRUManager.RemoveRecentFile(filename);
                Properties.Settings.Default.Save();
            }
        }

        private void saveToolStripMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                if (m_specFilename == null)
                    saveAsToolStripMenuItem_Click(sender, e);
                else
                    SaveSpecFile(m_specFilename);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error saving file: " + ex.Message);
            }
        }

        private void saveAsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            SaveFileDialog sfd = new SaveFileDialog();
            sfd.Title = Messages.SpecFileSaveTitle;
            sfd.Filter = Messages.SpecFileFilter;

            if (sfd.ShowDialog() == DialogResult.OK)
            {
                SaveSpecFile(sfd.FileName);
                m_specFilename = sfd.FileName;
                ModifyTitle();
            }
        }

        private void ModifyTitle()
        {
            if (m_specFilename == null)
                this.Text = m_savedTitle;
            else
                this.Text = String.Format("{0} - {1}", Path.GetFileNameWithoutExtension(m_specFilename), m_savedTitle);
        }

        private DeploymentSpecFile CreateSpecFile(string rootPath)
        {
            var spec = new DeploymentSpecFile();
            spec.Name = textBoxPackageName.Text;
            spec.Description = textBoxDescription.Text;

            spec.Files = fileTreeControl.Files
                .Select(f => new FileSpec { Path = Paths.MakeRelativeFilename(rootPath, f.Path), OnlyOnFirstInstall=f.OnlyOnFirstInstall})
                .OrderBy(f => f.Path)
                .ToArray();

            spec.Dictionaries = checkedListBoxDictionaries.Items.Cast<CSPro.Dictionary.DictionaryDescription>()
                .Select(r => new DeploymentSpecFile.DictionarySync { Path = Paths.MakeRelativeFilename(rootPath, r.Path), UploadForSync = checkedListBoxDictionaries.CheckedItems.Contains(r)  })
                .OrderBy(r => r.Path)
                .ToArray();

            var deploymentType = Deployment;
            spec.Deployment.Type = deploymentType.ToString();
            spec.Deployment.CSWebUrl = String.IsNullOrEmpty(textBoxCSWebURL.Text) ? null : textBoxCSWebURL.Text;
            spec.Deployment.FtpUrl = String.IsNullOrEmpty(textBoxFtpServerUrl.Text) ? null : textBoxFtpServerUrl.Text;
            if (!String.IsNullOrEmpty(textBoxOutputFolder.Text))
                spec.Deployment.LocalFolderPath = Paths.MakeRelativeFilename(rootPath, textBoxOutputFolder.Text);
            if (!String.IsNullOrEmpty(textBoxZipFile.Text))
                spec.Deployment.LocalFilePath = Paths.MakeRelativeFilename(rootPath, textBoxZipFile.Text);

            return spec;
        }

        private void SaveSpecFile(string specFilename)
        {
            var spec = CreateSpecFile(specFilename);
            spec.Save(specFilename);

            saveLastServerSettings();

            // save a PFF file for this spec file if one doesn't already exist
            string pffFilename = Path.GetFileNameWithoutExtension(specFilename) + PFF.Extension;
            pffFilename = Path.Combine(Path.GetDirectoryName(specFilename),pffFilename);

            if( !File.Exists(pffFilename) )
            {
                PFF pff = new PFF();
                pff.AppType = AppType.DEPLOY_TYPE;
                pff.AppFName = specFilename;
                pff.Save(pffFilename);
            }

            menuStripMRUManager.AddRecentFile(specFilename);
            Properties.Settings.Default.Save();
        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void aboutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            new WinFormsShared.AboutForm("CSPro Deploy Application",
                "The Deploy Application tool is used to deploy applications to a server so that they can be downloaded onto interviewer devices.",
                Properties.Resources.MainIcon).ShowDialog();
        }

        private void helpTopicsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Help.ShowHelp(null, "CSDeploy.chm");
        }

        private void textBoxPackageName_TextChanged(object sender, EventArgs e)
        {
            if (!String.IsNullOrEmpty(textBoxPackageName.Text))
                errorProvider.SetError(textBoxPackageName, null);
        }

        private void textBoxOutputFile_TextChanged(object sender, EventArgs e)
        {
            if (!String.IsNullOrEmpty(textBoxOutputFolder.Text))
                errorProvider.SetError(textBoxOutputFolder, null);
        }

        private void textBoxZipFile_TextChanged(object sender, EventArgs e)
        {
            if (!String.IsNullOrEmpty(textBoxZipFile.Text))
                errorProvider.SetError(textBoxZipFile, null);
        }

        private void textBoxFtpServerUrl_TextChanged(object sender, EventArgs e)
        {
            if (IsValidServerUrl(textBoxFtpServerUrl.Text))
                errorProvider.SetError(textBoxFtpServerUrl, null);
        }

        private void textBoxCSWebURL_TextChanged(object sender, EventArgs e)
        {
            if (IsValidServerUrl(textBoxCSWebURL.Text))
                errorProvider.SetError(textBoxCSWebURL, null);
        }

        private void OnFilesModified(object sender, EventArgs e)
        {
            errorProvider.SetError(fileTreeControl, null);
            UpdateDictionaries();
        }

        private bool ValidateAllFields()
        {
            return ValidatePackageName() && ValidateFiles() && ValidateDeployment();
        }

        private bool ValidatePackageName()
        {
            if (String.IsNullOrEmpty(textBoxPackageName.Text))
            {
                errorProvider.SetError(textBoxPackageName, Messages.ErrorMissingPackageName);
                MessageBox.Show(Messages.ErrorMissingPackageName);
                return false;
            }
            return true;
        }

        private bool ValidateFiles()
        {

            if (fileTreeControl.Files.Count() == 0)
            {
                errorProvider.SetError(fileTreeControl, Messages.ErrorNoInputFiles);
                MessageBox.Show(Messages.ErrorNoInputFiles);
                return false;
            }

            if (fileTreeControl.Files.Count(f => Path.GetExtension(f.Path).ToLower() == PFF.Extension) == 0)
            {
                if (MessageBox.Show(Messages.NoPffFiles, m_savedTitle, MessageBoxButtons.YesNo) == DialogResult.No)
                    return false;
            }

            if (fileTreeControl.Files.Count(f => Path.GetExtension(f.Path).ToLower() == ".ent") == 0)
            {
                if (MessageBox.Show(Messages.NoEntFiles, m_savedTitle, MessageBoxButtons.YesNo) == DialogResult.No)
                    return false;
            }

            return true;
        }
        private bool ValidateDeployment()
        {
            switch (Deployment)
            {
                case DeploymentType.CSWeb:
                    if (!IsValidServerUrl(textBoxCSWebURL.Text))
                    {
                        errorProvider.SetError(textBoxCSWebURL, Messages.ErrorInvalidServerURL);
                        MessageBox.Show(Messages.ErrorInvalidServerURL);
                        return false;
                    }
                    break;
                case DeploymentType.FTP:
                    if (!IsValidServerUrl(textBoxFtpServerUrl.Text))
                    {
                        errorProvider.SetError(textBoxFtpServerUrl, Messages.ErrorInvalidServerURL);
                        MessageBox.Show(Messages.ErrorInvalidServerURL);
                        return false;
                    }
                    break;
                case DeploymentType.LocalFile:
                    if (String.IsNullOrEmpty(textBoxZipFile.Text))
                    {
                        errorProvider.SetError(textBoxZipFile, Messages.ErrorNoOutputFile);
                        MessageBox.Show(Messages.ErrorNoOutputFile);
                        return false;
                    }
                    else
                    {
                        // If just filename is specified without directory add
                        // add default path
                        if (String.IsNullOrEmpty(Path.GetDirectoryName(textBoxZipFile.Text)))
                        {
                            textBoxZipFile.Text = Path.Combine(fileTreeControl.RootPath, textBoxZipFile.Text);
                        }

                        if (Path.GetExtension(textBoxZipFile.Text).ToLower() != ".zip")
                        {
                            textBoxZipFile.Text += ".zip";
                        }
                    }
                    break;
                case DeploymentType.LocalFolder:
                    if (String.IsNullOrEmpty(textBoxOutputFolder.Text))
                    {
                        errorProvider.SetError(textBoxOutputFolder, Messages.ErrorNoOutputFolder);
                        MessageBox.Show(Messages.ErrorNoOutputFolder);
                        return false;
                    }
                    else
                    {
                        // If just directory is specified without full path then add default path
                        if (String.IsNullOrEmpty(Path.GetDirectoryName(textBoxOutputFolder.Text)))
                        {
                            textBoxOutputFolder.Text = Path.Combine(fileTreeControl.RootPath, textBoxOutputFolder.Text);
                        }
                    }
                    break;
                case DeploymentType.None:
                    errorProvider.SetError(radioButtonDeployFile, Messages.ErrorNoDeploymentType);
                    MessageBox.Show(Messages.ErrorNoDeploymentType);
                    return false;
            }

            return true;
        }

        private async Task DeployPackage()
        {
            if (!ValidateAllFields())
                return;

            var tempFilePath = Path.GetTempFileName();
            ProgressDialog progressDialog = new ProgressDialog();
            progressDialog.Show(this);

            try
            {
                var specFile = CreateSpecFile(fileTreeControl.RootPath);
                BuildPackage(specFile, tempFilePath);

                if (Deployment == DeploymentType.LocalFile)
                {
                    DeployToZipFile(tempFilePath, textBoxZipFile.Text);
                }
                else if (Deployment == DeploymentType.LocalFolder)
                {
                    DeployToLocalFolder(tempFilePath, textBoxOutputFolder.Text);
                }
                else
                {
                    await DeployToServer(tempFilePath, progressDialog, specFile);
                }

                saveLastServerSettings();

                if (RunningPff) // close if running from a PFF
                    Close();
            }

            finally
            {
                progressDialog.Hide();
                File.Delete(tempFilePath);
            }
        }

        private void BuildPackage(DeploymentSpecFile specFile, string destinationPackagePath)
        {
            PackageBuilder builder = new PackageBuilder();
            builder.SetRootFolder(fileTreeControl.RootPath)
                .SetBundleSpec(specFile)
                .SetOutputFile(destinationPackagePath);

            builder.Build();

            // Builder, by launching CSEntry, causes the app to get minimized, this brings it back
            Activate();
        }

        private async Task DeployToServer(string tempFilePath, ProgressDialog progressDialog, DeploymentSpecFile specFile)
        {
            PackageUploader uploader = new PackageUploader(progressDialog.ProgressPercent, progressDialog.ProgressMessage, progressDialog.CancellationToken);
            PackageUploader.ServerType serverType = (Deployment == DeploymentType.CSWeb) ?
                PackageUploader.ServerType.CSWeb : (Deployment == DeploymentType.Dropbox) ?
                PackageUploader.ServerType.Dropbox : PackageUploader.ServerType.FTP;
            var dictionariesToUpload = checkedListBoxDictionaries.CheckedItems.Cast<CSPro.Dictionary.DictionaryDescription>().Select(r => r.Path);
            var url = Deployment == DeploymentType.CSWeb ? textBoxCSWebURL.Text.Trim() : textBoxFtpServerUrl.Text.Trim();
            bool ok = await uploader.UploadPackage(tempFilePath, textBoxPackageName.Text, specFile.SaveToString(),
                                serverType, url, dictionariesToUpload, this);
            progressDialog.Hide();

            if (ok && !RunningPff)
                MessageBox.Show(Messages.UploadSuccess);
        }

        private void DeployToZipFile(string packagePath, string destinationPath)
        {
            if (File.Exists(destinationPath))
                File.Delete(destinationPath);

            if (!Directory.Exists(Path.GetDirectoryName(destinationPath)))
            {
                Directory.CreateDirectory(Path.GetDirectoryName(destinationPath));
            }

            File.Move(packagePath, destinationPath);

            if (!RunningPff)
                System.Diagnostics.Process.Start("explorer.exe", "/select, \"" + destinationPath + "\"");
        }

        private void DeployToLocalFolder(string packagePath, string destinationFolder)
        {
            if (!Directory.Exists(destinationFolder))
            {
                Directory.CreateDirectory(destinationFolder);
            }

            using (ZipArchive archive = ZipFile.OpenRead(packagePath))
            {
                foreach (ZipArchiveEntry entry in archive.Entries)
                {
                    var destFilePath = Path.Combine(destinationFolder, entry.FullName);
                    var destFolder = Path.GetDirectoryName(destFilePath);
                    if (!Directory.Exists(destFolder))
                        Directory.CreateDirectory(destFolder);
                    entry.ExtractToFile(Path.Combine(destinationFolder, entry.FullName), true);
                }
            }

            if (!RunningPff)
                System.Diagnostics.Process.Start("explorer.exe", "\"" + destinationFolder + "\"");
        }

        private async void buttonDeployPackage_ClickAsync(object sender, EventArgs e)
        {
            try
            {
                await DeployPackage();
            }

            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        private enum DeploymentType
        {
            None, CSWeb, Dropbox, FTP, LocalFile, LocalFolder
        }

        private DeploymentType Deployment
        {
            get
            {
                if (radioButtonDeployCSWeb.Checked)
                    return DeploymentType.CSWeb;
                else if (radioButtonDeployDropbox.Checked)
                    return DeploymentType.Dropbox;
                else if (radioButtonDeployFTP.Checked)
                    return DeploymentType.FTP;
                else if (radioButtonDeployFile.Checked)
                    return DeploymentType.LocalFile;
                else if (radioButtonDeployFolder.Checked)
                    return DeploymentType.LocalFolder;
                else
                    return DeploymentType.None;
            }

            set
            {
                switch (value)
                {
                    case DeploymentType.CSWeb:
                        radioButtonDeployCSWeb.Checked = true;
                        break;
                    case DeploymentType.Dropbox:
                        radioButtonDeployDropbox.Checked = true;
                        break;
                    case DeploymentType.FTP:
                        radioButtonDeployFTP.Checked = true;
                        break;
                    case DeploymentType.LocalFile:
                        radioButtonDeployFile.Checked = true;
                        break;
                    case DeploymentType.LocalFolder:
                        radioButtonDeployFolder.Checked = true;
                        break;
                }
            }
        }

        private void radioButtonDeployType_CheckedChanged(object sender, EventArgs e)
        {
            textBoxCSWebURL.Enabled = radioButtonDeployCSWeb.Checked;
            textBoxFtpServerUrl.Enabled = radioButtonDeployFTP.Checked;
            textBoxOutputFolder.Enabled = buttonBrowseOutputDirectory.Enabled = radioButtonDeployFolder.Checked;
            textBoxZipFile.Enabled = buttonBrowseZipFile.Enabled = radioButtonDeployFile.Checked;

            errorProvider.SetError(radioButtonDeployFile, null);
            errorProvider.SetError(textBoxCSWebURL, null);
            errorProvider.SetError(textBoxFtpServerUrl, null);
            errorProvider.SetError(textBoxOutputFolder, null);
            errorProvider.SetError(textBoxZipFile, null);
        }

        private static bool IsValidServerUrl(string url)
        {
            if (url == null)
                return false;

            var scheme = getUrlScheme(url);
            if (scheme.Length == 0)
                return false;

            // Check for a path after the scheme (and the "://")
            if (url.Length <= scheme.Length + 3)
                return false;

            return true;
        }

        private static string getUrlScheme(string url)
        {
            if (url == null)
                return "";

            int sepPos = url.IndexOf("://");
            if (sepPos < 0)
            {
                return "";
            }
            else
            {
                return url.Substring(0, sepPos);
            }
        }

        private void buttonBrowseOutputDirectory_Click(object sender, EventArgs e)
        {
            CommonOpenFileDialog fdlg = new CommonOpenFileDialog();
            fdlg.Title = Messages.PackageFileSaveTitle;
            fdlg.IsFolderPicker = true;
            if (fdlg.ShowDialog() == CommonFileDialogResult.Ok)
            {
                textBoxOutputFolder.Text = fdlg.FileName;
            }
        }

        private void buttonBrowseZipFile_Click(object sender, EventArgs e)
        {
            SaveFileDialog fdlg = new SaveFileDialog();
            fdlg.Title = Messages.PackageFileSaveTitle;
            if (!String.IsNullOrEmpty(textBoxZipFile.Text))
            {
                fdlg.FileName = textBoxZipFile.Text;
            } else
            {
                if (!String.IsNullOrEmpty(textBoxPackageName.Text))
                {
                    fdlg.FileName = Util.replaceInvalidFileChars(textBoxPackageName.Text) + ".zip";
                }
            }
            fdlg.Filter = Messages.ZipFileFilter;
            if (fdlg.ShowDialog() == DialogResult.OK)
            {
                textBoxZipFile.Text = fdlg.FileName;
            }
        }

        private const string InterAppKey = @"Software\U.S. Census Bureau\InterApp";
        private const string LastSyncUrlKey = "Last Sync URL";

        private void saveLastServerSettings()
        {
            var key = Microsoft.Win32.Registry.CurrentUser.OpenSubKey(InterAppKey, true);
            if (key == null)
            {
                key = Microsoft.Win32.Registry.CurrentUser.CreateSubKey(InterAppKey);
            }

            switch (Deployment)
            {
                case DeploymentType.CSWeb:
                    if (!String.IsNullOrEmpty(textBoxCSWebURL.Text))
                        key.SetValue(LastSyncUrlKey, textBoxCSWebURL.Text);
                    break;
                case DeploymentType.FTP:
                    if (!String.IsNullOrEmpty(textBoxFtpServerUrl.Text))
                        key.SetValue(LastSyncUrlKey, textBoxFtpServerUrl.Text);
                    break;
                case DeploymentType.Dropbox:
                    key.SetValue(LastSyncUrlKey, "Dropbox");
                    break;
            }
        }

        private void loadLastServerSettings()
        {
            try
            {
                var key = Microsoft.Win32.Registry.CurrentUser.OpenSubKey(InterAppKey, false);
                if (key != null)
                {
                    var keyVal = key.GetValue(LastSyncUrlKey);
                    if (keyVal != null && keyVal is string)
                    {
                        string lastUrl = (string)keyVal;
                        if (lastUrl == "Dropbox")
                        {
                            Deployment = DeploymentType.Dropbox;
                        }
                        else if (lastUrl.StartsWith("http://", StringComparison.OrdinalIgnoreCase) ||
                          lastUrl.StartsWith("https://", StringComparison.OrdinalIgnoreCase))
                        {
                            Deployment = DeploymentType.CSWeb;
                            textBoxCSWebURL.Text = lastUrl;
                        }
                        else if (lastUrl.StartsWith("ftp://", StringComparison.OrdinalIgnoreCase) ||
                                lastUrl.StartsWith("ftps://", StringComparison.OrdinalIgnoreCase) ||
                                lastUrl.StartsWith("ftpes://", StringComparison.OrdinalIgnoreCase))
                        {
                            Deployment = DeploymentType.FTP;
                            textBoxFtpServerUrl.Text = lastUrl;
                        }

                    }
                }
            }
            catch (Exception)
            {
            }
        }

        private async void MainForm_Shown(object sender,EventArgs e)
        {
            Array commandArgs = Environment.GetCommandLineArgs();

            if( commandArgs.Length > 1 )
            {
                try
                {
                    string filename = (string)commandArgs.GetValue(1);

                    if( File.Exists(filename) )
                    {
                        var fileExt = Path.GetExtension(filename).ToLower();

                        if (fileExt == PFF.Extension )
                        {
                            PFF pff = new PFF(filename);
                            if (!pff.Load())
                                throw new Exception("Unable to read PFF file.");

                            if (pff.AppType == AppType.DEPLOY_TYPE )
                            {
                                filename = pff.AppFName;
                                m_executedPff = pff;
                                OpenSpecFile(filename);

                                // overriding the deployment destination
                                if (pff.DeployToOverride != DeployToOverride.None)
                                {
                                    switch (pff.DeployToOverride)
                                    {
                                        case DeployToOverride.CSWeb:
                                            radioButtonDeployCSWeb.Checked = true;
                                            break;
                                        case DeployToOverride.Dropbox:
                                            radioButtonDeployDropbox.Checked = true;
                                            break;
                                        case DeployToOverride.FTP:
                                            radioButtonDeployFTP.Checked = true;
                                            break;
                                        case DeployToOverride.LocalFile:
                                            radioButtonDeployFile.Checked = true;
                                            break;
                                        case DeployToOverride.LocalFolder:
                                            radioButtonDeployFolder.Checked = true;
                                            break;
                                    }
                                }

                                // overriding the server URL
                                if (!string.IsNullOrWhiteSpace(pff.SyncUrl))
                                {
                                    if (radioButtonDeployCSWeb.Checked)
                                        textBoxCSWebURL.Text = pff.SyncUrl;
                                    else if (radioButtonDeployFTP.Checked)
                                        textBoxFtpServerUrl.Text = pff.SyncUrl;
                                }

                                await DeployPackage();
                            }

                            else if (pff.AppType == AppType.ENTRY_TYPE)
                            {
                                textBoxPackageName.Text = Path.GetFileNameWithoutExtension(filename);
                                fileTreeControl.Files = new FileSpec[] {
                                    new FileSpec {Path = filename },
                                    new FileSpec {Path = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(filename), pff.AppFName)) } };
                            }

                            else
                                throw new Exception("Incorrect type of PFF file. Only deployment and entry PFF files are supported.");

                        }

                        else if (fileExt == ".csds")
                        {
                            OpenSpecFile(filename);
                        }
                    }
                }

                catch( Exception ex )
                {
                    MessageBox.Show(ex.Message);
                }
            }
        }

        private void MainForm_FormClosed(object sender,FormClosedEventArgs e)
        {
            if( m_executedPff != null )
                m_executedPff.ExecuteOnExitPff();
        }

        private void UpdateDictionaries()
        {
            // Gather state check of existing dictionaries in control so that it can be preserved
            var previouslyChecked = checkedListBoxDictionaries.Items.Cast<CSPro.Dictionary.DictionaryDescription>()
                .ToDictionary(i => i.Path, i => checkedListBoxDictionaries.CheckedItems.Contains(i));

            checkedListBoxDictionaries.Items.Clear();

            // Get all dictionaries referenced in ent files, excluding working storage
            var allRefs = fileTreeControl.Files
                .Where(f => Path.GetExtension(f.Path).ToLower() == ".ent")
                .SelectMany(f => CSPro.Dictionary.DictionaryDescription.GetFromApplication(f.Path).Where(r => r.Type != "Working")).ToList();

            // Add any dictionaries explicitly added
            allRefs.AddRange(fileTreeControl.Files
                .Where(f => Path.GetExtension(f.Path).ToLower() == ".dcf")
                .Select(x => new CSPro.Dictionary.DictionaryDescription() { Path = x.Path, Type = "External" }));

            // Remove any duplicates (if two ent files reference same dictionary)
            // and determine if any of dictionaries are used as a main (input) dictionary
            var uniqueRefs = allRefs
                .GroupBy(r => r.Path)
                .Select(g => new CSPro.Dictionary.DictionaryDescription { Path = g.Key,
                                                                          Type = g.Any(r => r.Type == "Input") ? "Input" : "External" });

            // Add each of the dictionaries found to list control
            foreach (var dict in uniqueRefs)
            {
                // Skip those already in list box
                if (checkedListBoxDictionaries.Items.Cast<CSPro.Dictionary.DictionaryDescription>().Any(r => r.Path == dict.Path))
                    continue;

                checkedListBoxDictionaries.Items.Add(dict);

                if (previouslyChecked.ContainsKey(dict.Path))
                {
                    // Dict was in list before update - restore old check state
                    if (previouslyChecked[dict.Path])
                        checkedListBoxDictionaries.SetItemChecked(checkedListBoxDictionaries.Items.IndexOf(dict), true);
                }
                else
                {
                    // New entry - by default main dicts are checked
                    if (dict.Type == "Input")
                    {
                        checkedListBoxDictionaries.SetItemChecked(checkedListBoxDictionaries.Items.IndexOf(dict), true);
                    }
                }
            }
        }

        private void buttonBarcode_Click(object sender, EventArgs e)
        {
            if (!ValidatePackageName())
                return;

            String packageName = textBoxPackageName.Text;
            String server;
            switch (Deployment)
            {
                case DeploymentType.CSWeb:
                    server = textBoxCSWebURL.Text;
                    break;
                case DeploymentType.FTP:
                    server = textBoxFtpServerUrl.Text;
                    break;
                case DeploymentType.Dropbox:
                    server = "Dropbox";
                    break;
                default:
                    MessageBox.Show(Messages.ErrorQRCodeNotSupported);
                    return;
            }

            if (!ValidateDeployment())
                return;

            new BarcodeDialog(server, packageName).ShowDialog();
        }
    }
}

