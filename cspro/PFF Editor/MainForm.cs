using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using Microsoft.Win32;
using Microsoft.WindowsAPICodePack.Dialogs;

namespace PFF_Editor
{
    public partial class MainForm : Form
    {
        private const string RegistryKey = @"HKEY_CURRENT_USER\Software\U.S. Census Bureau\Tools\PFF Editor";
        private const string RegistryExpertModeValue = "ExpertMode";

        private const string FileFilter = "Program Information Files (*.pff)|*.pff";
        private static string applicationName;

        private PFF pff;
        private string pffFilename;
        private bool isModified = false;
        private List<TextBox> filenameTextBoxes;
        private Dictionary<TextBox,RepositoryUserControl> textboxRepositoryUserControls;

        private string runpffExecutable;
        
        private string logicContents;
        private MethodInfo colorizerMethod;

        public MainForm()
        {
            InitializeComponent();
            applicationName = this.Text;
            comboBoxObjectType.SelectedIndex = 0;
            textBoxObjectName.Text = "dynamic_pff";

            object expertModeValue = Registry.GetValue(RegistryKey,RegistryExpertModeValue,1);

            if( expertModeValue == null || (int)expertModeValue == 1 )
                menuExpertMode.Checked = true;

            else
            {
                tabControl.Controls.Remove(tabPagePFF);
                tabControl.Controls.Remove(tabPageLogic);
            }
        }

        private void MainForm_Shown(object sender,EventArgs e)
        {
            Array commandArgs = Environment.GetCommandLineArgs();

            if( commandArgs.Length >= 2 )
                LoadFile((string)commandArgs.GetValue(1));

            else // savy requested that the program automatically prompt for a PFF
            {
                OpenFileDialog ofd = new OpenFileDialog();
                ofd.Filter = FileFilter;

                if( ofd.ShowDialog() == DialogResult.OK )
                    LoadFile(ofd.FileName);

                else
                    Close();
            }
        }

        private void MainForm_DragEnter(object sender,DragEventArgs e)
        {
            e.Effect = e.Data.GetDataPresent(DataFormats.FileDrop) ? DragDropEffects.Copy : DragDropEffects.None;
        }

        private void MainForm_DragDrop(object sender,DragEventArgs e)
        {
            Array filenames = (Array)e.Data.GetData(DataFormats.FileDrop);

            foreach( string filename in filenames )
            {
                if( !Directory.Exists(filename) )
                {
                    if( ConfirmFileClose() )
                        LoadFile(filename);

                    return;
                }
            }
        }

        private void MainForm_FormClosing(object sender,FormClosingEventArgs e)
        {
            if( !ConfirmFileClose() )
                e.Cancel = true;
        }

        private void menuOpen_Click(object sender,EventArgs e)
        {
            if( !ConfirmFileClose() )
                return;

            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Filter = FileFilter;

            if( ofd.ShowDialog() == DialogResult.OK )
                LoadFile(ofd.FileName);
        }


        private void SetWindowTitle()
        {
            this.Text = String.Format("{0}{1} - {2}",isModified ? "*" : "",Path.GetFileNameWithoutExtension(pffFilename),applicationName);
        }

        private void LoadFile(string filename)
        {
            try
            {
                var line_errors = new List<string>();

                pff = PFF.Open(filename, line_errors);
                pff.SetVerboseOutput(menuOutputAllOptions.Checked);
                pffFilename = filename;

                isModified = false;
                menuSave.Enabled = true;
                menuSaveAs.Enabled = true;
                menuRun.Enabled = true;
                menuOptions.Enabled = true;
                menuAdd.Enabled = true;
                SetWindowTitle();

                DrawPFF();

                if( line_errors.Count > 0 )
                    MessageBox.Show(String.Format("The file had the following errors:\n\n{0}", String.Join("\n\n", line_errors)));

                tabControl_SelectedIndexChanged(null,null);
            }

            catch( Exception exception )
            {
                MessageBox.Show(String.Format("There was a problem reading {0}:\n\n{1}",filename,exception.Message));
            }
        }

        private void menuSave_Click(object sender,EventArgs e)
        {
            if( !isModified )
                return;

            try
            {
                pff.Save(pffFilename);
                isModified = false;
                SetWindowTitle();
            }

            catch( Exception exception )
            {
                MessageBox.Show(String.Format("There was a problem saving {0}:\n\n{1}",pffFilename,exception.Message));
            }
        }

        private void menuSaveAs_Click(object sender,EventArgs e)
        {
            SaveFileDialog sfd = new SaveFileDialog();
            sfd.Filter = FileFilter;

            if( sfd.ShowDialog() != DialogResult.OK )
                return;

            string originalDirectory = Path.GetDirectoryName(pffFilename);
            string newDirectory = Path.GetDirectoryName(sfd.FileName);

            if( !originalDirectory.Equals(newDirectory) ) // see if relative paths should be adjusted
            {
                bool hasRelativePaths = false;

                foreach( TextBox textBox in filenameTextBoxes )
                {
                    try
                    {
                        if( PathHelper.IsRelative(textBox.Text) )
                        {
                            hasRelativePaths = true;
                            break;
                        }
                    }
                    catch( Exception )
                    {
                    }
                }

                if( hasRelativePaths )
                {
                    if( MessageBox.Show("The original PFF has relative paths but the new PFF is being saved to a different path. Do you want to adjust the paths automatically?",
                        applicationName,MessageBoxButtons.YesNo) == DialogResult.Yes )
                    {
                        foreach( TextBox textBox in filenameTextBoxes )
                        {
                            try
                            {
                                if( PathHelper.IsRelative(textBox.Text) )
                                {
                                    string absoluteFilename = PathHelper.GetAbsolutePath(pffFilename,textBox.Text);
                                    textBox.Text = PathHelper.GetRelativePath(sfd.FileName,absoluteFilename);
                                }
                            }
                            catch( Exception )
                            {
                            }
                        }
                    }
                }
            }

            try
            {
                pff.Save(sfd.FileName);
                isModified = false;
                pffFilename = sfd.FileName;
                SetWindowTitle();
            }

            catch( Exception exception )
            {
                MessageBox.Show(String.Format("There was a problem saving {0}:\n\n{1}",pffFilename,exception.Message));
            }
        }

        private string FindLatestCSProFile(string filename)
        {
            // get the versions of CSPro on the machine
            String programFilesPath = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles);
            DirectoryInfo[] programDirs = new DirectoryInfo(programFilesPath).GetDirectories("CSPro*");

            FileInfo latestFile = null;

            foreach( DirectoryInfo dir in programDirs )
            {
                FileInfo[] files = dir.GetFiles(filename);

                foreach( FileInfo fi in files )
                {
                    if( latestFile == null || fi.LastAccessTimeUtc > latestFile.LastAccessTimeUtc )
                        latestFile = fi;
                }
            }

            return latestFile != null ? latestFile.FullName : null;
        }

        private void menuRun_Click(object sender,EventArgs e)
        {
            if( runpffExecutable == null ) // find the most up to date runpff.exe
            {
                runpffExecutable = FindLatestCSProFile("runpff.exe");

                if( runpffExecutable == null )
                {
                    MessageBox.Show("The CSPro application folder with runpff.exe could not be located.");
                    return;
                }
            }

            if( isModified )
            {
                DialogResult result = MessageBox.Show("Do you want to save the PFF before running it?",applicationName,MessageBoxButtons.YesNoCancel);

                if( result == DialogResult.Yes )
                    menuSave_Click(null,null);

                else if( result == DialogResult.Cancel )
                    return;
            }

            System.Diagnostics.Process.Start(runpffExecutable,String.Format("\"{0}\"",pffFilename));
        }

        private void menuExit_Click(object sender,EventArgs e)
        {
            Close();
        }

        private bool ConfirmFileClose()
        {
            if( isModified )
            {
                DialogResult result = MessageBox.Show("Do you want to save changes to the open PFF file?",applicationName,MessageBoxButtons.YesNoCancel);

                if( result == DialogResult.Yes )
                    menuSave_Click(null,null);

                else if( result == DialogResult.Cancel )
                    return false;
            }

            return true;
        }

        private void menuAbsolutePaths_Click(object sender,EventArgs e)
        {
            foreach( TextBox textBox in filenameTextBoxes )
                textBox.Text = PathHelper.GetAbsolutePath(pffFilename,textBox.Text);

            tabControl_SelectedIndexChanged(null,null);
        }

        private void menuRelativePaths_Click(object sender,EventArgs e)
        {
            foreach( TextBox textBox in filenameTextBoxes )
                textBox.Text = PathHelper.GetRelativePath(pffFilename,textBox.Text);

            tabControl_SelectedIndexChanged(null,null);
        }

        private void menuOutputAllOptions_Click(object sender,EventArgs e)
        {
            menuOutputAllOptions.Checked = !menuOutputAllOptions.Checked;
            pff.SetVerboseOutput(menuOutputAllOptions.Checked);
            tabControl_SelectedIndexChanged(null,null);
        }

        private void menuExpertMode_Click(object sender,EventArgs e)
        {
            menuExpertMode.Checked = !menuExpertMode.Checked;
            Registry.SetValue(RegistryKey,RegistryExpertModeValue,menuExpertMode.Checked ? 1 : 0);

            if( menuExpertMode.Checked )
            {
                tabControl.Controls.Add(tabPagePFF);
                tabControl.Controls.Add(tabPageLogic);
            }

            else
            {
                tabControl.Controls.Remove(tabPagePFF);
                tabControl.Controls.Remove(tabPageLogic);
            }

            DrawPFF();
        }

