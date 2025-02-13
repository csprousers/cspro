// SourceCode.cpp: implementation of the CSourceCode class.
//
//////////////////////////////////////////////////////////////////////
// CHANGES      : Jul 13, 1999, RHF, Creation
//                Apr 05, 2000, RHF, Eliminate PROC UNKNOWN
//                        This PROC is keeped like a "NoProc" event.

#include "StdAfx.h"
#include "SrcCode.h"
#include <zToolsO/Tools.h>
#include <zLogicO/LogicScanner.h>


constexpr const TCHAR* HeadingProcName = _T("__HEADING__");
constexpr const TCHAR* GlobalProcName  = _T("GLOBAL");

constexpr const TCHAR* pszNextCharBreak = _T(" \t\n{+-*/%^()[]&|!,;:<=>$#\"\'\\");
constexpr const TCHAR* pszPrevCharBreak = _T(" \t\n}+-*/%^()[]&|!,;:<=>$#\"\'\\");

const std::wstring CSOURCECODE_PROC            = _T("PROC");
constexpr const TCHAR* CSOURCECODE_PREPROC     = _T("PreProc");
constexpr const TCHAR* CSOURCECODE_POSTPROC    = _T("PostProc");
constexpr const TCHAR* CSOURCECODE_ONFOCUS     = _T("OnFocus");
constexpr const TCHAR* CSOURCECODE_KILLFOCUS   = _T("KillFocus");
constexpr const TCHAR* CSOURCECODE_ONOCCCHANGE = _T("OnOccChange");
constexpr const TCHAR* CSOURCECODE_TALLY       = _T("Tally");
constexpr const TCHAR* CSOURCECODE_POSTCALC    = _T("PostCalc");


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CProcEventArray::CProcEventArray()
{
    m_procEvents.emplace_back(CSOURCECODE_PROC, CSourceCode_ProcProc);
    m_procEvents.emplace_back(CSOURCECODE_PREPROC, CSourceCode_PreProc);
    m_procEvents.emplace_back(CSOURCECODE_ONFOCUS, CSourceCode_OnFocus);
    m_procEvents.emplace_back(CSOURCECODE_KILLFOCUS, CSourceCode_KillFocus);
    m_procEvents.emplace_back(CSOURCECODE_POSTPROC, CSourceCode_PostProc);
    m_procEvents.emplace_back(CSOURCECODE_ONOCCCHANGE, CSourceCode_OnOccChange);
    m_procEvents.emplace_back(CSOURCECODE_TALLY, CSourceCode_Tally);
    m_procEvents.emplace_back(CSOURCECODE_POSTCALC, CSourceCode_PostCalc);
}

const std::wstring& CProcEventArray::GetEventName(CSourceCode_EventType eEventType) const
{
    const auto& lookup = std::find_if(m_procEvents.cbegin(), m_procEvents.cend(),
                                      [&](const auto& pc) { return ( pc.GetEventType() == eEventType ); });
    return ( lookup != m_procEvents.cend() ) ? lookup->GetEventName() : SO::EmptyString;
}



CSourceCode::CSourceCode(Application& application)
    :   m_application(application),
        m_bModifiedFlag(false)
{
    CodeFile* logic_main_code_file = application.GetLogicMainCodeFile();

    if( logic_main_code_file != nullptr )
    {
        m_logicTextSource = logic_main_code_file->GetSharedTextSource();
    }

    else
    {
        ASSERT(false);
        m_logicTextSource = std::make_shared<TextSource>();
    }

    //SAVY
    m_aEventCode.SetSize(0,10);
}

CSourceCode::~CSourceCode()
{
    for( int i = 0; i < m_aEventCode.GetSize(); i++ )
        delete m_aEventCode[i];
}


// Return true is csLine begins with CSOURCECODE_PROC. Additionally obtain the
// procedure name in csSymbol
bool CSourceCode::IsProcLine( const CString& csLine, CString& csSymbol, bool* bHasMoreInfo ) const
{
    const TCHAR* p = SkipSpecialZone(csLine);
    bool bIsProcLine = SO::StartsWithNoCase(p, CSOURCECODE_PROC);

    if( bIsProcLine ) {
        p += CSOURCECODE_PROC.length();
        bIsProcLine = ( _tcschr( pszNextCharBreak, *p ) != NULL );
    }

    if(bIsProcLine) {
        CIMSAString csTemp = SkipSpecialZone(p);
        TCHAR cLast[2];

        csSymbol = csTemp.GetToken(pszNextCharBreak, cLast, false );

        if( bHasMoreInfo )
            *bHasMoreInfo  = !(*cLast == 0 || *cLast == '\n');

        // RHF INIC Aug 31, 2001
        if( csSymbol.GetLength() == 0 )
            bIsProcLine = false;
        // RHF END Aug 31, 2001


        //if( !IsValidName( csSymbol ) )
        //    bIsProcLine = false;
    }

    return( bIsProcLine );
}

