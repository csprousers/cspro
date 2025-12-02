using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Windows.Forms;

// DISCLAIMER: this program is not programmed particularly well, but it does what it needs to do

namespace UpdateVersion
{
    partial class MainForm : Form
    {
        private class FileContent
        {
            public enum Type { TwoWithDot, FourWithDot, FourWithComma };
            public Type? type;
            public string text;

            public FileContent(Type type_)   { type = type_; }
            public FileContent(string text_) { text = text_; }
        }

        private class VersionFile
        {
            public FileInfo file_info;
            public Encoding encoding;
            public List<FileContent> contents = new List<FileContent>();
        }

        private const string CSProPrefix = "CSPro ";

        private List<VersionFile> _versionFiles = new List<VersionFile>();
        private Tuple<int, int, int> _version;

        public MainForm()
        {
            InitializeComponent();
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            try
            {
                string exe_directory = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
                string code_directory = Path.GetFullPath(Path.Combine(exe_directory, @"..\..\..\..\..\..\cspro"));
                var code_directory_info = new DirectoryInfo(code_directory);

                if( !code_directory_info.Exists )
                    throw new Exception($"Could not find the CSPro code directory: {code_directory_info.FullName}");

                GetFilesWithVersions(code_directory_info, 0);

                if( _version != null )
                    textBoxVersion.Text = GetDisplayableVersionNumber();
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
                Close();
            }
        }

        private void buttonUpdateVersion_Click(object sender, EventArgs e)
        {
            try
            {
                if( !ParseVersion(textBoxVersion.Text, 3, '.') )
                    throw new Exception("Invalid version number");

                textBoxLog.Clear();

                UpdateFileVersions();

                MessageBox.Show($"Successfully changed the version to {GetDisplayableVersionNumber()}");
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }
        }

        private void AddToLog(string text)
        {
            textBoxLog.AppendText(text + "\r\n");
        }

        private string GetDisplayableVersionNumber()
        {
            return $"{_version.Item1}.{_version.Item2}.{_version.Item3}";
        }

        private void GetFilesWithVersions(DirectoryInfo directory_info, int subdirectory_depth)
        {
            if( subdirectory_depth < 3 )
            {
                foreach( var subdirectory_info in directory_info.GetDirectories() )
                    GetFilesWithVersions(subdirectory_info, subdirectory_depth + 1);
            }

            foreach( string filter in new [] { "AssemblyInfo.cs", "AssemblyInfo.cpp", "*.rc" } )
                directory_info.GetFiles(filter).ToList().ForEach(x => ReadFileVersion(x));
        }

        private void ReadFileVersion(FileInfo file_info)
        {
            var version_file = new VersionFile();
            version_file.file_info = file_info;

            bool use_reader_encoding = true;

            using( var bom_reader = new FileStream(file_info.FullName, FileMode.Open, FileAccess.Read) )
            {
                int first_byte = bom_reader.ReadByte();
                use_reader_encoding = ( first_byte == 0xef || first_byte == 0xfe || first_byte == 0xff );
            }

            using( var reader = new StreamReader(file_info.FullName, true) )
            {
                reader.Peek();
                version_file.encoding = use_reader_encoding ? reader.CurrentEncoding : Encoding.ASCII;

                string text = reader.ReadToEnd();
                
                if( file_info.Extension == ".rc" )
                    ParseRC(text, version_file.contents);

                else
                    ParseAssemblyInfo(text, version_file.contents);

                if( version_file.contents.Count(x => x.type != null) > 0 )
                {
                    AddToLog($"Read {GetDisplayableVersionNumber()} in {file_info.FullName}");
                    _versionFiles.Add(version_file);
                }

                else
                    AddToLog($"Could not find version numbers in {file_info.FullName}");
            }
        }

        private void ParseRC(string text, List<FileContent> contents)
        {
            foreach( string line in System.Text.RegularExpressions.Regex.Split(text, "(\r|\n)") )
            {
                FileContent.Type? version_type = null;
                int? version_start_pos = null;
                int? version_end_pos = null;

                Func<string, bool> parse_attribute = (string attribute) =>
                {
                    int start_pos = line.IndexOf(attribute);

                    if( start_pos >= 0 )
                    {
                        if( ParseVersion(line.Substring(attribute.Length), 4, ',') )
                        {
                            version_type = FileContent.Type.FourWithComma;
                            version_start_pos = start_pos + attribute.Length;

                            return true;
                        }
                    }
                    
                    return false;
                };

                bool success = parse_attribute("FILEVERSION ") ||
                               parse_attribute("PRODUCTVERSION ");

                if( !success )
                {
                    Func<string, bool, bool> parse_value_attribute = (string value_text, bool has_cspro_text) =>
                    {
                        int start_pos = line.IndexOf(value_text);

                        if( start_pos < 0 )
                            return false;

                        int first_quote_pos = line.IndexOf('"', start_pos + value_text.Length + 1);
                        int second_quote_pos = line.IndexOf('"', first_quote_pos + 1);

                        if( first_quote_pos < start_pos || second_quote_pos < first_quote_pos )
                            return false;

                        version_start_pos = first_quote_pos + 1;
                        version_end_pos = second_quote_pos;

                        string quoted_text = line.Substring(first_quote_pos + 1, second_quote_pos - first_quote_pos - 1);

                        if( has_cspro_text )
                        {
                            if( quoted_text.IndexOf(CSProPrefix) == 0 && ParseVersion(quoted_text.Substring(CSProPrefix.Length), 2, '.') )
                            {
                                version_type = FileContent.Type.TwoWithDot;
                                version_start_pos += CSProPrefix.Length;                                
                                return true;
                            }
                        }

                        else if( ParseVersion(quoted_text, 4, '.') )
                        {
                            version_type = FileContent.Type.FourWithDot;
                            return true;
                        }
                    
                        return false;
                    };

                    success = parse_value_attribute("\"FileVersion\"", false) ||
                              parse_value_attribute("\"ProductName\"", true) ||
                              parse_value_attribute("\"ProductVersion\"", false);
                }

                if( version_type == null )
                    contents.Add(new FileContent(line));

                else
                {
                    contents.Add(new FileContent(line.Substring(0, (int)version_start_pos)));
                    contents.Add(new FileContent((FileContent.Type)version_type));

                    if( version_end_pos != null )
                        contents.Add(new FileContent(line.Substring((int)version_end_pos)));
                }
            }
        }