        private void tabControl_SelectedIndexChanged(object sender,EventArgs e)
        {
            if( pff == null )
                return;

            else if( tabControl.SelectedTab == tabPagePFF )
                textBoxPFFContents.Text = pff.GeneratePFFContents(null);

            else if( tabControl.SelectedTab == tabPageLogic )
                logicContentsParameterChanged(null,null);
        }

        private void buttonCopyPFF_Click(object sender,EventArgs e)
        {
            if( pff != null )
                Clipboard.SetText(textBoxPFFContents.Text);
        }

        private void buttonCopyLogic_Click(object sender,EventArgs e)
        {
            if( pff != null )
                Clipboard.SetText(logicContents);
        }

        private void logicContentsParameterChanged(object sender,EventArgs e)
        {
            checkBoxOptionsAsParameters.Enabled = ( comboBoxObjectType.SelectedIndex == 1 );

            if( pff != null && tabControl.SelectedTab == tabPageLogic )
            {
                var pffCreationOptions = new PFF.PFFCreationOptions(comboBoxObjectType.SelectedIndex == 0, textBoxObjectName.Text,
                    checkBoxRunPFF.Checked, checkBoxOptionsAsParameters.Checked);
                logicContents = pff.GeneratePFFContents(pffCreationOptions);
                ShowLogicContentsAsHtml();
            }
        }

        private void ShowLogicContentsAsHtml()
        {
            string html = null;
            
            // see if the zLogicCLR DLL exists, and if so colorizize the logic
            try
            {
                if( colorizerMethod == null )
                {
                    string exe_directory = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
                    string dll_filename = Path.Combine(exe_directory, "zLogicCLR.dll");
                    var assembly = Assembly.LoadFrom(dll_filename);

                    var colorizer_object = assembly.GetType("CSPro.Logic.Colorizer");
                    colorizerMethod = colorizer_object.GetMethod("LogicToHtml");
                }

                html = (string)colorizerMethod.Invoke(null, new object[] { logicContents });
            }
            catch { }

            // if not, just format the text to look nice as HTML
            if( html == null )
            {
                html = logicContents.Replace("\t", "&nbsp;&nbsp;&nbsp;&nbsp;").Replace("\r\n", "<br />");
                html = $"<html><body><div style='font-family: Consolas, monaco, monospace; font-size:10pt;'>{html}</div></body></html>";
            }

            webBrowserLogicContents.DocumentText = html;
        }

        private void menuAddCustomParameter_Click(object sender,EventArgs e)
        {
            if( new AddParameterForm("Custom Parameter",pff.m_customParameters,true).ShowDialog() == DialogResult.OK )
                DrawPFF(true);
        }

        private void menuAddExternalFile_Click(object sender,EventArgs e)
        {
            if( new AddParameterForm("External Dictionary",pff.m_externalFiles).ShowDialog() == DialogResult.OK )
                DrawPFF(true);
        }

        private void menuAddUserFile_Click(object sender,EventArgs e)
        {
            if( new AddParameterForm("File Handler",pff.m_userFiles).ShowDialog() == DialogResult.OK )
                DrawPFF(true);
        }

        private void menuDataEntryID_Click(object sender,EventArgs e)
        {
            if( new AddParameterForm("Persistent ID",pff.m_dataEntryIds).ShowDialog() == DialogResult.OK )
                DrawPFF(true);
        }

        private void menuAddExportOutputFile_Click(object sender,EventArgs e)
        {
            pff.m_exportedData.Add("");
            DrawPFF(true);
        }

        private void menuAddExportCSPro_Click(object sender,EventArgs e)
        {
            pff.m_CSProDescFile = " ";
            DrawPFF(true);
        }

        private void menuAddExportR_Click(object sender,EventArgs e)
        {
            pff.m_RDescFile = " ";
            DrawPFF(true);
        }

        private void menuAddExportSAS_Click(object sender,EventArgs e)
        {
            pff.m_SASDescFile = " ";
            DrawPFF(true);
        }

        private void menuAddExportSPSS_Click(object sender,EventArgs e)
        {
            pff.m_SPSSDescFile = " ";
            DrawPFF(true);
        }

        private void menuAddExportStataDescription_Click(object sender,EventArgs e)
        {
            pff.m_STATADescFile = " ";
            DrawPFF(true);
        }

        private void menuAddExportStataLabels_Click(object sender,EventArgs e)
        {
            pff.m_STATALabelsFile = " ";
            DrawPFF(true);
        }

        private void menuAddAreaNamesFile_Click(object sender,EventArgs e)
        {
            pff.m_areaNames = " ";
            DrawPFF(true);
        }

        private void menuAddConsolidationInputTab_Click(object sender,EventArgs e)
        {
            pff.m_conInputTABs.Add("");
            DrawPFF(true);
        }

        private void menuAddExtraFile_Click(object sender,EventArgs e)
        {
            pff.m_extraFiles.Add("");
            DrawPFF(true);
        }

        private void menuAddFrequencyReport_Click(object sender, EventArgs e)
        {
            pff.m_freqs = " ";
            DrawPFF(true);
        }

        private void menuAddImputationFrequencyReport_Click(object sender,EventArgs e)
        {
            pff.m_imputeFreqs = " ";
            DrawPFF(true);
        }

        private void menuAddImputationStatistics_Click(object sender, EventArgs e)
        {
            pff.m_imputeStat = " ";
            DrawPFF(true);
        }

        private void menuAddInputDataFile_Click(object sender,EventArgs e)
        {
            pff.m_inputData.Add("");
            DrawPFF(true);
        }

        private void menuAddOutputDataFile_Click(object sender,EventArgs e)
        {
            pff.m_outputData.Add("");
            DrawPFF(true);
        }

        private void menuAddInputParadataFile_Click(object sender, EventArgs e)
        {
            pff.m_inputParadata.Add("");
            DrawPFF(true);
        }

		private void menuAddParadata_Click(object sender,EventArgs e)
		{
            pff.m_paradata = " ";
            DrawPFF(true);
		}

        private void menuAddWriteFile_Click(object sender,EventArgs e)
        {
            pff.m_writeData = " ";
            DrawPFF(true);
        }
        
        private void menuAddSaveArrayFile_Click(object sender, EventArgs e)
        {
            pff.m_saveArrayFilename = " ";
            DrawPFF(true);
        }

        private void menuAddCommonStore_Click(object sender,EventArgs e)
        {
            pff.m_commonStore = " ";
            DrawPFF(true);
        }

        private void menuAddHtmlDialogsDirectory_Click(object sender, EventArgs e)
        {
            pff.m_htmlDialogsDirectory = " ";
            DrawPFF(true);
        }

        private void menuAddBaseMap_Click(object sender,EventArgs e)
        {
            pff.m_baseMap = " ";
            DrawPFF(true);
        }

        private void menuAddOnExit_Click(object sender,EventArgs e)
        {
            pff.m_onExit = " ";
            DrawPFF(true);
        }

        private const int startMarginX = 10;
        private const int startMarginY = 25;
        private const int startLabelX = startMarginX;
        private const int startControlX = 180;
        private const int controlMarginDiffY = -3;
        private const int controlMarginButtonDiffY = -5;
        private const int controlMarginRepositoryDiffY = -4;
        private const int radioButtonMarginX = 35;
        private const int radioButtonMinX = 130;
        private const int buttonWidth = 30;

        private const int marginY = 30;

        private List<GroupBox> groupBoxes;
        private GroupBox groupBox;
        private int panelPosY,groupPosY;

