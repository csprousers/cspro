using System.IO;

namespace CSDeploy
{
    static class Util
    {
        static internal string replaceInvalidFileChars(string filename)
        {
            foreach (char c in Path.GetInvalidFileNameChars())
            {
                filename = filename.Replace(c, '_');
            }
            return filename;
        }
    }
}
