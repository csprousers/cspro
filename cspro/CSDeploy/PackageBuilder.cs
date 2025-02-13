using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO.Compression;
using System.IO;
using System.Diagnostics;
using System.Security.Cryptography;

namespace CSDeploy
{
    class PackageBuilder
    {
        private string m_rootPath;
        private string m_outputFile;
        private DeploymentSpecFile m_specFile;

        public PackageBuilder SetOutputFile(string f)
        {
            m_outputFile = f;
            return this;
        }

        public PackageBuilder SetRootFolder(string root)
        {
            m_rootPath = root;
            return this;
        }

        public PackageBuilder SetBundleSpec(DeploymentSpecFile deploymentSpecFile)
        {
            m_specFile = deploymentSpecFile;
            return this;
        }

        public PackageBuilder Build()
        {
            if (m_specFile == null)
                throw new Exception("Missing spec file");

            if (String.IsNullOrEmpty(m_specFile.Name))
                throw new Exception("Package name required");

            if (m_specFile.Files.Length == 0)
                throw new Exception("No input files specified for package");

            if (String.IsNullOrEmpty(m_outputFile))
                throw new Exception("No output file specified for package");

            string tempFolder = CreateTemporaryFolder();
            string packageTmpFile = Path.Combine(tempFolder, Util.replaceInvalidFileChars(m_specFile.Name));
            try
            {
                CopyInputFiles(packageTmpFile);

                CopyBundleSpecFile(packageTmpFile);

                BuildZip(packageTmpFile);
            }
            finally
            {
                Directory.Delete(tempFolder, true);
            }

            return this;
        }

        private void CopyInputFiles(string packageDirectory)
        {
            foreach (FileSpec file in m_specFile.Files)
            {
                var srcPath = Path.GetFullPath(Path.Combine(m_rootPath, file.Path));

                if (!File.Exists(srcPath))
                    throw new IOException(String.Format(Messages.ErrorFileNotFound, srcPath));

                string destPath = Path.Combine(packageDirectory, file.Path);
                Directory.CreateDirectory(Path.GetDirectoryName(destPath));

                if (Path.GetExtension(file.Path).ToLower() == ".ent")
                {
                    destPath = Path.ChangeExtension(destPath, ".pen");
                    CreatePen(srcPath, destPath);
                    file.Path = Path.ChangeExtension(file.Path, ".pen");
                }
                else
                {
                    File.Copy(srcPath, destPath);
                }

                file.Signature = CalculateMD5(destPath);
            }
        }

        private void CopyBundleSpecFile(string tempFolder)
        {
            m_specFile.BuildTime = DateTime.Now.ToUniversalTime();
            m_specFile.Save(Path.Combine(tempFolder, "package.json"));
            // package.json is the new name but to avoid issues with people using old versions
            // of CSWeb, continue to use package.csds for CSWeb. Client code now supports both.
            if (m_specFile.Deployment.Type == "CSWeb")
                m_specFile.Save(Path.Combine(tempFolder, "package.csds"));
        }

        private static string CalculateMD5(string file)
        {
            MD5 md5 = MD5.Create();
            using (FileStream stream = File.OpenRead(file))
            {
                byte[] checksum = md5.ComputeHash(stream);
                return (BitConverter.ToString(checksum).Replace("-", string.Empty));
            }
        }

        private void CreatePen(string file, string destPath)
        {
            var currentExeFilename = System.Reflection.Assembly.GetExecutingAssembly().Location;
            var csentryfilename = Path.Combine(Path.GetDirectoryName(currentExeFilename), "CSEntry.exe");

            if (!File.Exists(csentryfilename))
                throw new Exception("Failed to find CSEntry at " + csentryfilename + ". Check that CSPro is correcly installed.");

            // Create the pen file in same directory as the .ent file so that relative paths
            // of value set images and the like are maintained. Then move the pen to the destination
            // location afterwards.
            var tmpPenPath = Path.Combine(Path.GetDirectoryName(file), Guid.NewGuid().ToString() + ".pen");

            var process = new Process();
            var args = "\"" + file + "\" /pen /binaryName " + "\"" + tmpPenPath + "\"";

            process.StartInfo = new ProcessStartInfo(csentryfilename, args);
            process.Start();
            process.WaitForExit();

            var penInfo = new FileInfo(tmpPenPath);
            if (!penInfo.Exists || penInfo.Length == 0)
            {
                if( penInfo.Exists )
                {
                    try
                    {
                        penInfo.Delete();
                    }
                    catch { }
                }

                throw new Exception("Failed to publish pen file for " + Path.GetFileName(file) + ". Try opening it in CSPro Designer and compiling it to make sure there are no errors or missing files.");
            }

            File.Move(tmpPenPath, destPath);
        }

        private void BuildZip(string packageDirectory)
        {
            if (File.Exists(m_outputFile))
                File.Delete(m_outputFile);

            ZipFile.CreateFromDirectory(packageDirectory, m_outputFile, CompressionLevel.Optimal, false);
        }

        private static string CreateTemporaryFolder()
        {
            string tempDir;
            do
            {
               tempDir = Path.GetTempPath() + Guid.NewGuid().ToString();
            } while (Directory.Exists(tempDir));
            Directory.CreateDirectory(tempDir);
            return tempDir;
        }
    }
}