bool CSourceCode::IsEventLine(const CString& line, const std::wstring& event_name)
{
    const TCHAR* p = SkipSpecialZone(line);
    bool bEventLine = SO::StartsWithNoCase(p, event_name);

    if( bEventLine ) {
        p += event_name.length();
        bEventLine = ( _tcschr( pszNextCharBreak, *p ) != NULL );
    }

    return bEventLine;
}


static CMap<CString,LPCTSTR,int,int>* aProcOrder=NULL;

static inline int CompareProc( const void* elem1, const void* elem2 ) {
    CEventCode*      pEventCode1;
    CEventCode*      pEventCode2;
    CSourceCode_EventType eEventType1;
    CSourceCode_EventType eEventType2;


    pEventCode1 = *( (CEventCode** ) elem1 );
    pEventCode2 = *( (CEventCode** ) elem2 );
    eEventType1 = pEventCode1->GetEventType();
    eEventType2 = pEventCode2->GetEventType();

    // RHF INIC Apr 05, 2000
    // PROC UNKNOWN Always is the First
    if( eEventType1 == CSourceCode_NoProc )
        return( -1 );
    else if(  eEventType2 == CSourceCode_NoProc )
        return( 1 );
    // RHF END Apr 05, 2000


    // RHF INIC Jun 15, 2000
    if( pEventCode1->GetSymbolName().CompareNoCase( GlobalProcName ) == 0 )
        return( -1 );
    else if( pEventCode2->GetSymbolName().CompareNoCase( GlobalProcName ) == 0 )
        return( 1 );
    // RHF END Jun 15, 2000

    CString     csName1=pEventCode1->GetSymbolName();
    CString     csName2=pEventCode2->GetSymbolName();
    int         iRet;

    // Default: Sort by names
    iRet = csName1.CompareNoCase( csName2 );

    if( iRet != 0 && aProcOrder != NULL ) {// Sort by the order list provided by SetOrder function
        int     iPos1, iPos2;

        csName1.MakeUpper();
        csName2.MakeUpper();
        if( !aProcOrder->Lookup( csName1, iPos1 ) )
            iPos1 = 0;

        if( !aProcOrder->Lookup( csName2, iPos2 ) )
            iPos2 = 0;

        // The symbols not in the order map are saved at the beggining (and sorted by name)
        if( iPos1 == 0 && iPos2 == 0 ) // csName1 and csName2 are not in the order map
            // Unnecessay...Already done ... iRet = csName1.CompareNoCase( csName2 );
            ;
        else if( iPos1 == 0 )
            iRet = -1;
        else if( iPos2 == 0 )
            iRet = 1;
        else {
            ASSERT( iPos1 != iPos2 );
            iRet = (iPos1 > iPos2 ) ? 1 : -1;
        }
    }


    // pEventCode1 and pEventCode2 have the same name
    if( iRet == 0 ){
        ASSERT( !(eEventType1 == eEventType2) );
        iRet = ( eEventType1 > eEventType2 ) ? 1 : -1;
    }

    return iRet;
}

// Sort by name and event
void CSourceCode::SortProc()
{
    aProcOrder = ( m_aProcOrder.GetCount() > 0 ) ? &m_aProcOrder : NULL;
    qsort(m_aEventCode.GetData(), m_aEventCode.GetSize(), sizeof(CEventCode*), CompareProc);
    aProcOrder = NULL;
}


int CSourceCode::RemoveProc( const CString csSymbolName, CSourceCode_EventType eEventType )
{
    int             iNumEventProc=m_aEventCode.GetSize();
    int                             iRemovedEvents=0;
    CEventCode*     pEventCode;
    CArray<int,int> aToRemove;

    aToRemove.RemoveAll();
    for( int i = 0; i < iNumEventProc; i++ ) {
        pEventCode = m_aEventCode[i];
        if( pEventCode->GetSymbolName().CompareNoCase( csSymbolName ) == 0 &&
            ( eEventType == CSourceCode_AllEvents ||
            eEventType == pEventCode->GetEventType() ) ) {
            delete pEventCode;
            aToRemove.Add( i );
            iRemovedEvents++;
        }
    }

    for( int i = aToRemove.GetSize()-1; i >= 0; i-- ) {
        int iProcEvent = aToRemove.GetAt(i);
        m_aEventCode.RemoveAt(iProcEvent);
    }

    return( iRemovedEvents );
}



