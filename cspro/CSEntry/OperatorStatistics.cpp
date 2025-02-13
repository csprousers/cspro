// OperatorStatistics.cpp: implementation of the COperatorStatistics class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "OperatorStatistics.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
COperatorStatistics::COperatorStatistics()
{
    m_iNumRecords = 0;
    m_iNumCases = 0;
    m_iKeyStrokes = 0;
    m_iNumEErr = 0;
    m_iFieldsVer = 0;
    m_iVErr = 0;
    m_iKErr = 0;

    m_bIsPaused = false;
}


void COperatorStatistics::Init(CString sMode, CString sOpID)
{
    m_sMode = sMode;
    m_sOperatorID = sOpID;

    m_sEndTime = _T("");
    m_sTotalTime = _T("");

    m_iNumRecords = 0;
    m_iNumCases = 0;
    m_iKeyStrokes = 0;
    m_iNumEErr = 0;
    m_iVErr = 0;
    m_iKErr = 0;
    m_iFieldsVer = 0;

    m_EDateTime = m_SDateTime = COleDateTime::GetCurrentTime(); // inititalize the time
    m_sStartDate = m_SDateTime.Format(_T("%m/%d/%Y"));
    m_sStartTime = m_SDateTime.Format(_T("%H:%M:%S"));

    m_bIsPaused = false;
}


// Function name    : COperatorStatistics::Stop
// Description      : Call this if you want to stop the current stats and record the values
void COperatorStatistics::Stop()
{
    if( IsPaused() ) // add the pause time (which will happen in start) and then we'll stop
        Start();

    m_EDateTime = COleDateTime::GetCurrentTime(); // end the clock
    m_sEndTime = m_EDateTime.Format(_T("%H:%M:%S")); //End Time

    COleDateTimeSpan timeSpan = m_EDateTime - m_SDateTime;

    m_sTotalTime = CIMSAString::AdjustLenLeft(IntToString((int)timeSpan.GetTotalSeconds()), NCHAR);
    m_sPauseTime = CIMSAString::AdjustLenLeft(IntToString((int)m_PauseTimeSpan.GetTotalSeconds()), NCHAR);
}


UINT COperatorStatistics::GetKeyStrokesPHr()
{
    UINT iRet = 0;

    //Compute the KeyStrokes/Hr
    CString sTotalTime = m_sTotalTime;
    sTotalTime.Trim();

    if(m_sTotalTime.IsEmpty()) {
        COleDateTimeSpan timeSpan = m_EDateTime - m_SDateTime;
        COleDateTimeSpan netTime = timeSpan - m_PauseTimeSpan;

        if(netTime.GetTotalSeconds()) {
            iRet = static_cast<UINT>(m_iKeyStrokes * DateHelper::SecondsInHour<UINT>() / netTime.GetTotalSeconds());
        }
    }

    else {
        int iTotalTime = _ttoi(sTotalTime);
        int iPauseTime = _ttoi(m_sPauseTime);
        int iTimeSpan = iTotalTime - iPauseTime ;
        if(iTimeSpan > 0 ) {
            iRet = static_cast<UINT>(m_iKeyStrokes * DateHelper::SecondsInHour<UINT>() / iTimeSpan);
        }
    }

    return iRet;
}


UINT COperatorStatistics::GetErrPKKeyStrokes()     //errors per thousand key strokes
{
    UINT iRet = 0;

    if( m_iKeyStrokes )
        iRet = (UINT)(( m_iNumEErr * 1000 ) / m_iKeyStrokes);

    return iRet;
}


float COperatorStatistics::GetPercentFWVErr()  //with Verifier errors
{
    float fRet = 0;

    if( GetNVerrors() && GetNFVerified() )
        fRet = (GetNVerrors() * 100.0f) / GetNFVerified();

    return fRet;
}


float COperatorStatistics::GetPercentFWKErr()  //With Keyer errors
{
    float fRet=0;

    if( GetNKerrors() && GetNFVerified() )
        fRet = ( GetNKerrors() * 100.0f ) / GetNFVerified();

    return fRet;
}


float COperatorStatistics::GetPercentFWErr()   //with total verify errors
{
    float fRet=0;

    UINT iTotErr = GetNKerrors() + GetNVerrors();

    if( iTotErr && GetNFVerified() )
        fRet = ( iTotErr * 100.0f ) / GetNFVerified();

    return fRet;
}


// Function name    : COperatorStatistics::Start
// Description      : Call this if only if you call pause
void COperatorStatistics::Start()
{
    //Get the pause time and add it to the current one
    m_PauseTimeSpan +=  COleDateTime::GetCurrentTime() - m_EDateTime;
    m_bIsPaused = false;
}


void COperatorStatistics::Pause()
{
    m_EDateTime = COleDateTime::GetCurrentTime();
    m_bIsPaused = true;
}
