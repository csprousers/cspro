#pragma once

class COperatorStatistics;


class COperatorStatisticsLog
{
public:
    COperatorStatisticsLog();
    ~COperatorStatisticsLog();

    //Get the array names
    const CArray<COperatorStatistics*,COperatorStatistics*>& GetOpStatsArray() const { return m_arrOpStats; }

    COperatorStatistics* GetCurrentStatsObj() { return m_pCurrentObj; }
    void StopStatsObj();

    // Input/Output
    void Open(std::string log_file_path);
    void Save();

    CString MakeOpStatsLine(const COperatorStatistics& opStat);
    void BuildStatObj(CIMSAString sLine);

    void NewStatsObj(CString csMode,CString csOpID);

private:
    //Attributes
    std::string m_logFilePath;    // Fully qualified path+file name
    CArray<COperatorStatistics*,COperatorStatistics*> m_arrOpStats;  // array of op stats
    COperatorStatistics* m_pCurrentObj;
};
