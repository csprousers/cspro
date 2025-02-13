using System;
using System.IO;
using System.Windows.Forms;

namespace ToolbarCreator
{
    internal static class Program
    {
        [STAThread]
        static void Main(string[] args)
        {
            if( args.Length > 0 )
            {
                try
                {
                    var creator = new Creator(Path.GetFullPath(args[0]), ( args.Length > 1 ) ? Path.GetFullPath(args[1]) : null);
                    creator.Create(false);
                }
                
                catch( Exception exception )
                {
                    MessageBox.Show(exception.Message);
                }
            }

            else
            {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new MainForm());
            }
        }
    }
}
