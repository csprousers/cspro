using System;
using System.Collections;
using System.IO;

namespace Operator_Statistics_Viewer
{
    enum EntryMode
    {
        Add,
        Modify,
        Verify
    }

    class OperatorStatistics
    {
        EntryMode mode;
        string id;
        DateTime startTime;
        DateTime endTime;
        int keyingTime;
        int pauseTime;
        int cases;
        int records;
        int keystrokes;
        int badKeystrokes;
        int keyerErrors;
        int verifierErrors;
        int fieldsVerified;

        public EntryMode Mode { get { return mode; } }
        public string ModeString { get { return mode == EntryMode.Add ? "Add" : mode == EntryMode.Modify ? "Modify" : "Verify"; } }
        public string ID { get { return id; } }
        public DateTime StartTime { get { return startTime; } }
        public DateTime EndTime { get { return endTime; } }
        public int KeyingTime { get { return keyingTime; } }
        public int PauseTime { get { return pauseTime; } }
        public int Cases { get { return cases; } }
        public int Records { get { return records; } }
        public int Keystrokes { get { return keystrokes; } }
        public int BadKeystrokes { get { return badKeystrokes; } }
        public int KeyerErrors { get { return keyerErrors; } }
        public int VerifierErrors { get { return verifierErrors; } }
        public int FieldsVerified { get { return fieldsVerified; } }

        string ParseCell(ref string str)
        {
            int nextComma = str.IndexOf(',');

            string cell;

            if( nextComma < 0 )
            {
                cell = str;
                str = null;
            }

            else
            {
                cell = str.Substring(0,nextComma);
                str = str.Substring(nextComma + 1);
            }

            return cell.Trim();
        }

        void ParseTime(string time,out int hour,out int minute,out int second)
        {
            hour = Int32.Parse(time.Substring(0,2));
            minute = Int32.Parse(time.Substring(3,2));
            second = Int32.Parse(time.Substring(6,2));
        }

        public OperatorStatistics(string line)
        {
            string origLine = line;

            try
            {
                int year = 0, month = 0, day = 0;
                int hour = 0, minute, second;

                for( int i = 0; i < 14; i++ )
                {
                    string token = ParseCell(ref line);

                    if( token.Length == 0 ) // every cell must have an entry
                        throw new Exception();

                    switch( i )
                    {
                        case 0: // entry mode
                            if( token.Equals("ADD",StringComparison.CurrentCultureIgnoreCase) )
                                mode = EntryMode.Add;

                            else if( token.Equals("MOD",StringComparison.CurrentCultureIgnoreCase) )
                                mode = EntryMode.Modify;

                            else if( token.Equals("VER",StringComparison.CurrentCultureIgnoreCase) )
                                mode = EntryMode.Verify;

                            else
                                throw new Exception();

                            break;

                        case 1: // operator id
                            id = token;
                            break;

                        case 2: // start date
                            year = Int32.Parse(token.Substring(6,4));
                            month = Int32.Parse(token.Substring(0,2));
                            day = Int32.Parse(token.Substring(3,2));
                            break;

                        case 3: // start time
                            ParseTime(token,out hour,out minute,out second);
                            startTime = new DateTime(year,month,day,hour,minute,second);
                            break;

                        case 4: // end time
                            int startHour = hour; // the start hour
                            ParseTime(token,out hour,out minute,out second);
                            endTime = new DateTime(year,month,day,hour,minute,second);

                            if( hour < startHour ) // we'll assume that you can't key for more than 24 hours
                                endTime = endTime.AddDays(1);

                            break;

                        case 5: // keying time
                            keyingTime = Int32.Parse(token);
                            break;

                        case 6: // pause time
                            pauseTime = Int32.Parse(token);
                            break;

                        case 7: // cases
                            cases = Int32.Parse(token);
                            break;

                        case 8: // records
                            records = Int32.Parse(token);
                            break;

                        case 9: // keystrokes
                            keystrokes = Int32.Parse(token);
                            break;

                        case 10: // bad keystrokes
                            badKeystrokes = Int32.Parse(token);
                            break;

                        case 11: // keyer errors
                            keyerErrors = Int32.Parse(token);
                            break;

                        case 12: // verifier errors
                            verifierErrors = Int32.Parse(token);
                            break;

                        case 13: // fields verified
                            fieldsVerified = Int32.Parse(token);
                            break;
                    }
                }
            }

            catch( Exception )
            {
                throw new Exception("Error parsing operator statistics line:\n\n" + origLine);
            }

            if( line != null )
                throw new Exception("Too many values on operator statistics line:\n\n" + origLine);
        }
    }


    public class OperatorStatisticsDateSorter : IComparer
    {
        int IComparer.Compare(Object x,Object y)
        {
            OperatorStatistics obj1 = (OperatorStatistics)x;
            OperatorStatistics obj2 = (OperatorStatistics)y;

            return obj1.EndTime.CompareTo(obj2.EndTime);
        }
    }


    class OperatorStatisticsFile
    {
        ArrayList statistics;

        public ArrayList Statistics { get { return statistics; } }

        public OperatorStatisticsFile(string filename)
        {
            statistics = new ArrayList();

            string[] fileContents = File.ReadAllLines(filename);

            foreach( string str in fileContents )
                statistics.Add(new OperatorStatistics(str));
        }
    }
}
