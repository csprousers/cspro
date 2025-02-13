using System;
using System.Collections;
using System.IO;

namespace Operator_Statistics_Viewer
{
    class ViewingConfigurationFile
    {
        public enum AddType
        {
            Single,
            Multiple,
            MultipleWithSubitems
        }

        public class AddAction
        {
            public AddType addType;
            public string filename;
        }

        ArrayList addActions;

        public ArrayList AddActions { get { return addActions; } }

        public ViewingConfigurationFile()
        {
            addActions = new ArrayList();
        }

        public void Clear()
        {
            addActions.Clear();
        }

        public void AddSingle(string filename)
        {
            AddAction aa = new AddAction();
            aa.addType = AddType.Single;
            aa.filename = filename;
            addActions.Add(aa);
        }

        public void AddMultiple(string pathname,bool useSubfolders)
        {
            AddAction aa = new AddAction();
            aa.addType = useSubfolders ? AddType.MultipleWithSubitems : AddType.Multiple;
            aa.filename = pathname;
            addActions.Add(aa);
        }

        const string OSV_HEADER = "[Operator Statistics Viewer]";
        const string OSV_CASES = "ExpectedCases";
        const string OSV_DATE = "DesiredEndDate";
        const string OSV_NAMES = "OperatorNames";
        const string OSV_SINGLE = "Log";
        const string OSV_MULTIPLE = "Logs";
        const string OSV_MULTIPLE_SUBFOLDERS = "RecursiveLogs";


        public void Save(string filename,int expectedCases,DateTime endDate,string operatorNamesFile)
        {
            StreamWriter output = new StreamWriter(filename,false,System.Text.Encoding.UTF8);

            output.WriteLine(OSV_HEADER);

            if( expectedCases > 0 )
            {
                output.WriteLine(OSV_CASES + "=" + expectedCases);
                output.WriteLine(OSV_DATE + "=" + endDate.ToString("yyyy-MM-dd"));
            }

            if( operatorNamesFile != null )
                output.WriteLine(OSV_NAMES + "=" + operatorNamesFile);

            output.WriteLine();

            foreach( AddAction aa in addActions )
            {
                if( aa.addType == AddType.Single )
                    output.WriteLine(OSV_SINGLE + "=" + aa.filename);
                
                else if( aa.addType == AddType.Multiple )
                    output.WriteLine(OSV_MULTIPLE + "=" + aa.filename);
                
                else
                    output.WriteLine(OSV_MULTIPLE_SUBFOLDERS + "=" + aa.filename);
            }

            output.Close();
        }


        public static ViewingConfigurationFile Load(string filename,out int expectedCases,out DateTime endDate,out string operatorNamesFile)
        {
            ViewingConfigurationFile vcf = new ViewingConfigurationFile();

            string[] fileContents = File.ReadAllLines(filename);

            expectedCases = 0;
            endDate = DateTime.MinValue;
            operatorNamesFile = null;

            if( fileContents.Length == 0 )
                throw new Exception("The file was empty!");

            else if( !fileContents[0].Equals(OSV_HEADER,StringComparison.CurrentCultureIgnoreCase) )
                throw new Exception("The file had a bad header!");

            for( int i = 1; i < fileContents.Length; i++ )
            {
                try
                {
                    if( fileContents[i].Trim().Length == 0 )
                        continue;

                    int equalPos = fileContents[i].IndexOf('=');

                    if( equalPos < 0 )
                        throw new Exception();

                    string lhs = fileContents[i].Substring(0,equalPos).Trim();
                    string rhs = fileContents[i].Substring(equalPos + 1).Trim();

                    if( lhs.Equals(OSV_CASES,StringComparison.CurrentCultureIgnoreCase) )
                        expectedCases = Int32.Parse(rhs);

                    else if( lhs.Equals(OSV_DATE,StringComparison.CurrentCultureIgnoreCase) )
                        endDate = new DateTime(Int32.Parse(rhs.Substring(0,4)),Int32.Parse(rhs.Substring(5,2)),Int32.Parse(rhs.Substring(8,2)));

                    else if( lhs.Equals(OSV_NAMES,StringComparison.CurrentCultureIgnoreCase) )
                        operatorNamesFile = rhs;

                    else if( lhs.Equals(OSV_SINGLE,StringComparison.CurrentCultureIgnoreCase) )
                        vcf.AddSingle(rhs);

                    else if( lhs.Equals(OSV_MULTIPLE,StringComparison.CurrentCultureIgnoreCase) )
                        vcf.AddMultiple(rhs,false);

                    else if( lhs.Equals(OSV_MULTIPLE_SUBFOLDERS,StringComparison.CurrentCultureIgnoreCase) )
                        vcf.AddMultiple(rhs,true);

                    else
                        throw new Exception();
                }

                catch( Exception )
                {
                    throw new Exception("Error in line: " + fileContents[i]);
                }
            }

            return vcf;
        }
    }
}
