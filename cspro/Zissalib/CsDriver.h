#pragma once

//---------------------------------------------------------------------------
//  File name: CsDriver.h
//
//  Description:
//          Header for the 3-D driver
//
//  History:    Date       Author   Comment
//              ---------------------------
//              16 Jan 01   vc      Created for final expansion to 3 dimensions
//              30 Jan 01   vc      Finishing basic features, including occurrences updating
//              27 Apr 01   vc      Enhancements following preliminary run tests
//              10 May 01   vc      Makeup of basic functionality, introducing hands-shaking controls
//              21 May 01   vc      Adding DeFld' version of current object (HANDSHAKING3D),
//                                  plus methods for managing field values and colors
//              28 May 01   vc      Adding progress-bitmap management
//              30 May 01   vc      Adding support for level' processing
//              07 Jun 01   vc      Adding more controls on Level-tails, improving relationship with RunCase
//              14 Jun 01   RHF+vc  Splitting constructor in two parts (constant and reentrant data),
//                                  refining relationship' calculation
//              18 Jun 01   RHF     Add evaluation of "same branch"
//              20 Jun 01   RHF     Add trimming of empty occurrences
//              21 Jun 01   vc      New methods to manage pending-advances, dealing with volatile "field color" and "origin" marks, integrating occurrences' trim
//              10 Dec 01   vc      Full revision and remake
//              20 Feb 02   RHF+vc  Final check for 1st delivery
//              12 Mar 02   vc      Activating detection of IdCollision (for case-ids only)
//              25 Mar 02   vc      Add FurnishGroupHead/FurnishFirstField methods, and modify SolveReenter to allow for reenter to GroupHead
//
//---------------------------------------------------------------------------

#include <Zissalib/flowatom.h>
#include <Zissalib/CsKernel.h>
#include <Zissalib/FlowCore.h>
#include <engine/DEFLD.H>
#include <engine/3dException.h>

class CSymbolFlow;
class CEngineDriver;
class CEngineArea;
class CIntDriver;
class CEntryDriver;
class CFlAdmin;
struct EngineData;
class LogicStackSaver;

namespace Paradata
{
    struct FieldMovementTypeInfo;
    class FieldMovementInstance;
    class FieldEntryEvent;
}

