using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace PFF_Editor
{
    class PFF
    {
        private string m_pffFilename;
        private bool m_verbose;

        public void SetVerboseOutput(bool flag) { m_verbose = flag; }

        public bool IsEngineRunningApp()
        {
            return m_appType == AppType.Batch || m_appType == AppType.Entry || m_appType == AppType.Frequencies || m_appType == AppType.Export || m_appType == AppType.Tabulation;
        }


        // [Run Information]
        public string m_version;
        public enum AppType { Batch, Compare, Concatenate, Deploy, Entry, Excel2CSPro, Export, Frequencies, Index, Pack, ParadataConcatenate, Reformat, Sort, Sync, Tabulation, View };
        public AppType m_appType;
        public enum Operation { All, Con, Tab, Format };
        public Operation m_operation;
        public string m_description;

        // [DataEntryInit]
        public string m_operatorID;
        public enum StartMode { None, Add, Modify, Verify };
        public StartMode m_startMode;
        public string m_startCase;
        public string m_key;
        public bool m_noFileOpen;
        public bool m_lockAdd;
        public bool m_lockModify;
        public bool m_lockDelete;
        public bool m_lockVerify;
        public bool m_lockView;
        public bool m_lockStats;
        public bool m_lockCaseListing;
        public enum ShowInApplicationListing { Always, Hidden, Never };
        public ShowInApplicationListing m_showInApplicationListing;
        public enum Interactive { Ask, Both, ErrMsg, Range, Off };
        public Interactive m_interactive;
        public bool m_interactiveLock;
        public enum FullScreen { Yes, No, NoMenus }
        public FullScreen m_fullScreen;
        public bool m_autoAdd;
        public string m_language;
        public string m_caseListingFilter;

        // [Files]
        public string m_application;
        public List<String> m_inputData;
        public string m_excel;
        public List<String> m_exportedData;
        public List<String> m_outputData;
        public string m_tabOutputTAB;
        public List<String> m_conInputTABs;
        public string m_conOutputTAB;
        public string m_formatInputTAB;
        public string m_outputTBW;
        public string m_areaNames;
        public string m_inputDict;
        public string m_outputDict;
        public string m_referenceData;
        public string m_SPSSDescFile;
        public string m_SASDescFile;
        public string m_STATADescFile;
        public string m_STATALabelsFile;
        public string m_CSProDescFile;
        public string m_RDescFile;
        public string m_zipFile;
        public List<String> m_extraFiles;
        public List<String> m_inputParadata;
        public string m_outputParadata;
		public string m_paradata;
        public string m_listing;
        public string m_freqs;
        public string m_imputeFreqs;
        public string m_imputeStat;
        public string m_writeData;
        public string m_saveArrayFilename;
        public string m_commonStore;
        public string m_htmlDialogsDirectory;
        public string m_baseMap;

        // [ExternalFiles]
        public Dictionary<string,string> m_externalFiles;

        // [UserFiles]
        public Dictionary<string,string> m_userFiles;

        // [Parameters]
        public bool m_viewResults;
        public bool m_skipStructure;
        public bool m_checkRanges;
        public enum ViewListing { Always, OnError, Never };
        public ViewListing m_viewListing;
        public string m_parameter;
        public Dictionary<string,string> m_customParameters;
        public bool m_packIncludeVSImages;
        public bool m_packIncludeResources;
        public bool m_packIncludeInputFile;
        public bool m_packIncludeExternalFiles;
        public bool m_packIncludeUserFiles;
        public enum DuplicateCase { List, View, Prompt, PromptIfDifferent, KeepFirst };
        public DuplicateCase m_duplicateCase;
        public int m_listingWidth;
        public bool m_messageWrap;
        public enum ErrmsgOverride { No, Summary, Case };
        public ErrmsgOverride m_errmsgOverride;
        public bool m_displayNames;
        public bool m_inputOrderIndexed;
        public bool m_concatMethodCase;
        public enum SyncType { CSWeb, Dropbox, FTP, LocalDropbox, LocalFiles };
        public SyncType m_syncType;
        public enum SyncDirection { Get, Put, Both };
        public SyncDirection m_syncDirection;
        public string m_syncUrl;
        public enum DeployToOverride { None, CSWeb, Dropbox, FTP, LocalFile, LocalFolder };
        public DeployToOverride m_deployToOverride;
        public bool m_silent;
        public string m_onExit;

        // [DataEntryIds]
        public Dictionary<string,string> m_dataEntryIds;



        private PFF() // keep the user from instantiating a PFF directly
        {
            // set up the default values
            m_version = "CSPro 7.3";
            m_appType = AppType.Entry;
            m_operation = Operation.All;
            m_description = "";

            m_operatorID = "";
            m_startMode = StartMode.None;
            m_startCase = "";
            m_key = "";
            m_noFileOpen = false;
            m_lockAdd = false;
            m_lockModify = false;
            m_lockDelete = false;
            m_lockVerify = false;
            m_lockView = false;
            m_lockStats = false;
            m_lockCaseListing = false;
            m_showInApplicationListing = ShowInApplicationListing.Always;
            m_interactive = Interactive.Ask;
            m_interactiveLock = false;
            m_fullScreen = FullScreen.No;
            m_autoAdd = true;
            m_language = "";
            m_caseListingFilter = "";

            m_application = "";
            m_inputData = new List<String>();
            m_excel = "";
            m_exportedData = new List<String>();
            m_outputData = new List<String>();
            m_inputDict = "";
            m_outputDict = "";
            m_referenceData = "";
            m_tabOutputTAB = "";
            m_conInputTABs = new List<String>();
            m_conOutputTAB = "";
            m_formatInputTAB = "";
            m_outputTBW = "";
            m_areaNames = "";
            m_SPSSDescFile = "";
            m_SASDescFile = "";
            m_STATADescFile = "";
            m_STATALabelsFile = "";
            m_CSProDescFile = "";
            m_RDescFile = "";
            m_zipFile = "";
            m_extraFiles = new List<String>();
            m_inputParadata = new List<String>();
            m_outputParadata = "";
			m_paradata = "";
            m_listing = "";
            m_freqs = "";
            m_imputeFreqs = "";
            m_imputeStat = "";
            m_writeData = "";
            m_saveArrayFilename = "";
            m_commonStore = "";
            m_htmlDialogsDirectory = "";
            m_baseMap = "";

            m_externalFiles = new Dictionary<string,string>();

            m_userFiles = new Dictionary<string,string>();

            m_viewResults = true;
            m_skipStructure = false;
            m_checkRanges = false;
            m_viewListing = ViewListing.Always;
            m_parameter = "";
            m_customParameters = new Dictionary<string,string>();
            m_packIncludeVSImages = false;
            m_packIncludeResources = false;
            m_packIncludeInputFile = false;
            m_packIncludeExternalFiles = false;
            m_packIncludeUserFiles = false;
            m_duplicateCase = DuplicateCase.List;
            m_listingWidth = 80;
            m_messageWrap = false;
            m_errmsgOverride = ErrmsgOverride.No;
            m_displayNames = false;
            m_inputOrderIndexed = false;
            m_concatMethodCase = false;
            m_syncType = SyncType.CSWeb;
            m_syncDirection = SyncDirection.Get;
            m_syncUrl = "";
            m_deployToOverride = DeployToOverride.None;
            m_silent = false;
            m_onExit = "";

            m_dataEntryIds = new Dictionary<string,string>();
        }


        private enum ProcessingSection { RunInformation, DataEntryInit, Files, ExternalFiles, UserFiles, Parameters, DataEntryIds };

        public static PFF Open(string filename, List<string> line_errors)
        {
            PFF pff = new PFF();

            pff.m_pffFilename = filename;

            StreamReader tr = null;

            try
            {
                tr = new StreamReader(filename);

                string line = "";
                ProcessingSection currentSection = ProcessingSection.RunInformation;
                string currentSectionString = "";
                bool skipNextRead = false;

                while( skipNextRead || ( ( line = tr.ReadLine() ) != null ) )
                {
                    string original_line = line;
                    line = line.Trim();

                    if( line.Length == 0 )
                        continue;

                    try
                    {
                        if( line[0] == '[' ) // we're in a new section
                        {
                            currentSection = ParseSection(line);
                            currentSectionString = line;
                        }

                        else
                        {
                            string command,argument;
                            SplitCommandArgument(line,out command,out argument);

                            try
                            {
                                switch( currentSection )
                                {
                                    case ProcessingSection.RunInformation:
                                        pff.ParseRunInformation(command,argument);
                                        break;

                                    case ProcessingSection.DataEntryInit:
                                        pff.ParseDataEntryInit(command,argument,original_line);
                                        break;

                                    case ProcessingSection.Files:
                                        pff.ParseFiles(command,argument);
                                        break;

                                    case ProcessingSection.ExternalFiles:
                                        pff.ParseDictionaryMapping(pff.m_externalFiles,command,ParseConnectionString(argument));
                                        break;

                                    case ProcessingSection.UserFiles:
                                        pff.ParseDictionaryMapping(pff.m_userFiles,command,argument);
                                        break;

                                    case ProcessingSection.Parameters:
                                        pff.ParseParameters(command,argument);
                                        break;

                                    case ProcessingSection.DataEntryIds:
                                        pff.ParseDictionaryMapping(pff.m_dataEntryIds,command,argument);
                                        break;
                                }
                            }

                            catch( Exception )
                            {
                                throw new Exception(String.Format("Unrecognized option in section '{0}' with command '{1}' and argument '{2}'",currentSectionString,command,argument));
                            }
                        }
                    }

                    catch( Exception exception )
                    {
                        if( line_errors == null )
                            throw exception;

                        else
                            line_errors.Add(exception.Message);
                    }
                }
            }

            finally
            {
                if( tr != null )
                    tr.Close();
            }

            return pff;
        }


        private static void SplitCommandArgument(string line,out string command,out string argument)
        {
            int equalsPos = line.IndexOf('=');

            if( equalsPos >= 0 )
            {
                command = line.Substring(0,equalsPos).Trim();
                argument = line.Substring(equalsPos + 1).Trim();
            }

            else
            {
                command = line;
                argument = "";
            }
        }


        private bool ParseBinary(string argument)
        {
            if( argument.Equals(Commands.Yes,StringComparison.CurrentCultureIgnoreCase) )
                return true;

            else if( argument.Equals(Commands.No,StringComparison.CurrentCultureIgnoreCase) )
                return false;

            else
                throw new Exception();
        }


        private static ProcessingSection ParseSection(string line)
        {
            if( line.Equals(Sections.RunInformation,StringComparison.CurrentCultureIgnoreCase) )
                return ProcessingSection.RunInformation;

            else if( line.Equals(Sections.DataEntryInit,StringComparison.CurrentCultureIgnoreCase) )
                return ProcessingSection.DataEntryInit;

            else if( line.Equals(Sections.Files,StringComparison.CurrentCultureIgnoreCase) )
                return ProcessingSection.Files;

            else if( line.Equals(Sections.ExternalFiles,StringComparison.CurrentCultureIgnoreCase) )
                return ProcessingSection.ExternalFiles;

            else if( line.Equals(Sections.UserFiles,StringComparison.CurrentCultureIgnoreCase) )
                return ProcessingSection.UserFiles;

            else if( line.Equals(Sections.Parameters,StringComparison.CurrentCultureIgnoreCase) )
                return ProcessingSection.Parameters;

            else if( line.Equals(Sections.DataEntryIds,StringComparison.CurrentCultureIgnoreCase) )
                return ProcessingSection.DataEntryIds;

            throw new Exception("Unrecognized section: " + line);
        }


        private void ParseRunInformation(string command,string argument)
        {
            if( command.Equals(RunInformation.Version,StringComparison.CurrentCultureIgnoreCase) )
            {
                m_version = argument;
            }

            else if( command.Equals(RunInformation.AppType,StringComparison.CurrentCultureIgnoreCase) )
            {
                m_appType = (AppType)ParseEnum(argument,Commands.Batch,AppType.Batch,Commands.Compare,AppType.Compare,
                    Commands.Concatenate,AppType.Concatenate,Commands.Deploy,AppType.Deploy,Commands.Entry,AppType.Entry,
					Commands.Excel2CSPro,AppType.Excel2CSPro,Commands.Export,AppType.Export,Commands.Frequencies,AppType.Frequencies,
					Commands.Index,AppType.Index,Commands.Pack,AppType.Pack,Commands.ParadataConcatenate,AppType.ParadataConcatenate,
					Commands.Reformat,AppType.Reformat,Commands.Sort,AppType.Sort,Commands.Sync,AppType.Sync,
					Commands.Tabulation,AppType.Tabulation,Commands.View,AppType.View);
            }

            else if( command.Equals(RunInformation.Description,StringComparison.CurrentCultureIgnoreCase) )
            {
                // 20140528 if a description was defined, but blank, add a space so that it will be saved out
                m_description = argument.Length > 0 ? argument : " ";
            }

            else if( command.Equals(DataEntryInit.ShowInApplicationListing,StringComparison.CurrentCultureIgnoreCase) )
            {
                m_showInApplicationListing = (ShowInApplicationListing)ParseEnum(argument,
                    Commands.Always,ShowInApplicationListing.Always,Commands.Hidden,ShowInApplicationListing.Hidden,
                    Commands.Never,ShowInApplicationListing.Never);
            }

            else if( command.Equals(RunInformation.Operation,StringComparison.CurrentCultureIgnoreCase) )
            {
                m_operation = (Operation)ParseEnum(argument,Commands.All,Operation.All,Commands.Con,Operation.Con,
                    Commands.Tab,Operation.Tab,Commands.Format,Operation.Format);
            }

            else
            {
                throw new Exception();
            }
        }


        private void ParseDataEntryInit(string command, string argument, string original_line)
        {
            if( command.Equals(DataEntryInit.OperatorID,StringComparison.CurrentCultureIgnoreCase) )
                m_operatorID = argument;

            else if( command.Equals(DataEntryInit.StartMode,StringComparison.CurrentCultureIgnoreCase) )
            {
                int semicolonPos = argument.IndexOf(';');
                string startMode = semicolonPos >= 0 ? argument.Substring(0,semicolonPos) : argument;

                m_startMode = (StartMode)ParseEnum(startMode,Commands.Add,StartMode.Add,Commands.Modify,StartMode.Modify,
                    Commands.Verify,StartMode.Verify);

                if( semicolonPos >= 0 )
                    m_startCase = argument.Substring(semicolonPos + 1);
            }

            else if( command.Equals(DataEntryInit.Key, StringComparison.CurrentCultureIgnoreCase) )
            {
                // the argument to Key shouldn't be trimmed
                int equals_pos = original_line.IndexOf('=');

                if( equals_pos >= 0 )
                    m_key = original_line.Substring(equals_pos + 1);
            }

            else if( command.Equals(DataEntryInit.NoFileOpen,StringComparison.CurrentCultureIgnoreCase) )
                m_noFileOpen = ParseBinary(argument);

            else if( command.Equals(DataEntryInit.Lock,StringComparison.CurrentCultureIgnoreCase) )
            {
                while( argument.Length > 0 )
                {
                    int commaPos = argument.IndexOf(',');
                    string thisArgument;

                    if( commaPos >= 0 )
                    {
                        thisArgument = argument.Substring(0,commaPos);
                        argument = argument.Substring(commaPos + 1);
                    }

                    else
                    {
                        thisArgument = argument;
                        argument = "";
                    }

                    thisArgument = thisArgument.Trim();

                    if( thisArgument.Equals(Commands.Add,StringComparison.CurrentCultureIgnoreCase) )
                        m_lockAdd = true;

                    else if( thisArgument.Equals(Commands.Modify,StringComparison.CurrentCultureIgnoreCase) )
                        m_lockModify = true;

                    else if( thisArgument.Equals(Commands.Delete,StringComparison.CurrentCultureIgnoreCase) )
                        m_lockDelete = true;

                    else if( thisArgument.Equals(Commands.Verify,StringComparison.CurrentCultureIgnoreCase) )
                        m_lockVerify = true;

                    else if( thisArgument.Equals(Commands.View,StringComparison.CurrentCultureIgnoreCase) )
                        m_lockView = true;

                    else if( thisArgument.Equals(Commands.Stats,StringComparison.CurrentCultureIgnoreCase) )
                        m_lockStats = true;

                    else if( thisArgument.Equals(Commands.CaseListing,StringComparison.CurrentCultureIgnoreCase) )
                        m_lockCaseListing = true;

                    else
                        throw new Exception();
                }
            }

            else if( command.Equals(DataEntryInit.CaseListingFilter,StringComparison.CurrentCultureIgnoreCase) )
                m_caseListingFilter = argument;

            // for backwards compatability, this can also be read in the data entry section
            else if( command.Equals(DataEntryInit.ShowInApplicationListing,StringComparison.CurrentCultureIgnoreCase) )
            {
                m_showInApplicationListing = (ShowInApplicationListing)ParseEnum(argument,
                    Commands.Always,ShowInApplicationListing.Always,Commands.Hidden,ShowInApplicationListing.Hidden,
                    Commands.Never,ShowInApplicationListing.Never);
            }

            else if( command.Equals(DataEntryInit.Interactive,StringComparison.CurrentCultureIgnoreCase) )
            {
                while( argument.Length > 0 )
                {
                    int commaPos = argument.IndexOf(',');
                    string thisArgument;

                    if( commaPos >= 0 )
                    {
                        thisArgument = argument.Substring(0,commaPos);
                        argument = argument.Substring(commaPos + 1);
                    }

                    else
                    {
                        thisArgument = argument;
                        argument = "";
                    }

                    if( thisArgument.Equals(Commands.ErrMsg,StringComparison.CurrentCultureIgnoreCase) )
                        m_interactive = Interactive.ErrMsg;

                    else if( thisArgument.Equals(Commands.Range,StringComparison.CurrentCultureIgnoreCase) )
                        m_interactive = Interactive.Range;

                    else if( thisArgument.Equals(Commands.Both,StringComparison.CurrentCultureIgnoreCase) )
                        m_interactive = Interactive.Both;

                    else if( thisArgument.Equals(Commands.Ask,StringComparison.CurrentCultureIgnoreCase) )
                        m_interactive = Interactive.Ask;

                    else if( thisArgument.Equals(Commands.Off,StringComparison.CurrentCultureIgnoreCase) )
                        m_interactive = Interactive.Off;

                    else if( thisArgument.Equals(Commands.Lock,StringComparison.CurrentCultureIgnoreCase) )
                        m_interactiveLock = true;

                    else
                        throw new Exception();
                }
            }

            else if( command.Equals(DataEntryInit.FullScreen,StringComparison.CurrentCultureIgnoreCase) )
                m_fullScreen = (FullScreen)ParseEnum(argument,Commands.Yes,FullScreen.Yes,Commands.No,FullScreen.No,
                    Commands.NoMenus,FullScreen.NoMenus);

            else if( command.Equals(DataEntryInit.AutoAdd,StringComparison.CurrentCultureIgnoreCase) )
                m_autoAdd = ParseBinary(argument);

			// language is now in the parameters section but used to be here
            else if( command.Equals(DataEntryInit.Language,StringComparison.CurrentCultureIgnoreCase) )
                m_language = argument;

            else
                throw new Exception();
        }


        private void ParseFiles(string command,string argument)
        {
            if( command.Equals(Files.Application,StringComparison.CurrentCultureIgnoreCase) )
                m_application = argument;

            else if( command.Equals(Files.InputData,StringComparison.CurrentCultureIgnoreCase) )
                m_inputData.Add(ParseConnectionString(argument));

            else if( command.Equals(Files.Excel,StringComparison.CurrentCultureIgnoreCase) )
                m_excel = argument;

            else if( command.Equals(Files.ExportedData,StringComparison.CurrentCultureIgnoreCase) )
                m_exportedData.Add(argument);

            else if( command.Equals(Files.OutputData,StringComparison.CurrentCultureIgnoreCase) )
                m_outputData.Add(ParseConnectionString(argument));

            else if( command.Equals(Files.InputDict,StringComparison.CurrentCultureIgnoreCase) )
                m_inputDict = argument;

            else if( command.Equals(Files.OutputDict,StringComparison.CurrentCultureIgnoreCase) )
                m_outputDict = argument;

            else if( command.Equals(Files.ReferenceData,StringComparison.CurrentCultureIgnoreCase) )
                m_referenceData = ParseConnectionString(argument);

            else if( command.Equals(Files.TabOutputTAB,StringComparison.CurrentCultureIgnoreCase) )
                m_tabOutputTAB = argument;

            else if( command.Equals(Files.ConInputTAB,StringComparison.CurrentCultureIgnoreCase) )
                m_conInputTABs.Add(argument);

            else if( command.Equals(Files.ConOutputTAB,StringComparison.CurrentCultureIgnoreCase) )
                m_conOutputTAB = argument;

            else if( command.Equals(Files.FormatInputTAB,StringComparison.CurrentCultureIgnoreCase) )
                m_formatInputTAB = argument;

            else if( command.Equals(Files.OutputTBW,StringComparison.CurrentCultureIgnoreCase) )
                m_outputTBW = argument;

            else if( command.Equals(Files.AreaNames,StringComparison.CurrentCultureIgnoreCase) )
                m_areaNames = argument;

            else if( command.Equals(Files.SPSSDescFile,StringComparison.CurrentCultureIgnoreCase) )
                m_SPSSDescFile = argument;

            else if( command.Equals(Files.SASDescFile,StringComparison.CurrentCultureIgnoreCase) )
                m_SASDescFile = argument;

            else if( command.Equals(Files.STATADescFile,StringComparison.CurrentCultureIgnoreCase) )
                m_STATADescFile = argument;

            else if( command.Equals(Files.STATALabelsFile,StringComparison.CurrentCultureIgnoreCase) )
                m_STATALabelsFile = argument;

            else if( command.Equals(Files.CSProDescFile,StringComparison.CurrentCultureIgnoreCase) )
                m_CSProDescFile = argument;

            else if( command.Equals(Files.RDescFile,StringComparison.CurrentCultureIgnoreCase) )
                m_RDescFile = argument;

            else if( command.Equals(Files.ZipFile,StringComparison.CurrentCultureIgnoreCase) )
                m_zipFile = argument;

            else if( command.Equals(Files.ExtraFile,StringComparison.CurrentCultureIgnoreCase) )
                m_extraFiles.Add(argument);

            else if( command.Equals(Files.InputParadata,StringComparison.CurrentCultureIgnoreCase) )
                m_inputParadata.Add(argument);

            else if( command.Equals(Files.OutputParadata,StringComparison.CurrentCultureIgnoreCase) )
                m_outputParadata = argument;

            else if( command.Equals(Files.Paradata,StringComparison.CurrentCultureIgnoreCase) )
                m_paradata = argument;
            
			else if( command.Equals(Files.Listing,StringComparison.CurrentCultureIgnoreCase) )
                m_listing = argument;

            else if( command.Equals(Files.Freqs,StringComparison.CurrentCultureIgnoreCase) )
                m_freqs = argument;

            else if( command.Equals(Files.ImputeFreqs,StringComparison.CurrentCultureIgnoreCase) )
                m_imputeFreqs = argument;

            else if( command.Equals(Files.ImputeStat, StringComparison.CurrentCultureIgnoreCase) )
                m_imputeStat = argument;

            else if( command.Equals(Files.WriteData,StringComparison.CurrentCultureIgnoreCase) )
                m_writeData = argument;

            else if( command.Equals(Files.SaveArray, StringComparison.CurrentCultureIgnoreCase) )
                m_saveArrayFilename = argument;

            else if( command.Equals(Files.CommonStore,StringComparison.CurrentCultureIgnoreCase) )
                m_commonStore = argument;                

            else if( command.Equals(Files.HtmlDialogs,StringComparison.CurrentCultureIgnoreCase) )
                m_htmlDialogsDirectory = argument;                

            else if( command.Equals(Files.BaseMap,StringComparison.CurrentCultureIgnoreCase) )
                m_baseMap = argument;                

            else
                throw new Exception();
        }


        private void ParseDictionaryMapping(Dictionary<string,string> dictionary,string command,string argument)
        {
            if( dictionary.ContainsKey(command) )
                throw new Exception();

            dictionary[command] = argument;
        }


        private void ParseParameters(string command,string argument)
        {
            if( command.Equals(Parameters.ViewResults,StringComparison.CurrentCultureIgnoreCase) )
                m_viewResults = ParseBinary(argument);

            else if( command.Equals(Parameters.SkipStructure,StringComparison.CurrentCultureIgnoreCase) )
                m_skipStructure = ParseBinary(argument);

            else if( command.Equals(Parameters.CheckRanges,StringComparison.CurrentCultureIgnoreCase) )
                m_checkRanges = ParseBinary(argument);

            else if( command.Equals(Parameters.ViewListing,StringComparison.CurrentCultureIgnoreCase) )
                m_viewListing = (ViewListing)ParseEnum(argument,Commands.Never,ViewListing.Never,
                    Commands.OnError,ViewListing.OnError,Commands.Always,ViewListing.Always);

            else if( command.Equals(DataEntryInit.Language,StringComparison.CurrentCultureIgnoreCase) )
                m_language = argument;

            else if( command.Equals(Parameters.Parameter,StringComparison.CurrentCultureIgnoreCase) )
                m_parameter = argument;

            else if( command.Equals(Parameters.PackIncludes,StringComparison.CurrentCultureIgnoreCase) )
            {
                while( argument.Length > 0 )
                {
                    int commaPos = argument.IndexOf(',');
                    string thisArgument;

                    if( commaPos >= 0 )
                    {
                        thisArgument = argument.Substring(0,commaPos);
                        argument = argument.Substring(commaPos + 1);
                    }

                    else
                    {
                        thisArgument = argument;
                        argument = "";
                    }

                    if( thisArgument.Equals(Commands.VSImages,StringComparison.CurrentCultureIgnoreCase) )
                        m_packIncludeVSImages = true;

                    else if( thisArgument.Equals(Commands.Resources,StringComparison.CurrentCultureIgnoreCase) )
                        m_packIncludeResources = true;

                    else if( thisArgument.Equals(Commands.InputFiles,StringComparison.CurrentCultureIgnoreCase) )
                    {
                        m_packIncludeInputFile = true;
                        m_packIncludeExternalFiles = true;
                    }

                    else if( thisArgument.Equals(Commands.InputFile,StringComparison.CurrentCultureIgnoreCase) )
                        m_packIncludeInputFile = true;

                    else if( thisArgument.Equals(Commands.ExternalFiles,StringComparison.CurrentCultureIgnoreCase) )
                        m_packIncludeExternalFiles = true;

                    else if( thisArgument.Equals(Commands.UserFiles,StringComparison.CurrentCultureIgnoreCase) )
                        m_packIncludeUserFiles  = true;

                    else
                        throw new Exception();
                }
            }

            else if( command.Equals(Parameters.DuplicateCase,StringComparison.CurrentCultureIgnoreCase) )
                m_duplicateCase = (DuplicateCase)ParseEnum(argument,Commands.List,DuplicateCase.List,Commands.View,DuplicateCase.View,
                    Commands.Prompt,DuplicateCase.Prompt,Commands.PromptIfDifferent,DuplicateCase.PromptIfDifferent,Commands.KeepFirst,DuplicateCase.KeepFirst);

            else if( command.Equals(Parameters.ListingWidth,StringComparison.CurrentCultureIgnoreCase) )
                m_listingWidth = Int32.Parse(argument);

            else if( command.Equals(Parameters.MessageWrap,StringComparison.CurrentCultureIgnoreCase) )
                m_messageWrap = ParseBinary(argument);

            else if( command.Equals(Parameters.ErrmsgOverride,StringComparison.CurrentCultureIgnoreCase) )
                m_errmsgOverride = (ErrmsgOverride)ParseEnum(argument,Commands.No,ErrmsgOverride.No,
                    Commands.Summary,ErrmsgOverride.Summary,Commands.Case,ErrmsgOverride.Case);

            else if( command.Equals(Parameters.DisplayNames,StringComparison.CurrentCultureIgnoreCase) )
                m_displayNames = ParseBinary(argument);

            else if( command.Equals(Parameters.InputOrder,StringComparison.CurrentCultureIgnoreCase) )
                m_inputOrderIndexed = argument.Equals(Commands.Indexed,StringComparison.CurrentCultureIgnoreCase);

            else if( command.Equals(Parameters.ConcatMethod,StringComparison.CurrentCultureIgnoreCase) )
                m_concatMethodCase = argument.Equals(Commands.Case,StringComparison.CurrentCultureIgnoreCase);

            else if( command.Equals(Parameters.SyncType,StringComparison.CurrentCultureIgnoreCase) )
                m_syncType = (SyncType)ParseEnum(argument, Commands.CSWeb, SyncType.CSWeb,
                    Commands.Dropbox, SyncType.Dropbox, Commands.FTP, SyncType.FTP, Commands.LocalDropbox, SyncType.LocalDropbox, Commands.LocalFiles, SyncType.LocalFiles);
            
            else if( command.Equals(Parameters.SyncDirection,StringComparison.CurrentCultureIgnoreCase) )
                m_syncDirection = (SyncDirection)ParseEnum(argument,Commands.Get,SyncDirection.Get,
                    Commands.Put,SyncDirection.Put,Commands.Both,SyncDirection.Both);
            
            else if( command.Equals(Parameters.SyncUrl,StringComparison.CurrentCultureIgnoreCase) )
                m_syncUrl = argument;

            else if( command.Equals(Parameters.DeployToOverride, StringComparison.CurrentCultureIgnoreCase) )
                m_deployToOverride = (DeployToOverride)Enum.Parse(typeof(DeployToOverride), argument, true);

            else if( command.Equals(Parameters.Silent,StringComparison.CurrentCultureIgnoreCase) )
                m_silent = ParseBinary(argument);

            else if( command.Equals(Parameters.OnExit,StringComparison.CurrentCultureIgnoreCase) )
                m_onExit = argument;				
				
            else
            {
                if( !m_customParameters.ContainsKey(command) )
                    m_customParameters.Add(command,argument);
            }
        }


        private object ParseEnum(string argument,params object[] parameters)
        {
            for( int i = 0; i < parameters.Length; i += 2 )
            {
                if( argument.Equals((string)parameters[i],StringComparison.CurrentCultureIgnoreCase) )
                    return parameters[i + 1];
            }
            
            throw new Exception();
        }

        private static string ParseConnectionString(string argument)
        {
            int pipe_pos = argument.IndexOf('|');
            string filename = argument;

            if( pipe_pos >= 0 )
            {
                filename = argument.Substring(0, pipe_pos).Trim();
                string type = argument.Substring(pipe_pos + 1).Trim();

                if( type.Equals("type=None", StringComparison.InvariantCultureIgnoreCase) ||
                    type.Equals("None", StringComparison.InvariantCultureIgnoreCase) )
                {
                    return " ";
                }
            }

            return filename;
        }


        public void Save(string filename)
        {
            string contents = GeneratePFFContents(null);

            TextWriter tw = null;

            try
            {
                tw = new StreamWriter(filename,false,Encoding.UTF8);
                tw.Write(contents);
            }

            finally
            {
                if( tw != null )
                    tw.Close();
            }

            m_pffFilename = filename;
        }


        public class PFFCreationOptions
        {
            public PFFCreationOptions(bool usingPffObject,string objectName,bool runAfterCreating,bool separateParameters)
            {
                m_usingPffObject = usingPffObject;
                m_objectName = objectName;
                m_runAfterCreating = runAfterCreating;
                m_separateParameters = separateParameters;
            }

            public bool m_usingPffObject;
            public string m_objectName;
            public bool m_runAfterCreating;
            public bool m_separateParameters;
        }

        private static string GetHeaderPrefix(PFFCreationOptions options, string headerName)
        {
            return ( options == null || !options.m_usingPffObject ) ? "" :
                ( headerName.Substring(1, headerName.Length - 2) + "." );
        }

        private static string GenerateFilename(PFFCreationOptions options, string filename)
        {
            if( options != null )
            {
                if( options.m_usingPffObject )
                {
                    if( filename.Length >= 3 && filename[0] == '.' && ( filename[1] == '\\' || filename[1] == '/' ) )
                        filename = filename.Substring(2);
                }
                
                else
                {
                    filename = filename.Replace('\\','/');
                }
            }

            return filename;
        }

        private static string GenerateConnectionString(PFFCreationOptions options, string connection_string)
        {
            if( String.IsNullOrWhiteSpace(connection_string) )
                return "|type=None";

            else
                return GenerateFilename(options, connection_string.Trim());
        }

        public string GeneratePFFContents(PFFCreationOptions options)
        {
            StringBuilder sb = new StringBuilder();
            string preText = "";
            string postText = "\r\n";

            bool createLogic = ( options != null );
            bool writeHeaders = ( !createLogic || !options.m_usingPffObject );
            string separatorText = "=";
            string separatorParametersText = separatorText;
            string objectName = null;

            if( createLogic )
            {
                if( options.m_usingPffObject )
                {
                    separatorText = "\", \"";
                    separatorParametersText = separatorText;
                }
                
                else if( options.m_separateParameters )
                    separatorParametersText = "=%v\", \"";

                objectName = options.m_objectName.Trim();

                if( objectName.Length == 0 )
                    objectName = "dynamic_pff";

                sb.AppendFormat("function Create{0}PFF()\r\n\r\n", options.m_runAfterCreating ? "AndRun" : "");

                if( options.m_usingPffObject )
                {
                    sb.AppendFormat("\tpff {0};\r\n\r\n", objectName);

                    preText = String.Format("\t{0}.setproperty(\"", objectName);
                }

                else
                {
                    sb.Append("\t// change the following filename to your desired output\r\n");
                    sb.AppendFormat("\tstring pff_filename = path.concat(Application, \"{0}\");\r\n\r\n", Path.GetFileName(m_pffFilename));
                    sb.AppendFormat("\tfile {0};\r\n",objectName);
                    sb.AppendFormat("\tsetfile({0}, pff_filename, create);\r\n\r\n",objectName);

                    preText = String.Format("\tfilewrite({0}, \"",objectName);                    
                }

                postText = "\");\r\n";
            }


            // [Run Information]
            if( writeHeaders )
                sb.AppendFormat("{0}{1}{2}",preText,Sections.RunInformation,postText);

            sb.AppendFormat("{0}{1}{4}{2}{3}",preText,RunInformation.Version,m_version,postText,separatorText);

            sb.AppendFormat("{0}{1}{4}{2}{3}",preText,RunInformation.AppType,m_appType.ToString(),postText,separatorText);

            if( m_appType == AppType.Tabulation )
                sb.AppendFormat("{0}{1}{4}{2}{3}",preText,RunInformation.Operation,m_operation.ToString(),postText,separatorText);

            if( m_description.Length > 0 )
                sb.AppendFormat("{0}{1}{4}{2}{3}",preText,RunInformation.Description,m_description,postText,separatorText);

            if( ( m_showInApplicationListing != ShowInApplicationListing.Always ) || m_verbose )
                sb.AppendFormat("{0}{1}{4}{2}{3}",preText,DataEntryInit.ShowInApplicationListing,m_showInApplicationListing.ToString(),postText,separatorText);


            // [DataEntryInit]
            if( m_appType == AppType.Entry )
            {
                sb.AppendLine();
                if( writeHeaders )
                    sb.AppendFormat("{0}{1}{2}",preText,Sections.DataEntryInit,postText);

                if( m_operatorID.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,DataEntryInit.OperatorID,m_operatorID,postText,separatorParametersText);

                if( m_startMode != StartMode.None )
                {
                    string formatter = "{0}{1}{6}{2}{3}";
                    string thisSeparatorParametersText = separatorParametersText;

                    if( m_startMode != StartMode.Verify && m_startCase.Length > 0 )
                    {
                        formatter = "{0}{1}{6}{2};{4}{3}";

                        if( options != null )
                        {
                            if( options.m_usingPffObject )
                                formatter = "{0}{1}{5}{2};{4}{3}";
                            
                            else
                            {
                                formatter = "{0}{1}{6}{2};{5}{4}{3}";
                                thisSeparatorParametersText = thisSeparatorParametersText.Replace("=", "");
                            }
                        }
                    }
                    
                    sb.AppendFormat(formatter,preText,DataEntryInit.StartMode,m_startMode.ToString(),postText,m_startCase,thisSeparatorParametersText,separatorText);
                }

                if( m_key.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,DataEntryInit.Key,m_key,postText,separatorParametersText);

                StringBuilder lockString = new StringBuilder();

                if( m_lockAdd )
                    lockString.Append(Commands.Add + ",");

                if( m_lockModify )
                    lockString.Append(Commands.Modify + ",");

                if( m_lockDelete )
                    lockString.Append(Commands.Delete + ",");

                if( m_lockVerify )
                    lockString.Append(Commands.Verify + ",");

                if( m_lockView )
                    lockString.Append(Commands.View + ",");

                if( m_lockStats )
                    lockString.Append(Commands.Stats + ",");

                if( m_lockCaseListing )
                    lockString.Append(Commands.CaseListing + ",");

                if( lockString.Length > 0 )
                {
                    lockString.Remove(lockString.Length - 1,1); // remove the last comma
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,DataEntryInit.Lock,lockString.ToString(),postText,separatorText);
                }

                if( m_caseListingFilter.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,DataEntryInit.CaseListingFilter,m_caseListingFilter,postText,separatorParametersText);

                if( m_fullScreen != FullScreen.No || m_verbose )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,DataEntryInit.FullScreen,m_fullScreen.ToString(),postText,separatorText);

                if( !m_autoAdd || m_verbose )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,DataEntryInit.AutoAdd,m_autoAdd ? Commands.Yes : Commands.No,postText,separatorText);

                if( m_noFileOpen || m_verbose )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,DataEntryInit.NoFileOpen,m_noFileOpen ? Commands.Yes : Commands.No,postText,separatorText);

                if( m_interactive != Interactive.Ask || m_verbose )
                    sb.AppendFormat("{0}{1}{5}{2}{4}{3}",preText,DataEntryInit.Interactive,m_interactive.ToString(),postText,m_interactiveLock ? ",Lock" : "",separatorText);
            }


            // [Files]
            if( m_appType != AppType.Sync )
            {
                sb.AppendLine();
                if( writeHeaders )
                    sb.AppendFormat("{0}{1}{2}",preText,Sections.Files,postText);

                if( ( m_appType != AppType.Concatenate && m_appType != AppType.Index && m_appType != AppType.ParadataConcatenate && m_appType != AppType.Reformat ) && m_application.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.Application,GenerateFilename(options,m_application),postText,separatorParametersText);

                if( m_appType != AppType.Pack )
                {
                    foreach( string str in m_inputData )
                    {
                        sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.InputData,GenerateConnectionString(options,str),postText,separatorParametersText);

                        if( m_appType == AppType.Compare || m_appType == AppType.Entry || m_appType == AppType.Reformat || m_appType == AppType.Sort )
                            break; // only one input data file allowed
                    }
                }

                if( m_appType == AppType.Excel2CSPro )
                {
                    if( m_excel.Length > 0 )
                        sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.Excel,GenerateFilename(options,m_excel),postText,separatorParametersText);
                }

                if( m_appType == AppType.Export )
                {
                    foreach( string str in m_exportedData )
                        sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.ExportedData,GenerateFilename(options,str),postText,separatorParametersText);
                }

                if( m_appType == AppType.Batch || m_appType == AppType.Concatenate || m_appType == AppType.Excel2CSPro || m_appType == AppType.Index ||
                    m_appType == AppType.ParadataConcatenate || m_appType == AppType.Reformat || m_appType == AppType.Sort )
                {
                    foreach( string str in m_outputData )
                    {
                        sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.OutputData,GenerateConnectionString(options,str),postText,separatorParametersText);

                        if( m_appType != AppType.Batch )
                            break; // only one output data file allowed
                    }
                }

                if( ( m_appType == AppType.Excel2CSPro || m_appType == AppType.Index || m_appType == AppType.Reformat || ( m_appType == AppType.Concatenate && m_concatMethodCase ) ) && m_inputDict.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.InputDict,GenerateFilename(options,m_inputDict),postText,separatorParametersText);

                if( m_appType == AppType.Reformat && m_outputDict.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.OutputDict,GenerateFilename(options,m_outputDict),postText,separatorParametersText);

                if( m_appType == AppType.Compare && m_referenceData.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.ReferenceData,GenerateConnectionString(options,m_referenceData),postText,separatorParametersText);

                if( m_appType == AppType.Tabulation )
                {
                    if( m_operation == Operation.Tab && m_tabOutputTAB.Length > 0 )
                        sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.TabOutputTAB,GenerateFilename(options,m_tabOutputTAB),postText,separatorParametersText);

                    if( m_operation == Operation.Con )
                    {
                        foreach( string str in m_conInputTABs )
                            sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.ConInputTAB,GenerateFilename(options,str),postText,separatorParametersText);

                        if( m_conOutputTAB.Length > 0 )
                            sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.ConOutputTAB,GenerateFilename(options,m_conOutputTAB),postText,separatorParametersText);
                    }

                    if( m_operation == Operation.Format && m_formatInputTAB.Length > 0 )
                        sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.FormatInputTAB,GenerateFilename(options,m_formatInputTAB),postText,separatorParametersText);

                    if( m_operation == Operation.Format || m_operation == Operation.All )
                    {
                        if( m_outputTBW.Length > 0 )
                            sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.OutputTBW,GenerateFilename(options,m_outputTBW),postText,separatorParametersText);

                        if( m_areaNames.Length > 0 )
                            sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.AreaNames,GenerateFilename(options,m_areaNames),postText,separatorParametersText);
                    }
                }

                if( m_SPSSDescFile.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.SPSSDescFile,GenerateFilename(options,m_SPSSDescFile),postText,separatorParametersText);

                if( m_SASDescFile.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.SASDescFile,GenerateFilename(options,m_SASDescFile),postText,separatorParametersText);

                if( m_STATADescFile.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.STATADescFile,GenerateFilename(options,m_STATADescFile),postText,separatorParametersText);

                if( m_STATALabelsFile.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.STATALabelsFile,GenerateFilename(options,m_STATALabelsFile),postText,separatorParametersText);

                if( m_CSProDescFile.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.CSProDescFile,GenerateFilename(options,m_CSProDescFile),postText,separatorParametersText);

                if( m_RDescFile.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.RDescFile,GenerateFilename(options,m_RDescFile),postText,separatorParametersText);

                if( m_appType == AppType.Pack )
                {
                    if( m_zipFile.Length > 0 )
                        sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.ZipFile,GenerateFilename(options,m_zipFile),postText,separatorParametersText);

                    foreach( string str in m_extraFiles )
                        sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.ExtraFile,GenerateFilename(options,str),postText,separatorParametersText);
                }

                if( m_appType == AppType.ParadataConcatenate )
                {
                    foreach( string str in m_inputParadata )
                        sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.InputParadata,GenerateFilename(options,str),postText,separatorParametersText);

                    if( m_outputParadata.Length > 0 )
                        sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.OutputParadata,GenerateFilename(options,m_outputParadata),postText,separatorParametersText);
                }

                if( IsEngineRunningApp() && m_paradata.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.Paradata,GenerateFilename(options,m_paradata),postText,separatorParametersText);

                if( m_listing.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.Listing,GenerateFilename(options,m_listing),postText,separatorParametersText);

                if( IsEngineRunningApp() && m_freqs.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.Freqs,GenerateFilename(options,m_freqs),postText,separatorParametersText);

                if( IsEngineRunningApp() && m_imputeFreqs.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.ImputeFreqs,GenerateFilename(options,m_imputeFreqs),postText,separatorParametersText);

                if( IsEngineRunningApp() && m_imputeStat.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.ImputeStat,GenerateFilename(options,m_imputeStat),postText,separatorParametersText);

                if( IsEngineRunningApp() && m_writeData.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.WriteData,GenerateFilename(options,m_writeData),postText,separatorParametersText);

                if( IsEngineRunningApp() && m_saveArrayFilename.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.SaveArray,GenerateFilename(options,m_saveArrayFilename),postText,separatorParametersText);

                if( IsEngineRunningApp() && m_commonStore.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.CommonStore,GenerateFilename(options,m_commonStore),postText,separatorParametersText);
                
                if( IsEngineRunningApp() && m_htmlDialogsDirectory.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.HtmlDialogs,GenerateFilename(options,m_htmlDialogsDirectory),postText,separatorParametersText);
                
                if( IsEngineRunningApp() && m_baseMap.Length > 0 )
                    sb.AppendFormat("{0}{1}{4}{2}{3}",preText,Files.BaseMap,GenerateFilename(options,m_baseMap),postText,separatorParametersText);
            }


            // [ExternalFiles]
            if( ( IsEngineRunningApp() || m_appType == AppType.Sync ) && m_externalFiles.Count > 0 )
            {
                sb.AppendLine();
                if( writeHeaders )
                    sb.AppendFormat("{0}{1}{2}",preText,Sections.ExternalFiles,postText);

                string headerPrefix = GetHeaderPrefix(options, Sections.ExternalFiles);

                foreach( var kp in m_externalFiles )
                    sb.AppendFormat("{0}{5}{1}{4}{2}{3}",preText,kp.Key,GenerateConnectionString(options,kp.Value),postText,separatorParametersText,headerPrefix);
            }


            // [UserFiles]
            if( IsEngineRunningApp() && m_userFiles.Count > 0 )
            {
                sb.AppendLine();
                if( writeHeaders )
                    sb.AppendFormat("{0}{1}{2}",preText,Sections.UserFiles,postText);

                string headerPrefix = GetHeaderPrefix(options, Sections.UserFiles);

                foreach( var kp in m_userFiles )
                    sb.AppendFormat("{0}{5}{1}{4}{2}{3}",preText,kp.Key,GenerateFilename(options,kp.Value),postText,separatorParametersText,headerPrefix);
            }


            // [Parameters]
			StringBuilder sbParameters = new StringBuilder();

            if( IsEngineRunningApp() )
            {
                if( m_language.Length > 0 || m_verbose )
                    sbParameters.AppendFormat("{0}{1}{4}{2}{3}",preText,DataEntryInit.Language,m_language,postText,separatorParametersText);

                if( m_parameter.Length > 0 || m_verbose )
                    sbParameters.AppendFormat("{0}{1}{4}{2}{3}",preText,Parameters.Parameter,m_parameter,postText,separatorParametersText);
			}

			if( IsEngineRunningApp() || m_appType == AppType.Excel2CSPro || m_appType == AppType.View )
			{
                string headerPrefix = GetHeaderPrefix(options, Sections.Parameters);

                foreach( var kp in m_customParameters )
                    sbParameters.AppendFormat("{0}{5}{1}{4}{2}{3}",preText,kp.Key,kp.Value,postText,separatorParametersText,headerPrefix);
            }

            if( m_appType != AppType.Deploy && m_appType != AppType.Entry && m_appType != AppType.Excel2CSPro && m_appType != AppType.Sync && m_appType != AppType.View )
                sbParameters.AppendFormat("{0}{1}{4}{2}{3}",preText,Parameters.ViewListing,m_viewListing.ToString(),postText,separatorText);

            if( m_appType == AppType.Batch )
            {
                if( m_skipStructure || m_verbose )
                    sbParameters.AppendFormat("{0}{1}{4}{2}{3}",preText,Parameters.SkipStructure,m_skipStructure ? Commands.Yes : Commands.No,postText,separatorText);

                if( m_checkRanges || m_verbose )
                    sbParameters.AppendFormat("{0}{1}{4}{2}{3}",preText,Parameters.CheckRanges,m_checkRanges ? Commands.Yes : Commands.No,postText,separatorText);
            }

            if( m_appType != AppType.Compare && m_appType != AppType.Deploy && m_appType != AppType.Entry && m_appType != AppType.Excel2CSPro &&
				m_appType != AppType.Index && m_appType != AppType.Pack && m_appType != AppType.ParadataConcatenate && m_appType != AppType.Sync && m_appType != AppType.View )
			{
                sbParameters.AppendFormat("{0}{1}{4}{2}{3}",preText,Parameters.ViewResults,m_viewResults ? Commands.Yes : Commands.No,postText,separatorText);
			}

            if( m_appType == AppType.Pack )
            {
                StringBuilder packIncludeString = new StringBuilder();

                if( m_packIncludeVSImages )
                    packIncludeString.Append(Commands.VSImages + ",");

                if( m_packIncludeResources )
                    packIncludeString.Append(Commands.Resources + ",");

                if( m_packIncludeInputFile )
                    packIncludeString.Append(Commands.InputFile + ",");

                if( m_packIncludeExternalFiles )
                    packIncludeString.Append(Commands.ExternalFiles + ",");

                if( m_packIncludeUserFiles )
                    packIncludeString.Append(Commands.UserFiles + ",");

                if( packIncludeString.Length > 0 )
                {
                    packIncludeString.Remove(packIncludeString.Length - 1,1); // remove the last comma
                    sbParameters.AppendFormat("{0}{1}{4}{2}{3}",preText,Parameters.PackIncludes,packIncludeString.ToString(),postText,separatorText);
                }
            }

            if( m_appType == AppType.Index )
                sbParameters.AppendFormat("{0}{1}{4}{2}{3}",preText,Parameters.DuplicateCase,m_duplicateCase.ToString(),postText,separatorText);

            if( m_appType == AppType.Batch )
            {
                sbParameters.AppendFormat("{0}{1}{4}{2}{3}",preText,Parameters.ListingWidth,m_listingWidth,postText,separatorText);
                sbParameters.AppendFormat("{0}{1}{4}{2}{3}",preText,Parameters.MessageWrap,m_messageWrap ? Commands.Yes : Commands.No,postText,separatorText);
                sbParameters.AppendFormat("{0}{1}{4}{2}{3}",preText,Parameters.ErrmsgOverride,m_errmsgOverride.ToString(),postText,separatorText);
            }

            if( m_appType == AppType.Reformat )
            {
                if( m_displayNames || m_verbose )
                    sbParameters.AppendFormat("{0}{1}{4}{2}{3}",preText,Parameters.DisplayNames,m_displayNames ? Commands.Yes : Commands.No,postText,separatorText);
            }

            if( IsEngineRunningApp() && m_appType != AppType.Entry )
                sbParameters.AppendFormat("{0}{1}{4}{2}{3}",preText,Parameters.InputOrder,m_inputOrderIndexed ? Commands.Indexed : Commands.Sequential,postText,separatorText);
            
            if( m_appType == AppType.Concatenate )
                sbParameters.AppendFormat("{0}{1}{4}{2}{3}",preText,Parameters.ConcatMethod,m_concatMethodCase ? Commands.Case : Commands.Text,postText,separatorText);

            if( m_appType == AppType.Sync )
            {
                sbParameters.AppendFormat("{0}{1}{4}{2}{3}",preText,Parameters.SyncType,m_syncType.ToString(),postText,separatorText);
                sbParameters.AppendFormat("{0}{1}{4}{2}{3}",preText,Parameters.SyncDirection,m_syncDirection.ToString(),postText,separatorText);
            }

            if( ( m_appType == PFF.AppType.Sync && m_syncType != PFF.SyncType.Dropbox && m_syncType != PFF.SyncType.LocalDropbox ) ||
                ( m_appType == AppType.Deploy && ( m_verbose || !string.IsNullOrWhiteSpace(m_syncUrl) ) ) )
            {
                sbParameters.AppendFormat("{0}{1}{4}{2}{3}",preText,Parameters.SyncUrl,m_syncUrl,postText,separatorParametersText);
            }

            if( m_appType == AppType.Deploy )
            {
                sbParameters.AppendFormat("{0}{1}{4}{2}{3}", preText, Parameters.DeployToOverride, m_deployToOverride.ToString(), postText, separatorText);
            }

            if( m_appType == AppType.Pack || ( m_silent && m_appType == AppType.Sync ) )
                sbParameters.AppendFormat("{0}{1}{4}{2}{3}",preText,Parameters.Silent,m_silent ? Commands.Yes : Commands.No,postText,separatorText);

            if( m_onExit.Length > 0 )
                sbParameters.AppendFormat("{0}{1}{4}{2}{3}",preText,Parameters.OnExit,GenerateFilename(options,m_onExit),postText,separatorParametersText);

			if( m_verbose || sbParameters.Length > 0 )
			{
                sb.AppendLine();
                if( writeHeaders)
                    sb.AppendFormat("{0}{1}{2}",preText,Sections.Parameters,postText);
				sb.Append(sbParameters);
			}


            // [DataEntryIds]
            if( m_appType == AppType.Entry && m_dataEntryIds.Count > 0 )
            {
                sb.AppendLine();
                if( writeHeaders )
                    sb.AppendFormat("{0}{1}{2}",preText,Sections.DataEntryIds,postText);

                string headerPrefix = GetHeaderPrefix(options, Sections.DataEntryIds);

                foreach( var kp in m_dataEntryIds )
                    sb.AppendFormat("{0}{5}{1}{4}{2}{3}",preText,kp.Key,kp.Value,postText,separatorParametersText,headerPrefix);
            }


            if( createLogic )
            {
                if( options.m_usingPffObject )
                {
                    if( options.m_runAfterCreating )
                        sb.AppendFormat("\r\n\t{0}.exec();\r\n",objectName);
                }

                else
                {
                    sb.AppendFormat("\r\n\tclose({0});\r\n",objectName);

                    if( options.m_runAfterCreating )
                        sb.AppendFormat("\r\n\texecpff(filename({0}));\r\n",objectName);
                }

                sb.Append("\r\nend;\r\n");
            }

            string contents = sb.ToString();

            // remove any excessive newlines (which can occur when generating logic with the pff object)
            while( true )
            {
                string new_contents = contents.Replace("\r\n\r\n\r\n","\r\n\r\n");

                if( contents.Length == new_contents.Length )
                    break;

                contents = new_contents;
            }
            
            return contents;
        }



        public class Sections
        {
            public const string RunInformation = "[Run Information]";
            public const string DataEntryInit = "[DataEntryInit]";
            public const string Files = "[Files]";
            public const string ExternalFiles = "[ExternalFiles]";
            public const string UserFiles = "[UserFiles]";
            public const string Parameters = "[Parameters]";
            public const string DataEntryIds = "[DataEntryIds]";
        }

        public class RunInformation
        {
            public const string Version = "Version";
            public const string AppType = "AppType";
            public const string Description = "Description";
            public const string Operation = "Operation";
        }

        public class DataEntryInit
        {
            public const string OperatorID = "OperatorID";
            public const string StartMode = "StartMode";
            public const string Key = "Key";
            public const string NoFileOpen = "NoFileOpen";
            public const string Lock = "Lock";
            public const string ShowInApplicationListing = "ShowInApplicationListing";
            public const string Interactive = "Interactive";
            public const string FullScreen = "FullScreen";
            public const string AutoAdd = "AutoAdd";
            public const string Language = "Language";
            public const string CaseListingFilter = "CaseListingFilter";
        }

        public class Files
        {
            public const string Application = "Application";
            public const string InputData = "InputData";
            public const string Excel = "Excel";
            public const string ExportedData = "ExportedData";
            public const string OutputData = "OutputData";
            public const string InputDict = "InputDict";
            public const string OutputDict = "OutputDict";
            public const string ReferenceData = "ReferenceData";
            public const string TabOutputTAB = "TabOutputTAB";
            public const string ConInputTAB = "ConInputTAB";
            public const string ConOutputTAB = "ConOutputTAB";
            public const string FormatInputTAB = "FormatInputTAB";
            public const string OutputTBW = "OutputTBW";
            public const string AreaNames = "AreaNames";
            public const string TablesOutput = "TablesOutput";
            public const string SPSSDescFile = "SPSSDescFile";
            public const string SASDescFile = "SASDescFile";
            public const string STATADescFile = "STATADescFile";
            public const string STATALabelsFile = "STATALabelsFile";
            public const string CSProDescFile = "CSProDescFile";
            public const string RDescFile = "RDescFile";
            public const string ZipFile = "ZipFile";
            public const string ExtraFile = "ExtraFile";
            public const string InputParadata = "InputParadata";
            public const string OutputParadata = "OutputParadata";
            public const string Paradata = "Paradata";
            public const string Listing = "Listing";
            public const string Freqs = "Freqs";
            public const string ImputeFreqs = "ImputeFreqs";
            public const string ImputeStat = "ImputeStat";
            public const string WriteData = "WriteData";
            public const string SaveArray = "SaveArray";
            public const string CommonStore = "CommonStore";
            public const string HtmlDialogs = "HtmlDialogs";
            public const string BaseMap = "BaseMap";
        }

        public class Parameters
        {
            public const string ViewResults = "ViewResults";
            public const string SkipStructure = "SkipStructure";
            public const string CheckRanges = "CheckRanges";
            public const string ViewListing = "ViewListing";
            public const string Parameter = "Parameter";
            public const string Silent = "Silent";
            public const string PackIncludes = "PackInclude";
            public const string DuplicateCase = "DuplicateCase";
            public const string ListingWidth = "ListingWidth";
            public const string MessageWrap = "MessageWrap";
            public const string ErrmsgOverride = "ErrmsgOverride";
            public const string DisplayNames = "DisplayNames";
            public const string InputOrder = "InputOrder";
            public const string ConcatMethod = "ConcatMethod";
            public const string OnExit = "OnExit";
            public const string SyncType = "SyncType";
            public const string SyncDirection = "SyncDirection";
            public const string SyncUrl = "SyncUrl";
            public const string DeployToOverride = "DeployToOverride";
        }

        public class Commands
        {
            public const string Add = "Add";
            public const string All = "All";
            public const string Always = "Always";
            public const string Ask = "Ask";
            public const string Batch = "Batch";
            public const string Both = "Both";
            public const string Case = "Case";
            public const string CaseListing = "CaseListing";
            public const string Compare = "Compare";
            public const string Con = "Con";
            public const string Concatenate = "Concatenate";
            public const string CSWeb = "CSWeb";
            public const string Delete = "Delete";
			public const string Deploy = "Deploy";
            public const string Dropbox = "Dropbox";
            public const string Entry = "Entry";
            public const string ErrMsg = "ErrMsg";
			public const string Excel2CSPro = "Excel2CSPro";
            public const string Export = "Export";
            public const string ExternalFiles = "ExternalFiles";
            public const string Format = "Format";
            public const string Frequencies = "Frequencies";
            public const string FTP = "FTP";
            public const string Get = "Get";
            public const string Hidden = "Hidden";
            public const string Index = "Index";
            public const string Indexed = "Indexed";
            public const string InputFile = "InputFile";
            public const string InputFiles = "InputFiles";
            public const string KeepFirst = "KeepFirst";
            public const string List = "List";
            public const string LocalDropbox = "LocalDropbox";
            public const string LocalFiles = "LocalFiles";
            public const string Lock = "Lock";
            public const string Modify = "Modify";
            public const string Never = "Never";
            public const string No = "No";
            public const string NoMenus = "NoMenus";
            public const string Off = "Off";
            public const string OnError = "OnError";
            public const string Pack = "Pack";
			public const string ParadataConcatenate = "ParadataConcatenate";
            public const string Prompt = "Prompt";
            public const string PromptIfDifferent = "PromptIfDifferent";
            public const string Put = "Put";
            public const string Range = "Range";
            public const string Reformat = "Reformat";
            public const string Resources = "Resources";
            public const string Sequential = "Sequential";
            public const string Sort = "Sort";
            public const string Stats = "Stats";
            public const string Summary = "Summary";
            public const string Sync = "Sync";
            public const string Tab = "Tab";
            public const string Tabulation = "Tabulation";
            public const string Text = "Text";
            public const string UserFiles = "UserFiles";
            public const string Verify = "Verify";
            public const string View = "View";
            public const string VSImages = "VSImages";
            public const string Yes = "Yes";
        }
    }
}
