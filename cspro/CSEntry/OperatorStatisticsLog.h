#pragma once
// OperatorStatisticsLog.h: interface for the COperatorStatisticsLog class.
//
//////////////////////////////////////////////////////////////////////

class COperatorStatistics;

class COperatorStatisticsLog
{
public:
    COperatorStatisticsLog();
    ~COperatorStatisticsLog();

private:
    //Attributes
    CString m_csLogFilename;    // Fully qualified path+file name
    CArray<COperatorStatistics*,COperatorStatistics*> m_arrOpStats;  // array of op stats
    COperatorStatistics* m_pCurrentObj;

public:
    //Get the array names
    const CArray<COperatorStatistics*,COperatorStatistics*>& GetOpStatsArray() const { return m_arrOpStats; }

    COperatorStatistics* GetCurrentStatsObj() { return m_pCurrentObj; }
    void StopStatsObj();

    // Input/Output
    void Open(CString csLogFilename);
    void Save();

    CString MakeOpStatsLine(const COperatorStatistics& opStat);
    void BuildStatObj(CIMSAString sLine);

    void NewStatsObj(CString csMode,CString csOpID);
};