//---------------------------------------------------------------------------
//
//  class CsDriver : public CObject
//
//
//  Description:
//      Management of (every/entry) driver
//
//  * Basic model:
//
//      Any CsDriver works based on a FlowCore' strip for one and only one Flow.
//      TODO add description of events, Dynamic/Static bitmaps, ... please JOC/RHF
//
//  Methods:
//
//  Construction/destruction/initialization
//      CsDriver                    Constructor
//      ~CsDriver                   Destructor
//      InitSessionConditions       Initialize reentrant conditions for a session
//
//  Current flow, current object-in-flow, way
//      SetFlAdmin                  Set the FLOW administrator governing this CsDriver
//      GetFlAdmin                  Get the FLOW administrator governing this CsDriver
//      SetFlow                     Set the FLOW and the flow-strip of this CsDriver
//      GetFlow                     Get the FLOW of this CsDriver
//      GetFlowCore                 Get the flow-strip of this CsDriver
//      SetCurObject                Set the current 3-D object in process
//      GetCurObject                Return the current 3-D object in process
//      SetForwardWay               Set the way forward or backward
//      IsForwardWay                True if the current way is forward
//      IsBackwardWay               True if the current way is backward
//      ExtractInfoFromAtom         Makes 3D-CurObject from a given atom, and adjust the occurrences
//      ExtractInfoFromGroupHeadAtom  ... idem, specialized for a GroupHead atom
//      ExtractInfoFromGroupTailAtom  ... idem, specialized for a GroupTail atom
//      ExtractInfoFromHTOccAtom      ... idem, specialized for a HTOcc atom
//      ExtractInfoFromItemAtom       ... idem, specialized for an Item atom
//      SetExtractedObject          Pass the info of the given atom to the bridge, extracted 3D-object
//
//  Operation control
//      DoorConditonIs              Return the current condition of the "door" controlling the execution of level-0' procs
//      SetDoorCondition            Set a given condition on the "door" controlling the execution of level-0' procs
//      ResetDoorCondition          Set a condition "locked" on the "door" controlling the execution of level-0' procs
//      SetSourceOfNodeAdvance      Set the source of the NodeAdvance to a given node-number
//      ResetSourceOfNodeAdvance    Reset the source of NodeAdvance
//      GetSourceOfNodeAdvance      Return the node-number where a NodeAdvance was requested
//      SetTargetNode               Set the target of a NodeAdvance to a given node-number
//      GetTargetNode               Return the node-number of a NodeAdvance
//      GetGroupTRoot               Return the root of the group-tree of the attached flow
//      GetMaxLevel                 Return the maximum level to be reached in the attached flow
//      GetSymbolOfLevel            Return the symbol of a given level
//      GetLevelOfCurObject         Return the level of the current object
//      SetLastItemEntered          Store the atom of the last item entered
//      GetLastItemEntered          Return the atom of the last item entered
//      RefreshGroupOccs            Adjust the occurrences of the group linked to the atom
//      GroupTrimOcc                Remove trailing empty (no colored) occurrences of a given group
//      SetRefreshGroupOccsLimit    Set the refresh-group-occs-limit to a given limit
//      ResetRefreshGroupOccsLimit  Set the refresh-group-occs-limit to "1-up to DataOccs" if PathOff & Modify, to "0-no limit" otherwise
//      GetRefreshGroupOccsLimit    Return the current refresh-group-occs-limit
//      SetLevelTailReached         Set the level-tail-reached mark to a given Level (-1 for none)
//      GetLevelTailReached         Return the Level in the level-tail-reached mark
//      WasLevelTailReached         True if there is a Level in the level-tail-reached mark (false if none)
//      GetNumOfPendingAdvances     Return the number of pending advances
//      EnableNoInput               Set the "noinput" mark to ON
//      NoInputRequested            Return the "noinput" mark AND immediatly set it to OFF
//      SavePendingAdvance          Store the current "target for advance-to" into the list of pending advance' requests
//      GetBestPendingAdvance       From the list of pending advance' requests, choose the "best" (the farther one, indeed), copy it to the "target for advance-to", then clean the list
//      KillPendingAdvances         Erases all pending advances
//      DontRemakeOrigin            Set the "remake origin" mark to OFF
//      RemakeOrigin                Return the "remake origin" mark AND immediatly set it to ON
//      SetEnterFlowStarted         Set the "enter-flow started" mark as requested
//      EnterFlowJustStarted        Return the "enter-flow started" mark AND immediatly set it to OFF
//      SetEnterFlowReturnWay       Set the "enter-flow returned" way to forward or backward as given
//      EnterFlowJustReturned       Return true if the "enter-flow returned" way is marked either backward or forward
//      EnterFlowReturnedForward    Return true if the "enter-flow returned" way is forward (requires a way marked) AND immediatly set the way to "none"
//
//  Requests (and targets) from Interface and Logic
//    (1) validating requests by source
//      BuildRequestValid           Build the validation table for incoming requests
//      IsRequestValid              Check if a given request is valid for the source of the request
//    (2) identification of the request
//      SetInterRequestNature       Set (checking it is possible for interface) the kind of request
//      SetLogicRequestNature       Set (checking it is possible for logic) the kind of request
//      ResetRequest                Forgets any request prior to return to the interface
//      SetRequestSource            Set "request coming from interface/logic"
//      IsRequestFromLogic          True if the request came from logic
//      IsRequestFromInter          True if the request came from interface
//      SetRequestNature            Set the kind of request
//      GetRequestNature            Return the kind of request
//      IsRequestBreakingEventsChain True if the request breaks the events' chain
//      SetRequestOrigin            Set the request-origin to the flow-atom where the the request arised
//      GetRequestOrigin            Return the flow-atom of the request-origin
//      SetRequestEventType         Set the event-type "origin" of the request to a given event-type
//      GetRequestEventType         Return the event-type "origin" of the request
//    (3) requested from Interface and Logic
//      Set3DTarget                 Set a 3-D object as the specific target of the request
//      IsNodeBoundTarget           True if the searched target is a node-bound
//      GetNodeBoundTarget          Whether the searched target is a node-bound, return the symbol of the searched level' group-tail
//      SetNodeBoundTarget          Set the node-bound target flag to a given level-symbol
//    (4) requested from Logic only
//      SetSymTarget                Set a group or var' symbol as the specific target for the request
//    (5) requested from Interface only
//      AtEndOccur_SetEnterField    Set 'EnterField' parameter for an InterEndOccur request
//      AtEndOccur_GetEnterField
//      AtEndGroup_SetEnterField    Set 'EnterField' parameter for an InterEndGroup request
//      AtEndGroup_GetEnterField
//      AtEndLevel_SetParameters    Set all parameters for an InterEndLevel request
//      AtEndLevel_GetEnterField
//      AtEndLevel_GetNextLevel
//      AtEndLevel_GetWriteNode
//    (6) other requests from Interface
//      --- none ---                                    // victor May 10, 01
//
//  Information interchange
//      FurnishCurObject            Places a copy of the current object into a given 3D-object
//      FurnishGroupHead            Prepares a 3D-object with the GroupHead of the current group
//      FurnishFirstField           Prepares a 3D-object with the first field either in the current occurrence or in the current group
//      FurnishFieldAsciiAddr       Return the ascii-address of a given field (described by a 3D-object)
//      FurnishFieldValue           Places a copy of the text-value of a given field (described by a 3D-object) into a given text-area
//      FurnishFieldLight           Places the light of a given field (described by a 3D-object) into a given light-descriptor
//      FurnishPrecedence           Calculates the precedence of two 3D-objects
//
//  Driver front-end
//      DriverBrain                 Solve one request coming from the interface, and every request arising from the logic before return to the interface
//      CheckImplicitActions        Take care of killing or retrieving pending advances, and of protected fields
//      SearchingTargetNode         True if searching for a target-node as requested in a NodeAdvance
//      GotoLevelZeroTail           Solve the special request of immediatly ending the session
//      RequestHasValidParms        Checks the request has valid parameters
//      IsValidSkipTarget           Check if the target is valid for an skip' request
//      IsValidAdvanceTarget        Check if the target is valid for an advance' request
//      IsValidReenterTarget        Check if the target is valid for an reenter' request
//      InvalidTargetMessage        Display a given message explaining why a target is invalid for a request
//
//  Methods solving requests
//  (a) requested from Interface and Logic
//      SolveSkipOrAdvanceTo        Move to the target of SkipTo or AdvanceTo requests
//      SolveReenterTo              Move to the target of a Reenter request
//  (b) requested from Logic only
//      SolveAdvanceToNext          Advance to the next occurrence of the current field
//      SolveSkipToNext             Skip to the next occurrence of the current field
//      SolveLogicEndGroup          Perform an end-of-group according to the rules of the logic
//      SolveLogicEndLevel          Perform an end-of-level according to the rules of the logic
//  (c) requested from Interface only
//      SolveEnterFlowEnd           Restores the environment once Entifaz has finished another CsDriver to solve an Enter-flow operation
//      SolveNextField              Move to the atom of the nearest field forward
//      SolvePrevField              Move to the atom of the nearest field backwards (or to an exact reenter' target)
//      SolveInterEndOccur          Perform an end-of-occur according to the parameters provided by the interface
//      SolveInterEndGroup          Perform an end-of-group according to the parameters provided by the interface
//      SolveInterEndLevel          Perform an end-of-level according to the parameters provided by the interface
//  (d) other requests from Interface
//      --- none ---                                    // victor May 10, 01
//
//  Basic support to solve requests
//      AskForNextField             Set an "internal" request for next field
//      ReachNextField              Move to the atom of the nearest field forward, and leave it ready for the interface
//      ReachNextNodeAtLevel        * searches a Level-node at a given level
//      LocateNextAtom              Looks for the next atom in the flow-strip
//      LocatePrevAtom              Looks for the previous atom in the flow-strip
//
//  Calling the interpreter
//      RunEventProcs               The only method in charge to drive the execution of every procs for the current-atom.
//                                  Calculate the condition of the atom (related to the origin flow-atom and the 3D-target).
//                                  Retrieve both the static and the dynamic bitmaps and combine them,
//                                  prepares an array of procs to be executed, and calls the interpreter
//                                  for each one, taking care of the requests issued.
//      FinishLevel                 Manages to complete a level' node processing, including closing a session
//
//  Support for level' processing
//      LevelPrologue               Performs preliminary tasks for a Level-node before running the procs for its groupHead-atom
//      LevelOpenNode               ???
//      LevelEpilogue               Performs ending tasks for a Level-node after running the procs  for its groupTail-atom
//      LevelCloseNode              ???
//      LevelRestart                For a given Level, goes straight to the groupHead-atom of the level, wipes colors, and cleans the data
//      LevelCleanData              For a given Level, clean data areas and ALL occurrences in that Level and descendant-levels also
//      LevelCleanColors            For a given Level, wipes out the color of all item-atoms in that Level and descendant-levels also
//
//  Generic evaluation and searching methods
//      SameBranch                  Return true if a given observed-object is in the same branch of a given reference-object
//      CanDecrementOcc             Return true if a given group-occurrence is empty
//      FindActualOccs              Scan backward the occurrences of a group starting at a given HTOcc (Last or Tail) atom and returns the effective number of occurrences entered
//      GetTargetLocation( C3DObject* p3DTarget, int iSearchWay ) { // victor Dec 10, 01
//      SearchTargetLocation        Compares the target with a reference atom (the current atom by default) and return
//                                   0 ... are the same (or invalid target, or not found in this flow)
//                                   1 ... target is located after the reference atom
//                                  -1 ... target is located before the reference atom
//      SearchFieldWithValue        Starting at a given atom, searchs backward for the nearest field with color and non-blank ascii contents and return a 3D-object with its description
//      SearchFieldHighColor        Starting at the current or previous atom, searchs backward for the nearest "high color" field and return a 3D-object with its description
//      SearchFieldWithColor        Starting at the current or previous atom, searchs backward for the nearest "low or high color" field and return a 3D-object with its description
//      SearchFieldByColor          Starting at a given atom, searchs backward for the nearest field colored as requested and return a 3D-object with its description
//      SearchFieldPrevToGroup      Starting at the current atom, searchs backward for the first field preceeding the group and return a 3D-object with its description
//      SearchFieldNextToGroup      Starting at the current atom, searchs forward for the first field following the group and return a 3D-object with its description
//
//  Managing field values and colors (of fields given by an item-atom or a 3D-object
//      AcceptFieldValue            For numeric fields, check if "keyed value" is in ranges (and let the operator take a decission if suitable)
//      SomeIdCollision             For primary flow, check if the case-key already exists
//      SetFieldColorLow            Set the color of a field to "low"
//      SetFieldColorMid            Set the color of a field to "mid"
//      SetFieldColorHigh           Set the color of a field to "high"
//      SetFieldColorNone           Wipes out the color of a field
//      GetFieldFromAtom            Get a field from a given atom and translates its indexes to engine
//      GetFieldFrom3D              Get a field from a given 3D-object and translates its indexes to engine
//
//  Adjusting bitmaps according to procedures attached to the atom
//      AdjustBitmap                Adjusts a given bitmap either for a Group or an Item
//      AdjustGroupBitmap           Void bits corresponding to events without proc for a given Group
//      AdjustItemBitmap            Void bits corresponding to events without proc for a given Var
//
//  Static bitmaps for flow-strip atoms
//      GetStaticBitmap             Build and return the static bitmap for the current way and the current-atom in the flow-strip
//      BuildPresetStaticBitmaps    Build the (either forward or backward) static bitmaps, storing the values into the giving array
//      SeedStaticBitmap            Set all bits of a given byte to a static pattern, depending on the way and a given atom-type
//
//  Dynamic bitmaps for flow-strip atoms
//      GetDynamicBitmap            Build and return the dynamic bitmap for the current way and the current-atom (and for the calculated condition) in the flow-strip
//      GetDynamicBitmapIndex       Get the index of a dynamic bitmap for a given couple of RequestNature/AtomType
//      EvaluateCondition           Calculate a condition-index for the current-atom based on a given couple PathOn/Off & request-nature
//      EvaluateRelationship        Get a simplified relationship between the current-object and a given reference-object
//      EvaluateParenthood          Calculate the relationship of an observed-object against a reference-object
//      BuildPresetDynamicBitmaps   Build all bitmaps for each of possible dynamic bitmaps, storing the values into the corresponding array
//      SeedDynamicBitmap           Set all bytes of a given bitmap to a dynamic pattern for a given request, depending on PathOn/Off behavior and a given atom-type
//
//  Progress bitmap (event-procs execution)
//      IsProgressBeforeInterface   True if the progress' bitmap is before the Interface' event
//      IsProgressBeforePreProc     True if the progress' bitmap is before the PreProc' event
//      IsProgressBeforeOnFocus     True if the progress' bitmap is before the OnFocus' event
//      GetProgressBitmap           Return the progress' bitmap
//      SetProgressBitmapOffUpTo    Set the progress' bitmap off up to a given bit-number
//      SetProgressBitmapOnFrom     Set the progress' bitmap on starting at a given bit-number
//      SetProgressBitmapOff        Set a given bit-number of the progress' bitmap to off
//      SetProgressBitmapOn         Set a given bit-number of the progress' bitmap to on (the whole progress' bitmap by default)
//
//  Elementary bitmap operations
//      SetupBitMasks               Initialize definitions for bit operations
//      ResetBitmap                 Set the whole bitmap to zero (false)
//      SetBitOfBitmap              Set one bit of a given byte to on or off (target bit-index given as an integer)
//      GetBitOfEventBitmap         Get one bit of a given bitmap (requested bit-index given as an event-type)
//      SetBitOfEventBitmap         Set one bit of a given bitmap to on (target bit-index given as an event-type)
//      ResetBitOfEventBitmap       Set one bit of a given bitmap to off (target bit-index given as an event-type)
//
//  Saving/restoring environment for Enter-flow actions
//      SaveEnvironmentInfo         Saves current values of the environment for Enter-flow actions
//      RestoreEnvironmentInfo      Restores saved values of the environment for Enter-flow actions
//
//  Interacting with the flow-strip
//      GetCurrAtom                 Return the current atom in the flow-strip
//      GetNextAtom                 Return the next atom in the flow-strip
//      GetPrevAtom                 Return the previous atom in the flow-strip
//      GetAtomAt                   Return the atom at given index in the flow-strip
//      GetCurrAtomLevel            Return the level of the current atom in the flow-strip
//      GetCurrAtomIndex            Return the index of the current atom in the flow-strip
//      SetCurrAtomIndex            Set the index of the current atom in the flow-strip to a given atom number
//      RestartFlow                 Restart the flow-strip
//
//  Link to engine
//      SetEngineDriver             Installs engine environment
//
//  TRANSITION-TO-3D
//      SetCurDeFld                 Pass current 3D-object to the equivalent m_CurDeFld (only if a Var)
//      GetCurDeFld                 Return the (equivalent of the current 3D-object) DeFld' pointer, in versions 3D or mono index (hard-coded)
//      PassTo3D                    Pass a given couple symbol/occurrence to a given 3D-object
//      PassFrom3DToDeFld           Pass a given 3D-object to an equivalent, given pDeFld (only if a Var)
//      PassFrom3D                  Pass a given 3D-object to a given couple symbol/occurrence
//
//  Operators
//      <none>
//
//---------------------------------------------------------------------------

