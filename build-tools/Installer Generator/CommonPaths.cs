using System.IO;
using System.Reflection;

namespace CSPro_Installer_Generator
{
    class CommonPaths
    {
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
            BuildToolsDirectory = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), @"..\..\..\.."));
            InstallerDirectory = Path.Combine(BuildToolsDirectory, "Installer Inputs");
            RootDirectory = Path.GetFullPath(Path.Combine(BuildToolsDirectory, ".."));
            CSProDirectory = Path.Combine(RootDirectory, "cspro");
            CSProDebugDirectory = Path.Combine(CSProDirectory, "build", Build.PlatformTarget, @"Debug\bin");
            CSProReleaseDirectory = Path.Combine(CSProDirectory, "build", Build.PlatformTarget, @"Release\bin");
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
