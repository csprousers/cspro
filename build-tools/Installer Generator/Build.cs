using System.Diagnostics;
using System.IO;

namespace CSPro_Installer_Generator
{
    class Build
    {
        public static bool Is32Bit = true;
        public static string PlatformTarget { get { return Is32Bit ? "x86" : "x64"; } }
        public static string Platform { get { return Is32Bit ? "Win32" : "x64"; } }

        private string _msbuildFilePath;
        private string _solutionFilePath;
        private bool _release;
        private string Configuration { get { return _release ? "Release" : "Debug"; } }

        public Build(string msbuild_file_path, string solution_file_path, bool release)
        {
            _msbuildFilePath = msbuild_file_path;
            _solutionFilePath = solution_file_path;
            _release = release;
        }

        public void Run(bool rebuild)
        {
            string arguments =
                $"\"{_solutionFilePath}\" " +
                $"/p:Configuration={Configuration} " +
                $"/p:Platform={Platform} " +
                $"/t:{( rebuild ? "Clean," : "" )}Build";

            var process = new Process();
            process.StartInfo = new ProcessStartInfo(_msbuildFilePath, arguments);
            process.Start();
            process.WaitForExit();
        }

        public string GetBuiltFilePath(string filename)
        {
            return Path.Combine(Path.GetDirectoryName(_solutionFilePath), $"build\\{PlatformTarget}\\{Configuration}\\bin\\{filename}");
        }

        public string GetExecutableFilePath(string project_name)
        {
            return GetBuiltFilePath(project_name + ".exe");
        }
    }
}
