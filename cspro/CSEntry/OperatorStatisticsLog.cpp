// OperatorStatisticsLog.cpp: implementation of the COperatorStatisticsLog class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "OperatorStatistics.h"
#include "OperatorStatisticsLog.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

#define SEPARATOR _T(",")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
COperatorStatisticsLog::COperatorStatisticsLog()
{
    m_pCurrentObj = NULL;
}

COperatorStatisticsLog::~COperatorStatisticsLog()
{
    for( int iIndex = 0; iIndex < m_arrOpStats.GetSize(); iIndex++ )
        delete m_arrOpStats[iIndex];
}

void COperatorStatisticsLog::Open(CString csLogFilename)
{
    m_csLogFilename = csLogFilename;

    if( PortableFunctions::FileExists(m_csLogFilename) )
    {
        CSpecFile statFile;

        if( statFile.Open(m_csLogFilename,CFile::modeRead) )
        {
            CString csLine;

            while( statFile.ReadString(csLine) )
            {
                if( !csLine.IsEmpty() )
                    BuildStatObj(csLine);
            }

            statFile.Close();
        }

        else
            AfxMessageBox(_T("There was an error opening the operator statistics log."),MB_ICONSTOP | MB_OK);
    }
}

void COperatorStatisticsLog::Save()
{
    if( m_pCurrentObj != NULL )
        m_pCurrentObj->Stop();

    CSpecFile statFile;

    if( statFile.Open(m_csLogFilename,CFile::modeWrite) )
    {
        for( int iIndex = 0; iIndex < m_arrOpStats.GetSize(); iIndex++ )
        {
            COperatorStatistics* opStat = m_arrOpStats.GetAt(iIndex);
            CString csLine = MakeOpStatsLine(*opStat);
            statFile.PutLine(csLine);
        }

        statFile.Close();
    }

    else
        AfxMessageBox(_T("There was an error saving the operator statistics log."),MB_ICONSTOP | MB_OK);
}


CString COperatorStatisticsLog::MakeOpStatsLine(const COperatorStatistics& opStat)
{
    CString sRet;

    sRet  = opStat.GetMode() + SEPARATOR;       //application mode
    sRet += opStat.GetOPID() + SEPARATOR;       //operator id
    sRet += opStat.GetStartDate() + SEPARATOR;  //Start date
    sRet += opStat.GetStartTime() + SEPARATOR;  //Start time
    sRet += opStat.GetEndTime() + SEPARATOR;    //end time
    sRet += opStat.GetTotalTime() + SEPARATOR;  //total time
    sRet += opStat.GetPauseTime() + SEPARATOR;  //amount of pause time

    CString sString;
    _ultot(opStat.GetTotalCases(),sString.GetBuffer(NCHAR),10);
    sString.ReleaseBuffer();
    sString = CIMSAString::AdjustLenLeft(sString,NCHAR);
    sRet += sString + SEPARATOR;    //Total number of cases

    sString.Empty();
    _ultot(opStat.GetTotalRecords(),sString.GetBuffer(NCHAR),10);
    sString.ReleaseBuffer();
    sString = CIMSAString::AdjustLenLeft(sString,NCHAR);
    sRet += sString + SEPARATOR;    //Total records

    sString.Empty();
    _ultot(opStat.GetNKeyStrokes(),sString.GetBuffer(NCHAR),10);
    sString.ReleaseBuffer();
    sString = CIMSAString::AdjustLenLeft(sString,NCHAR);
    sRet += sString + SEPARATOR;    //number of keystrokes

    sString.Empty();
    _ultot(opStat.GetNEerrors(),sString.GetBuffer(NCHAR),10);
    sString.ReleaseBuffer();
    sString = CIMSAString::AdjustLenLeft(sString,NCHAR);
    sRet += sString + SEPARATOR;    //number of entry errors

    sString.Empty();
    _ultot(opStat.GetNKerrors(),sString.GetBuffer(NCHAR),10);
    sString.ReleaseBuffer();
    sString = CIMSAString::AdjustLenLeft(sString,NCHAR);
    sRet += sString + SEPARATOR;    //number of Keyer errors

    sString.Empty();
    _ultot(opStat.GetNVerrors(),sString.GetBuffer(NCHAR),10);
    sString.ReleaseBuffer();
    sString = CIMSAString::AdjustLenLeft(sString,NCHAR);
    sRet += sString + SEPARATOR;    //number of verify errors

    sString.Empty();
    _ultot(opStat.GetNFVerified(),sString.GetBuffer(NCHAR),10);
    sString.ReleaseBuffer();
    sString = CIMSAString::AdjustLenLeft(sString,NCHAR);
    sRet += sString;

    return sRet;
}


void COperatorStatisticsLog::BuildStatObj(CIMSAString sLine)
{
    CString sValue = sLine.GetToken(SEPARATOR);

    if( sValue.IsEmpty() )
        return;

    COperatorStatistics* pOpStats = new COperatorStatistics();
    m_arrOpStats.Add(pOpStats);

    pOpStats->SetMode(sValue);              //Mode

    sValue = sLine.GetToken(SEPARATOR);
    pOpStats->SetOpID(sValue);              // op ID

    sValue = sLine.GetToken(SEPARATOR);
    pOpStats->SetStartDate(sValue);         //Start date

    sValue = sLine.GetToken(SEPARATOR);
    pOpStats->SetStartTime(sValue);         //Start Time

    sValue = sLine.GetToken(SEPARATOR);
    pOpStats->SetEndTime(sValue);           //End Time

    sValue = sLine.GetToken(SEPARATOR);
    pOpStats->SetTotalTime(sValue);         //Total Time

    sValue = sLine.GetToken(SEPARATOR);
    pOpStats->SetPauseTime(sValue);         //Pause time

    sValue = sLine.GetToken(SEPARATOR);
    pOpStats->SetTotalCases(_ttol(sValue));  //Total cases

    sValue = sLine.GetToken(SEPARATOR);
    pOpStats->SetTotalRecords(_ttol(sValue));    //Total Records

    sValue = sLine.GetToken(SEPARATOR);
    pOpStats->SetNKeyStrokes(_ttol(sValue)); //Total Key Strokes

    sValue = sLine.GetToken(SEPARATOR);
    pOpStats->SetNEerrors(_ttol(sValue));    //Num of entry errrors

    sValue = sLine.GetToken(SEPARATOR);
    pOpStats->SetNKerrors(_ttol(sValue));    //Num of keyer errrors

    sValue = sLine.GetToken(SEPARATOR);
    pOpStats->SetNVerrors(_ttol(sValue));    //Num of verifier errrors

    sValue = sLine.GetToken(SEPARATOR);
    pOpStats->SetNFVerified(_ttol(sValue));    //Num of fields verified
}

void COperatorStatisticsLog::NewStatsObj(CString csMode,CString csOpID)
{
    COperatorStatistics* pOpStats = new COperatorStatistics();

    if( csOpID.IsEmpty() )
        csOpID = _T("<No ID Entered>");

    pOpStats->Init(csMode,csOpID); // Initializes the start time

    m_arrOpStats.Add(pOpStats);
    m_pCurrentObj = pOpStats;
}

void COperatorStatisticsLog::StopStatsObj()
{
    if( m_pCurrentObj != NULL )
    {
        m_pCurrentObj->Stop();
        m_pCurrentObj = NULL;
    }
}