int CSourceCode::GetEventProcList( const CString& csSymbolName, CArray <CEventCode*,CEventCode*>& aEventProcList )
{
    for( int i = 0; i < m_aEventCode.GetSize(); i++ ) {
        CEventCode* pEventCode = m_aEventCode[i];
        if( pEventCode->GetSymbolName().CompareNoCase( csSymbolName ) == 0 )
            aEventProcList.Add( pEventCode );
    }

    return aEventProcList.GetSize();
}

CEventCode* CSourceCode::SearchEvent( const CArray <CEventCode*,CEventCode*>& aEventProcList, CSourceCode_EventType eEventType ) const
{
    int     iNumEvent=aEventProcList.GetSize();

    for( int i = 0; i < iNumEvent; i++ )
        if( eEventType == aEventProcList[i]->GetEventType() )
            return aEventProcList[i];


        return( NULL );
}

CEventCode* CSourceCode::GetEvent( const CString& csSymbolName, CSourceCode_EventType eEventType)
{
    CArray <CEventCode*,CEventCode*> aEventProcList;
    aEventProcList.SetSize(0,10); //SAVY 09/27/00
    int iNumEventProc;

    if( ( iNumEventProc = GetEventProcList( csSymbolName, aEventProcList ) ) > 0 )
        return SearchEvent( aEventProcList, eEventType );

    return NULL;
}


// Copy without initial blank lines
void CSourceCode::CopyWithoutBlankLines(const CStringArray& aProcLines, CStringArray& aProcLinesExtended ) {
    int     iStartLine=0;
    CString csLine;

    for( int i = 0; i < aProcLines.GetSize(); i++, iStartLine++ ) {
        csLine.Format( _T("%s"), aProcLines.GetAt(i).GetString() ); // No reference
        if( _tcstok( (TCHAR*)(LPCTSTR) csLine, _T(" \t\n") ) != NULL )  // Has something
            break;
    }

    if( iStartLine == 0 )
        aProcLinesExtended.Append( aProcLines );
    else {
        for( int i = iStartLine; i < aProcLines.GetSize(); i++ )
            aProcLinesExtended.Add( aProcLines.GetAt(i) );
    }
}

// Remove initial blank lines and Insert event line if needed
void CSourceCode::InsertEventLine(const CStringArray& aProcLines, CStringArray& aProcLinesExtended, CSourceCode_EventType eEventType ) {
#ifdef _DEBUG
    ASSERT(eEventType==CSourceCode_PreProc ||
           eEventType==CSourceCode_PostProc||
           eEventType==CSourceCode_OnFocus ||
           eEventType==CSourceCode_KillFocus ||
           eEventType==CSourceCode_OnOccChange ||
           eEventType==CSourceCode_Tally ||
           eEventType==CSourceCode_PostCalc);
#endif

    CopyWithoutBlankLines( aProcLines, aProcLinesExtended );

    std::wstring event_name = m_cProcEventArray.GetEventName(eEventType);
    bool bHasSomeLine = (aProcLinesExtended.GetSize() > 0 );

    if( bHasSomeLine ) {
        CString csLine;
        bool bHasEvent=false;

        csLine = aProcLinesExtended.GetAt(0);
        if( IsEventLine( csLine, event_name ) )
            bHasEvent = true;

        if( !bHasEvent )  // Insert  Event line
            aProcLinesExtended.InsertAt( 0, WS2CS(event_name) );
    }
}