class CsDriver : public CObject {
    // - definitions for bit operations (restricted to 1-byte only)

    typedef _TUCHAR byte;

    //
    //         +-----+-----------------+-----------------+
    //         | bit |  --BitMaskOn--  |  --BitMaskOff-- |
    //         |-----+-----------------+-----------------|
    //         |  0  |  1000 0000 128  |  0111 1111 127  |
    //         |  1  |  0100 0000  64  |  1011 1111 191  |
    //         |  2  |  0010 0000  32  |  1101 1111 223  |
    //         |  3  |  0001 0000  16  |  1110 1111 239  |
    //         |  4  |  0000 1000   8  |  1111 0111 247  |
    //         |  5  |  0000 0100   4  |  1111 1011 251  |
    //         |  6  |  0000 0010   2  |  1111 1101 253  |
    //         |  7  |  0000 0001   1  |  1111 1110 254  |
    //         +-----+-----------------+-----------------+
public:
    enum    EventType {
                EventPreProc     = 0,
                EventOnFocus     = 1,
                EventInterface   = 2,
                EventRefresh     = 3,
                EventKillFocus   = 4,
                EventPostProc    = 5,
                EventOnOccChange = 6,
                EventNone        = 7
    };
public:
    enum    DoorCondition {             // controlling execution of level-0' procs
                Locked           = 0,   // ... nothing executed
                Open             = 1,   // ... Pre-0 already executed
                Closed           = 2    // ... Post-0 already executed (or must not be executed)
    };
private:
    byte        m_aBitMaskOn[8];        // 128| 64| 32| 16|  8|  4|  2|  1
    byte        m_aBitMaskOff[8];       // 127|191|223|239|247|251|253|254
    byte        m_cFullBitmap;          // constant full-bitmap, all 1's


    // - RequestNature
public:
    enum    RequestNature {             //   Inter
                                        //   face    Logic  Description
                // (a) from Interface and Logic
                AdvanceTo        = 0,   //     Y       Y    advance to a given field or group
                SkipTo,                 //     Y       Y    skip to a given field or group
                Reenter,                //     Y       Y    reenter a given field or group

