#pragma once
// OperatorStatistics.h: interface for the COperatorStatistics class.
//
//////////////////////////////////////////////////////////////////////

#define NCHAR   8
#define NMODE   3
#define OPID    32
#define NDATE   10

class COperatorStatistics
{
public:
    COperatorStatistics();

private:
    CString             m_sMode;
    CString             m_sOperatorID;
    CString             m_sStartDate;
    CString             m_sStartTime;
    CString             m_sEndTime;
    CString             m_sTotalTime;
    CString             m_sPauseTime;

    UINT                m_iNumRecords;
    UINT                m_iNumCases;

    UINT                m_iKeyStrokes;
    UINT                m_iNumEErr;         // Number of entry Errors

    UINT                m_iKErr;            // Number of Keyer  errors
    UINT                m_iVErr;            // Number of Verifier errors;

    UINT                m_iFieldsVer;       // Number of fields verified

    bool                m_bIsPaused;

    COleDateTime        m_SDateTime;        // For internal use (Start Date Time)
    COleDateTime        m_EDateTime;        // For internal use (End Date Time)
    COleDateTimeSpan    m_PauseTimeSpan;    // Pause time span

public:
    CString GetMode() const { return CIMSAString::AdjustLenLeft(m_sMode,NMODE); }
    void SetMode(CString sMode) { m_sMode  = sMode; }

    CString GetOPID() const  { return CIMSAString::AdjustLenLeft(m_sOperatorID,OPID); }
    void SetOpID(CString sOpID) { m_sOperatorID  = sOpID; }

    CString GetEndTime() const { return CIMSAString::AdjustLenLeft(m_sEndTime,NCHAR); }
    void SetEndTime(CString sTime) { m_sEndTime = sTime; }

    CString GetStartDate() const { return CIMSAString::AdjustLenLeft(m_sStartDate,NDATE); }
    void  SetStartDate(CString sDate) { m_sStartDate = sDate; }

    CString GetStartTime() const { return CIMSAString::AdjustLenLeft(m_sStartTime,NCHAR); }
    void  SetStartTime(CString sTime) { m_sStartTime = sTime; }

    CString GetPauseTime() const { return CIMSAString::AdjustLenLeft(m_sPauseTime,NCHAR); }
    void SetPauseTime(CString sTime) { m_sPauseTime = sTime; }

    CString GetTotalTime() const { return CIMSAString::AdjustLenLeft(m_sTotalTime,NCHAR); }
    void SetTotalTime(CString sTotalTime) { m_sTotalTime = sTotalTime; }

    UINT GetTotalCases() const { return m_iNumCases; }
    void SetTotalCases(UINT iTotalCases) { m_iNumCases = iTotalCases; }

    UINT GetTotalRecords() const { return m_iNumRecords; }
    void SetTotalRecords(UINT iTotalRecords) { m_iNumRecords = iTotalRecords; }

    UINT GetKeyStrokesPHr();
    UINT GetErrPKKeyStrokes();     //errors per thousand key strokes

    float GetPercentFWVErr();  //with Verifier errors
    float GetPercentFWKErr();  //With Keyer errors
    float GetPercentFWErr();   //with total verify errors

    void Init(CString sMode, CString sOpID);
    void Start(void);
    void Pause(void);
    void Stop(void);

    void SetStartDateTime() { m_SDateTime.ParseDateTime(m_sStartDate+m_sStartTime,LOCALE_NOUSEROVERRIDE); }
    void SetEndDateTime() { m_EDateTime.ParseDateTime(m_sEndTime,VAR_TIMEVALUEONLY|LOCALE_NOUSEROVERRIDE);}
    void SetPauseTimeSpan() {
        COleDateTime timeSpan;
        timeSpan.ParseDateTime(m_sPauseTime,VAR_TIMEVALUEONLY|LOCALE_NOUSEROVERRIDE);
        m_PauseTimeSpan.SetDateTimeSpan(0,timeSpan.GetHour(),timeSpan.GetMinute(),timeSpan.GetSecond());
    }

    void IncKStroke() { m_iKeyStrokes++; } // increment keystroke
    void IncNumErr() { m_iNumEErr++; } //increment errors

    void IncNumVErr() { m_iVErr++; } //increment verifier error
    void IncNumKErr() { m_iKErr++; } //increment Keyer error

    void IncFldVerified() { m_iFieldsVer++; }   //increment number of fields  verified

    UINT GetNKeyStrokes() const { return m_iKeyStrokes; }
    void SetNKeyStrokes(UINT iKeyStrokes) { m_iKeyStrokes = iKeyStrokes; }
    UINT GetNEerrors() const { return m_iNumEErr; }
    void SetNEerrors(UINT iNEerrors) { m_iNumEErr = iNEerrors; }

    UINT GetNVerrors() const  { return  m_iVErr; }
    void SetNVerrors(UINT iNVerrors) { m_iVErr = iNVerrors; }

    UINT GetNKerrors() const  { return  m_iKErr; }
    void SetNKerrors(UINT iNKerrors) { m_iKErr = iNKerrors; }

    UINT GetNFVerified() const  { return  m_iFieldsVer; }
    void SetNFVerified(UINT iFieldsVer) { m_iFieldsVer = iFieldsVer; }

    bool IsPaused() const { return m_bIsPaused; }
};