// Remove initial blank lines and Insert PROC line if needed
void CSourceCode::InsertProcLine(const CStringArray& aProcLines, CStringArray& aProcLinesExtended, CString csSymbolName ) {
    if( &aProcLines != &aProcLinesExtended ) // RHF May 30, 2003
    CopyWithoutBlankLines( aProcLines, aProcLinesExtended );

    bool bHasSomeLine = (aProcLinesExtended.GetSize() > 0 );
    if( bHasSomeLine ) {
        CString csLine;
        CString csSymbolNameAux;
        bool bHasTargetProcLine = false;
        bool bHasOtherProcLine = false;

        //csLine = aProcLinesExtended.GetAt(0);
        csLine = aProcLinesExtended.ElementAt(0);//SAVY CHANGED VAL TO REFERENCE 09/27/00
        if( IsProcLine( csLine, csSymbolNameAux ) ) {
            if( csSymbolName.CompareNoCase( csSymbolNameAux ) == 0 ) {
                bHasTargetProcLine = true;
            }
            else {
                /*
                CString csMsg;
                csMsg.Format( "You are creating a new PROC %s", csSymbolNameAux );
                AfxMessageBox( csMsg );
                */
                bHasOtherProcLine = true;
            }
        }

        if( !bHasTargetProcLine && !bHasOtherProcLine ) { // Insert line PROC target
            csLine.Format( _T("%s %s"), CSOURCECODE_PROC.c_str(), csSymbolName.GetString() );
            aProcLinesExtended.InsertAt( 0, csLine );
        }
    }
}

bool CSourceCode::PutProc( const CStringArray& aProcLines )
{
    // Remove All Procedures
    for( int i = 0; i < m_aEventCode.GetSize(); i++ )
        delete m_aEventCode[i];
    m_aEventCode.RemoveAll();

    CStringArray aProcLinesExtended;
    aProcLinesExtended.InsertAt( 0, (CStringArray*)&aProcLines);// RHF Apr 05, 2000

    return Load(aProcLinesExtended, false);
}

// if we use PutProc( Buffer, "Symbol", "PROC/PreProc/PostProc" )
// aProcLines must contain the respective prefixs
bool CSourceCode::PutProc( const CStringArray& aProcLines, const CString& csSymbolName, CSourceCode_EventType eEventType )
{
    CStringArray aProcLinesExtended;
    CString csSymbolNameAux;
    bool bRet=false;

    // Remove only the target PROC
    RemoveProc( csSymbolName, eEventType );

    bool            bNeedProcLine;
    bool            bIgnoreProcProc;// RHF May 30, 2003 Add bIgnoreProcProc

    bNeedProcLine = ( eEventType == CSourceCode_AllEvents ||
                      eEventType == CSourceCode_ProcProc );

    // RHF INIC May 30, 2003
    bIgnoreProcProc = ( eEventType==CSourceCode_PreProc ||
                        eEventType==CSourceCode_PostProc ||
                        eEventType==CSourceCode_OnFocus ||
                        eEventType==CSourceCode_KillFocus ||
                        eEventType==CSourceCode_OnOccChange ||
                        eEventType==CSourceCode_Tally ||
                        eEventType==CSourceCode_PostCalc );
    // RHF END May 30, 2003

#ifdef _DEBUG
    ASSERT(eEventType==CSourceCode_AllEvents ||
           eEventType==CSourceCode_ProcProc ||
           eEventType==CSourceCode_PreProc ||
           eEventType==CSourceCode_PostProc ||
           eEventType==CSourceCode_OnFocus ||
           eEventType==CSourceCode_KillFocus ||
           eEventType==CSourceCode_OnOccChange ||
           eEventType==CSourceCode_Tally ||
           eEventType==CSourceCode_PostCalc);
#endif


    if( bNeedProcLine )
        InsertProcLine( aProcLines, aProcLinesExtended, csSymbolName );
    else {
        InsertEventLine( aProcLines, aProcLinesExtended, eEventType );
        InsertProcLine( aProcLinesExtended, aProcLinesExtended, csSymbolName ); // RHF May 30, 2003
    }


    bRet = Load( aProcLinesExtended, bIgnoreProcProc );// RHF May 30, 2003 Add bIgnoreProcProc

    // Don't save when only 1 line containg "PROC symbol" is found
    bool    bHasMoreInfo;
    if( bRet && aProcLinesExtended.GetSize() == 1 &&
       // IsProcLine( aProcLinesExtended.GetAt(0), csSymbolNameAux, &bHasMoreInfo ) &&
        IsProcLine( aProcLinesExtended.ElementAt(0), csSymbolNameAux, &bHasMoreInfo ) && //SAVY CHANGED VALUE TO REFERENCE 09/27/00
        csSymbolName.CompareNoCase( csSymbolNameAux ) == 0 &&
        !bHasMoreInfo ) {
        RemoveProc( csSymbolName, eEventType );
    }

    return bRet;
}