                // (b) from Logic only
                AdvanceToNext,          //     N       Y    advance to the next occurrence of a given field
                SkipToNext,             //     N       Y    skip to the next occurrence of a given field
                LogicEndGroup,          //     N       Y    end the current group following the standard behavior
                LogicEndLevel,          //     N       Y    end the current level following the standard behavior
                EnterFlowEnd,           //     N       Y    restore after another flow was fully entered

                // (c) from Interface only
                NextField,              //     Y       N    provide next field (natural' next can be modified by the logic)
                PrevField,              //     Y       N    provide previous field
                InterEndOccur,          //     Y       N    end the current occurrence following a behavior provided by the interface
                InterEndGroup,          //     Y       N    end the current group following a behavior provided by the interface
                InterEndLevel,          //     Y       N    end the current level following a behavior provided by the interface

                // (d) other requests from Interface
                //      --- none ---                    // victor May 10, 01

                None                    //     N       N    ---
    };

    // RHF INIC Oct 17, 2002
    bool    BackRequest( RequestNature xNature ) {
        return ( xNature == Reenter || xNature == PrevField );
    }
    // RHF END Oct 17, 2002

// --- Internal objects ----------------------------------------------------

class RequestValid {
private:
    RequestNature   m_xRequestNature;   // identification of the request
    bool            m_bInterValid;
    bool            m_bLogicValid;

public:
    RequestValid( RequestNature xRequestNature, bool bInterValid, bool bLogicValid ) {
                    m_xRequestNature = xRequestNature;
                    m_bInterValid    = bInterValid;
                    m_bLogicValid    = bLogicValid;
    }
    ~RequestValid( void ) {}
    RequestNature   GetNature( void )                         { return m_xRequestNature; }
    bool            GetInterValid( void )                     { return m_bInterValid; }
    bool            GetLogicValid( void )                     { return m_bLogicValid; }
};

class EndOccurParms {
private:
    bool            m_bEnterField;      // the interface' field-contents will be "entered" or not

public:
    EndOccurParms( void )                                     { m_bEnterField = false; }
    ~EndOccurParms( void ) {}
    void            SetEnterField( bool bX )                  { m_bEnterField = bX; }
    bool            GetEnterField( void )                     { return m_bEnterField; }
};

class EndGroupParms {
private:
    bool            m_bEnterField;      // the interface' field-contents will be "entered" or not

public:
    EndGroupParms( void )                                     { m_bEnterField = false; }
    ~EndGroupParms( void ) {}
    void            SetEnterField( bool bX )                  { m_bEnterField = bX; }
    bool            GetEnterField( void )                     { return m_bEnterField; }
};

class EndLevelParms {                   // version with same old parms // victor Mar 05, 02
private:
    bool            m_bEnterField;      // the interface' field-contents will be "entered" or not
    int             m_iNextLevel;       // the next level to be accessed
    bool            m_bWriteNode;       // whether the node will be written or not

public:
    EndLevelParms( void )                                     { SetParameters( false, -1, false ); }
    ~EndLevelParms( void ) {}
    void            SetParameters( bool bEnterField, int iNextLevel, bool bWriteNode ) {
                    m_bEnterField = bEnterField;
                    m_iNextLevel  = iNextLevel;
                    m_bWriteNode  = bWriteNode;
    }
    bool            GetEnterField( void )                     { return m_bEnterField; }
    int             GetNextLevel( void )                      { return m_iNextLevel; }
    bool            GetWriteNode( void )                      { return m_bWriteNode; }
};

// BUCEN_2003 changes
// RHF INIC Dec 30, 2003
class SkipParms {
private:
    bool            m_bEnterField;      // the interface' field-contents will be "entered" or not
    bool            m_bFromInterface;

public:
    SkipParms( void )                                     { SetParameters( false, false ); }
    ~SkipParms( void ) {}
    void            SetParameters( bool bEnterField, bool bFromInterface ) {
        m_bEnterField = bEnterField;
        m_bFromInterface = bFromInterface;
    }
    bool            GetEnterField( void )                     { return m_bEnterField; }
    bool            GetFromInterface( void )                  { return m_bFromInterface; }
};
// RHF END Dec 30, 2003
// BUCEN_2003 changes

// --- Data members --------------------------------------------------------

    // --- current flow, current object-in-flow, way
private:
    CFlAdmin*       m_pFlAdmin;         // the Flow-administrator
    CSymbolFlow*    m_pFlow;            // current FLOW
    CFlowCore*      m_pFlowCore;        // FlowCore attached to the current FLOW
    C3DObject       m_CurObject;        // current object-in-flow
    bool            m_bForwardWay;      // false when going backward
    C3DObject       m_ExtractedObject;  // object-in-flow extracted from a given atom by ExtractInfoFromAtom

    // --- operation control
    DoorCondition   m_eDoorCondition;   // controlling execution of level-0' procs
    int             m_iSourceOfNodeAdvance; // the node-number (in primary flow) at the source of a NodeAdvance // victor Feb 23, 02
    CFlowAtom*      m_pLastItemEntered; // the most recent item-atom entered in the flow
    int             m_iRefreshGroupOccsLimit; // the refresh-group-occs-limit: 0-no limit, 1-up to DataOccs, 2-don't refresh
    int             m_iLevelTailReached;// the Level of the Level' GroupTail reached (-1 if not this situation)
    bool            m_bNoInputField;    // avoids interface ONLY ONCE// victor Dec 10, 01
    std::vector<C3DObject*> m_aPendingAdvances; // pending advance' requests // victor Jun 21, 01
    bool            m_bRemakeOrigin;    // "internal" requests       // victor Jun 21, 01
    bool            m_bEnterStarted;    // true when Enter-flow just launched
    int             m_iEnterReturned;   // 1: forward, -1: backward, 0: no
    int             m_iTargetNode;      // -1 for "no target-node"   // victor Dec 10, 01

    // --- requests (and targets) from Interface and Logic
private:
    // (1) validating requests by source
    std::vector<RequestValid*> m_aRequestValid;    // request validation array
    // (2) identification of the request
    bool            m_bRequestFromLogic;// false if coming from the interface
    RequestNature   m_xRequestNature;   // identification of the request

    bool            m_bRequestBreaking; // true if the request breaks the chain of event-procs
    CFlowAtom*      m_pRequestOrigin;   // flow-atom "origin" of the request
    int             m_iRequestOriginIndex;// flow-atom index "origin" of the request
    EventType       m_pRequestEventType;// event-type "origin" of the request
    // (3) requested from Interface and Logic
    int             m_iSymNodeBound;    // target is a node-bound // victor Mar 07, 02
    C3DObject       m_TgtAdvanceTo;     // target for advance-to
    C3DObject       m_TgtSkipTo;        // target for skip-to
    C3DObject       m_TgtReenter;       // target for reenter
    // (4) requested from Logic only
    int             m_iTgtSymFlow;      // target for enter-flow
    int             m_iTgtAdvanceToNext;// target for advance-to-next (an iSymVar)
    int             m_iTgtSkipToNext;   // target for skip-to-next (an iSymVar)
    // (5) requested from Interface only
    EndOccurParms   m_EndOccurParms;    // behavior requested to end the current occurrence
    EndGroupParms   m_EndGroupParms;    // behavior requested to end the current group
    EndLevelParms   m_EndLevelParms;    // behavior requested to manage the EndLevel
    SkipParms       m_SkipParms;        // behavior requested to manage the skip // RHF Dec 30, 2003
    // (6) other requests from Interface
    //  --- none ---                                    // victor May 10, 01

