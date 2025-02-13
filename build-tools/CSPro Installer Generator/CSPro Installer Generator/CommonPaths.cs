using System.IO;
using System.Reflection;

namespace CSPro_Installer_Generator
{
    class CommonPaths
    {
        public string ExeFilename { get; private set; }

        public string InstallerDirectory { get; private set; }

        public string RootDirectory { get; private set; }

        public string BuildToolsDirectory { get; private set; }
        public string CSProDirectory { get; private set; }
        public string CSProDebugDirectory { get; private set; }
        public string CSProReleaseDirectory { get; private set; }
        public string ToolsDirectory { get; private set; }

		public string[] AssetsDirectories { get; private set; }
		public string AndroidAssetsDirectory { get; private set; }


        public CommonPaths()
        {
            ExeFilename = Assembly.GetExecutingAssembly().Location;

            InstallerDirectory = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(ExeFilename), @"..\..\..\"));

            RootDirectory = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(InstallerDirectory), @"..\.."));

            BuildToolsDirectory = Path.Combine(RootDirectory, "build-tools");
            CSProDirectory = Path.Combine(RootDirectory, "cspro");
            CSProDebugDirectory = Path.Combine(CSProDirectory, @"debug\bin");
            CSProReleaseDirectory = Path.Combine(CSProDirectory, @"release\bin");
            ToolsDirectory = Path.Combine(RootDirectory, "tools");

            AndroidAssetsDirectory = Path.Combine(CSProDirectory, @"CSEntryDroid\app\src\main\assets");
            
            AssetsDirectories = new string[] 
            { 
                AndroidAssetsDirectory
                // reenable for WASM --> Path.Combine(CSProDirectory, @"WASM\Assets") 
            };
        }

        public string GetLatestCSProBuildFile(string filename)
        {
            var fi_debug = new FileInfo(Path.Combine(CSProDebugDirectory, filename));
            var fi_release = new FileInfo(Path.Combine(CSProReleaseDirectory, filename));
            return ( fi_debug.LastWriteTime > fi_release.LastWriteTime ) ? fi_debug.FullName : fi_release.FullName;
        }

        public string GetLatestCSDocument()
        {
            return GetLatestCSProBuildFile("CSDocument.exe");
        }
    }
}
