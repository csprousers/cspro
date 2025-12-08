using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using Newtonsoft.Json;

namespace CSPro_Installer_Generator
{
    class Inputs
    {
        private static Dictionary<CommonPaths, RawInputs> _rawInputs = new Dictionary<CommonPaths, RawInputs>();

        private CommonPaths _commonPaths;
        private RawInputs _rawInput;

        public Inputs(CommonPaths common_paths)
        {
            _commonPaths = common_paths;

            if( !_rawInputs.TryGetValue(_commonPaths, out _rawInput) )
            {
                string input_path = Path.Combine(common_paths.InstallerDirectory, "inputs.json");
                string input_json = File.ReadAllText(input_path);

                _rawInput = JsonConvert.DeserializeObject<RawInputs>(input_json);

                _rawInputs.Add(common_paths, _rawInput);
            }
        }


        // --------------------------------------------------------------------------
        // assets
        // --------------------------------------------------------------------------
        public struct Asset
        {
            public string source_path;
            public string source_base_directory;
            public string destination_directory;
            public bool desktop_only;
        }

        public List<Asset> Assets
        {
            get
            {
                List<Asset> assets = new List<Asset>();

                foreach( RawInputs.Asset raw_asset in _rawInput.assets )
                {
                    string source_directory = ( raw_asset.domain == "build-tools" ) ? _commonPaths.BuildToolsDirectory :
                                              ( raw_asset.domain == "cspro" )       ? _commonPaths.CSProDirectory:
                                                                                      throw new Exception($"Unknown domain: {raw_asset.domain}");

                    string source_path = Path.Combine(source_directory, raw_asset.source_name);

                    string source_base_directory;
                    string destination_directory;
                    List<string> source_file_paths;

                    // if the source path is a directory, add everything in it
                    if( Directory.Exists(source_path) )
                    {
                        DirectoryInfo source_directory_info = new DirectoryInfo(source_path);
                        source_base_directory = source_directory_info.FullName;
                        destination_directory = null;
                        source_file_paths = source_directory_info.GetFiles("*", SearchOption.AllDirectories).Select(x => x.FullName).ToList();
                    }

                    // otherwise evaluate the source path as if it might contain a wildcard
                    else
                    {
                        source_base_directory = source_directory;
                        destination_directory = raw_asset.destination_directory ?? "";
                        source_file_paths = new DirectoryInfo(source_directory).GetFiles(raw_asset.source_name).Select(x => x.FullName).ToList();

                        // if nothing was matched, add the source path and let the program handle the file-not-found exception at a later point
                        if( source_file_paths.Count == 0 )
                            source_file_paths.Add(source_path);
                    }

                    Dictionary<string, string> evaluated_exclusions = new Dictionary<string, string>(); // source name -> build
                    HashSet<string> used_evaluated_exclusions = new HashSet<string>();

                    if( raw_asset.exclusions != null )
                        raw_asset.exclusions.ForEach(x => evaluated_exclusions.Add(Path.GetFullPath(Path.Combine(source_path, x.source_name)), x.build));

                    Debug.Assert(raw_asset.build == null || raw_asset.build == "desktop");
                    bool base_desktop_only = ( raw_asset.build == "desktop" );

                    foreach( string source_file_path in source_file_paths )
                    {
                        bool desktop_only = base_desktop_only;
                        string exclusion_build = null;

                        // evaluate exceptions by full path...
                        bool found_exclusion = evaluated_exclusions.TryGetValue(source_file_path, out exclusion_build);

                        if( found_exclusion )
                        {
                            used_evaluated_exclusions.Add(source_file_path);
                        }

                        // ...or by directory
                        else
                        {
                            foreach( var kp in evaluated_exclusions.Where(x => source_file_path.StartsWith(x.Key, StringComparison.InvariantCultureIgnoreCase)) )
                            {
                                Debug.Assert(Directory.Exists(kp.Key));

                                used_evaluated_exclusions.Add(kp.Key);

                                exclusion_build = kp.Value;
                                found_exclusion = true;

                                break;
                            }
                        }

                        if( found_exclusion )
                        {
                            Debug.Assert(!desktop_only);
                            Debug.Assert(exclusion_build == null || exclusion_build == "nonDesktop");

                            if( exclusion_build == null )
                                continue;

                            if( exclusion_build == "nonDesktop" )
                                desktop_only = true;
                        }

                        Asset asset = new Asset()
                        {
                            source_path = source_file_path,
                            source_base_directory = source_base_directory,
                            destination_directory = destination_directory,
                            desktop_only = desktop_only
                        };

                        // asset.destination_directory is null when files from a directory are included because the
                        // destination directory (with a subdirectory) of the file has to be calculated on a per-file basis
                        if( asset.destination_directory == null )
                        {
                            Debug.Assert(Directory.Exists(source_path));

                            // calculate the appropriate subdirectory
                            char[] slash_chars = new char[] { '/', '\\' };

                            FileInfo asset_source_path_file_info = new FileInfo(asset.source_path);

                            string asset_source_base_directory_no_trailing_slash = asset.source_base_directory.TrimEnd(slash_chars);
                            Debug.Assert(asset_source_path_file_info.Directory.FullName.StartsWith(asset_source_base_directory_no_trailing_slash));

                            asset.destination_directory = Path.Combine(raw_asset.destination_directory, asset_source_path_file_info.Directory.FullName.Substring(asset_source_base_directory_no_trailing_slash.Length).TrimStart(slash_chars));
                        }

                        assets.Add(asset);
                    }

                    foreach( var kp in evaluated_exclusions )
                    {
                        if( !used_evaluated_exclusions.Contains(kp.Key) )
                            throw new Exception("Some asset exclusions did not map to evaluated paths, including: " + kp.Key);
                    }
                }

                return assets;
            }
        }