        private void DrawPFF(bool drawAfterModify = false)
        {
            if( drawAfterModify )
                setModified();

            // enable the add parameter options
            menuAddDataEntryID.Enabled = pff.m_appType == PFF.AppType.Entry;
            menuAddExternalFile.Enabled = pff.IsEngineRunningApp() || pff.m_appType == PFF.AppType.Sync;
            menuAddUserFile.Enabled = pff.IsEngineRunningApp();

            menuTabulationFiles.Enabled = pff.m_appType == PFF.AppType.Tabulation;

            if( menuTabulationFiles.Enabled )
            {
                menuAddAreaNamesFile.Enabled = ( pff.m_operation == PFF.Operation.All || pff.m_operation == PFF.Operation.Format ) && pff.m_areaNames.Length == 0;
                menuAddConsolidationInputTab.Enabled = pff.m_operation == PFF.Operation.Con;
            }

            menuExportFiles.Enabled = pff.m_appType == PFF.AppType.Batch || pff.m_appType == PFF.AppType.Export;

            if( menuExportFiles.Enabled )
            {
                menuAddExportOutputFile.Enabled = pff.m_appType == PFF.AppType.Export;
                menuAddExportCSPro.Enabled = pff.m_CSProDescFile.Length == 0;
                menuAddExportR.Enabled = pff.m_RDescFile.Length == 0;
                menuAddExportSAS.Enabled = pff.m_SASDescFile.Length == 0;
                menuAddExportSPSS.Enabled = pff.m_SPSSDescFile.Length == 0;
                menuAddExportStataDescription.Enabled = pff.m_STATADescFile.Length == 0;
                menuAddExportStataLabels.Enabled = pff.m_STATALabelsFile.Length == 0;
            }

            menuAddExtraFile.Enabled = pff.m_appType == PFF.AppType.Pack;
            menuAddFrequencies.Enabled = ( pff.IsEngineRunningApp() && pff.m_freqs.Length == 0 );
            menuAddImputationFrequencies.Enabled = ( pff.IsEngineRunningApp() && pff.m_imputeFreqs.Length == 0 );
            menuAddImputationStatistics.Enabled = ( pff.IsEngineRunningApp() && pff.m_imputeStat.Length == 0 );
            menuAddInputDataFile.Enabled = ( pff.m_appType == PFF.AppType.Entry && pff.m_inputData.Count == 0 ) ||
                ( pff.m_appType == PFF.AppType.Batch || pff.m_appType == PFF.AppType.Concatenate || pff.m_appType == PFF.AppType.Export ||
                  pff.m_appType == PFF.AppType.Frequencies || pff.m_appType == PFF.AppType.Index || pff.m_appType == PFF.AppType.Tabulation );
            menuAddOutputDataFile.Enabled = ( pff.m_appType == PFF.AppType.Batch ) ||
                ( ( pff.m_appType == PFF.AppType.Excel2CSPro || pff.m_appType == PFF.AppType.Index ) && pff.m_outputData.Count == 0 );
			menuAddInputParadataFile.Enabled = pff.m_appType == PFF.AppType.ParadataConcatenate;
			menuAddParadata.Enabled = pff.IsEngineRunningApp() && pff.m_paradata.Length == 0;
            menuAddWriteFile.Enabled = pff.IsEngineRunningApp() && pff.m_writeData.Length == 0;
            menuAddSaveArrayFile.Enabled = pff.IsEngineRunningApp() && pff.m_saveArrayFilename.Length == 0;
            menuAddCommonStore.Enabled = pff.IsEngineRunningApp() && pff.m_commonStore.Length == 0;
            menuAddHtmlDialogsDirectory.Enabled = pff.IsEngineRunningApp() && pff.m_htmlDialogsDirectory.Length == 0;
            menuAddBaseMap.Enabled = pff.IsEngineRunningApp() && pff.m_baseMap.Length == 0;
            menuAddOnExit.Enabled = pff.m_onExit.Length == 0;

            filenameTextBoxes = new List<TextBox>();
            textboxRepositoryUserControls = new Dictionary<TextBox,RepositoryUserControl>();

            panelPFF.Controls.Clear();
            panelPosY = 0;

            groupBoxes = new List<GroupBox>();

            DrawRunInformation();

            if( pff.m_appType == PFF.AppType.Entry )
                DrawDataEntryInit();

            DrawParameters();

            if( pff.m_appType == PFF.AppType.Entry && pff.m_dataEntryIds.Count > 0 )
                DrawDataEntryIds();

            if( pff.m_appType != PFF.AppType.Sync )
                DrawFiles();

            if( ( pff.IsEngineRunningApp() || pff.m_appType == PFF.AppType.Sync ) && pff.m_externalFiles.Count > 0 )
                DrawExternalFiles();

            if( pff.IsEngineRunningApp() && pff.m_userFiles.Count > 0 )
                DrawUserFiles();

            // size all the group boxes again
            // this was previously done in StartGroupBox but that code led to spacing problems on some boxes when a scroll bar was present
            foreach( GroupBox gb in groupBoxes )
                gb.Width = panelPFF.Width - System.Windows.Forms.SystemInformation.VerticalScrollBarWidth - 10;
        }

        private void DrawRunInformation()
        {
            StartGroupBox(PFF.Sections.RunInformation);

            if( menuExpertMode.Checked )
                AddTextBox(VersionModifier,"Version",pff.m_version);

            AddApplicationType();

            if( menuExpertMode.Checked && pff.m_appType == PFF.AppType.Tabulation )
                AddRadioButtons(OperationModifier,"Tabulation Operation",new string[] { "All","Consolidate","Tabulate","Format" },(int)pff.m_operation);

            if( menuExpertMode.Checked || pff.m_appType == PFF.AppType.Entry )
                AddTextBox(DescriptionModifier,"Description",pff.m_description);

            if( menuExpertMode.Checked )
                AddRadioButtons(ShowInApplicationListingModifier,"Show in Application Listing",new string[] { "Always","Hidden By Default","Never" },(int)pff.m_showInApplicationListing);

            EndGroupBox();
        }

        private void DrawDataEntryInit()
        {
            StartGroupBox(PFF.Sections.DataEntryInit);

            AddTextBox(OperatorIDModifier,"Operator ID",pff.m_operatorID);

            AddRadioButtons(StartModeModifier,"Start Mode",new string[] { "Default", PFF.Commands.Add,PFF.Commands.Modify,PFF.Commands.Verify },(int)pff.m_startMode);

            if( pff.m_startMode == PFF.StartMode.Add || pff.m_startMode == PFF.StartMode.Modify )
                AddTextBox(StartModeCaseModifier,"Starting Case ID",pff.m_startCase);

            AddTextBox(KeyModifier,"Key", pff.m_key);

            AddRadioButtons(FullScreenModifier,"Full Screen",new string[] { PFF.Commands.Yes,PFF.Commands.No,"Yes, Without Menus" },(int)pff.m_fullScreen);

            if( menuExpertMode.Checked )
            {
                AddBinaryChoice(AutoAddModifier,"Continuously Add Cases",pff.m_autoAdd);

                AddBinaryChoice(NoFileOpenModifier,"Disable File Open",pff.m_noFileOpen);

                AddRadioButtons(InteractiveModifier,"Interactive Edit Mode",new string[] { "Ask User","Error Messages and Ranges","Error Messages Only","Ranges Only","None" },(int)pff.m_interactive);

                if( pff.m_interactive != PFF.Interactive.Ask )
                    AddBinaryChoice(InteractiveLockModifier,"Lock Interactive Edit Mode",pff.m_interactiveLock);

                AddCheckBoxes(LockModifier, "Lock Features", new string[] { "Add Mode", "Modify Mode", "Verify Mode", "Deleting Cases", "Viewing Cases", "Viewing Operator Statistics", "Viewing the Case Listing" },
                    new bool[] { pff.m_lockAdd, pff.m_lockModify, pff.m_lockVerify, pff.m_lockDelete, pff.m_lockView, pff.m_lockStats, pff.m_lockCaseListing });

                AddTextBox(CaseListingFilterModifier,"Case Listing Filter",pff.m_caseListingFilter);
            }

            EndGroupBox();
        }

        private void DrawParameters()
        {
            StartGroupBox(PFF.Sections.Parameters);

            if( pff.IsEngineRunningApp() )
			{
                AddTextBox(ParameterModifier,"Parameter",pff.m_parameter);
				
				if( menuExpertMode.Checked )
					AddTextBox(LanguageModifier,"Starting Language",pff.m_language);
			}

            foreach( var kp in pff.m_customParameters )
                AddMapping(CustomParametersModifier,CustomParametersDeleter,kp.Key,kp.Value,MappingType.Text);

            if( pff.m_appType != PFF.AppType.Compare && pff.m_appType != PFF.AppType.Deploy && pff.m_appType != PFF.AppType.Entry && 
                pff.m_appType != PFF.AppType.Excel2CSPro && pff.m_appType != PFF.AppType.Index && pff.m_appType != PFF.AppType.Pack && 
                pff.m_appType != PFF.AppType.ParadataConcatenate && pff.m_appType != PFF.AppType.Sync && pff.m_appType != PFF.AppType.View )
            {
                AddBinaryChoice(ViewResultsModifier,"View Run Results",pff.m_viewResults);
            }

            if( pff.m_appType != PFF.AppType.Deploy && pff.m_appType != PFF.AppType.Entry && pff.m_appType != PFF.AppType.Excel2CSPro && pff.m_appType != PFF.AppType.Sync && pff.m_appType != PFF.AppType.View )
                AddRadioButtons(ViewListingModifier,"View Listing File",new string[] { PFF.Commands.Always,"Only on Error",PFF.Commands.Never },(int)pff.m_viewListing);

            if( pff.m_appType == PFF.AppType.Batch )
            {
                AddTextBox(ListingWidthModifier,"Listing File Width",pff.m_listingWidth.ToString());

                if( menuExpertMode.Checked )
                {
                    AddBinaryChoice(MessageWrapModifier,"Wrap Summary Messages",pff.m_messageWrap);

                    AddRadioButtons(ErrmsgOverrideModifier,"Override Error Messages",new string[] { "Default","All Summary","All Case" },(int)pff.m_errmsgOverride);

                    AddCheckBoxes(RunEntryAsBatchModifier,"Run Entry as Batch",new string[] { "Skip Structure","Check Ranges" },new bool[] { pff.m_skipStructure,pff.m_checkRanges });
                }
            }

            if( pff.m_appType == PFF.AppType.Index )
            {
                AddRadioButtons(DuplicateCaseModifier,"Duplicate Case Action", new string[] { PFF.Commands.List, PFF.Commands.View,
                    "Prompt For All", "Prompt for Non-Identical", "Keep First" }, (int)pff.m_duplicateCase);
            }

            if( pff.m_appType == PFF.AppType.Reformat )
                AddBinaryChoice(DisplayNamesModifier,"Display Names in Reports",pff.m_displayNames);

            if( pff.IsEngineRunningApp() && pff.m_appType != PFF.AppType.Entry )
                AddRadioButtons(InputOrderModifier,"Input Order",new string[] { "Sequential","Indexed" },pff.m_inputOrderIndexed ? 1 : 0);

            if( pff.m_appType == PFF.AppType.Concatenate )
                AddRadioButtons(ConcatMethodModifier,"Concatenation Method",new string[] { "Case","Text" },pff.m_concatMethodCase ? 0 : 1);

            if( pff.m_appType == PFF.AppType.Sync )
            {
                AddRadioButtons(SyncTypeModifier,"Sync Type",new string[] { PFF.Commands.CSWeb, PFF.Commands.Dropbox, PFF.Commands.FTP, PFF.Commands.LocalDropbox, PFF.Commands.LocalFiles },(int)pff.m_syncType);
                AddRadioButtons(SyncDirectionModifier,"Sync Direction",new string[] { PFF.Commands.Get,PFF.Commands.Put,PFF.Commands.Both },(int)pff.m_syncDirection);
            }

            if( ( pff.m_appType == PFF.AppType.Sync && pff.m_syncType != PFF.SyncType.Dropbox && pff.m_syncType != PFF.SyncType.LocalDropbox ) ||
                ( pff.m_appType == PFF.AppType.Deploy ) )
            {
                AddMapping(SyncUrlModifier,null,"Sync URL",pff.m_syncUrl,MappingType.Text);
            }

            if ( pff.m_appType == PFF.AppType.Deploy )
            {
                AddRadioButtons(DeployToOverrideModifier, "Deploy to Override", Enum.GetValues(typeof(PFF.DeployToOverride)).Cast<PFF.DeployToOverride>().Select(v => v.ToString()).ToArray(), (int)pff.m_deployToOverride);
            }

            if ( menuExpertMode.Checked && pff.m_appType == PFF.AppType.Sync )
                AddBinaryChoice(SilentModifier,"Run Silently",pff.m_silent);

            if( pff.m_onExit.Length > 0 )
                AddMapping(OnExitModifier,OnExitDeleter,"On Exit PFF",pff.m_onExit,MappingType.File);

            EndGroupBox();
        }