    // --- miscellaneous constants
    int             m_iNumRequestNature;// RequestNature::None
    int             m_iNumAtomTypes;    // CFlowAtom::AtomType::BeyondStrip
    int             m_iNumConditions;   // 10 in each AtomType
    int             m_iDynamicBitmapSize;

    // --- static bitmaps for flow-strip atoms
private:
    byte            m_cStaticForward;
    byte            m_cStaticBackward;
    // ... preset forward & backwards static bitmaps
    byte            m_aStaticForward[(int)CFlowAtom::AtomType::BeyondStrip];
    byte            m_aStaticBackward[(int)CFlowAtom::AtomType::BeyondStrip];
    // --- dynamic bitmaps for flow-strip atoms         // victor May 10, 01
private:
    byte*           m_aDynamicPathOn;
    byte*           m_aDynamicPathOff;

    // --- progress bitmap (event-procs execution)      // victor May 28, 01
private:
    byte            m_cProgress;                        // victor May 28, 01

    // --- saving/restoring environment for Enter-flow actions
private:
                    // info saved to allow later returning to this CsDriver
    int*            m_Progbase;         // program partition starting address
    int             m_Prognext;         // current next sentence to be executed
    int             m_ProgType;         // proc being executed: pre/post
    int             m_ExLevel;          //                    : level
    int             m_ExSymbol;         //                    : isym

    // --- engine links
public:
    CEngineDriver*  m_pEngineDriver;
    CEngineArea*    m_pEngineArea;
    CIntDriver*     m_pIntDriver;
    CEntryDriver*   m_pEntryDriver;
    EngineData*     m_engineData;

    // --- flags
private:
    bool            m_bSolveInterEndLevel;
    bool            m_bKillFocus;
    bool            m_bMidColor;

    // --- TRANSITION-TO-3D
public:
    DEFLD3          m_CurDeFld;         // to be filled by SetCurDeFld

    // --- paradata
private:
    std::shared_ptr<Paradata::FieldMovementTypeInfo> CreateFieldMovementType(RequestNature request_nature);
    std::shared_ptr<Paradata::FieldMovementInstance> m_currentFieldMovementInstance;
    std::shared_ptr<Paradata::FieldMovementInstance> m_currentFieldMovementInstanceForValidationEvent;
    std::shared_ptr<Paradata::FieldEntryEvent> m_currentFieldEntryEvent;


// --- Methods -------------------------------------------------------------
    // --- construction/destruction/initialization
public:
    CsDriver( void );
    ~CsDriver( void );
    void            InitSessionConditions( void );

    // --- current flow, current object-in-flow, way
public:
    void            SetFlAdmin( CFlAdmin* pFlAdmin );   // victor Aug 02, 01
    CFlAdmin*       GetFlAdmin( void );                 // victor Aug 02, 01
    void            SetFlow( CSymbolFlow* pFlow );
private:
    CFlowCore*      GetFlowCore( void )                       { return m_pFlowCore; }
    CSymbolFlow*    GetFlow( void )                           { return m_pFlow; }
    void            SetCurObject( C3DObject* p3DObject );
    C3DObject*      GetCurObject( void )                      { return &m_CurObject; }
    void            SetForwardWay( bool bForward )            { m_bForwardWay = bForward; }
    bool            IsForwardWay( void )                      { return m_bForwardWay; }
    bool            IsBackwardWay( void )                     { return !m_bForwardWay; }
    C3DObject*      ExtractInfoFromAtom( CFlowAtom* pAtom );
    void            ExtractInfoFromGroupHeadAtom( CFlowAtom* pAtom );
    void            ExtractInfoFromGroupTailAtom( CFlowAtom* pAtom );
    void            ExtractInfoFromHTOccAtom( CFlowAtom* pAtom );
    void            ExtractInfoFromBlockHeadAtom(CFlowAtom* pAtom);
    void            ExtractInfoFromBlockTailAtom(CFlowAtom* pAtom);
    void            ExtractInfoFromItemAtom( CFlowAtom* pAtom );

    void setExtractedObject( C3DObject& other );
    void setExtractedObject( int iSymbol );
    void setExtractedObject( int iSymbol, C3DIndexes& the_Indexes );
    void setExtractedObject( int iSymbol, int* pIndex );

    // --- operation control
public:
    DoorCondition   DoorConditionValue( void )                { return m_eDoorCondition; }
    bool            DoorConditionIs( DoorCondition d )        { return m_eDoorCondition == d; }
    bool            SetDoorCondition( DoorCondition xNewCondition );
    void            ResetDoorCondition( void )
                    {
                        m_eDoorCondition = Locked;
                    }
    void            SetSourceOfNodeAdvance( void );                                              // victor Feb 23, 02
    void            ResetSourceOfNodeAdvance( void )          { m_iSourceOfNodeAdvance = -1; }   // victor Feb 23, 02
    int             GetSourceOfNodeAdvance( void )            { return m_iSourceOfNodeAdvance; } // victor Feb 23, 02
    void            SetTargetNode( int iNode );                                         // victor Dec 10, 01
private:
    int             GetTargetNode( void ) const               { return m_iTargetNode; } // victor Dec 10, 01
public:
    GROUPT*         GetGroupTRoot( void );              // victor Mar 07, 02
    int             GetMaxLevel( void );                // victor Mar 07, 02
    int             GetSymbolOfLevel( int iLevel );     // victor Mar 07, 02
    int             GetLevelOfCurObject( void );        // victor Mar 07, 02
private:
    void            SetLastItemEntered( CFlowAtom* pAtom )    { m_pLastItemEntered = pAtom; }
    CFlowAtom*      GetLastItemEntered( void )                { return m_pLastItemEntered; }
    void            RefreshGroupOccs( int iSymGroup, CNDIndexes& theIndex, int iKindUse );
    int             GroupTrimOcc( bool bRestore = true );       // victor Dec 10, 01
    void            TrimAllOcc( int iEndingLevel );             // RHF Sep 30, 2004
public:                                 // now public           // victor Mar 26, 02
    void            SetRefreshGroupOccsLimit( int iLimit = 0 ){ m_iRefreshGroupOccsLimit = ( iLimit >= 0 && iLimit <= 2 ? iLimit : 0 ); }
    void            ResetRefreshGroupOccsLimit( void );
private:
    int             GetRefreshGroupOccsLimit( void )          { return m_iRefreshGroupOccsLimit; }
    void            SetLevelTailReached( int iLevel = -1 )    { m_iLevelTailReached = iLevel; }       // victor May 07, 01
    int             GetLevelTailReached( void )               { return m_iLevelTailReached; }         // victor May 07, 01
    bool            WasLevelTailReached( void )               { return( m_iLevelTailReached >= 0 ); } // victor May 07, 01
public:
    int             GetNumOfPendingAdvances( void )           { return (int)m_aPendingAdvances.size(); }// victor Aug 08, 01

    // RHF INIC Feb 13, 2003
    C3DObject*      GetPendingAdvance( int i )                { return m_aPendingAdvances[i]; }
    void            AddPendingAdvance(C3DObject* p3DObject );
    void            CopyPendingAdvance(CsDriver* pCsDriver );
    // RHF END Feb 13, 2003

