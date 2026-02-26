using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using Ionic.Zip;
using LibGit2Sharp;
using RestSharp;

namespace CSPro_Installer_Generator
{
    partial class MainForm : Form
    {
        private string _settingsFilename;

        private bool _beta;
        private int _versionMajor;
        private int _versionMinor;
        private int _versionBuild;
        private DateTime _releaseDate;

        CommonPaths _commonPaths;

        private string _installerExe;

        private string _componentsDirectory;
        private string _componentsExamplesDirectory;
        private string _componentsHelpsDirectory;
        private string _componentsMiscellaneousDirectory;
        private string _componentsRedistributablesDirectory;
        private string _componentsReleaseDirectory;
        private string _componentsToolsDirectory;

        public MainForm()
        {
            InitializeComponent();
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            try
            {
                _commonPaths = new CommonPaths();
                textBoxRoot.Text = _commonPaths.RootDirectory;

                string settings_directory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "CSPro");
                _settingsFilename = Path.Combine(settings_directory, "Installer Generator.txt");

                if( !File.Exists(_settingsFilename) )
                {
                    Directory.CreateDirectory(settings_directory);
                    File.WriteAllText(_settingsFilename, Properties.Resources.DefaultSettings);
                }
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
                Close();
                return;
            }

            // load the settings
            try
            {
                using( TextReader tr = new StreamReader(_settingsFilename, Encoding.UTF8) )
                {
                    textBoxExamples.Text = tr.ReadLine();
                    textBoxHelps.Text = tr.ReadLine();
                    textBoxMSBuild.Text = tr.ReadLine();
                    textBoxNSIS.Text = tr.ReadLine();
                    textBoxGitHubPAT.Text = tr.ReadLine();
                    checkBoxCheckGithub.Checked = ( tr.ReadLine() == "1" );
                    checkBoxBuildCSPro.Checked = ( tr.ReadLine() == "1" );
                    checkBoxBuildTools.Checked = ( tr.ReadLine() == "1" );
                    checkBoxBuildHelps.Checked = ( tr.ReadLine() == "1" );
                    ( ( tr.ReadLine() == "32" ) ? radioButton32Bit : radioButton64Bit ).Checked = true;
                }

                AddToLog(LogType.Log, "Successfully loaded settings.", LogExtra.EndSection);
            }

            // ignore read errors
            catch( Exception )
            {
                AddToLog(LogType.Log, "The settings file could not be found. Enter the settings.", LogExtra.EndSection);
            }
        }

        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            // save the settings
            try
            {
                using( TextWriter tw = new StreamWriter(_settingsFilename, false, Encoding.UTF8) )
                {
                    tw.WriteLine(textBoxExamples.Text);
                    tw.WriteLine(textBoxHelps.Text);
                    tw.WriteLine(textBoxMSBuild.Text);
                    tw.WriteLine(textBoxNSIS.Text);
                    tw.WriteLine(textBoxGitHubPAT.Text);
                    tw.WriteLine(checkBoxCheckGithub.Checked ? "1" : "0");
                    tw.WriteLine(checkBoxBuildCSPro.Checked ? "1" : "0");
                    tw.WriteLine(checkBoxBuildTools.Checked ? "1" : "0");
                    tw.WriteLine(checkBoxBuildHelps.Checked ? "1" : "0");
                    tw.WriteLine(radioButton32Bit.Checked ? "32" : "64");
                }
            }

