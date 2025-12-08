using System;
using System.Windows.Forms;

namespace CSPro_Installer_Generator
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            Array commandArgs = Environment.GetCommandLineArgs();

			if( commandArgs.Length >= 2 && (string)commandArgs.GetValue(1) == "/android-assets-update-html" )
            {
                AndroidAssets.UpdateHtml();
            }

            else
            {
                Application.Run(new MainForm());
            }
        }
    }
}
