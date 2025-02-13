using System;
using System.IO;
using System.Windows.Forms;

namespace Excel2CSPro
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

            // run a conversion from the command line, or show the main form
            try
            {
                bool ranConversion = false;
                string filename = null;
                int overrideLines = 0;

                Array commandArgs = Environment.GetCommandLineArgs();

                if( commandArgs.Length > 1 )
                {
                    CSPro.Util.PFF pff = null;
                    bool run = false;
                    bool runIfNewer = false;

                    for( int i = 1; i < commandArgs.Length; i++ )
                    {
                        string argument = (string)commandArgs.GetValue(i);

                        if( i == 1 && argument.Equals(RunCommandLineArgument,StringComparison.InvariantCultureIgnoreCase) )
                            run = true;

                        else if( i == 1 && argument.Equals(RunIfNewerCommandLineArgument,StringComparison.InvariantCultureIgnoreCase) )
                            runIfNewer = true;

                        else if( filename == null && File.Exists(argument) )
                            filename = Path.GetFullPath(argument);

                        else
                            ++overrideLines;
                    }

                    if( overrideLines > 0 )
                        MessageBox.Show($"Starting with CSPro 8.0, specifying overrides on the command line is not allowed and the {overrideLines} override line(s) will be ignored");

                    if( filename != null && Path.GetExtension(filename).ToLower() == CSPro.Util.PFF.Extension )
                    {
                        pff = ProcessPffCommandLineArgument(ref filename);
                        run = true;
                    }

                    if( run || runIfNewer )
                    {
                        Excel2CSProManager excel2CSProManager = new Excel2CSProManager();
                        excel2CSProManager.LoadSpecFile(filename);

                        if( runIfNewer )
                            excel2CSProManager._spec.RunOnlyIfNewer = true;

                        // the PFF can override some values
                        if( pff != null )
                        {
                            if( !string.IsNullOrWhiteSpace(pff.ExcelFilename) )
                                excel2CSProManager._spec.ExcelFilename = pff.ExcelFilename;

                            if( !string.IsNullOrWhiteSpace(pff.InputDictFName) )
                                excel2CSProManager._spec.DictionaryFilename = pff.InputDictFName;

                            if( pff.SingleOutputDataConnectionString != null )
                                excel2CSProManager._spec.OutputConnectionString = pff.SingleOutputDataConnectionString;
                        }

                        // run the conversion
                        excel2CSProManager.RunConversion();
                        ranConversion = true;

                        if( pff != null )
                            pff.ExecuteOnExitPff();
                    }
                }

                if( !ranConversion )
                    Application.Run(new MainForm(filename));
            }

            catch( Exception exception )
            {
                MessageBox.Show(exception.Message);
            }

            WorkbookController.Close();
        }

        static private CSPro.Util.PFF ProcessPffCommandLineArgument(ref string filename)
        {
            CSPro.Util.PFF pff = new CSPro.Util.PFF(filename);

            if( !pff.Load() )
                throw new Exception($"Unable to read file {filename}. Check that this is a valid file from the correct version of CSPro.");

            if( pff.AppType != CSPro.Util.AppType.EXCEL2CSPRO_TYPE )
                throw new Exception("Incorrect type of PFF file. Only Excel2CSPro files are supported.");

            filename = pff.AppFName;

            return pff;
        }

        private const string RunCommandLineArgument = "/run";
        private const string RunIfNewerCommandLineArgument = "/runifnewer";
    }
}