        public List<Asset> NonDesktopAssets
        {
            get
            {
                return Assets.Where(x => !x.desktop_only).ToList();
            }
        }


        // --------------------------------------------------------------------------
        // tools
        // --------------------------------------------------------------------------
        public struct Tool
        {
            public string directory;
            public string solution;
        }

        public List<Tool> Tools
        {
            get
            {
                return _rawInput.tools.Select(x => new Tool { directory = x.directory, solution = x.solution ?? x.directory }).ToList();
            }
        }


        // --------------------------------------------------------------------------
        // helps
        // --------------------------------------------------------------------------
        public List<string> Helps
        {
            get
            {
                return _rawInput.helps;
            }
        }


        // --------------------------------------------------------------------------
        // bin
        // --------------------------------------------------------------------------
        public List<string> Bin
        {
            get
            {
                // ~ is used for comments
                return _rawInput.bin.Where(x => !x.StartsWith("~")).ToList();
            }
        }


        // --------------------------------------------------------------------------
        // raw inputs
        // --------------------------------------------------------------------------
        private struct RawInputs
        {
            internal struct Asset
            {
                internal struct Exclusion
                {
                    [JsonProperty("sourceName")]
                    internal string source_name;

                    [JsonProperty("build")]
                    internal string build;
                }

                [JsonProperty("domain")]
                internal string domain;

                [JsonProperty("sourceName")]
                internal string source_name;

                [JsonProperty("destinationDirectory")]
                internal string destination_directory;

                [JsonProperty("build")]
                internal string build;

                [JsonProperty("exclusions")]
                internal List<Exclusion> exclusions;
            }

            internal struct Tool
            {
                [JsonProperty("directory")]
                internal string directory;

                [JsonProperty("solution")]
                internal string solution;
            }

            [JsonProperty("assets")]
            internal List<Asset> assets;

            [JsonProperty("tools")]
            internal List<Tool> tools;

            [JsonProperty("helps")]
            internal List<string> helps;

            [JsonProperty("bin")]
            internal List<string> bin;
        }
    }
}
