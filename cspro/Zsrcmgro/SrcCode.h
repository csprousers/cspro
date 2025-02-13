#pragma once

// SourceCode.h: interface for:
//                CProcEvent
//                CProcEventArray
//                CEventCode
//                CSourceCode
//////////////////////////////////////////////////////////////////////

#include <Zsrcmgro/zSrcMgrO.h>
#include <zAppO/Application.h>


enum CSourceCode_EventType
{
    CSourceCode_AllEvents,
    CSourceCode_ProcProc, // RHF Mar 21, 2000
    CSourceCode_NoProc,
    CSourceCode_PreProc,
    CSourceCode_OnFocus, CSourceCode_OnOccChange, CSourceCode_KillFocus,
    CSourceCode_Tally, CSourceCode_PostCalc,
    CSourceCode_PostProc
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CLASS NAME   : CProcEvent
//
// PROTOTYPE    : class CProcEvent
//
// OBJECTIVE    : Class for keeping a IMSA Language event
//
// REMARKS      : none.
//
// CHANGES      : 13 Jul 1999, RHF, Creation
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CProcEvent
{
public:
    CProcEvent(std::wstring event_name, CSourceCode_EventType eEventType)
        :   m_eventName(std::move(event_name)),
            m_eEventType(eEventType)
    {
    }

    const std::wstring& GetEventName() const   { return m_eventName; }
    CSourceCode_EventType GetEventType() const { return m_eEventType; }

private:
    std::wstring m_eventName;
    CSourceCode_EventType m_eEventType;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CLASS NAME   : CProcEventArray
//
// PROTOTYPE    : class CProcEventArray
//
// OBJECTIVE    : Class for keeping a list of IMSA Language events
//
// REMARKS      : none.
//
// CHANGES      : 13 Jul 1999, RHF, Creation
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: CProcEventArray::GetEvent
//
// PROTOTYPE    : bool GetEvent( const int iEventNum,
//                               CProcEvent& cProcEvent ) const {
//
// OBJECTIVE    : Obtain a event.
//
// PARAMETERS   : iEventNum indicating the event number (0 based)
//                cProcEvent where the corresponding event will be obtained.
//
// RETURN       : TRUE if everything was OK. FALSE if not.
//
// REMARKS      : none.
//
// CHANGES      : 13 Jul 1999, RHF, Creation
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: CProcEventArray::GetEventNum
//
// PROTOTYPE    : int GetEventNum( const CSourceCode_EventType eEventType,
//                                 CProcEvent& cProcEvent ) const;
//                int GetEventNum( const CString csEventName,
//                                 CProcEvent& cProcEvent ) const;
//
// OBJECTIVE    : Obtain a event number using eEventype (or csEventName) and
//                the corresponding event.
//
// PARAMETERS   : eEventType indicating the event type (or csEventName indicating
//                the event name.
//                cProcEvent where the corresponding event will be obtained.
//
// RETURN       : 0 to n indicating the event number or -1 if the event was not
//                found
//
// REMARKS      : none.
//
// CHANGES      : 13 Jul 1999, RHF, Creation
////////////////////////////////////////////////////////////////////////////////
class CProcEventArray
{
public:
    CProcEventArray();

    const std::vector<CProcEvent>& GetProcEvents() const { return m_procEvents; }

    const std::wstring& GetEventName(CSourceCode_EventType eEventType) const;

private:
    std::vector<CProcEvent> m_procEvents;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CLASS NAME   : CEventCode
//
// PROTOTYPE    : class CEventCode
//
// OBJECTIVE    : Class for keeping a piece of a IMSA code (procedure). All IMSA
//                code has a "symbol name" (m_csSymbolName ), "Event Type"
//                (m_aEventType) and the code (m_aCode). The Event Type is the
//                list of events that appear on CProcEventArray class.
//
// REMARKS      : none.
//
// CHANGES      : 13 Jul 1999, RHF, Creation
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CEventCode
{
private:
    CString                 m_csSymbolName;
    CSourceCode_EventType   m_eEventType;
    int                     m_iEventLine; // Line relative to the beginning of the text
    CStringArray            m_aCode;

public:
    CEventCode() {
        m_eEventType = CSourceCode_NoProc;
        m_iEventLine = -1;
    }

    void SetInfo( const CString& csSymbolName, CSourceCode_EventType eEventType, int iEventLine ) {
        m_csSymbolName = csSymbolName;
        m_eEventType = eEventType;
        m_iEventLine = iEventLine;
    }

    int AddLine( const CString& csLine ) {
        m_aCode.Add( csLine );
        return( m_aCode.GetSize() );
    }

    int AddLines( CStringArray* paLines ) {
        m_aCode.InsertAt( m_aCode.GetSize(), paLines );
        return( m_aCode.GetSize() );
    }

    CString& GetLine( int iLine ) {
        return( m_aCode[iLine] );
    }

    int GetNumLines() {
        return( m_aCode.GetSize() );
    }

    void RemoveAll() {
        m_aCode.RemoveAll();
    }

    const CString& GetSymbolName() const       { return m_csSymbolName; }
    CSourceCode_EventType GetEventType() const { return m_eEventType; }
    int GetEventLine() const                   { return m_iEventLine; }
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CLASS NAME   : CSourceCode
//
// PROTOTYPE    : class CLASS_DECL_ZSRCMGR CSourceCode
//
// OBJECTIVE    : Class for manipulating IMSA code (procedures).
//
// REMARKS      : There is a asumption about the order of the procedures. The
//                first procedure written to disk is the application procedure.
//
// CHANGES      : 13 Jul 1999, RHF, Creation
//                24 Mar 2000, Remake the scanner to support comment between diferent events.
//                             There is a new event: PROC. This event can keep information
//                             like comments.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: CSourceCode::IsProcLine
//
// PROTOTYPE    : bool IsProcLine( const CString csLine,
//                                 CString& csSymbol, bool* bHasMoreInfo=null );
//
// OBJECTIVE    : Check if csLine is a line of procedure ( "PROC xxxx" ) and
//                obtain the corresponding symbol
//
// PARAMETERS   : csLine the source string to check.
//                csSymbol where the corresponding symbol will be obtained.
//                bHasMoreInfo (is used when the IsProcLine return true) is filled in true
//                if there is more information after the symbol name
//
// RETURN       : true if csLine begins with CSOURCECODE_PROC. Additionally
//                obtain the procedure name in csSymbol
//
// REMARKS      : none.
//
// CHANGES      : 13 Jul 1999, RHF, Creation
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: CSourceCode::GetEventProcList
//
// PROTOTYPE    : int GetEventProcList( const CString csSymbolName,
//                                      CArray <CEventCode*,CEventCode*>& aEventProcList );
//
// OBJECTIVE    : Obtain a list of event procedures associated to csSymbolName.
//
// PARAMETERS   : csLine the source string to check.
//                csSymbolName where the corresponding symbol will be obtained.
//
// RETURN       : true if csLine begins with CSOURCECODE_PROC. Additionally
//                obtain the procedure name in csSymbol
//
// REMARKS      : none.
//
// CHANGES      : 13 Jul 1999, RHF, Creation
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: CSourceCode::SearchEvent
//
// PROTOTYPE    : CEventCode* SearchEvent(const CArray <CEventCode*,CEventCode*>
//                                        &aEventProcList,
//                         const CSourceCode_EventType eEventType ) const;
//
// OBJECTIVE    : Search a certain event in a list.
//
// PARAMETERS   : aEvenProcList: a list a EventCode associated to a symbol.
//                eEventType: the event to search.
//
// RETURN       : A Pointer to CEventCode if the event eEventType was found.
//                NULL if the event was not found.
//
// REMARKS      : none.
//
// CHANGES      : 13 Jul 1999, RHF, Creation
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: CSourceCode::CanAddEventProc
//
// PROTOTYPE    : bool CanAddEventProc(const CString csSymbolName,
//                                const CSourceCode_EventType eEventType,
//                                CEventCode*& pEventCode, bool bCheck );
//
// OBJECTIVE    : Check is a certain event of a symbol already exist.
//
// PARAMETERS   : csSymbolName: a symbol name to check.
//                eEventType: the event associated.
//                pEventCode: a CEventCode element that will obtain the respective
//                event code if CanAddEventProc return true.
//                bCheck: indicates if consistence checking will be done
//
// RETURN       : true if is posible add a eEventType procedure to csSymbolName.
//                If the event procedure already exist, in pEventCode is returned
//                in order to the calling function remove the lines of the existing
//                procedure.
//                false if is not posible.
//
// REMARKS      : none.
//
// CHANGES      : 13 Jul 1999, RHF, Creation
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: CSourceCode::GetProc
//
// PROTOTYPE    :  bool GetProc( CStringArray& aProcLines,
//                   const CString csSymbolName="",
//                   const CSourceCode_EventType eEvenType=CSourceCode_AllEvents);
//
// OBJECTIVE    : Obtain a event procedure, a symbol procedure or all the
//                program (from the internal memory buffer).
//
// PARAMETERS   : aProcLines: where the respective program code will be obtained.
//                csSymbolName: the symbol name
//                eEventType: the event type.
//
// RETURN       : true if some procedure is obtained.
//                -If eEventype is used, only this event is obtained "excluding"
//                "PROC" and event keyword.
//                -If eEventype is not used, all the events procedure of
//                csSymbolName are obtained ("including" PROC and events keywords).
//                -If csSymbolName is not provided (and eEvenType neither) all the
//                procedures are obtained regardless the symbol name ("including"
//                PROC and events keywords).
//                false if no procedure is obtained.
//
// REMARKS      : A typicall use is call it after of calling to Load function.
//
// CHANGES      : 13 Jul 1999, RHF, Creation
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: CSourceCode::PutProc
//
// PROTOTYPE    : bool    PutProc( const CStringArray& aProcLines );
//
//                bool    PutProc( const CStringArray& aProcLines,
//                                 const CString csSymbolName,
//                                 CSourceCode_EventType eEvenType );
//
// OBJECTIVE    : Save a event procedure to internal memory buffer.
//
// PARAMETERS   : aProcLines: containing the code to be saved.
//                csSymbolName: the symbol name
//                eEventType: the event type.
//
// RETURN       : true if there is no errors.
//                -If eEventype and csSymbolName are used, aProcLines must
//                contain the corresponding event procedure ("excluding" PROC
//                and event keywords)
//                -else, aProcLines contains a full procedure or list of them
//                ("including" PROC and events keywords).
//                false if there are errors. The errors was produced because
//                some event can't be added to internal buffers or there is
//                incompatibility (for example: a PreProc was defined in the
//                application procedure)
//
// REMARKS      : none
//
// CHANGES      : 13 Jul 1999, RHF, Creation
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: CSourceCode::RemoveProc
//
// PROTOTYPE    : int RemoveProc( const CString csSymbolName,
//                               const CSourceCode_EventType eEventType=CSourceCode_AllEvents );
//
// OBJECTIVE    : Remove the source code of a Event.
//
// PARAMETERS   : csSymbolName: the symbol name
//                eEventType: the event type to delete.
//
// RETURN       : Number of events deleted
//
// REMARKS      : none
//
// CHANGES      : 21 Oct 1999, RHF, Creation
////////////////////////////////////////////////////////////////////////////////

class CLASS_DECL_ZSRCMGR CSourceCode
{
private:
    const Application& m_application;
    std::shared_ptr<TextSource> m_logicTextSource;
    bool m_bModifiedFlag;

    CProcEventArray m_cProcEventArray;
    CMap<CString, LPCTSTR, int, int> m_aProcOrder;
    CArray <CEventCode*, CEventCode*> m_aEventCode;

    int GetEventProcList( const CString& csSymbolName, CArray <CEventCode*,CEventCode*>& aEventProcList );

    CEventCode* SearchEvent( const CArray <CEventCode*,CEventCode*>& aEventProcList, CSourceCode_EventType eEventType ) const;

    void SortProc();

private:
    // Obtain the name of the event eEvenType

    bool Load(CString csAllLines, bool bIgnoreProcProc);
    bool Load(const CStringArray& aProcLines, bool bIgnoreProcProc);


    const TCHAR* ScanEvent(const TCHAR* pszBuf, CSourceCode_EventType& eEventType,
                           CString& csProcName,
                           CMap<CSourceCode_EventType,CSourceCode_EventType,CSourceCode_EventType,CSourceCode_EventType>*  pIgnoreEvents=NULL,
                           int *iEventLine=NULL ); // RHF Mar 23, 2000
    bool    AddEvent( const CString& csText,  CSourceCode_EventType eType, const CString& csProcName, int iEventLine );// RHF Mar 22, 2000

    void CopyWithoutBlankLines(const CStringArray& aProcLines, CStringArray& aProcLinesExtended );
    void InsertProcLine(const CStringArray& aProcLines, CStringArray& aProcLinesExtended, CString csSymbolName );

    bool IsEventLine(const CString& line, const std::wstring& event_name);
    void InsertEventLine(const CStringArray& aProcLines, CStringArray& aProcLinesExtended, CSourceCode_EventType eEventType );


public:
    CSourceCode(Application& application);
    virtual ~CSourceCode();

    CEventCode* GetEvent( const CString& csSymbolName, CSourceCode_EventType eEventType );

    static int ArrayToString( const CStringArray* paLinesCode, CString& csAllLines, bool bAddNewLine );// RHF Mar 22, 2000
    static int StringToArray( const CString& csAllLines, CStringArray* paLinesCode ); // RHF Mar 22, 2000
    bool IsProcLine( const CString& csLine, CString& csSymbol, bool* bHasMoreInfo = NULL ) const;   // RHF Mar 23, 2000

    // Skip comments, string literals, and whitespace.
    const TCHAR* SkipSpecialZone(const TCHAR* buffer) const;

    void SetModifiedFlag(bool bFlag) { m_bModifiedFlag = bFlag; }
    bool IsModified() const          { return m_bModifiedFlag; }

    void SetOrder(const std::vector<CString>& proc_names); // RHF Mar 28, 2000

    // MEMORY BASED FUNCTIONS.
    // GetProc("") Load all the aplication (include PROC and prefixs)
    // GetProc( "Symbol" ); Load the complete procedure (include PROC and prefixs)
    // GetProc ("Symbol", "PROC/PreProc/PostProc/PreSkipProc/PostSkipProc" ); Load the
    // respective event include prefixs
    bool  GetProc( CStringArray& aProcLines, const CString csSymbolName=_T(""),
                   CSourceCode_EventType eEvenType=CSourceCode_AllEvents);

    int   RemoveProc( const CString csSymbolName,
                      CSourceCode_EventType eEventType=CSourceCode_AllEvents );

    bool  PutProc( const CStringArray& aProcLines );

    // if we use PutProc( Buffer, "Symbol", "PROC/PreProc/PostProc/PreSkipProc/PostSkipProc" )
    // aProcLines must contain the respective prefixs
    bool PutProc( const CStringArray& aProcLines, const CString& csSymbolName, CSourceCode_EventType eEventType );

    // Load logic from and save logic to the text source
    bool Load();
    bool Save();


    // MISCELLANEOUS FUNCTIONS

    // Get List of Procedure Names
    int GetProcNames( CStringArray& aProcNames );

    std::map<std::wstring, int> GetProcLineNumberMap() const;

    //SAVY Code for speeding up code availability check
    bool IsProcAvailable(const CString& csSymbolName, CSourceCode_EventType eEventType = CSourceCode_AllEvents);

    bool IsCompilingGlobal(); // GHM 20101206

    bool IsOnlyThisProcPresent(const CStringArray& aProcLines, const CString& csSymbolName); // GHM 20120613
};