// GetProc("") Load all the aplication (include PROC and prefixs)
// GetProc( "Symbol" ); Load the complete procedure (include PROC and prefixs)
// GetProc ("Symbol", "PROC/PreProc/PostProc" ); Load the
// respective event include prefixs
bool CSourceCode::GetProc( CStringArray& aProcLines, const CString csSymbolName/*=""*/,
                          CSourceCode_EventType eEventType/*=CSourceCode_AllEvents*/) {
    bool                    bRet=false;
    int                     iNumEventProc=m_aEventCode.GetSize();
    int                     iNumLines;
    CEventCode*             pEventCode;
    CString                 csCurrentSymbolName;
    CString                 csOldSymbolName=_T("");
    bool                    bWriteProc;
    CSourceCode_EventType   eCurrentEventType;

    aProcLines.RemoveAll();

    //SAVY ADDED THIS BLCK OF CODE TO AVOID MULTIPLE REALLOCATIONS 09/27/00
    aProcLines.SetSize(0,100);

    if( iNumEventProc > 1 )
        SortProc();

    //Save the Proc to a buffer
    for( int i = 0; i < iNumEventProc; i++ ) {
        pEventCode = m_aEventCode[i];
        if( ( iNumLines = pEventCode->GetNumLines() ) == 0 )
            continue;

        csCurrentSymbolName = pEventCode->GetSymbolName();
        eCurrentEventType = pEventCode->GetEventType();

        // Match Symbol
        if( csSymbolName.GetLength() > 0 &&
            csCurrentSymbolName.CompareNoCase( csSymbolName ) != 0 )
            continue;

        // Match Event
        if( eEventType != CSourceCode_AllEvents &&
            eCurrentEventType != eEventType )
            continue;

        bWriteProc = ( csCurrentSymbolName.CompareNoCase( csOldSymbolName ) != 0 );
        csOldSymbolName = csCurrentSymbolName;


        if( !bRet )
            bRet = ( iNumLines > 0 );

        for( int j=0; j < iNumLines; j++ ) {
            aProcLines.Add( pEventCode->GetLine(j) );
        }
    }
    //SAVY 09/27/00 FREE EXTRA MEMORY ALLOCATED THRU SETSIZE
    aProcLines.FreeExtra();
    return bRet;
}

int CSourceCode::GetProcNames( CStringArray& aProcNames ) {
    CString csSymbolName;
    CString csOldSymbolName;
    int     iNumEventProc=m_aEventCode.GetSize();
    int     iNumSymbols=0;
    bool    bCanAdd=false;

    if( iNumEventProc > 1 )
        SortProc();

    csOldSymbolName = _T("");
    for( int i = 0; i < iNumEventProc; i++ ) {
        csSymbolName = m_aEventCode[i]->GetSymbolName();

        if( csOldSymbolName.GetLength() > 0 )
            bCanAdd = ( csSymbolName.CompareNoCase( csOldSymbolName ) != 0 );
        else
            bCanAdd = true;

        if( bCanAdd ) {
            aProcNames.Add( csSymbolName );
            iNumSymbols++;
        }

        csOldSymbolName = csSymbolName;
    }

    return( iNumSymbols );
}

int CSourceCode::StringToArray( const CString& csAllLines, CStringArray* paLinesCode ) {
    TCHAR*   p=(TCHAR*)(LPCTSTR) csAllLines;
    TCHAR*   q;

    while( p ) {
        if( ( q = _tcschr( p, '\n' ) ) != NULL )
            *q = 0;
        paLinesCode->Add( p );

        if( q == NULL )
            break;

        p = q + 1;
    }

    int     iLine;

    // Remove last empty line
    if( (iLine=paLinesCode->GetSize()-1) >= 0 ) {
        //if( paLinesCode->GetAt(iLine).GetLength() == 0 )
        if( paLinesCode->ElementAt(iLine).GetLength() == 0 ) // SAVY CHANGED TO USE THE REFERENCE INSTEAD OF A COPY 09/27/00
            paLinesCode->RemoveAt( iLine );
    }

    return( paLinesCode->GetSize() );
}