        private void DrawExternalFiles()
        {
            StartGroupBox(PFF.Sections.ExternalFiles);

            foreach( var kp in pff.m_externalFiles )
                AddMapping(ExternalFilesModifier,new EventHandler(ExternalFilesDeleter),kp.Key,kp.Value,MappingType.Repository);

            EndGroupBox();
        }


        private void DrawUserFiles()
        {
            StartGroupBox(PFF.Sections.UserFiles);

            foreach( var kp in pff.m_userFiles )
                AddMapping(UserFilesModifier,UserFilesDeleter,kp.Key,kp.Value,MappingType.File);

            EndGroupBox();
        }


        private void DrawDataEntryIds()
        {
            StartGroupBox(PFF.Sections.DataEntryIds);

            foreach( var kp in pff.m_dataEntryIds )
                AddMapping(DataEntryIdsModifier,DataEntryIdsDeleter,kp.Key,kp.Value,MappingType.Text);

            EndGroupBox();
        }


        private void DrawFiles()
        {
            StartGroupBox(PFF.Sections.Files);

            if( pff.m_appType != PFF.AppType.Concatenate && pff.m_appType != PFF.AppType.Index && pff.m_appType != PFF.AppType.ParadataConcatenate && pff.m_appType != PFF.AppType.Reformat )
                AddMapping(ApplicationModifier,null,"Application",pff.m_application,MappingType.File);

            // input data
            if( pff.m_appType != PFF.AppType.Deploy && pff.m_appType != PFF.AppType.Excel2CSPro &&
                pff.m_appType != PFF.AppType.Pack && pff.m_appType != PFF.AppType.ParadataConcatenate &&
                pff.m_appType != PFF.AppType.View )
            {
                bool inputDataRequired = pff.m_appType != PFF.AppType.Batch && pff.m_appType != PFF.AppType.Entry;

                if( pff.m_inputData.Count == 0 )
                {
                    if( inputDataRequired )
                        AddMapping(InputDataModifier,null,"Input Data File","",MappingType.Repository);
                }

                else
                {
                    bool onlyOneAllowed = pff.m_appType == PFF.AppType.Compare || pff.m_appType == PFF.AppType.Entry ||
                        pff.m_appType == PFF.AppType.Reformat || pff.m_appType == PFF.AppType.Sort;

                    for( int i = 0; i < ( onlyOneAllowed ? 1 : pff.m_inputData.Count ); i++ )
                    {
                        EventHandler deleter = null;

                        if( !inputDataRequired || i > 0 )
                            deleter = InputDataDeleter;

                        AddMapping(InputDataModifier,deleter,( onlyOneAllowed || pff.m_inputData.Count == 1 ) ? "Input Data File" :
                            String.Format("Input Data File ({0})",i + 1),pff.m_inputData[i],MappingType.Repository);
                    }
                }
            }


            if( pff.m_appType == PFF.AppType.Excel2CSPro )
                AddMapping(ExcelModifier,null,"Excel File",pff.m_excel,MappingType.File, "Excel Worksheets (*.xlsx;*.xlsm;*.xlsb;*.xls)|*.xlsx;*.xlsm;*.xlsb;*.xls");


            // output data
            bool outputDataRequired = pff.m_appType == PFF.AppType.Concatenate || 
                                      pff.m_appType == PFF.AppType.Reformat ||
                                      pff.m_appType == PFF.AppType.Sort;

            if( outputDataRequired || pff.m_appType == PFF.AppType.Batch || 
                                      pff.m_appType == PFF.AppType.Excel2CSPro || 
                                      pff.m_appType == PFF.AppType.Index )
            {
                if( outputDataRequired && pff.m_outputData.Count == 0 )
                    AddMapping(OutputDataModifier, null, "Output Data File", "", MappingType.Repository);

                else
                {
                    bool onlyOneAllowed = pff.m_appType != PFF.AppType.Batch;

                    if( onlyOneAllowed && pff.m_outputData.Count > 1 )
                        pff.m_outputData.RemoveRange(1, pff.m_outputData.Count - 1);

                    for( int i = 0; i < pff.m_outputData.Count; i++ )
                    {
                        EventHandler deleter = null;

                        if( !outputDataRequired || i > 0 )
                            deleter = OutputDataDeleter;

                        AddMapping(OutputDataModifier, deleter, 
                            ( onlyOneAllowed || pff.m_outputData.Count == 1 ) ? "Output Data File" : String.Format("Output Data File ({0})", i + 1),
                            ( i < pff.m_outputData.Count ) ? pff.m_outputData[i] : "", 
                            MappingType.Repository);
                    }
                }
            }


            if( pff.m_appType == PFF.AppType.Export )
            {
                for( int i = 0; i < pff.m_exportedData.Count; i++ )
                    AddMapping(ExportedDataModifier,ExportedDataDeleter,String.Format("Export Output File ({0})",i + 1),pff.m_exportedData[i],MappingType.File);
            }

            if( pff.m_appType == PFF.AppType.Tabulation )
            {
                if( pff.m_operation == PFF.Operation.Tab )
                    AddMapping(TabOutputTABModifier,null,"Tabulation Output Tab File",pff.m_tabOutputTAB,MappingType.File);

                if( pff.m_operation == PFF.Operation.Con )
                {
                    if( pff.m_conInputTABs.Count == 0 )
                        AddMapping(ConInputTABModifier,null,"Consolidation Input Tab File (1)","",MappingType.File);

                    else
                    {
                        for( int i = 0; i < pff.m_conInputTABs.Count; i++ )
                            AddMapping(ConInputTABModifier,ConInputTABDeleter,String.Format("Consolidation Input Tab File ({0})",i + 1),pff.m_conInputTABs[i],MappingType.File);
                    }

                    AddMapping(ConOutputTABModifier,null,"Consolidation Output Tab File",pff.m_conOutputTAB,MappingType.File);
                }

                if( pff.m_operation == PFF.Operation.Format )
                    AddMapping(FormatInputTABModifier,null,"Format Input Tab File",pff.m_formatInputTAB,MappingType.File);

                if( pff.m_operation == PFF.Operation.All || pff.m_operation == PFF.Operation.Format )
                {
                    AddMapping(OutputTBWModifier,null,"Output Table File",pff.m_outputTBW,MappingType.File);

                    if( pff.m_areaNames.Length > 0 )
                        AddMapping(AreaNamesModifier,AreaNamesDeleter,"Area Names File",pff.m_areaNames,MappingType.File);
                }
            }

            if( pff.m_appType == PFF.AppType.Excel2CSPro || pff.m_appType == PFF.AppType.Index || pff.m_appType == PFF.AppType.Reformat || ( pff.m_appType == PFF.AppType.Concatenate && pff.m_concatMethodCase ) )
                AddMapping(InputDictModifier,null,"Input Dictionary",pff.m_inputDict,MappingType.File);

            if( pff.m_appType == PFF.AppType.Reformat )
                AddMapping(OutputDictModifier,null,"Output Dictionary",pff.m_outputDict,MappingType.File);

            if( pff.m_appType == PFF.AppType.Compare )
                AddMapping(ReferenceDataModifier,null,"Reference Data File",pff.m_referenceData,MappingType.Repository);

            if( pff.m_appType == PFF.AppType.Batch || pff.m_appType == PFF.AppType.Export )
            {
                if( pff.m_CSProDescFile.Length > 0 )
                    AddMapping(CSProDescFileModifier,CSProDescFileDeleter,"Exported CSPro Dictionary",pff.m_CSProDescFile,MappingType.File);

                if( pff.m_RDescFile.Length > 0 )
                    AddMapping(RDescFileModifier,RDescFileDeleter,"R Description File",pff.m_RDescFile,MappingType.File);

                if( pff.m_SASDescFile.Length > 0 )
                    AddMapping(SASDescFileModifier,SASDescFileDeleter,"SAS Description File",pff.m_SASDescFile,MappingType.File);

                if( pff.m_SPSSDescFile.Length > 0 )
                    AddMapping(SPSSDescFileModifier,SPSSDescFileDeleter,"SPSS Description File",pff.m_SPSSDescFile,MappingType.File);

                if( pff.m_STATADescFile.Length > 0 )
                    AddMapping(STATADescFileModifier,STATADescFileDeleter,"Stata Description File",pff.m_STATADescFile,MappingType.File);

                if( pff.m_STATALabelsFile.Length > 0 )
                    AddMapping(STATALabelsFileModifier,STATALabelsFileDeleter,"Stata Labels File",pff.m_STATALabelsFile,MappingType.File);
            }

            if( pff.m_appType == PFF.AppType.Pack )
            {
                AddMapping(ZipFileModifier,null,"Zip Name",pff.m_zipFile,MappingType.File);

                for( int i = 0; i < pff.m_extraFiles.Count; i++ )
                    AddMapping(ExtraFileModifier,ExtraFileDeleter,String.Format("Extra File for Zip ({0})",i + 1),pff.m_extraFiles[i],MappingType.File);
            }

            const string ParadataFilter = "Paradata Logs (*.cslog)|*.cslog";

            if( pff.m_appType == PFF.AppType.ParadataConcatenate )
            {
                for( int i = 0; i < Math.Max(1, pff.m_inputParadata.Count); i++ )
                {
                    EventHandler deleter = ( i > 0 ) ? InputParadataDeleter : (EventHandler)null;

                    AddMapping(InputParadataModifier, deleter, ( pff.m_inputParadata.Count <= 1 ) ? "Input Paradata File" :
                        String.Format("Input Paradata File ({0})", i + 1),
                        ( pff.m_inputParadata.Count == 0 ) ? "" : pff.m_inputParadata[i], MappingType.File, ParadataFilter);
                }
                
                AddMapping(OutputParadataModifier, null, "Output Paradata Log", pff.m_outputParadata, MappingType.File, ParadataFilter);
            }

            if( pff.IsEngineRunningApp() && pff.m_paradata.Length > 0 )
                AddMapping(ParadataModifier,ParadataDeleter,"Paradata Log",pff.m_paradata,MappingType.File,ParadataFilter);

            if( pff.m_appType != PFF.AppType.Deploy && pff.m_appType != PFF.AppType.Excel2CSPro && pff.m_appType != PFF.AppType.View )
                AddMapping(ListingModifier,null,"Listing Report",pff.m_listing,MappingType.File);

            if( pff.IsEngineRunningApp() && pff.m_freqs.Length > 0 )
                AddMapping(FreqsModifier, FreqsDeleter, "Frequencies Report", pff.m_freqs, MappingType.File);

            if( pff.IsEngineRunningApp() && pff.m_imputeFreqs.Length > 0 )
                AddMapping(ImputeFreqsModifier, ImputeFreqsDeleter, "Imputation Frequencies Report", pff.m_imputeFreqs, MappingType.File);

            if( pff.IsEngineRunningApp() && pff.m_imputeStat.Length > 0 )
                AddMapping(ImputeStatModifier, ImputeStatDeleter, "Imputation Statistics Data File", pff.m_imputeStat, MappingType.Repository);

            if( pff.IsEngineRunningApp() && pff.m_writeData.Length > 0 )
                AddMapping(WriteDataModifier,WriteDataDeleter,"Write File",pff.m_writeData,MappingType.File);

            if( pff.IsEngineRunningApp() && pff.m_saveArrayFilename.Length > 0 )
            {
                AddMapping(SaveArrayModifier, SaveArrayDeleter,"Save Array File", pff.m_saveArrayFilename, MappingType.File,
                    "Save Array Files (*.sva)|*.sva");
            }

            if( pff.IsEngineRunningApp() && pff.m_commonStore.Length > 0 )
                AddMapping(CommonStoreModifier,CommonStoreDeleter,"Common Store",pff.m_commonStore,MappingType.File);

            if( pff.IsEngineRunningApp() && pff.m_htmlDialogsDirectory.Length > 0 )
                AddMapping(HtmlDialogsModifier,HtmlDialogsDeleter,"HTML Dialogs Directory",pff.m_htmlDialogsDirectory,MappingType.Directory);

            if( pff.IsEngineRunningApp() && pff.m_baseMap.Length > 0 )
            {
                AddMapping(BaseMapModifier,BaseMapDeleter,"Base Map",pff.m_baseMap,MappingType.File, 
                    "All map files (*.mbtiles,*.tpk)|*.mbtiles;*.tpk|Mapbox mbtiles files (*.mbtiles)|*.mbtiles|ArcGIS tile packages (*.tpk)|*.tpk");
            }

            EndGroupBox();
        }


