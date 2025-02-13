using System;
using System.IO;

namespace PFF_Editor
{
    class PathHelper
    {
        private static char[] wildcardChars = new char[] { '?','*' };
        private static char[] slashChars = new char[] { '\\','/' };

        public static bool IsRelative(string filename)
        {
            return !Path.IsPathRooted(filename) && !IsUNC(filename);
        }

        public static bool IsUNC(string filename)
        {
            filename = filename.Trim();
            return filename.Length >= 2 && filename.IndexOf(@"\\") == 0;
        }

        public static string GetAbsolutePath(string relativePFF,string filename)
        {
            try
            {
                filename = filename.Trim();

                if( filename.Length > 0 && IsRelative(filename) )
                {
                    if( filename.IndexOfAny(wildcardChars) >= 0 ) // handle paths with wildcards
                    {
                        int lastSlash = filename.LastIndexOfAny(slashChars);
                        string mainPath = Path.GetFullPath(( new Uri(Path.Combine(Path.GetDirectoryName(relativePFF),filename.Substring(0,lastSlash + 1))) ).LocalPath);
                        filename = mainPath + filename.Substring(lastSlash + 1);
                    }

                    else
                        filename = Path.GetFullPath(( new Uri(Path.Combine(Path.GetDirectoryName(relativePFF),filename)) ).LocalPath);
                }
            }

            catch( Exception )
            {
            }

            return filename;
        }

        public static string GetRelativePath(string relativePFF,string filename)
        {
            try
            {
                filename = filename.Trim();

                if( filename.Length == 0 || IsUNC(filename) || IsRelative(filename) )
                    return filename;

                Uri fileUri = new Uri(filename);
                Uri referenceUri = new Uri(Path.GetDirectoryName(relativePFF) + "/");

                string relativePath = referenceUri.MakeRelativeUri(fileUri).ToString();
                relativePath = Uri.UnescapeDataString(relativePath);
                relativePath = relativePath.Replace('/','\\');

                if( relativePath.IndexOf(':') < 0 && relativePath[0] != '.' ) // add a .\ (as CSPro does)
                    relativePath = ".\\" + relativePath;

                return relativePath;
            }
            
            catch( Exception )
            {
            }

            return filename;
        }

    }
}