    void            EnableNoInput( void )                     { m_bNoInputField = true; }           // victor Dec 10, 01
private:
    bool            NoInputRequested( void ) {                                                      // victor Dec 10, 01
                        bool    bNoInputField = m_bNoInputField;

                        m_bNoInputField = false; // is turned off immediatly

                        return bNoInputField;
    }
    void            SavePendingAdvance( void );                                                     // victor Jun 21, 01
    bool            GetBestPendingAdvance();                                                        // victor Jun 21, 01
    void            KillPendingAdvances( void );                                                    // victor Jun 21, 01
    void            DontRemakeOrigin( void )                  { m_bRemakeOrigin = false; }          // victor Jun 21, 01
public:
    bool            RemakeOrigin( void ) {                                                          // victor Jun 21, 01
                        bool    bMustRemake = m_bRemakeOrigin;
                        m_bRemakeOrigin = true; // is turned on immediatly
                        return bMustRemake;
    }

    void            SetEnterFlowStarted( bool bStarted )      { m_bEnterStarted = bStarted; }
    bool            EnterFlowJustStarted( void )              { return m_bEnterStarted; }
    void            SetEnterFlowReturnWay( int iReturnWay ) {
                        ASSERT( !m_bEnterStarted );
                        ASSERT( iReturnWay == -1 || iReturnWay == 1 );
                        ASSERT( m_iEnterReturned == 0 );
                        m_iEnterReturned = iReturnWay;
    }
    bool            EnterFlowJustReturned( void )             { return( m_iEnterReturned != 0 ); }
    bool            EnterFlowReturnedForward( void ) {
                        ASSERT( !m_bEnterStarted );
                        ASSERT( m_iEnterReturned != 0 );
                        bool    bForward = ( m_iEnterReturned > 0 );

                        m_iEnterReturned = 0;     // is set to "none" immediatly

                        return bForward;
    }

    // --- requests (and targets) from Interface and Logic
    // (1) validating requests by source
private:
    void            BuildRequestValid( void );
public:
    bool            IsRequestValid( void );
    // (2) identification of the request
public:
    bool            SetInterRequestNature( RequestNature xNature, bool bResetAdvance=false );// RHF Oct 17, 2002 Add bResetAdvance
    bool            SetLogicRequestNature( RequestNature xNature, bool bResetAdvance=false );// RHF Oct 17, 2002 Add bResetAdvance
    void            ResetRequest();
    void            ResetRequests(RequestNature xFirstRequestNature = RequestNature::AdvanceTo, RequestNature xLastRequestNature = RequestNature::None);

    RequestNature   GetRequestNature( void )                  { return m_xRequestNature; }

private:
    void            SetRequestSource( bool bFromLogic )       { m_bRequestFromLogic = bFromLogic; }
    bool            IsRequestFromLogic( void )                { return m_bRequestFromLogic; }
    bool            IsRequestFromInter( void )                { return !m_bRequestFromLogic; }
    void            SetRequestNature( RequestNature xNature );

    bool            IsRequestBreakingEventsChain( void )      { return m_bRequestBreaking; }
    void            SetRequestOrigin( void );
    CFlowAtom*      GetRequestOrigin( void )                  { return m_pRequestOrigin; }
    void            SetRequestEventType( EventType xEvent )   { m_pRequestEventType = xEvent; }
    EventType       GetRequestEventType( void )               { return m_pRequestEventType; }
    // (3) requested from Interface and Logic
public:
    bool            Set3DTarget( C3DObject* p3DObject );
private:
    bool            IsNodeBoundTarget( void )                 { return( m_iSymNodeBound > 0 ); }                       // victor Mar 07, 02
    int             GetNodeBoundTarget( void )                { return m_iSymNodeBound; }                              // victor Mar 07, 02
    void            SetNodeBoundTarget( int iSymLevel = 0 )   { m_iSymNodeBound = ( iSymLevel > 0 ) ? iSymLevel : 0; } // victor Mar 07, 02
    // (4) requested from Logic only
public:
    bool            SetSymTarget( int iSymbol = 0 );
    // (5) requested from Interface only
public:
    void            AtEndOccur_SetEnterField( bool bX )       { m_EndOccurParms.SetEnterField( bX ); }
    bool            AtEndOccur_GetEnterField( void )          { return m_EndOccurParms.GetEnterField(); }
    void            AtEndGroup_SetEnterField( bool bX )       { m_EndGroupParms.SetEnterField( bX ); }
    bool            AtEndGroup_GetEnterField( void )          { return m_EndGroupParms.GetEnterField(); }
    void            AtEndLevel_SetParameters( bool bEnterField, int iNextLevel, bool bWriteNode ) {
                    m_EndLevelParms.SetParameters( bEnterField, iNextLevel, bWriteNode );
    }
    bool            AtEndLevel_GetEnterField( void )          { return m_EndLevelParms.GetEnterField(); }
    int             AtEndLevel_GetNextLevel( void )           { return m_EndLevelParms.GetNextLevel(); }
    bool            AtEndLevel_GetWriteNode( void )           { return m_EndLevelParms.GetWriteNode(); }


    // RHF INIC Dec 30, 2003
    void            AtSkip_SetParameters( bool bEnterField, bool bFromInterface ) {
        m_SkipParms.SetParameters( bEnterField, bFromInterface );
    }
    bool            AtSkip_GetEnterField( void )          { return m_SkipParms.GetEnterField(); }
    bool            AtSkip_GetFromInterface( void )       { return m_SkipParms.GetFromInterface(); }
    // RHF END Dec 30, 2003

    // (6) other requests from Interface
public:
    //  --- none ---                                    // victor May 10, 01

    // --- information interchange
public:
    bool            FurnishCurObject( C3DObject* p3DObject );
    bool            FurnishGroupHead( C3DObject* p3DObject );                           // victor Mar 25, 02
    bool            FurnishFirstField( C3DObject* p3DObject, bool bInTheOccur = true ); // victor Mar 25, 02
    csprochar*      FurnishFieldAsciiAddr( C3DObject* p3DObject ); // RHF Jun 21, 2001
    bool            FurnishFieldValue( C3DObject* p3DObject, csprochar* pszValueText );
    bool            FurnishFieldLight( C3DObject* p3DObject, int* pFieldLight );
    int             FurnishPrecedence( C3DObject* p3DTarget, C3DObject* p3DSource = NULL );

    // --- driver front-end
public:
    C3DObject*      DriverBrain( void );

private:
    //---------------------------------------------------------------
    // A lot of common code from several methods
    // - CheckImplicitActions()
    // - SolveReenterTo()
    // - SolvePrevField()
    // - ReachNextField()
    //
    // refactorized in
    //    checkEndGroup() and checkEndGroupEndLevel() methods
    //
    // RCL, Sept 24, 2004
    //----------------------------------------------------------------

    /*
     * void checkEndGroup()
     *
     * checks if current position is an end group.
     * It is important to check before calling this method
     * that current atom is an Item type one
     * Answer is written in *pbEndGroup
     */
    void checkEndGroup( bool bIsModification, bool bPathOff,
                        GROUPT* pGroupT,
                        bool* pbEndGroup );