        private void ParseAssemblyInfo(string text, List<FileContent> contents)
        {
            int last_contents_pos = -1;
            int assembly_pos = -1;

            while( ( assembly_pos = text.IndexOf("assembly:", assembly_pos + 1) ) >= 0 )
            {
                int rn_pos = text.IndexOfAny(new [] { '\r', '\n' }, assembly_pos);
                string this_line = ( rn_pos < 0 ) ? text.Substring(assembly_pos) : text.Substring(assembly_pos, rn_pos - assembly_pos);

                int first_quote_pos = this_line.IndexOf('"');
                int second_quote_pos = this_line.IndexOf('"', first_quote_pos + 1);

                if( first_quote_pos < 0 || second_quote_pos < first_quote_pos )
                    continue;

                string quoted_text = this_line.Substring(first_quote_pos + 1, second_quote_pos - first_quote_pos - 1);

                FileContent.Type? version_type = null;
                int version_start_pos = first_quote_pos + 1;

                if( ( this_line.IndexOf("AssemblyVersion") >= 0 ) ||
                    ( this_line.IndexOf("AssemblyFileVersion") >= 0 ) ||
                    ( this_line.IndexOf("AssemblyVersionAttribute") >= 0 ) )
                {
                    if( ParseVersion(quoted_text, 4, '.') )
                        version_type = FileContent.Type.FourWithDot;
                }

                else if( this_line.IndexOf("AssemblyProduct") >= 0 )
                {
                    if( quoted_text.IndexOf(CSProPrefix) == 0 && ParseVersion(quoted_text.Substring(CSProPrefix.Length), 2, '.') )
                    {
                        version_type = FileContent.Type.TwoWithDot;
                        version_start_pos += CSProPrefix.Length;
                    }
                }

                if( version_type == null )
                {
                    contents.Add(new FileContent(text.Substring(last_contents_pos + 1, assembly_pos - last_contents_pos - 1)));
                    last_contents_pos = assembly_pos - 1;
                }

                else
                {
                    contents.Add(new FileContent(text.Substring(last_contents_pos + 1, assembly_pos + version_start_pos - last_contents_pos - 1)));
                    contents.Add(new FileContent((FileContent.Type)version_type));
                    last_contents_pos = assembly_pos + second_quote_pos - 1;
                }                
            }

            contents.Add(new FileContent(text.Substring(last_contents_pos + 1)));
        }

        private bool ParseVersion(string version_text, int parts, char separator)
        {
            string[] version_parts = version_text.Split(new [] { separator });

            if( version_parts.Length == parts )
            {
                try
                {
                    _version = Tuple.Create(int.Parse(version_parts[0]),
                                            int.Parse(version_parts[1]),
                                            version_parts.Length < 3 ? 0 : int.Parse(version_parts[2]));

                    return true;
                }

                catch { }
            }            

            return false;
        }

        private void UpdateFileVersions()
        {
            foreach( var version_file in _versionFiles )
            {
                int versions_modified = 0;

                using( var writer = new StreamWriter(version_file.file_info.FullName, false, version_file.encoding) )
                {
                    foreach( var content in version_file.contents )
                    {
                        if( content.type == null )
                            writer.Write(content.text);

                        else
                        {
                            ++versions_modified;

                            if( content.type == FileContent.Type.TwoWithDot )
                                writer.Write($"{_version.Item1}.{_version.Item2}");
                        
                            else if( content.type == FileContent.Type.FourWithDot )
                                writer.Write($"{_version.Item1}.{_version.Item2}.{_version.Item3}.0");
                            
                            else if( content.type == FileContent.Type.FourWithComma )
                                writer.Write($"{_version.Item1},{_version.Item2},{_version.Item3},0");
                        }
                    }
                }

                AddToLog($"Wrote {versions_modified} version numbers to {version_file.file_info.FullName}");
            }
        }
    }
}