        private void StartGroupBox(string label)
        {
            groupBox = new GroupBox();
            groupBox.Text = label;
            groupBox.Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top;
            groupBox.Location = new Point(0,panelPosY);
            groupBox.Width = panelPFF.Width - System.Windows.Forms.SystemInformation.VerticalScrollBarWidth;

            panelPFF.Controls.Add(groupBox);
            groupPosY = startMarginY;

            groupBoxes.Add(groupBox);
        }


        private void EndGroupBox()
        {
            groupBox.Height = groupPosY;
            panelPosY += groupPosY + marginY / 2;
        }



        delegate void ValueModifierFunction(Control sender,AttributeTag tag);

        class AttributeTag
        {
            public AttributeTag(ValueModifierFunction valueModifier, object value, string fileMask)
            {
                ValueModifier = valueModifier;
                Value = value;
                FileMask = fileMask;
            }

            public AttributeTag(ValueModifierFunction valueModifier, object value)
            {
                ValueModifier = valueModifier;
                Value = value;
            }

            public AttributeTag(ValueModifierFunction valueModifier)
            {
                ValueModifier = valueModifier;
            }

            public ValueModifierFunction ValueModifier;
            public object Value;
            public string FileMask;
        }


        private void AddLabel(string textLabel)
        {
            Label label = new Label();
            label.Text = textLabel;
            label.Location = new Point(startLabelX,groupPosY);
            label.AutoSize = true;
            groupBox.Controls.Add(label);
        }



        private void AddTextBox(ValueModifierFunction valueModifierFunction,string textLabel,string text)
        {
            AddLabel(textLabel);

            TextBox textBox = new TextBox();
            textBox.Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top;
            textBox.Location = new Point(startControlX,groupPosY + controlMarginDiffY);
            textBox.Width = groupBox.Width - startControlX - startMarginX;

            textBox.Text = text;
            textBox.Tag = new AttributeTag(valueModifierFunction);
            textBox.TextChanged += new EventHandler(valueChanged);

            groupBox.Controls.Add(textBox);

            groupPosY += marginY;
        }


        private void AddApplicationType()
        {
            AddLabel("Application Type");

            ComboBox comboBox = new ComboBox();
            comboBox.DropDownStyle = ComboBoxStyle.DropDownList;
			comboBox.Width = (int)( comboBox.Width * 1.2 );
            comboBox.Location = new Point(startControlX,groupPosY + controlMarginDiffY);

            PFF.AppType appType = PFF.AppType.Batch;

            while( true )
            {
                int item = comboBox.Items.Add(appType.ToString());

                if( pff.m_appType == appType )
                    comboBox.SelectedIndex = item;

                if( appType == PFF.AppType.View )
                    break;

                appType = (PFF.AppType)((int)appType + 1);
            }

            comboBox.Tag = new AttributeTag(AppTypeModifier);
            comboBox.SelectedIndexChanged += new EventHandler(valueChanged);

            if( !menuExpertMode.Checked )
                comboBox.Enabled = false;

            groupBox.Controls.Add(comboBox);

            groupPosY += marginY;
        }


        private void AddRadioButtons(ValueModifierFunction valueModifierFunction,string textLabel,string[] options,int selection)
        {
            AddLabel(textLabel);

            Panel panel = new Panel();
            panel.Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top;
            panel.Location = new Point(startControlX,groupPosY + controlMarginDiffY);
            panel.Width = groupBox.Width;
            panel.Height = marginY;
            groupBox.Controls.Add(panel);

            int posX = 0;

            for( int i = 0; i < options.Length; i++ )
            {
                RadioButton rb = new RadioButton();
                rb.Location = new Point(posX,0);

                rb.Text = options[i];
                rb.AutoSize = true;
                rb.Checked = i == selection;
                rb.Tag = new AttributeTag(valueModifierFunction,i);
                rb.CheckedChanged += new EventHandler(valueChanged);

                panel.Controls.Add(rb);

                posX += Math.Max(radioButtonMinX,rb.Width + radioButtonMarginX);
            }

            groupPosY += marginY;
        }


        private void AddBinaryChoice(ValueModifierFunction valueModifierFunction,string textLabel,bool selection)
        {
            AddRadioButtons(valueModifierFunction,textLabel,new string[] { PFF.Commands.Yes,PFF.Commands.No },selection ? 0 : 1);
        }