    /*
     * void checkEndGroupEndLevel()
     *
     * checks if current position is an end group and End Level
     * It is important to check before calling this method
     * that current atom is an Item type one
     * Answer is written in *pbEndGroup and *pbEndLevelOcc
     */
    void checkEndGroupEndLevel(
                    bool bIsModification, bool bPathOff,
                    GROUPT* pGroupT,
                    bool* pbEndGroup, bool* pbEndLevelOcc );

    /*
     * bool isCurrentPositionEndGroup( GROUPT* pGroupT )
     *
     * Only valid to be called when current atom is an item
     * called from checkEndGroup() and checkEndGroupEndLevel() methods
     */
    bool isCurrentPositionEndGroup( GROUPT* pGroupT );

    /*
     * bool isCurrentPositionEndLevel()
     *
     * Only valid to be called when current atom is an item
     * called from checkEndGroupEndLevel() method
     */
    bool isCurrentPositionEndLevel();

    /*
     * int getGroupMaxOccsUsingMaxDEField( GROUPT* pGroupT )
     *
     * Calculate max occs considering controller variable, if it has one
     * called from isCurrentPositionEndGroup() method
     */
    int getGroupMaxOccsUsingMaxDEField( GROUPT* pGroupT );

public:
    bool            CheckImplicitActions( bool bNewRequestReceived ); // RHF+vc Jun 21, 01
    bool            SearchingTargetNode( void );        // victor Dec 10, 01
    void            GotoLevelZeroTail( void );          // RHF+vc Jun 14, 01
private:
    bool            RequestHasValidParms( void );

    //////////////////////////////////////////////////////////////////////////
    // MessageIdException is an instrumental class, used in
    //   -   IsValidSkipTarget()
    //   -   IsValidAdvanceTarget()
    //   -   IsValidReenterTarget()
    // These three methods were very similar, so I created a generic
    // IsValidTargetGeneric() method that does the common tasks, and the methods
    // mentioned above just configure the IsValidTargetGeneric() method behaviour.
    // If there is something wrong (not valid target) a MessageIdException is thrown,
    // which is captured in each method
    //
    // rcl, Jul 7, 2004
    DEFINE_EXCEPTION_CLASS( MessageIdException, GenericNumberedException );
    //////////////////////////////////////////////////////////////////////////

    void            IsValidTargetGeneric(C3DObject* p3DTarget,bool bSilent,int iFlags); // rcl, Jul 7, 2004

public:
    bool            IsValidSkipTarget(bool bSilent = false, C3DObject* p3DTarget = nullptr);    // victor Dec 10, 01
    bool            IsValidAdvanceTarget(bool bSilent = false, C3DObject* p3DTarget = nullptr); // victor Dec 10, 01
    bool            IsValidReenterTarget(bool bAllowReenterToNoLight = false, bool bSilent = false, C3DObject* p3DTarget = nullptr); // victor Dec 10, 01
private:
    void            InvalidTargetMessage( int iMessage, C3DObject* p3DTarget );  // victor Dec 10, 01

    // --- methods solving requests
private:
    // (a) requested from Interface and Logic
    bool            SolveSkipOrAdvanceTo( void );
    bool            SolveReenterTo( void );
    // (b) requested from Logic only
    bool            SolveAdvanceToNext( void );         // victor May 10, 01
    bool            SolveSkipToNext( void );            // victor May 10, 01
    bool            SolveLogicEndGroup( void );         // victor May 10, 01
    bool            SolveLogicEndLevel( void );         // victor May 10, 01
    // (c) requested from Interface only
    bool            SolveEnterFlowEnd( void );          // victor Jul 25, 01
    bool            SolveNextField( bool bEnterTheField );
    bool            SolvePrevField( bool bStopAtReenterTarget = false, bool bIgnoreProtected=false ); // enhanced // victor Dec 10, 01
    bool            SolveInterEndOccur( void );         // victor May 10, 01
    bool            SolveInterEndGroup( void );         // victor May 10, 01
    bool            SolveInterEndLevel( void );         // victor May 10, 01
    // (d) other requests from Interface
    //  --- none ---                                    // victor May 10, 01

    // --- basic support to solve requests
private:
    bool            AskForNextField( bool bEnableAfterInterface = true );
    // ReachNextField: looks for the next item-atom in order to give it to the Interface
    bool            ReachNextField( void );
    void            ReachNextNodeAtLevel( int iTargetLevel );
    bool            LocateNextAtom( void );

    // Current behaviour prevents getting the previous atom
    // when the current atom is the first field, but we need to get
    // the previous one when the target is a group [form/roster]
    // so we will open the door when bSpecialCase is true
    // JOC, May 2005
    bool            LocatePrevAtom( bool bSpecialCase = false );

    // --- calling the interpreter
private:
    bool            RunEventProcs( int iProcsSubset = 0, bool bEnterTheField = false );
    bool            FinishLevel( int iEndingLevel, bool bRequestIssued ); // victor Feb 23, 02

    // --- support for level' processing                // victor May 30, 01
private:
    bool            LevelPrologue( int iLevel );
    bool            LevelOpenNode( int iOpeningLevel, bool bResetItAlso );
    bool            LevelEpilogue( int iLevel, bool bRequestPosted );
    bool            LevelCloseNode( int iLevel, bool bIgnoreWrite );
    bool            LevelRestart( int iLevel );
    bool            LevelCleanData( int iLevel );
    bool            LevelCleanColors( int iLevel );

    // --- generic evaluation and searching methods
public:
    bool            SameBranch( C3DObject* p3DObsObject, C3DObject* p3DRefObject); // RHF Jun 18, 2001
    bool            CanDecrementOcc( C3DObject* p3DSource, bool bPathOn ); // RHF Jun 20, 2001
    int             FindActualOccs( const C3DObject& the3DReference );
    //Savy -to optimize skipto target computation
    int             SearchTargetLocation(C3DObject* p3DTarget, int iRefAtom = -1, int iSearchWay = 0, bool bFillTargetAtomIndex = false ); // victor Dec 10, 01
    int             SearchTargetAtomIndex( C3DObject* p3DTarget, int iPivotAtom, int iWay );                  // victor Dec 10, 01
    C3DObject*      SearchFieldHighColor( bool bFromPrevAtom = true ) { return SearchFieldByColor( bFromPrevAtom, true, false ); } // victor Jun 21, 01
    C3DObject*      SearchFieldWithColor( bool bFromPrevAtom = true ) { return SearchFieldByColor( bFromPrevAtom, true, true );  } // victor Jun 21, 01
    C3DObject*      SearchFieldWithValue( bool bFromPrevAtom );       // victor Jun 21, 01
    C3DObject*      SearchFieldByColor( bool bFromPrevAtom, bool bHigh, bool bMid ); // victor Jun 21, 01
    C3DObject*      SearchFieldPrevToGroup( int iAtLevel = 0 );       // victor Jun 21, 01
    C3DObject*      SearchFieldNextToGroup( int iAtLevel = 0 );       // victor Jun 21, 01

