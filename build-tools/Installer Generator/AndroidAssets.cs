using System;
using System.IO;
using System.Linq;
using System.Windows.Forms;

namespace CSPro_Installer_Generator
{
    class AndroidAssets
    {
        public static void UpdateHtml()
        {
            int new_files = 0;
            int modified_files = 0;
            int unmodified_files = 0;

            try
            {
                CommonPaths common_paths = new CommonPaths();
                Inputs inputs = new Inputs(common_paths);

                foreach( Inputs.Asset asset in inputs.NonDesktopAssets.Where(x => x.destination_directory.StartsWith("html")) )
                {
                    string destination_directory = Path.Combine(common_paths.AndroidAssetsDirectory, asset.destination_directory);
                    DirectoryInfo destination_directory_info = new DirectoryInfo(destination_directory);
                    destination_directory_info.Create();

                    FileInfo source_file_info = new FileInfo(asset.source_path);
                    FileInfo destination_file_info = new FileInfo(Path.Combine(destination_directory_info.FullName,  source_file_info.Name));

                    if( !destination_file_info.Exists )
                    {
                        ++new_files;
                    }

                    else if( destination_file_info.LastWriteTimeUtc != source_file_info.LastWriteTimeUtc )
                    {
                        ++modified_files;
                    }

                    else
                    {
                        ++unmodified_files;
                    }

                    File.Copy(source_file_info.FullName, destination_file_info.FullName, true);
                }

                MessageBox.Show($"Updated Android HTML assets:\n\n{new_files} new\n{modified_files} modified\n{unmodified_files} unmodified");
            }

             catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }
       }
    }
}