            // ignore write errors
            catch( Exception )
            {
            }
        }

        private enum LogType { Log, Warning, Error };
        private enum LogExtra { None, InSection, EndSection };

        private void AddToLog(LogType type, string message, LogExtra extra = LogExtra.None)
        {
            if( extra == LogExtra.InSection )
                message = $"        {message}";

            string formatted_message = $"{DateTime.Now.ToShortTimeString()}: {message}\r\n";

            textBoxLog.AppendText(formatted_message);

            if( extra == LogExtra.EndSection )
                textBoxLog.AppendText("\r\n");

            if( type != LogType.Log )
            {
                textBoxErrors.AppendText(formatted_message);

                if( type == LogType.Error )
                    MessageBox.Show(message);
            }
        }

        private enum BuildMessage { Beginning, Build, Ending };

        private void AddBuildMessageToLog(BuildMessage build_message, string build_type, bool build, bool copy)
        {
            string action = ( build_message == BuildMessage.Beginning ) ?
                ( ( build && copy ) ? "Building and copying" : build ? "Building" : "Copying but not building" ) :
                ( ( build && copy ) ? "Successfully built and copied" : build ? "Successfully built" : "Successfully copied" );

            string ellipses = ( build_message == BuildMessage.Beginning ) ? "..." : "";

            LogExtra log_extra = ( build_message == BuildMessage.Beginning ) ? LogExtra.None :
                                 ( build_message == BuildMessage.Build )     ? LogExtra.InSection :
                                                                               LogExtra.EndSection;

            AddToLog(( build || build_message != BuildMessage.Beginning ) ? LogType.Log : LogType.Warning,
                $"{action} {build_type}{ellipses}", log_extra);
        }

        private void buttonClearLogs_Click(object sender, EventArgs e)
        {
            textBoxLog.Clear();
            textBoxErrors.Clear();
        }

        private void ResetPrepareValues()
        {
            buttonCreate.Enabled = false;
            textBoxBranchRoot.Text = "";
            textBoxBranchExamples.Text = "";
            textBoxBranchHelps.Text = "";
            labelBeta.Text = "...";
            labelVersion.Text = labelBeta.Text;
            labelReleaseDate.Text = labelBeta.Text;
        }

        private void textBoxDirectory_TextChanged(object sender, EventArgs e)
        {
            ResetPrepareValues();
        }

        private void buttonPrepare_Click(object sender, EventArgs e)
        {
            try
            {
                ResetPrepareValues();

                CheckInputs();

                CheckHasLatestFromGitHub();

                ReadVersionInformation();

                buttonCreate.Enabled = true;
            }

            catch( Exception exception )
            {
                AddToLog(LogType.Error, exception.Message);
            }
        }

        private Inputs CheckInputs()
        {
            try
            {
                AddToLog(LogType.Log, "Checking the inputs...");

                if( !Directory.Exists(textBoxExamples.Text) )
                    throw new Exception("examples");

                if( !Directory.Exists(textBoxHelps.Text) )
                    throw new Exception("helps");
            }

            catch( Exception exception )
            {
                throw new Exception($"You must specify a valid {exception.Message} directory.");
            }

            if( !File.Exists(textBoxMSBuild.Text) )
                throw new Exception($"Could not find MSBuild here: {textBoxMSBuild.Text}");

            if( !File.Exists(textBoxNSIS.Text) )
                throw new Exception($"Could not find NSIS here: {textBoxNSIS.Text}");

            Build.Is32Bit = radioButton32Bit.Checked;

            Inputs inputs = new Inputs(_commonPaths);

            AddToLog(LogType.Log, "Successfully checked the inputs.", LogExtra.EndSection);

            return inputs;
        }


        private class GitHubCommit
        {
            public string sha { get; set; }

            public class Commit
            {
                public string message { get; set; }
            }

            public Commit commit { get; set; }
        }

        private void CheckHasLatestFromGitHub()
        {
            if( !checkBoxCheckGithub.Checked )
            {
                AddToLog(LogType.Warning, "Skipped GitHub update checks.", LogExtra.EndSection);
                return;
            }

            var repository_details = new Tuple<string, string, TextBox, bool>[]
            {
                Tuple.Create("cspro", textBoxRoot.Text, textBoxBranchRoot, true),
                Tuple.Create("examples", textBoxExamples.Text, textBoxBranchExamples, false),
                Tuple.Create("helps", textBoxHelps.Text, textBoxBranchHelps, false),
            };

            foreach( ( string repository_name, string directory, TextBox branch_text_box, bool requires_authentication ) in repository_details )
            {
                AddToLog(LogType.Log, $"Checking GitHub that the repository in '{directory}' has the latest...");

                string branch_name;
                string last_local_commit;
                string github_url;

                using( var repository = new Repository(directory) )
                {
                    AddToLog(LogType.Log, "Checking the local git repository...", LogExtra.InSection);

                    // check if there are fetched commits that haven't been applied yet
                    branch_name = repository.Head.FriendlyName;
                    branch_text_box.Text = branch_name;

                    if( !repository.Head.IsTracking )
                        throw new Exception($"The repository '{repository_name}' is on a branch '{branch_name}' that is not tracked.");

                    var current_position = repository.Refs[$"refs/heads/{branch_name}"];
                    var fetched_position = repository.Refs[$"refs/remotes/origin/{branch_name}"];

                    if( current_position.TargetIdentifier != fetched_position.TargetIdentifier )
                        throw new Exception($"The repository '{repository_name}' does not have the latest because there are fetched commits that have not yet been applied.");

                    // check if there are pending changes
                    if( repository.RetrieveStatus().IsDirty )
                        throw new Exception($"The repository '{repository_name}' has uncommitted changes. Commit them before continuing.");

                    last_local_commit = current_position.TargetIdentifier;
                    github_url = repository.Network.Remotes.First().Url;

                    AddToLog(LogType.Log, $"The last commit is {last_local_commit}.", LogExtra.InSection);
                }

                if( requires_authentication && string.IsNullOrWhiteSpace(textBoxGitHubPAT.Text) )
                {
                    AddToLog(LogType.Log, $"Skipping GitHub check for {github_url} because a personal access token has not provided.".ToUpper(), LogExtra.InSection);
                    AddToLog(LogType.Log, $"The repository '{repository_name}' may not have the latest.".ToUpper(), LogExtra.EndSection);
                }

                else
                {
                    AddToLog(LogType.Log, $"Checking GitHub at {github_url}...", LogExtra.InSection);

                    string[] url_elements = github_url.Split(new char[] { '/' });
                    string owner = url_elements[url_elements.Length - 2];

                    // handle SSH urls that are of the form user@github.com:owner
                    int colon_pos = owner.IndexOf(':');
                    if( colon_pos >= 0 )
                        owner = owner.Substring(colon_pos + 1);

                    var client = new RestClient("https://api.github.com");

                    var request = new RestRequest($"repos/{owner}/{repository_name}/commits/{branch_name}", Method.Get);

                    if( requires_authentication )
                        request.AddHeader("Authorization", $"token {textBoxGitHubPAT.Text}");

                    var response = client.Execute<List<GitHubCommit>>(request);
                    var last_remote_commit = response.Data.First();

                    if( last_remote_commit?.sha == null )
                        throw new Exception("Unexpected data was received from GitHub.");

                    AddToLog(LogType.Log, $"The latest commit in '{repository_name}' is {last_remote_commit.sha}, \"{last_remote_commit.commit.message}\"", LogExtra.InSection);

                    if( last_local_commit != last_remote_commit.sha )
                        throw new Exception($"The repository '{repository_name}' does not have the latest.");

                    AddToLog(LogType.Log, $"The repository '{repository_name}' has the latest.", LogExtra.EndSection);
                }
            }
        }


        private void ReadVersionInformation()
        {
            // read in the defines
            var defines = new Dictionary<string, string>();

            foreach( string extension in new string[] { ".h", ".cpp" } )
            {
                string header_filename = Path.Combine(_commonPaths.CSProDirectory, @"zUtilO\Versioning" + extension);

                AddToLog(LogType.Log, $"Reading the version information from {header_filename}...");

                var constexpr_regex = new Regex(@"^.*constexpr.*\s(\S+)\s*=\s*(\S+);\s*$");
                var quote_chars = new char[] { '\"' };

                foreach( string full_line in File.ReadAllLines(header_filename) )
                {
                    var match = constexpr_regex.Match(full_line);

                    if( match.Success )
                        defines[match.Groups[1].Value] = match.Groups[2].Value.Trim(quote_chars);
                }
            }

            _beta = ( defines["ReleaseType"] != "ReleaseType::Release" );
            labelBeta.Text = _beta ? "Beta" : "Release";

            string version = defines["NumberDetailedTextOverride_sv"];

            if( version == "x.x.x" )
                version = defines["NumberDetailedText"];

            string[] versions = version.Split('.');
            _versionMajor = int.Parse(versions[0]);
            _versionMinor = int.Parse(versions[1]);
            _versionBuild = int.Parse(versions[2]);
            labelVersion.Text = $"{_versionMajor}.{_versionMinor}.{_versionBuild}";

            // for the release time, use the date that the installer is being created
            int release_date_yyyymmdd = int.Parse(defines["CSProReleaseDate"]);
            _releaseDate = new DateTime(release_date_yyyymmdd / 10000,
                                        ( release_date_yyyymmdd / 100 ) % 100,
                                        release_date_yyyymmdd % 100,
                                        DateTime.Now.Month, DateTime.Now.Day, 0, DateTimeKind.Local);
            labelReleaseDate.Text = _releaseDate.ToString("yyyy-MM-dd");
            labelReleaseTime.Text = _releaseDate.ToString("HH:mm:ss");

            AddToLog(LogType.Log, "Successfully read the version information.", LogExtra.EndSection);
        }


        private void SetUpPaths()
        {
            // set up the paths
            _installerExe = Path.Combine(_commonPaths.InstallerDirectory, $"Installer\\cspro{_versionMajor}{_versionMinor}{( Build.Is32Bit ? "" : "-x64" )}.exe");

            _componentsDirectory = Path.Combine(_commonPaths.InstallerDirectory, "Components");
            _componentsExamplesDirectory = Path.Combine(_componentsDirectory, "Examples");
            _componentsHelpsDirectory = Path.Combine(_componentsDirectory, "Helps");
            _componentsMiscellaneousDirectory = Path.Combine(_componentsDirectory, "Miscellaneous");
            _componentsRedistributablesDirectory = Path.Combine(_componentsDirectory, "Redistributables");
            _componentsReleaseDirectory = Path.Combine(_componentsDirectory, "Release");
            _componentsToolsDirectory = Path.Combine(_componentsDirectory, "CSPro Users Tools");
        }


        private void buttonCreate_Click(object sender, EventArgs e)
        {
            try
            {
                DateTime start_time = DateTime.Now;

                SetUpPaths();

                CleanComponents();

                Inputs inputs = new Inputs(_commonPaths);

                BuildCSPro(inputs, checkBoxBuildCSPro.Checked, true, true, true);

                BuildTools(inputs, checkBoxBuildTools.Checked, true, true);

                CopyExamples();

                BuildHelps(inputs, checkBoxBuildHelps.Checked, true);

                CopyMiscellaneous(inputs);

                // timestamp before copying the redistributables
                TimestampComponents();

                CopyRedistributables();

                CreateInstaller();

                // if the installer was created successfully, open it in Windows Explorer
                Process.Start("explorer.exe", $"/select,\"{_installerExe}\"");

                // display the amount of time it took to create the installer
                TimeSpan build_time = DateTime.Now.Subtract(start_time);
                AddToLog(LogType.Log, $"Creating the installer took {((int)( build_time.TotalSeconds / 60 ))}:{build_time.Seconds:D2}.");
            }

            catch( Exception exception )
            {
                AddToLog(LogType.Error, exception.Message);
            }
        }


        private void CleanComponents()
        {
            AddToLog(LogType.Log, "Removing all existing components...");

            CleanComponents(Directory.CreateDirectory(_componentsDirectory), 0, null);

            AddToLog(LogType.Log, "Successfully removed all existing components.", LogExtra.EndSection);
        }


        private Tuple<int, int> CleanComponents(DirectoryInfo di, int subdirectory_depth, Tuple<int, int> deletion_counter)
        {
            if( subdirectory_depth < 2 )
                deletion_counter = Tuple.Create(0, 0);

            foreach( var sub_di in di.GetDirectories() )
            {
                deletion_counter = CleanComponents(sub_di, subdirectory_depth + 1, deletion_counter);
                Directory.Delete(sub_di.FullName);
                deletion_counter = Tuple.Create(deletion_counter.Item1 + 1, deletion_counter.Item2);
            }

            foreach( var fi in di.GetFiles() )
            {
                File.SetAttributes(fi.FullName, FileAttributes.Normal);
                File.Delete(fi.FullName);
                deletion_counter = Tuple.Create(deletion_counter.Item1, deletion_counter.Item2 + 1);
            }

            if( subdirectory_depth == 1 )
                AddToLog(LogType.Log, $"Removed {deletion_counter.Item2} files and {deletion_counter.Item1} directories from {Path.GetFileName(di.Name)}.", LogExtra.InSection);

            return deletion_counter;
        }


        private void CopyExamples()
        {
            AddToLog(LogType.Log, "Copying examples...");

            int directories = 0;
            int files = 0;

            CopyExamples(new DirectoryInfo(textBoxExamples.Text), ref directories, ref files);

            AddToLog(LogType.Log, $"Successfully copied {files} files and {directories} directories as examples.", LogExtra.EndSection);
        }

        private void CopyExamples(DirectoryInfo di, ref int directories, ref int files, string current_copy_directory = null)
        {
            // ignore the files in the root directory and the .git directory
            if( current_copy_directory != null )
            {
                foreach( var fi in di.GetFiles() )
                {
                    File.Copy(fi.FullName, Path.Combine(current_copy_directory, fi.Name));
                    ++files;
                }
            }

            foreach( var sub_di in di.GetDirectories() )
            {
                // skip hidden folders like .git and .vs
                if( sub_di.Name[0] == '.' )
                    continue;

                string sub_directory_copy_directory = Path.Combine(( current_copy_directory == null ) ? _componentsExamplesDirectory : current_copy_directory, sub_di.Name);
                Directory.CreateDirectory(sub_directory_copy_directory);

                CopyExamples(sub_di, ref directories, ref files, sub_directory_copy_directory);

                ++directories;
            }
        }


        private void BuildHelps(Inputs inputs, bool build_helps, bool copy_helps)
        {
            const string build_type = "help document";

            AddBuildMessageToLog(BuildMessage.Beginning, $"{build_type}s", build_helps, copy_helps);

            if( copy_helps )
                Directory.CreateDirectory(_componentsHelpsDirectory);

            if( build_helps )
            {
                string command_line = "-build \"Compiled HTML Help\" -input";

                foreach( string program in inputs.Helps )
                    command_line = command_line + $" \"{Path.Combine(textBoxHelps.Text, program, $"{program}.csdocset")}\"";

                AddToLog(LogType.Log, $"Building helps using CSDocument...", LogExtra.InSection);

                var process = new Process();
                process.StartInfo = new ProcessStartInfo(_commonPaths.GetLatestCSDocument(), command_line);
                process.Start();
                process.WaitForExit();

                if( process.ExitCode != 0 )
                    throw new Exception($"There was an error building the helps using the arguments: {command_line}");
            }

            int number_helps = 0;

            foreach( string program in inputs.Helps )
            {
                string chm_filename = Path.Combine(textBoxHelps.Text, @"Outputs\CHM", $"{program}.chm");

                if( !File.Exists(chm_filename) )
                    throw new Exception($"The {build_type} {program} was not created successfully.");

                if( copy_helps )
                    File.Copy(chm_filename, Path.Combine(_componentsHelpsDirectory, Path.GetFileName(chm_filename)));

                AddBuildMessageToLog(BuildMessage.Build, $"{build_type} {Path.GetFileName(chm_filename)}", build_helps, copy_helps);

                ++number_helps;
            }

            AddBuildMessageToLog(BuildMessage.Ending, $"{number_helps} {build_type}s", build_helps, copy_helps);
        }


        private void CopyRedistributables()
        {
            AddToLog(LogType.Log, "Copying redistributables...");

            Directory.CreateDirectory(_componentsRedistributablesDirectory);

            int files = 0;

            foreach( string redistributable_with_wildcards in File.ReadAllLines(Path.Combine(_commonPaths.InstallerDirectory, "redistributables.txt")) )
            {
                // get the right redistributables for the target architecture
                string redistributable = redistributable_with_wildcards.Replace("{PlatformTarget}", Build.PlatformTarget);

                // a pipe character allows for multiple paths to be specified as options
                string[] redistributable_options = redistributable.Split(new char[] { '|' });

                for( int i = 0; i < redistributable_options.Length; ++i )
                {
                    try
                    {
                        string directory = Path.GetDirectoryName(redistributable_options[i]);
                        string wildcard = Path.GetFileName(redistributable_options[i]);

                        // process each component to get the full directory name
                        string[] directory_components = directory.Split(new char[] { '\\' });
                        directory = "";

                        foreach( string directory_component in directory_components )
                        {
                            string directory_name_to_add = directory_component;

                            if( directory_component.Contains("*") || directory_component.Contains("?") )
                            {
                                var possible_directories = new DirectoryInfo(directory).GetDirectories(directory_component);

                                if( possible_directories.Length == 0 )
                                    throw new Exception($"The redistributable directory with wildcard {directory_component} was not found.");

                                // use the last directory (which should be Professional vs. Community or should be the highest version number)
                                directory_name_to_add = possible_directories.Last().Name;
                            }

                            directory = Path.Combine(directory, directory_name_to_add + '\\');

                            if( !Directory.Exists(directory) )
                                throw new Exception($"The redistributable directory {directory} was not found.");
                        }

                        var file_listings = new DirectoryInfo(directory).GetFiles(wildcard);

                        if( file_listings.Length == 0 )
                            throw new Exception($"No redistributables found here: {redistributable}");

                        foreach( var fi in file_listings )
                        {
                            File.Copy(fi.FullName, Path.Combine(_componentsRedistributablesDirectory, fi.Name));
                            AddToLog(LogType.Log, $"Copied redistributable {fi.FullName}", LogExtra.InSection);
                            ++files;
                        }

                        // if successful, break out of the loop instead of trying the other options
                        break;
                    }

                    catch( Exception exception )
                    {
                        // ignore errors unless they occur on the last option
                        if( i == ( redistributable_options.Length - 1 ) )
                            throw exception;
                    }
                }
            }

            AddToLog(LogType.Log, $"Successfully copied {files} files as redistributables.", LogExtra.EndSection);
        }


        private void BuildTools(Inputs inputs, bool build_tools, bool copy_tools, bool rebuild)
        {
            const string build_type = "tool";

            AddBuildMessageToLog(BuildMessage.Beginning, $"{build_type}s", build_tools, copy_tools);

            if( copy_tools )
                Directory.CreateDirectory(_componentsToolsDirectory);

            int number_tools = 0;

            foreach( Inputs.Tool tool in inputs.Tools )
            {
                string solution = Path.Combine(_commonPaths.ToolsDirectory, tool.directory, $"{tool.solution}.sln");
                var build = new Build(textBoxMSBuild.Text, solution, true);

                if( build_tools )
                {
                    AddToLog(LogType.Log, $"Building {tool.solution}...", LogExtra.InSection);
                    build.Run(rebuild);
                }

                string exe = build.GetExecutableFilePath(tool.solution);

                if( !File.Exists(exe) )
                    throw new Exception($"The {build_type} {tool.solution} was not created successfully.");

                if( copy_tools )
                    File.Copy(exe, Path.Combine(_componentsToolsDirectory, Path.GetFileName(exe)));

                AddBuildMessageToLog(BuildMessage.Build, $"{build_type} {exe}", build_tools, copy_tools);

                ++number_tools;
            }

            AddBuildMessageToLog(BuildMessage.Ending, $"{number_tools} {build_type}s", build_tools, copy_tools);
        }


        private void CopyMiscellaneous(Inputs inputs)
        {
            AddToLog(LogType.Log, "Creating and copying miscellaneous files...");

            Directory.CreateDirectory(_componentsMiscellaneousDirectory);

            int files = 0;

            foreach( Inputs.Asset asset in inputs.Assets )
            {
                string destination_directory = Path.Combine(_componentsMiscellaneousDirectory, asset.destination_directory);
                Directory.CreateDirectory(destination_directory);

                string destination_filename = Path.Combine(destination_directory, Path.GetFileName(asset.source_path));

                File.Copy(asset.source_path, destination_filename);
                AddToLog(LogType.Log, $"Copied miscellaneous file {asset.source_path}", LogExtra.InSection);
                ++files;
            }

            // create the Notepad++ colorization file
            string csdocument_exe = _commonPaths.GetLatestCSDocument();

            AddToLog(LogType.Log, "Creating the Notepad++ colorization file...", LogExtra.InSection);

            var process = new Process();
            process.StartInfo = new ProcessStartInfo(csdocument_exe, "/Notepad++");
            process.Start();
            process.WaitForExit();

            string colorization_file = Path.Combine(Path.GetDirectoryName(csdocument_exe), "userDefineLang.xml");
            File.Move(colorization_file, Path.Combine(_componentsMiscellaneousDirectory, Path.GetFileName(colorization_file)));
            AddToLog(LogType.Log, $"Copied miscellaneous file {colorization_file}", LogExtra.InSection);
            ++files;

            AddToLog(LogType.Log, $"Successfully copied miscellaneous {files} files.", LogExtra.EndSection);
        }


        private void TimestampComponents()
        {
            AddToLog(LogType.Log, $"Timestamping the component files with the date \"{_releaseDate.ToShortDateString()} {_releaseDate.ToShortTimeString()}\"...");

            int directories = 0;
            int files = 0;

            TimestampDirectory(new DirectoryInfo(_componentsDirectory), ref directories, ref files);

            AddToLog(LogType.Log, $"Successfully timestamped {files} files in {directories} directories.", LogExtra.EndSection);
        }

        private void TimestampDirectory(DirectoryInfo di, ref int directories, ref int files, bool display_message = false)
        {
            foreach( var sub_di in di.GetDirectories() )
            {
                TimestampDirectory(sub_di, ref directories, ref files, display_message);
                ++directories;
            }

            foreach( var fi in di.GetFiles() )
            {
                TimestampFile(fi.FullName, display_message);
                ++files;
            }
        }

        private void TimestampFile(string filename, bool display_message = false)
        {
            File.SetCreationTime(filename, _releaseDate);
            File.SetLastAccessTime(filename, _releaseDate);
            File.SetLastWriteTime(filename, _releaseDate);

            if( display_message )
                AddToLog(LogType.Log, $"Timestamped the file {filename} with the date \"{_releaseDate.ToShortDateString()} {_releaseDate.ToShortTimeString()}.\"", LogExtra.InSection);
        }


        private void CreateInstaller()
        {
            AddToLog(LogType.Log, "Building the installer...");

            Directory.CreateDirectory(Path.GetDirectoryName(_installerExe));

            // write out the variables needed for the installer
            string nsis_directory = Path.Combine(_commonPaths.InstallerDirectory, "NSIS");

            using( TextWriter tw = new StreamWriter(Path.Combine(nsis_directory, "constants_from_installer_generator.nsh"), false, Encoding.ASCII) )
            {
                tw.WriteLine($"!define VERSIONMAJOR {_versionMajor}");
                tw.WriteLine($"!define VERSIONMINOR {_versionMinor}");
                tw.WriteLine($"!define VERSIONBUILD {_versionBuild}");
                tw.WriteLine($"!define IS_WIN32 {( Build.Is32Bit ? "1" : "0" )}");
                tw.WriteLine($"!define ISBETA {( _beta ? "1" : "0" )}");
                tw.WriteLine($"!define DATE \"{_releaseDate.ToString("dd MMMM yyyy")}\"");
                tw.WriteLine($"OutFile \"{_installerExe}\"");
            }

            var process = new Process();
            process.StartInfo = new ProcessStartInfo(textBoxNSIS.Text, $"\"{Path.Combine(nsis_directory, "cspro-installer.nsi")}\"");
            process.Start();
            process.WaitForExit();

            if( !File.Exists(_installerExe) )
                throw new Exception("There was an error creating the installer.");

            TimestampFile(_installerExe, true);

            AddToLog(LogType.Log, "Successfully built the installer.", LogExtra.EndSection);
        }


        private void BuildCSPro(Inputs inputs, bool build_cspro, bool copy_cspro, bool rebuild, bool release)
        {
            const string build_type = "CSPro";

            AddBuildMessageToLog(BuildMessage.Beginning, build_type, build_cspro, copy_cspro);

            if( copy_cspro )
                Directory.CreateDirectory(_componentsReleaseDirectory);

            string solution = Path.Combine(_commonPaths.CSProDirectory, "cspro.sln");
            var build = new Build(textBoxMSBuild.Text, solution, release);

            if( build_cspro )
            {
                AddToLog(LogType.Log, "Building the solution...", LogExtra.InSection);
                build.Run(rebuild);
            }

            int files = 0;

            foreach( string bin in inputs.Bin )
            {
                string release_file = build.GetBuiltFilePath(bin);

                if( !File.Exists(release_file) )
                    throw new Exception($"The file {Path.GetFileName(release_file)} was not created successfully.");

                if( copy_cspro )
                    File.Copy(release_file, Path.Combine(_componentsReleaseDirectory, Path.GetFileName(release_file)));

                AddBuildMessageToLog(BuildMessage.Build, $"{build_type} file {release_file}", build_cspro, copy_cspro);

                ++files;
            }

            AddBuildMessageToLog(BuildMessage.Ending, $"{files} {build_type} files", build_cspro, copy_cspro);
        }


        private void comboBoxBuildActions_SelectedIndexChanged(object sender, EventArgs e)
        {
            if( comboBoxBuildActions.SelectedIndex < 0 )
                return;

            try
            {
                Inputs inputs = CheckInputs();

                switch( comboBoxBuildActions.SelectedIndex )
                {
                    case 0:
                        BuildCSPro(inputs, true, false, false, false); // debug
                        break;

                    case 1:
                        BuildCSPro(inputs, true, false, false, true); // release
                        break;

                    case 2:
                        BuildTools(inputs, true, false, false);
                        break;

                    case 3:
                        BuildHelps(inputs, true,false);
                        break;
                }
            }

            catch( Exception exception )
            {
                AddToLog(LogType.Error, exception.Message);
            }

            comboBoxBuildActions.SelectedIndex = -1;
        }

        private void comboBoxZipActions_SelectedIndexChanged(object sender, EventArgs e)
        {
            if( comboBoxZipActions.SelectedIndex < 0 )
                return;

            try
            {
                bool rebuild_programs = ( comboBoxZipActions.SelectedIndex == 2 || comboBoxZipActions.SelectedIndex == 5 );
                bool build_programs = ( rebuild_programs || comboBoxZipActions.SelectedIndex == 1 || comboBoxZipActions.SelectedIndex == 4 );
                bool include_helps = ( comboBoxZipActions.SelectedIndex >= 3 );

                Inputs inputs = CheckInputs();

                ReadVersionInformation();

                SetUpPaths();

                CleanComponents();

                string zip_exe = Path.Combine(Path.GetDirectoryName(_installerExe), Path.GetFileNameWithoutExtension(_installerExe) + ".zip");
                File.Delete(zip_exe);

                BuildCSPro(inputs, build_programs, true, rebuild_programs, true);

                BuildTools(inputs, build_programs, true, rebuild_programs);

                CopyMiscellaneous(inputs);

                if( include_helps )
                    BuildHelps(inputs, build_programs, true);

                // timestamp before copying the redistributables
                TimestampComponents();

                CopyRedistributables();

                // create the zip file
                using( ZipFile zip = new ZipFile() )
                {
                    AddFilesToZip(zip, new DirectoryInfo(_componentsDirectory), 0);
                    zip.Save(zip_exe);
                }

                // go to the zip file in Windows Explorer
                Process.Start("explorer.exe", $"/select,\"{zip_exe}\"");
            }

            catch( Exception exception )
            {
                AddToLog(LogType.Error, exception.Message);
            }

            comboBoxZipActions.SelectedIndex = -1;
        }

        private void AddFilesToZip(ZipFile zip, DirectoryInfo di, int subdirectory_depth)
        {
            foreach( var sub_di in di.GetDirectories() )
                AddFilesToZip(zip, sub_di, subdirectory_depth + 1);

            foreach( var fi in di.GetFiles() )
            {
                string zip_directory = "";

                if( subdirectory_depth >= 2 )
                {
                    int slash_pos = di.FullName.IndexOf('\\', _componentsDirectory.Length + 1);
                    zip_directory = di.FullName.Substring(slash_pos + 1);
                }

                zip.AddFile(fi.FullName, zip_directory);
            }
        }

        private void buttonRegenerateAssets_Click(object sender, EventArgs e)
        {
            try
            {
                ResetPrepareValues();

                Inputs inputs = CheckInputs();

                // create the debug build of CSPro
                string csentry_exe = Path.Combine(_commonPaths.CSProDebugDirectory, "CSEntry.exe");

                try
                {
                    BuildCSPro(inputs, true, false, false, false);
                }

                // force CSEntry to be built properly
                catch( Exception exception )
                {
                    if( !File.Exists(csentry_exe) )
                        throw exception;
                }

                AddToLog(LogType.Log, "Creating the assets...");

                // create the runtime messages
                string create_messages_batch_file = Path.Combine(_commonPaths.BuildToolsDirectory, "Messages - Create Assets.bat");
                string messages_filename = Path.Combine(_commonPaths.BuildToolsDirectory, "system.mgf");

                var process = new Process();
                process.StartInfo = new ProcessStartInfo(create_messages_batch_file);
                process.StartInfo.WorkingDirectory = _commonPaths.BuildToolsDirectory;
                process.Start();
                process.WaitForExit();

                if( !File.Exists(messages_filename) )
                    throw new Exception($"There was an error creating the {Path.GetFileName(messages_filename)} file.");

                // create the Simple CAPI example
                string simple_capi_ent_filename = Path.Combine(textBoxExamples.Text, @"1 - Data Entry\Simple CAPI\Simple CAPI.ent");
                string simple_capi_pff_filename = Path.Combine(Path.GetDirectoryName(simple_capi_ent_filename), "Simple CAPI.pff");
                string simple_capi_pen_filename = Path.Combine(Path.GetDirectoryName(simple_capi_ent_filename), "Simple CAPI.pen");

                process = new Process();
                process.StartInfo = new ProcessStartInfo(csentry_exe, $"/pen \"{simple_capi_ent_filename}\" /penName \"{simple_capi_pen_filename}\" /noSystemMessageSerialization");
                process.StartInfo.WorkingDirectory = Path.GetDirectoryName(simple_capi_ent_filename);
                process.Start();
                process.WaitForExit();

                var sources_and_destination_subdirectories = new List<Tuple<string, string>>
                {
                    Tuple.Create(messages_filename, ""),
                    Tuple.Create(simple_capi_pen_filename, "examples"),
                    Tuple.Create(simple_capi_pff_filename, "examples")
                };

                foreach( Inputs.Asset asset in inputs.NonDesktopAssets )
                    sources_and_destination_subdirectories.Add(Tuple.Create(asset.source_path, asset.destination_directory));

                foreach( string assets_directory in _commonPaths.AssetsDirectories )
                {
                    foreach( var source_and_destination_subdirectory in sources_and_destination_subdirectories )
                    {
                        string source_filename = source_and_destination_subdirectory.Item1;

                        if( !File.Exists(source_filename) || ( new FileInfo(source_filename).Length == 0 ) )
                            throw new Exception($"{Path.GetFileName(source_filename)} was not created successfully");

                        string destination_directory = Path.Combine(assets_directory, source_and_destination_subdirectory.Item2);
                        Directory.CreateDirectory(destination_directory);

                        string destination_filename = Path.Combine(destination_directory, Path.GetFileName(source_filename));

                        File.Copy(source_filename, destination_filename, true);

                        AddToLog(LogType.Log, $"Copied {Path.GetFileName(source_filename)} to {destination_filename}", LogExtra.InSection);
                    }
                }

                File.Delete(messages_filename);
                File.Delete(simple_capi_pen_filename);

                AddToLog(LogType.Log, "Successfully created the assets.", LogExtra.EndSection);
            }

            catch( Exception exception )
            {
                AddToLog(LogType.Error, exception.Message);
            }
        }

        private void panelTimestampFiles_DragEnter(object sender, DragEventArgs e)
        {
            e.Effect = e.Data.GetDataPresent(DataFormats.FileDrop) ? DragDropEffects.Copy : DragDropEffects.None;
        }

        private void panelTimestampFiles_DragDrop(object sender, DragEventArgs e)
        {
            try
            {
                if( !buttonCreate.Enabled )
                    throw new Exception("You must prepare and analyze the inputs before continuing");

                AddToLog(LogType.Log, $"Timestamping the dragged files with the date \"{_releaseDate.ToShortDateString()} {_releaseDate.ToShortTimeString()}\"...");

                int directories = 0;
                int files = 0;

                foreach( string path_name in (string[])e.Data.GetData(DataFormats.FileDrop) )
                {
                    if( Directory.Exists(path_name) )
                    {
                        TimestampDirectory(new DirectoryInfo(path_name), ref directories, ref files, true);
                        ++directories;
                    }

                    else
                    {
                        TimestampFile(path_name, true);
                        ++files;
                    }
                }

                AddToLog(LogType.Log, $"Successfully timestamped {files} files in {directories} directories.", LogExtra.EndSection);
            }

            catch( Exception exception )
            {
                AddToLog(LogType.Error, exception.Message, LogExtra.EndSection);
            }
        }
    }
}