        private void AddCheckBoxes(ValueModifierFunction valueModifierFunction,string textLabel,string[] options,bool[] selections)
        {
            AddLabel(textLabel);

            Panel panel = new Panel();
            panel.Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top;
            panel.Location = new Point(startControlX,groupPosY + controlMarginDiffY);
            panel.Width = groupBox.Width;
            panel.Height = marginY;
            groupBox.Controls.Add(panel);

            const int MaximumCheckboxesOnRow = 4;

            int posX = 0;
            int posY = 0;

            for( int i = 0; i < options.Length; i++ )
            {
                if( i > 0 && i % MaximumCheckboxesOnRow == 0 ) // add a new row
                {
                    posX = 0;
                    posY += marginY;
                    panel.Height += marginY;
                }

                CheckBox cb = new CheckBox();
                cb.Location = new Point(posX,posY);

                cb.Text = options[i];
                cb.AutoSize = true;
                cb.Checked = selections[i];
                cb.Tag = new AttributeTag(valueModifierFunction,i);
                cb.CheckedChanged += new EventHandler(valueChanged);

                panel.Controls.Add(cb);

                posX += Math.Max(radioButtonMinX,cb.Width + radioButtonMarginX);
            }

            groupPosY += panel.Height;
        }

        private enum MappingType { Text, File, Directory, Repository };

        private void AddMapping(ValueModifierFunction valueModifierFunction,EventHandler deleteFunction,string textLabel,string text,MappingType mappingType, string fileMask = null)
        {
            AddLabel(textLabel);

            AttributeTag tag = new AttributeTag(valueModifierFunction, textLabel, fileMask);

            if( deleteFunction != null )
            {
                Button button = new Button();
                button.Anchor = AnchorStyles.Right | AnchorStyles.Top;
                button.Location = new Point(groupBox.Width - buttonWidth - startMarginX,groupPosY + controlMarginButtonDiffY);
                button.Width = buttonWidth;

                // button.Text = "Remove"; // "Delete";
                button.Image = PFF_Editor.Properties.Resources.DeleteButton;
                button.Tag = tag;
                button.Click += deleteFunction;

                groupBox.Controls.Add(button);
            }

            int spacingForButtons = buttonWidth + 2 * startMarginX;

            TextBox textBox = new TextBox();

            if( mappingType == MappingType.File || mappingType == MappingType.Directory || mappingType == MappingType.Repository )
            {
                Button button = new Button();
                button.Anchor = AnchorStyles.Right | AnchorStyles.Top;
                button.Location = new Point(groupBox.Width - spacingForButtons - buttonWidth,groupPosY + controlMarginButtonDiffY);
                button.Width = buttonWidth;

                // button.Text = "Browse";
                button.Image = PFF_Editor.Properties.Resources.OpenButton;
                button.Tag = textBox;

                if( mappingType == MappingType.Directory )
                    button.Click += new EventHandler(browseDirectory);

                else
                    button.Click += new EventHandler(browseFile);

                groupBox.Controls.Add(button);
                filenameTextBoxes.Add(textBox);

                spacingForButtons += buttonWidth + startMarginX;
            }

            if( mappingType == MappingType.Repository )
            {
                RepositoryUserControl repositoryUserControl = new RepositoryUserControl(textBox,text,setModified);
                repositoryUserControl.Anchor = AnchorStyles.Right | AnchorStyles.Top;
                repositoryUserControl.Location = new Point(groupBox.Width - spacingForButtons - repositoryUserControl.Width,groupPosY + controlMarginRepositoryDiffY);

                groupBox.Controls.Add(repositoryUserControl);
                textboxRepositoryUserControls.Add(textBox,repositoryUserControl);

                spacingForButtons += repositoryUserControl.Width + startMarginX;
            }

            textBox.Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top;
            textBox.Location = new Point(startControlX,groupPosY + controlMarginDiffY);
            textBox.Width = groupBox.Width - startControlX - spacingForButtons;

            textBox.Text = text;
            textBox.Tag = tag;
            textBox.TextChanged += new EventHandler(valueChanged);

            groupBox.Controls.Add(textBox);

            groupPosY += marginY;
        }


        private void setModified()
        {
            if( !isModified )
            {
                isModified = true;
                SetWindowTitle();
            }
        }


        private void valueChanged(object sender,EventArgs e)
        {
            setModified();

            Control control = (Control)sender;
            AttributeTag tag = (AttributeTag)control.Tag;

            bool sendChangeMessage = true;

            if( control is RadioButton )
            {
                if( !( (RadioButton)control ).Checked ) // only send the message once for the group
                    sendChangeMessage = false;
            }

            else if( control is TextBox )
            {
                TextBox textBox = (TextBox)control;
                RepositoryUserControl repositoryUserControl;

                if( textboxRepositoryUserControls.TryGetValue(textBox,out repositoryUserControl) )
                    repositoryUserControl.UpdateRepositoryType();
            }

            if( sendChangeMessage )
                tag.ValueModifier(control,tag);
        }


        string deleteMappingBase(object sender)
        {
            if( !isModified )
            {
                isModified = true;
                SetWindowTitle();
            }

            Control control = (Control)sender;
            AttributeTag tag = (AttributeTag)control.Tag;
            return (string)tag.Value;
        }

        void browseDirectory(object sender, EventArgs e)
        { 
            Control control = (Control)sender;
            TextBox textBox = (TextBox)control.Tag;

            var dlg = new CommonOpenFileDialog();
            dlg.IsFolderPicker = true;

            bool pathWasRelative = false;

            try
            {
                pathWasRelative = PathHelper.IsRelative(textBox.Text);
                dlg.InitialDirectory = PathHelper.GetAbsolutePath(pffFilename, textBox.Text);
            }

            catch( Exception )
            {
            }

            if( dlg.ShowDialog() != CommonFileDialogResult.Ok )
                return;
            
            textBox.Text = pathWasRelative ? PathHelper.GetRelativePath(pffFilename, dlg.FileName) : dlg.FileName;
            DrawPFF();
        }

        void browseFile(object sender,EventArgs e)
        {
            Control control = (Control)sender;
            TextBox textBox = (TextBox)control.Tag;
            AttributeTag tag = (AttributeTag)textBox.Tag;

            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Filter = tag.FileMask == null ? "All Files (*.*)|*.*" : tag.FileMask;
            ofd.CheckFileExists = false;
            ofd.CheckPathExists = false;

            bool pathWasRelative = false;

            try
            {
                pathWasRelative = PathHelper.IsRelative(textBox.Text);
                string filename = PathHelper.GetAbsolutePath(pffFilename,textBox.Text);
                string directoryName = Path.GetDirectoryName(filename);

                if( Directory.Exists(directoryName) )
                {
                    ofd.InitialDirectory = directoryName;
                    ofd.FileName = Path.GetFileName(filename);
                }
            }

            catch( Exception )
            {
            }

            List<String> multipleSelectionDestination = null;

            if( tag.ValueModifier == ExtraFileModifier )
                multipleSelectionDestination = pff.m_extraFiles;

            ofd.Multiselect = multipleSelectionDestination != null;

            if( ofd.ShowDialog() == DialogResult.OK )
            {
                if( ofd.FileNames.Length == 1 )
                    textBox.Text = pathWasRelative ? PathHelper.GetRelativePath(pffFilename,ofd.FileName) : ofd.FileName;

                else
                {
                    textBox.Text = pathWasRelative ? PathHelper.GetRelativePath(pffFilename,ofd.FileNames[0]) : ofd.FileNames[0]; // modify the first one

                    // then add all the other ones
                    for( int i = 1; i < ofd.FileNames.Length; i++ )
                        multipleSelectionDestination.Add(pathWasRelative ? PathHelper.GetRelativePath(pffFilename,ofd.FileNames[i]) : ofd.FileNames[i]);

                    DrawPFF();
                }
            }
        }


        private void VersionModifier(object sender,AttributeTag tag)
        {
            pff.m_version = ( (TextBox)sender ).Text;
        }

        private void AppTypeModifier(object sender,AttributeTag tag)
        {
            ComboBox comboBox = (ComboBox)sender;
            pff.m_appType = (PFF.AppType)comboBox.SelectedIndex;
            DrawPFF(); // redraw the options based on the new selection
        }

        private void OperationModifier(object sender,AttributeTag tag)
        {
            pff.m_operation = (PFF.Operation)tag.Value;
            DrawPFF();
        }

        private void DescriptionModifier(object sender,AttributeTag tag)
        {
            pff.m_description = ( (TextBox)sender ).Text;
        }

        private void OperatorIDModifier(object sender,AttributeTag tag)
        {
            pff.m_operatorID = ( (TextBox)sender ).Text;
        }

        private void StartModeModifier(object sender,AttributeTag tag)
        {
            pff.m_startMode = (PFF.StartMode)tag.Value;
            DrawPFF();
        }

        private void StartModeCaseModifier(object sender,AttributeTag tag)
        {
            pff.m_startCase = ( (TextBox)sender ).Text;
        }

        private void KeyModifier(object sender, AttributeTag tag)
        {
            pff.m_key = ( (TextBox)sender ).Text;
        }

        private void LanguageModifier(object sender,AttributeTag tag)
        {
            pff.m_language = ( (TextBox)sender ).Text;
        }