///////////////////////////////////////////////////////////////////////////////////////
//
//      int CSourceCode::ArrayToString( const CStringArray* paLinesCode, CString& csAllLines, bool bAddNewLine )
//
// 09/27/00 .to make the string concatenation more efficient
// RHF VERIFY
///////////////////////////////////////////////////////////////////////////////////////
// Return number of lines
int CSourceCode::ArrayToString( const CStringArray* paLinesCode, CString& csAllLines, bool bAddNewLine ) {
    int     iNumLines = paLinesCode->GetSize();
    CStringArray* pArray = const_cast<CStringArray*>(paLinesCode);

    csAllLines = _T("");
    //Get the total memory to allocate
    UINT uAlloc = 0;
    for(int iIndex =0; iIndex <iNumLines ;iIndex++){
        uAlloc  +=  pArray->ElementAt(iIndex).GetLength();
        if(bAddNewLine)
            uAlloc ++; // for the "\n"
    }
    uAlloc++; //for the "\0" @ the end
    LPTSTR pString = csAllLines.GetBufferSetLength(uAlloc);
    _tmemset(pString ,_T('\0'),uAlloc);
    for( int iLine=0; iLine < iNumLines; iLine++ ) {
        //CString& csLine = (*paLinesCode)[iLine];
        CString& csLine = (*pArray)[iLine];
        int iLength = 0;
        if( bAddNewLine ) {
            //csAllLines += csLine + "\n";
            iLength = csLine.GetLength();
            _tmemcpy(pString,csLine.GetBuffer(iLength),iLength);
            csLine.ReleaseBuffer();
            pString += iLength;
            _tmemcpy(pString,_T("\n"),1);
            pString++;

        }
        else {
            //csAllLines += csLine;
            iLength = csLine.GetLength();
            _tmemcpy(pString,csLine.GetBuffer(iLength),iLength);
            csLine.ReleaseBuffer();
            pString += iLength;
        }
    }

    csAllLines.ReleaseBuffer();
    return( iNumLines );
}

// New version of LOAD RHF Mar 23, 2000
// Accept comments
bool CSourceCode::Load(CString csAllLines, bool bIgnoreProcProc)
{
    TCHAR* pszBuf = csAllLines.GetBuffer();
    const TCHAR* p = pszBuf;
    bool bError=false;
    CString csText;
    CMap<CSourceCode_EventType,CSourceCode_EventType,CSourceCode_EventType,CSourceCode_EventType> aHasEvent;

    CSourceCode_EventType   eLastType;
    CSourceCode_EventType   eType;

    const TCHAR* pszText;

    CString csProcName;
    CString csLastProcName;

    bool    bProcFound=true;
    // RHF COM Nov 09, 2000 int     iEventLine=0;
    int     iLastEventLine=0;

    pszText = ScanEvent( p, eLastType, csLastProcName, &aHasEvent, &iLastEventLine );
    if( pszText == NULL ) {
        // AfxMessageBox( "No PROC Event Found. PROC UNKNOWN will be generated" );
        bError = true;
        bProcFound = false;
    }
    else if ( eLastType != CSourceCode_ProcProc ) {
        // AfxMessageBox( "PROC clause must be the first event. PROC UNKNOWN will be generated" );
        bError = true;
        bProcFound = false;
    }
    else if( p != pszText ) {
        // AfxMessageBox( "Skipped Lines before first PROC. PROC UNKNOWN will be generated" );
        bError = true;
        bProcFound = true;
    }

    int iEventLine = iLastEventLine; // RHF Nov 09, 2000

    if( bError ) {
        if( pszText != NULL ) {
            csText = CString(p, pszText - p);
        }
        else {
            csText = p;
        }

        // insert text that will be sorted above PROC GLOBAL
        ASSERT(GetEvent(HeadingProcName, CSourceCode_NoProc) == NULL);

        AddEvent(csText, CSourceCode_NoProc, HeadingProcName, 0); // At line 0

        if( !bProcFound )
            csLastProcName = HeadingProcName;
    }

    if( pszText != NULL && *pszText ) {
        p = pszText+1;
        while( *p ) {
            if( ( pszText = ScanEvent( p, eType, csProcName, &aHasEvent, &iEventLine ) ) == NULL ) {
                csText.Format( _T("%s"),  p-1 );

                if( !bIgnoreProcProc ||  bIgnoreProcProc && eLastType!=CSourceCode_ProcProc )// RHF May 30, 2003
                  AddEvent( csText, eLastType, csLastProcName, iLastEventLine );

                break;
            }

            csText = CString(p - 1, pszText - p + 1);

            if( !bIgnoreProcProc ||  bIgnoreProcProc && eLastType!=CSourceCode_ProcProc )// RHF May 30, 2003
                AddEvent( csText, eLastType, csLastProcName, iLastEventLine );

            if( eType == CSourceCode_ProcProc )
                csLastProcName = csProcName;

            eLastType = eType;
            iLastEventLine = iEventLine;

            p = pszText+1;
        }
    }

    return( true );
}

