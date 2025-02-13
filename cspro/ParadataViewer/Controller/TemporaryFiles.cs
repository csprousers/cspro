using System;
using System.Collections.Generic;
using System.IO;

namespace ParadataViewer
{
    partial class Controller
    {
        private List<string> _temporaryFilenames = new List<string>();

        private void DeleteTemporaryFiles()
        {
            foreach( var filename in _temporaryFilenames )
            {
                try
                {
                    File.Delete(filename);
                }
                catch { }
            }

            _temporaryFilenames.Clear();
        }

        internal string GetTemporaryFilename(string extension)
        {
            for( int i = 1; ; i++ )
            {
                string filename = Path.Combine(Path.GetTempPath(),String.Format("ParadataViewerTemp{0}{1}",i,extension));

                if( !File.Exists(filename) )
                {
                    _temporaryFilenames.Add(filename);
                    return filename;
                }
            }

            throw new InvalidOperationException();
        }
    }
}