        private void FullScreenModifier(object sender,AttributeTag tag)
        {
            pff.m_fullScreen = (PFF.FullScreen)tag.Value;
        }

        private void AutoAddModifier(object sender,AttributeTag tag)
        {
            pff.m_autoAdd = ( (int)tag.Value ) == 0;
        }

        private void NoFileOpenModifier(object sender,AttributeTag tag)
        {
            pff.m_noFileOpen = ( (int)tag.Value ) == 0;
        }

        private void InteractiveModifier(object sender,AttributeTag tag)
        {
            pff.m_interactive = (PFF.Interactive)tag.Value;
            DrawPFF();
        }

        private void InteractiveLockModifier(object sender,AttributeTag tag)
        {
            pff.m_interactiveLock = ( (int)tag.Value ) == 0;
        }

        private void LockModifier(object sender,AttributeTag tag)
        {
            switch( (int)tag.Value )
            {
                case 0:
                    pff.m_lockAdd = !pff.m_lockAdd;
                    break;

                case 1:
                    pff.m_lockModify = !pff.m_lockModify;
                    break;

                case 2:
                    pff.m_lockVerify = !pff.m_lockVerify;
                    break;

                case 3:
                    pff.m_lockDelete = !pff.m_lockDelete;
                    break;

                case 4:
                    pff.m_lockView = !pff.m_lockView;
                    break;

                case 5:
                    pff.m_lockStats = !pff.m_lockStats;
                    break;

                case 6:
                    pff.m_lockCaseListing = !pff.m_lockCaseListing;
                    break;
            }
        }

        private void CaseListingFilterModifier(object sender,AttributeTag tag)
        {
            pff.m_caseListingFilter = ( (TextBox)sender ).Text;
        }
        
        private void ShowInApplicationListingModifier(object sender,AttributeTag tag)
        {
            pff.m_showInApplicationListing = (PFF.ShowInApplicationListing)tag.Value;
        }

        private void ParameterModifier(object sender,AttributeTag tag)
        {
            pff.m_parameter = ( (TextBox)sender ).Text;
        }

        private void ViewResultsModifier(object sender,AttributeTag tag)
        {
            pff.m_viewResults = ( (int)tag.Value ) == 0;
        }

        private void ViewListingModifier(object sender,AttributeTag tag)
        {
            pff.m_viewListing = (PFF.ViewListing)tag.Value;
        }

        private void ListingWidthModifier(object sender,AttributeTag tag)
        {
            int width;

            if( Int32.TryParse(( (TextBox)sender ).Text,out width) )
                pff.m_listingWidth = width;
        }

        private void MessageWrapModifier(object sender,AttributeTag tag)
        {
            pff.m_messageWrap = ( (int)tag.Value ) == 0;
        }

        private void ErrmsgOverrideModifier(object sender,AttributeTag tag)
        {
            pff.m_errmsgOverride = (PFF.ErrmsgOverride)tag.Value;
        }

        private void RunEntryAsBatchModifier(object sender,AttributeTag tag)
        {
            switch( (int)tag.Value )
            {
                case 0:
                    pff.m_skipStructure = !pff.m_skipStructure;
                    break;

                case 1:
                    pff.m_checkRanges = !pff.m_checkRanges;
                    break;
            }
        }

        private void SilentModifier(object sender,AttributeTag tag)
        {
            pff.m_silent = ( (int)tag.Value ) == 0;
        }

        private void DuplicateCaseModifier(object sender,AttributeTag tag)
        {
            pff.m_duplicateCase = (PFF.DuplicateCase)tag.Value;
        }

        private void DisplayNamesModifier(object sender,AttributeTag tag)
        {
            pff.m_displayNames = ( (int)tag.Value ) == 0;
        }

        private void InputOrderModifier(object sender,AttributeTag tag)
        {
            pff.m_inputOrderIndexed = ( (int)tag.Value ) == 1;
        }

        private void ConcatMethodModifier(object sender,AttributeTag tag)
        {
            pff.m_concatMethodCase = ( (int)tag.Value ) == 0;
            DrawPFF(true);
        }

        private void SyncTypeModifier(object sender,AttributeTag tag)
        {
            pff.m_syncType = (PFF.SyncType)tag.Value;
            DrawPFF(true);
        }

        private void SyncDirectionModifier(object sender,AttributeTag tag)
        {
            pff.m_syncDirection = (PFF.SyncDirection)tag.Value;
        }

        private void SyncUrlModifier(object sender,AttributeTag tag)
        {
            pff.m_syncUrl = ( (TextBox)sender ).Text;
        }

        private void DeployToOverrideModifier(object sender, AttributeTag tag)
        {
            pff.m_deployToOverride = (PFF.DeployToOverride)tag.Value;
        }

        private void ExternalFilesModifier(object sender,AttributeTag tag)
        {
            string key = (string)tag.Value;
            pff.m_externalFiles[key] = ( (TextBox)sender ).Text;
        }

        private void ExternalFilesDeleter(object sender,EventArgs e)
        {
            pff.m_externalFiles.Remove(deleteMappingBase(sender));
            DrawPFF(true);
        }

        private void UserFilesModifier(object sender,AttributeTag tag)
        {
            string key = (string)tag.Value;
            pff.m_userFiles[key] = ( (TextBox)sender ).Text;
        }

        private void UserFilesDeleter(object sender,EventArgs e)
        {
            pff.m_userFiles.Remove(deleteMappingBase(sender));
            DrawPFF(true);
        }

        private void DataEntryIdsModifier(object sender,AttributeTag tag)
        {
            string key = (string)tag.Value;
            pff.m_dataEntryIds[key] = ( (TextBox)sender ).Text;
        }

        private void DataEntryIdsDeleter(object sender,EventArgs e)
        {
            pff.m_dataEntryIds.Remove(deleteMappingBase(sender));
            DrawPFF(true);
        }

        private void CustomParametersModifier(object sender,AttributeTag tag)
        {
            string key = (string)tag.Value;
            pff.m_customParameters[key] = ( (TextBox)sender ).Text;
        }

        private void CustomParametersDeleter(object sender,EventArgs e)
        {
            pff.m_customParameters.Remove(deleteMappingBase(sender));
            DrawPFF(true);
        }


        private static int GetIndexFromLabel(string label)
        {
            int startNumber = label.LastIndexOf('(') + 1; // if no parenthesis, then the first element
            return startNumber <= 0 ? 0 : ( Convert.ToInt32(label.Substring(startNumber,label.Length - startNumber - 1)) - 1 );
        }

        private void ApplicationModifier(object sender,AttributeTag tag)
        {
            pff.m_application = ( (TextBox)sender ).Text;
        }

        private void InputDataModifier(object sender,AttributeTag tag)
        {
            if( pff.m_inputData.Count == 0 )
                pff.m_inputData.Add("");

            pff.m_inputData[GetIndexFromLabel((string)tag.Value)] = ( (TextBox)sender ).Text;
        }

        private void InputDataDeleter(object sender,EventArgs e)
        {
            pff.m_inputData.RemoveAt(GetIndexFromLabel(deleteMappingBase(sender)));
            DrawPFF(true);
        }

        private void ExcelModifier(object sender,AttributeTag tag)
        {
            pff.m_excel = ( (TextBox)sender ).Text;
        }

        private void OutputDataModifier(object sender,AttributeTag tag)
        {
            if( pff.m_outputData.Count == 0 )
                pff.m_outputData.Add("");

            pff.m_outputData[GetIndexFromLabel((string)tag.Value)] = ( (TextBox)sender ).Text;
        }

        private void OutputDataDeleter(object sender,EventArgs e)
        {
            pff.m_outputData.RemoveAt(GetIndexFromLabel(deleteMappingBase(sender)));
            DrawPFF(true);
        }

        private void ExportedDataModifier(object sender,AttributeTag tag)
        {
            pff.m_exportedData[GetIndexFromLabel((string)tag.Value)] = ( (TextBox)sender ).Text;
        }

        private void ExportedDataDeleter(object sender,EventArgs e)
        {
            pff.m_exportedData.RemoveAt(GetIndexFromLabel(deleteMappingBase(sender)));
            DrawPFF(true);
        }

        private void TabOutputTABModifier(object sender,AttributeTag tag)
        {
            pff.m_tabOutputTAB = ( (TextBox)sender ).Text;
        }

        private void ConInputTABModifier(object sender,AttributeTag tag)
        {
            if( pff.m_conInputTABs.Count == 0 )
                pff.m_conInputTABs.Add("");

            pff.m_conInputTABs[GetIndexFromLabel((string)tag.Value)] = ( (TextBox)sender ).Text;
        }

        private void ConInputTABDeleter(object sender,EventArgs e)
        {
            pff.m_conInputTABs.RemoveAt(GetIndexFromLabel(deleteMappingBase(sender)));
            DrawPFF(true);
        }

        private void ConOutputTABModifier(object sender,AttributeTag tag)
        {
            pff.m_conOutputTAB = ( (TextBox)sender ).Text;
        }

        private void FormatInputTABModifier(object sender,AttributeTag tag)
        {
            pff.m_formatInputTAB = ( (TextBox)sender ).Text;
        }

        private void OutputTBWModifier(object sender,AttributeTag tag)
        {
            pff.m_outputTBW = ( (TextBox)sender ).Text;
        }

        private void AreaNamesModifier(object sender,AttributeTag tag)
        {
            pff.m_areaNames = ( (TextBox)sender ).Text;
        }