bool CSourceCode::Load(const CStringArray& aProcLines, bool bIgnoreProcProc)
{
    CString csAllLines;
    ArrayToString(&aProcLines, csAllLines, true);
    return Load(csAllLines, bIgnoreProcProc);
}


bool CSourceCode::Load()
{
    return Load(WS2CS(m_logicTextSource->GetText()), false);
}


bool CSourceCode::Save()
{
    if( m_aEventCode.GetSize() > 1 )
        SortProc();

    CStringArray aProcLines;
    CString csAllLines;

    if( GetProc(aProcLines) )
        ArrayToString(&aProcLines, csAllLines, true);

    m_logicTextSource->SetText(CS2WS(csAllLines));
    m_logicTextSource->Save();

    return true;
}


// Skip the special zone and any whitespace characters
const TCHAR* CSourceCode::SkipSpecialZone(const TCHAR* buffer) const
{
    if( buffer != nullptr )
    {
        Logic::LogicScanner logic_scanner(m_application.GetLogicSettings());

        for( ; ; ++buffer )
        {
            const TCHAR ch = *buffer;
            logic_scanner.ProcessCharacter(ch, ( ch == 0 ) ? 0 : buffer[1]);

            if( !logic_scanner.InSpecialSection() && !std::iswspace(ch) )
                break;
        }
    }

    return buffer;
}


// csProcName filled if EventType is a PROC
// eEventType can be: NOPROC or PROC or any event added in CProcEventArray::CProcEventArray()
const TCHAR* CSourceCode::ScanEvent(const TCHAR* pszBuf, CSourceCode_EventType& eEventType, CString& csProcName,
                                    CMap<CSourceCode_EventType,CSourceCode_EventType,CSourceCode_EventType,CSourceCode_EventType>* pIgnoreEvents,
                                    int* iEventLine )
{
    TCHAR* p = const_cast<TCHAR*>(pszBuf);
    bool bFound = false;

    // Special zones
    Logic::LogicScanner logic_scanner(m_application.GetLogicSettings());

    eEventType = CSourceCode_NoProc;

    for( ; *p; p++ ) { // Scanning loop
        const TCHAR ch = *p;

        if( ch == '\n' && iEventLine != NULL )
            (*iEventLine)++;

        logic_scanner.ProcessCharacter(ch, ( ch == 0 ) ? 0 : p[1]);

        if( logic_scanner.InSpecialSection() )
            continue;

        int iLen = 0;

        // Some character not in special zone or unbalanced close comment
        if( SO::StartsWithNoCase(p, CSOURCECODE_PROC) ) {
            bFound = true;
            iLen = CSOURCECODE_PROC.length();
            eEventType = CSourceCode_ProcProc;
        }
        else {
              //SAVY 09/27/00 this is being called a lot
            for( const CProcEvent& proc_event : m_cProcEventArray.GetProcEvents() ) {
                if( SO::StartsWithNoCase(p, proc_event.GetEventName()) ) {
                    eEventType = proc_event.GetEventType();
                    bFound = true;
                    iLen = proc_event.GetEventName().length();
                    break;
                }
            }
        }

        if( bFound ) {
            // Check next character
            bFound = ( _tcschr( pszNextCharBreak, *(p + iLen) ) != NULL );

            // Check previous character
            if( bFound ) {
                if( p > pszBuf )
                    bFound = ( _tcschr( pszPrevCharBreak, *(p-1) ) != NULL );
            }
        }

        // Check correct PROC clausule
        if( bFound && eEventType == CSourceCode_ProcProc ) {
            CString csLine;
            CString csProcNameAux;
            bool bIsProcLine;
            TCHAR* q;

            if( ( q = _tcschr( p, '\n' ) ) != NULL )
                *q = 0;
            csLine.Format( _T("%s"), p );
            if( q != NULL )
                *q = '\n';

            bIsProcLine = IsProcLine( csLine, csProcNameAux );

            if( bIsProcLine ) {
                csProcName = csProcNameAux;
            }
            else {
                bFound = false;
            }
        }


        // Don't considerate duplicated events
        if( bFound && pIgnoreEvents != NULL ) {
            CSourceCode_EventType eAuxEventType;

            if( 0 &&pIgnoreEvents->Lookup(eEventType, eAuxEventType ) ) {
                ASSERT( eEventType == eAuxEventType );
                bFound = false;
            }

        }

        if( bFound ) {
            if( pIgnoreEvents != NULL ) {
                if( eEventType == CSourceCode_ProcProc ) {
                    pIgnoreEvents->RemoveAll();
                    //// RHF COM Mar 23, 2000RemoveProc( csProcName );
                }
                else
                    pIgnoreEvents->SetAt( eEventType, eEventType);
            }
            break;
        }
    } // End Scanning Looop


    return( bFound ? p : NULL );
}