    // --- managing field values and colors (of fields given by an item-atom)
private:
    bool            AcceptFieldValue( CFlowAtom* pAtom );             // victor May 21, 01
    bool            SomeIdCollision( void );                          // victor Mar 12, 02
    int             GetFieldColor( CFlowAtom* pAtom );                // victor Jun 14, 01
    void            SetFieldColorLow( CFlowAtom* pAtom );             // victor May 21, 01
    void            SetFieldColorMid( CFlowAtom* pAtom );             // victor Jun 14, 01
    void            SetFieldColorHigh( CFlowAtom* pAtom );            // victor May 21, 01
    void            SetFieldColorNone( CFlowAtom* pAtom );            // victor May 21, 01
    VARX*           GetFieldFromAtom( CFlowAtom* pAtom, CNDIndexes& theIndex );    // rcl, Jun 21, 04
    VARX*           GetFieldFrom3D( C3DObject* p3DObject, CNDIndexes& theaIndex ); // rcl, Jun 21, 04

    // --- adjusting bitmaps according to procedures attached to the atom
private:
    bool            AdjustBitmap( byte* pByte, CFlowAtom* pAtom );
    void            AdjustGroupBitmap( byte* pByte, int iSymGroup );
    void            AdjustItemBitmap( byte* pByte, int iSymVar );

    // --- static bitmaps for flow-strip atoms
private:
    byte            GetStaticBitmap( void );
    void            BuildPresetStaticBitmaps( bool bForward );
    bool            SeedStaticBitmap( byte* pByte, bool bForward, CFlowAtom::AtomType xAtomType );

    // --- dynamic bitmaps for flow-strip atoms
private:
    byte            GetDynamicBitmap( bool bIsLevelHeadOrTail );
    int             GetDynamicBitmapIndex( int iRequestNature, int iAtomType ) { return ( iRequestNature * m_iNumAtomTypes + iAtomType ) * m_iNumConditions; }
    int             EvaluateCondition( bool bPathOn, RequestNature xRequestNature );
    int             EvaluateRelationship( C3DObject* p3DRefObject );
    int             EvaluateParenthood( C3DObject* p3DCurObject, C3DObject* p3DRefObject );
    void            BuildPresetDynamicBitmaps( bool bPathOn );
    bool            SeedDynamicBitmaps( byte* pBitmap, bool bPathOn, RequestNature xRequestNature, CFlowAtom::AtomType xAtomType );

    // --- progress bitmap (event-procs execution)      // victor May 28, 01
public:
    bool            IsProgressBeforeInterface( void );  // victor Aug 08, 01
    void            SetProgressForPreEntrySkip(); // GHM 20130415
private:
    bool            IsProgressBeforePreProc( void );    // victor Aug 08, 01
    bool            IsProgressBeforeOnFocus( void );    // victor Aug 08, 01
    byte            GetProgressBitmap( void )                 { return m_cProgress; }
    void            SetProgressBitmapOffUpTo( int iBitNumber ) {
                        for( int iBit = 0; iBit <= iBitNumber; iBit++ )
                            SetProgressBitmapOff( iBit );
    }
    void            SetProgressBitmapOnFrom( int iBitNumber ) {
                        for( int iBit = iBitNumber; iBit <= EventNone; iBit++ )
                            SetProgressBitmapOn( iBit );
    }
    void            SetProgressBitmapOff( int iBitNumber )    { SetBitOfBitmap( &m_cProgress, iBitNumber, false ); }
    void            SetProgressBitmapOn( int iBitNumber = -1 ) {
                        if( iBitNumber < 0 )
                            m_cProgress = m_cFullBitmap;
                        else
                            SetBitOfBitmap( &m_cProgress, iBitNumber, true );
    }

    // --- elementary bitmap operations
private:
    void            SetupBitMasks( void );
    void            SetEventBitmap( byte* pByte, bool bEventPreProc, bool bEventOnFocus, bool bEventInterface, bool bEventRefresh, bool bEventKillFocus, bool bEventPostProc, bool bEventOnOccChange );
    void            ResetBitmap( byte* pByte )                { *pByte = 0; }
    void            SetBitOfBitmap( byte* pByte, int iBitNumber, bool bSetOn ) {
                        if( bSetOn )
                            (*pByte) |= m_aBitMaskOn[iBitNumber];
                        else
                            (*pByte) &= m_aBitMaskOff[iBitNumber];
    }
    bool            GetBitOfEventBitmap( byte* pByte, int iBitNumber )   { return( ( (*pByte) & m_aBitMaskOn[iBitNumber] ) > 0 ); };
    void            SetBitOfEventBitmap( byte* pByte, int iBitNumber )   { SetBitOfBitmap( pByte, iBitNumber, true );  }
    void            ResetBitOfEventBitmap( byte* pByte, int iBitNumber ) { SetBitOfBitmap( pByte, iBitNumber, false ); }

    // --- saving/restoring environment for Enter-flow actions // victor Jul 25, 01
public:
    void            SaveEnvironmentInfo( void );
    void            RestoreEnvironmentInfo( void );

    // --- managing the enter flow stack
private:
    std::unique_ptr<LogicStackSaver> m_enterFlowLogicStack;
public:
    void SetEnterFlowLogicStack(const LogicStackSaver& logic_stack_saver);
    void ClearEnterFlowLogicStack();
    bool RunEnterFlowLogicStack();

    // --- interacting with the flow-strip              // victor Dec 10, 01
private:
    CFlowAtom*      GetCurrAtom( void );
    CFlowAtom*      GetNextAtom( void );
    CFlowAtom*      GetPrevAtom( void );
    CFlowAtom*      GetAtomAt( int iAtom );             // victor Dec 10, 01
public:
    int             GetCurrAtomIndex( void );
    int             GetCurrAtomLevel( void );           // victor Feb 23, 02
private:
    void            SetCurrAtomIndex( int iAtom );
    void            RestartFlow( void );

    // --- engine links
public:
    void            SetEngineDriver( CEngineDriver* pEngineDriver, CEntryDriver* pEntryDriver );

    // --- TRANSITION-TO-3D
private:
    void            SetCurDeFld( bool bFullDim );       // victor may 21, 01
    void            SetReenterTarget( VART* pVarT ); // RHF Jan 15, 2003 Fix protected id bug
public:
    DEFLD3*         GetCurDeFld( void );                // victor may 21, 01
    bool            PassLowestFieldToCurObject( void ); // victor Dec 10, 01
    static void     PassTo3D( C3DObject* p3DObject, const Symbol* pSymbol, const UserIndexesArray& theArray );
    void            PassTo3D( C3DObject* p3DObject, int iSymbol, const UserIndexesArray& theArray );
    void            PassTo3D( C3DObject* p3DObject, int iSymbol, int iOccur = 0 );
    void            PassFrom3DToDeFld( C3DObject* p3DObject, DEFLD3* pDeFld, bool bFullDim ); // victor Jun 14, 01
    void            PassFrom3D( C3DObject* p3DObject, int* pSymbol, int* pOccur = NULL, int iDefaultSingleOcc = 0 );

    bool            ReportToInterface( CFlowAtom* pAtom, int iDirection ); // RHF Mar 05, 2002

    //SAVY for auto endgroup/endlevel in pathoff mode
    bool C_IsAutoEndGroup();
    void ProcessSequentialFld(VART* pVarT);
    void C_FldPutVal( VART* pVarT, csprochar* pszNewVarvalue );
    CString FldGetVal(const CDictItem* pItem);

    // --- constant definitions
    #define CsDriverTopEvent EventType::EventOnOccChange

private:
    const Logic::SymbolTable& GetSymbolTable() const;
};