        private void AreaNamesDeleter(object sender,EventArgs e)
        {
            pff.m_areaNames = "";
            DrawPFF(true);
        }

        private void InputDictModifier(object sender,AttributeTag tag)
        {
            pff.m_inputDict = ( (TextBox)sender ).Text;
        }

        private void OutputDictModifier(object sender,AttributeTag tag)
        {
            pff.m_outputDict = ( (TextBox)sender ).Text;
        }

        private void ReferenceDataModifier(object sender,AttributeTag tag)
        {
            pff.m_referenceData = ( (TextBox)sender ).Text;
        }

        private void CSProDescFileModifier(object sender,AttributeTag tag)
        {
            pff.m_CSProDescFile= ( (TextBox)sender ).Text;
        }

        private void CSProDescFileDeleter(object sender,EventArgs e)
        {
            pff.m_CSProDescFile = "";
            DrawPFF(true);
        }

        private void RDescFileModifier(object sender,AttributeTag tag)
        {
            pff.m_RDescFile = ( (TextBox)sender ).Text;
        }

        private void RDescFileDeleter(object sender,EventArgs e)
        {
            pff.m_RDescFile = "";
            DrawPFF(true);
        }

        private void SASDescFileModifier(object sender,AttributeTag tag)
        {
            pff.m_SASDescFile = ( (TextBox)sender ).Text;
        }

        private void SASDescFileDeleter(object sender,EventArgs e)
        {
            pff.m_SASDescFile = "";
            DrawPFF(true);
        }

        private void SPSSDescFileModifier(object sender,AttributeTag tag)
        {
            pff.m_SPSSDescFile = ( (TextBox)sender ).Text;
        }

        private void SPSSDescFileDeleter(object sender,EventArgs e)
        {
            pff.m_SPSSDescFile = "";
            DrawPFF(true);
        }

        private void STATADescFileModifier(object sender,AttributeTag tag)
        {
            pff.m_STATADescFile = ( (TextBox)sender ).Text;
        }

        private void STATADescFileDeleter(object sender,EventArgs e)
        {
            pff.m_STATADescFile = "";
            DrawPFF(true);
        }

        private void STATALabelsFileModifier(object sender,AttributeTag tag)
        {
            pff.m_STATALabelsFile = ( (TextBox)sender ).Text;
        }

        private void STATALabelsFileDeleter(object sender,EventArgs e)
        {
            pff.m_STATALabelsFile = "";
            DrawPFF(true);
        }

        private void ZipFileModifier(object sender,AttributeTag tag)
        {
            pff.m_zipFile = ( (TextBox)sender ).Text;
        }

        private void ExtraFileModifier(object sender,AttributeTag tag)
        {
            pff.m_extraFiles[GetIndexFromLabel((string)tag.Value)] = ( (TextBox)sender ).Text;
        }

        private void ExtraFileDeleter(object sender,EventArgs e)
        {
            pff.m_extraFiles.RemoveAt(GetIndexFromLabel(deleteMappingBase(sender)));
            DrawPFF(true);
        }

        private void InputParadataModifier(object sender,AttributeTag tag)
        {
            if( pff.m_inputParadata.Count == 0 )
                pff.m_inputParadata.Add("");

            pff.m_inputParadata[GetIndexFromLabel((string)tag.Value)] = ( (TextBox)sender ).Text;
        }
        
        private void InputParadataDeleter(object sender,EventArgs e)
        {
            pff.m_inputParadata.RemoveAt(GetIndexFromLabel(deleteMappingBase(sender)));
            DrawPFF(true);
        }

        private void OutputParadataModifier(object sender,AttributeTag tag)
        {
            pff.m_outputParadata = ( (TextBox)sender ).Text;
        }

        private void ParadataModifier(object sender,AttributeTag tag)
        {
            pff.m_paradata = ( (TextBox)sender ).Text;
        }

        private void ParadataDeleter(object sender,EventArgs e)
        {
            pff.m_paradata = "";
            DrawPFF(true);
        }
 
		private void ListingModifier(object sender,AttributeTag tag)
        {
            pff.m_listing = ( (TextBox)sender ).Text;
        }

        private void FreqsModifier(object sender,AttributeTag tag)
        {
            pff.m_freqs = ( (TextBox)sender ).Text;
        }

        private void FreqsDeleter(object sender,EventArgs e)
        {
            pff.m_freqs = "";
            DrawPFF(true);
        }

        private void ImputeFreqsModifier(object sender,AttributeTag tag)
        {
            pff.m_imputeFreqs = ( (TextBox)sender ).Text;
        }

        private void ImputeFreqsDeleter(object sender,EventArgs e)
        {
            pff.m_imputeFreqs = "";
            DrawPFF(true);
        }

        private void ImputeStatModifier(object sender,AttributeTag tag)
        {
            pff.m_imputeStat = ( (TextBox)sender ).Text;
        }

        private void ImputeStatDeleter(object sender,EventArgs e)
        {
            pff.m_imputeStat = "";
            DrawPFF(true);
        }

        private void WriteDataModifier(object sender,AttributeTag tag)
        {
            pff.m_writeData = ( (TextBox)sender ).Text;
        }

        private void WriteDataDeleter(object sender,EventArgs e)
        {
            pff.m_writeData = "";
            DrawPFF(true);
        }

        private void SaveArrayModifier(object sender,AttributeTag tag)
        {
            pff.m_saveArrayFilename = ( (TextBox)sender ).Text;
        }

        private void SaveArrayDeleter(object sender,EventArgs e)
        {
            pff.m_saveArrayFilename = "";
            DrawPFF(true);
        }

        private void CommonStoreModifier(object sender,AttributeTag tag)
        {
            pff.m_commonStore = ( (TextBox)sender ).Text;
        }

        private void CommonStoreDeleter(object sender,EventArgs e)
        {
            pff.m_commonStore = "";
            DrawPFF(true);
        }

        private void HtmlDialogsModifier(object sender,AttributeTag tag)
        {
            pff.m_htmlDialogsDirectory = ( (TextBox)sender ).Text;
        }

        private void HtmlDialogsDeleter(object sender,EventArgs e)
        {
            pff.m_htmlDialogsDirectory = "";
            DrawPFF(true);
        }

        private void BaseMapModifier(object sender,AttributeTag tag)
        {
            pff.m_baseMap = ( (TextBox)sender ).Text;
        }

        private void BaseMapDeleter(object sender,EventArgs e)
        {
            pff.m_baseMap = "";
            DrawPFF(true);
        }

        private void OnExitModifier(object sender,AttributeTag tag)
        {
            pff.m_onExit = ( (TextBox)sender ).Text;
        }

        private void OnExitDeleter(object sender,EventArgs e)
        {
            pff.m_onExit = "";
            DrawPFF(true);
        }


        // set tab stops to a width of 4, modified from http://stackoverflow.com/questions/1298406/how-to-set-the-tab-width-in-a-windows-forms-textbox-control
        private const int EM_SETTABSTOPS = 0x00CB;

        [DllImport("User32.dll",CharSet = CharSet.Auto)]
        public static extern IntPtr SendMessage(IntPtr h,int msg,int wParam,int[] lParam);

        public static void SetTabWidth(TextBox textbox)
        {
            SendMessage(textbox.Handle,EM_SETTABSTOPS,1,new int[] { 16 });
        }







        private static string[] HelpFiles = {
                                                "CSPro.chm",
                                                "CSDiff.chm",
                                                "CSConcat.chm",
												"CSDeploy.chm",
                                                "CSPro.chm",
												"Excel2CSPro.chm",
                                                "CSExport.chm",
                                                "CSFreq.chm",
                                                "CSIndex.chm",
                                                "CSPack.chm",
												"ParadataConcat.chm",
                                                "CSRefmt.chm",
                                                "CSSort.chm",
                                                "DataViewer.chm",
                                                "CSPro.chm",
                                                "CSView.chm",
                                            };

        private static string[] TopicFiles = {
                                                "run_production_batch_edits.html",
                                                "run_production_compares.html",
                                                "run_production_concatenates.html",
												"run_production_deployments.html",
                                                "run_production_data_entry.html",
												"running_conversions_from_the_command_line.html",
                                                "run_production_exports.html",
                                                "run_production_frequencies.html",
                                                "run_csindex_in_production_mode.html",
                                                "run_production_pack.html",
												"run_production_paradata_concatenates.html",
                                                "run_production_reformats.html",
                                                "run_production_sorts.html",
                                                "run_production_synchronizations.html",
                                                "run_tabulate_in_batch.html",
                                                "run_production_views.html",
                                            };

        private void LaunchHelp(PFF.AppType appType)
        {
            string helpFilename = FindLatestCSProFile(HelpFiles[(int)appType]);

            if( helpFilename == null )
            {
                MessageBox.Show(String.Format("The CSPro application folder with {0} could not be located.",HelpFiles[(int)appType]));
                return;
            }


            Help.ShowHelp(this,helpFilename,TopicFiles[(int)appType]);
        }

        private void menuViewHelp_Click(object sender,EventArgs e)
        {
            if( pff != null )
                LaunchHelp(pff.m_appType);
        }

        private void menuViewHelpDetails_Click(object sender,EventArgs e)
        {
            LaunchHelp((PFF.AppType)Enum.Parse(typeof(PFF.AppType),((ToolStripMenuItem)sender).Text));
        }
    }
}