bool CSourceCode::AddEvent(const CString& csText, CSourceCode_EventType eEventType,
                           const CString& csProcName, int iEventLine )
{
    bool    bRet=true;
    CSourceCode_EventType eEventTypeAux = eEventType;
    CStringArray    aLinesCode;
    CEventCode*     pEventCode=NULL;

    StringToArray( csText, &aLinesCode );


    pEventCode = GetEvent( csProcName, eEventTypeAux );
    if( pEventCode == NULL ) {
        pEventCode = new CEventCode;

        pEventCode->SetInfo( csProcName, eEventTypeAux, iEventLine );
        m_aEventCode.Add( pEventCode );
    }

    pEventCode->AddLines( &aLinesCode );

    return bRet;
}

void CSourceCode::SetOrder(const std::vector<CString>& proc_names)
{
    m_aProcOrder.RemoveAll();

    int iProcNum = 0;

    for( CString proc_name : proc_names )
    {
        proc_name.MakeUpper();
        m_aProcOrder.SetAt(proc_name, iProcNum + 1);
        ++iProcNum;
    }
}


std::map<std::wstring, int> CSourceCode::GetProcLineNumberMap() const
{
    std::map<std::wstring, int> proc_line_number_map;

    for( int i = 0; i < m_aEventCode.GetSize(); ++i )
    {
        const CEventCode* pEventCode = m_aEventCode[i];

        if( pEventCode->GetEventType() == CSourceCode_ProcProc )
            proc_line_number_map.try_emplace(CS2WS(pEventCode->GetSymbolName()), pEventCode->GetEventLine());
    }

    return proc_line_number_map;
}


// savy, sept 8, 2000
bool CSourceCode::IsProcAvailable(const CString& csSymbolName, CSourceCode_EventType eEventType /* = CSourceCode_AllEvents*/)
{
    bool                    bRet=false;
    int                     iNumEventProc=m_aEventCode.GetSize();
    int                     iNumLines;
    CEventCode*             pEventCode;
    CString                 csLine;
    CString                 csCurrentSymbolName;
    CString                 csOldSymbolName=_T("");
    CSourceCode_EventType   eCurrentEventType;

    //Save the Proc to a buffer
    for( int i = 0; i < iNumEventProc; i++ ) {
        pEventCode = m_aEventCode[i];
        if( ( iNumLines = pEventCode->GetNumLines() ) == 0 )
            continue;

        csCurrentSymbolName = pEventCode->GetSymbolName();
        eCurrentEventType = pEventCode->GetEventType();

        // Match Symbol
        if( csSymbolName.GetLength() > 0 &&
            csCurrentSymbolName.CompareNoCase( csSymbolName ) != 0 )
            continue;

        // Match Event
        if( eEventType != CSourceCode_AllEvents &&
            eCurrentEventType != eEventType )
            continue;

        if(iNumLines > 0){
            bRet = true;
            break;
        }
    }

    return bRet;
}


bool CSourceCode::IsCompilingGlobal() // 20101206
{
    int             iNumEventProc=m_aEventCode.GetSize();
    CEventCode*     pEventCode;

    bool foundZero = false;

    for( int i = 0; i < iNumEventProc; i++ )
    {
        pEventCode = m_aEventCode[i];

        if( pEventCode->GetEventLine() == 0 )
        {
            if( foundZero )
                return false;

            foundZero = true;
        }
    }

    return true;
}


bool CSourceCode::IsOnlyThisProcPresent(const CStringArray& aProcLines, const CString& csSymbolName) // 20120613
{
    CString csAllLines;
    ArrayToString(&aProcLines, csAllLines, true);

    const TCHAR* pszText = csAllLines; // code modified from CSourceCode::Load

    CString csProcName;
    CSourceCode_EventType eType;

    int numProcs = 0;

    while( pszText != nullptr && *pszText != 0 && ( pszText = ScanEvent(pszText, eType, csProcName) ) != nullptr )
    {
        if( eType == CSourceCode_ProcProc )
        {
            numProcs++;

            if( csSymbolName.CompareNoCase(csProcName) )
                return false;

            else if( numProcs > 1 )
                return false;
        }

        pszText++;
    }

    return numProcs == 1;
}
