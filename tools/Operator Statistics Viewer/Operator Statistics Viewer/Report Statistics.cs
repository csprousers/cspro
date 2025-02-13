using System;
using System.Collections;

namespace Operator_Statistics_Viewer
{
    class ReportStatistics
    {
        Hashtable ids;

        DateTime startTime;
        int addTime;
        int modifyTime;
        int verifyTime;
        int pauseTime;
        int casesAdded;
        int recordsAdded;
        int casesVerified;
        int recordsVerified;
        int keystrokes;
        int badKeystrokes;

        public DateTime StartTime { get { return startTime; } }
        public int TotalTime { get { return addTime + modifyTime + verifyTime; } }
        public int Operators { get { return ids.Count; } }
        public int AddTime { get { return addTime; } }
        public int ModifyTime { get { return modifyTime; } }
        public int VerifyTime { get { return verifyTime; } }
        public int PauseTime { get { return pauseTime; } }
        public int CasesAdded { get { return casesAdded; } }
        public int RecordsAdded { get { return recordsAdded; } }
        public int CasesVerified { get { return casesVerified; } }
        public int RecordsVerified { get { return recordsVerified; } }
        public int Keystrokes { get { return keystrokes; } }
        public int BadKeystrokes { get { return badKeystrokes; } }

        public ReportStatistics(DateTime startTime)
        {
            ids = new Hashtable();

            this.startTime = startTime;
            addTime = 0;
            modifyTime = 0;
            verifyTime = 0;
            pauseTime = 0;
            casesAdded = 0;
            recordsAdded = 0;
            casesVerified = 0;
            recordsVerified = 0;
            keystrokes = 0;
            badKeystrokes = 0;
        }

        public void AddStatistics(OperatorStatistics os)
        {
            if( os.Mode == EntryMode.Add )
            {
                casesAdded += os.Cases;
                recordsAdded += os.Records;
                addTime += os.KeyingTime - os.PauseTime;
            }

            else if( os.Mode == EntryMode.Modify )
            {
                modifyTime += os.KeyingTime - os.PauseTime;
            }

            else
            {
                casesVerified += os.Cases;
                recordsVerified += os.Records;
                verifyTime += os.KeyingTime - os.PauseTime;
            }

            keystrokes += os.Keystrokes;
            badKeystrokes += os.BadKeystrokes;

            if( !ids.ContainsKey(os.ID) )
                ids.Add(os.ID,null);
        }
    }
}
