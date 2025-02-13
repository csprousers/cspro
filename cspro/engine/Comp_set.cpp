//---------------------------------------------------------------------------
//
//  COMPSET.cpp   syntax analyzer
//
//---------------------------------------------------------------------------
#include "StandardSystemIncludes.h"
#include "Exappl.h"
#include "COMPILAD.H"


#define MAX_SETSYMBOLS  100

//---------------------------------------------------------------------------//
// ci_set       : compile instruction SET (all flavors)                      //
//                                                                           //
//      1) create ACCESS_NODE                                                //
//          SET  access( Dic ) ASCEND | DESCEND                              //
//          SET  first( Dic )                                                //
//          SET  last( Dic )                                                 //
//                                                                           //
//      2) create SETOTHER_NODE                                              //
//          SET  OUTPUT   "XXXXXXXX" F1                                      //
//          SET  HEADING  "XXXXXXXX" FHEAD                                   //
//          SET  LINEPAGE "XXXXXXXX" Number                                  //
//          SET  FORMAT  US/SP                                               //
//                                                                           //
//---------------------------------------------------------------------------//

std::optional<int> CEngineCompFunc::ci_set()
{
    // some set commands are processed in other functions
    size_t keyword_type = NextKeyword(
        {
            _T("access"),   // 1
            _T("first"),    // 2
            _T("last"),     // 3
            _T("trace"),    // 4
            _T("impute"),   // 5
        });

    switch( keyword_type )
    {
        case 1:
            return CompileSetAccessFirstLast(SetAction::Access);

        case 2:
            return CompileSetAccessFirstLast(SetAction::First);

        case 3:
            return CompileSetAccessFirstLast(SetAction::Last);

        case 4:
            CompileSetTrace();
            return std::nullopt;

        case 5:
            CompileSetImpute();
            return std::nullopt;
    }

    bool    bGetNextToken = true;
    int     iSymVar = 0;
    int     iAttrMax = MAX_SETSYMBOLS;
    int     iAttrNumber = 0;
    int     aAttrSymbols[MAX_SETSYMBOLS+1];

    int     iAttrType = 0;
    int     iCapiMode = 0;

    ACCESS_NODE*    pset_ac;
    SETOTHER_NODE*  pset_ot;
    SET_ATTR_NODE*  pset_at;

    int     iIptSet = Prognext;

    int     size_acce = sizeof(ACCESS_NODE);

    // assume ACCESS node to make an easier compile
    bool ascending = true;

    pset_ac = (ACCESS_NODE*) (PPT(Prognext));
    if( Flagcomp ) {
        OC_CreateCompilationSpace(size_acce / sizeof(int));
        pset_ac->st_code = SET_CODE;
        pset_ac->next_st = -1;
        pset_ac->idic    = -1;
        pset_ac->ac      = ascending ? 1 : 0;
    }

    pset_ot = (SETOTHER_NODE*) pset_ac;
    pset_at = (SET_ATTR_NODE*) pset_ac;

    const std::vector<const TCHAR*> keyword_types =
    {
        _T("OUTPUT"),       //  1 No supported!      
        _T("HEADING"),      //  2 No supported!
        _T("LINEPAGE"),     //  3 No supported!
        _T("FORMAT"),       //  4
        _T("ATTRIBUTES"),   //  5
        _T("FILE"),         //  6
        _T("PATH"),         //  7
        _T("BEHAVIOR"),     //  8
        _T("LANGUAGE"),     //  9 RHF Nov 08, 2002
        _T("ERRMSG"),       // 10 20100518
    };

    keyword_type = NextKeyword(keyword_types);

    switch( keyword_type )
    {
        case  0: // unknown SET command
        {
            IssueError(7006);
            break;
        }

                 // removed SET commands
        case  1: // set OUTPUT
        case  2: // set HEADING "XX" FHEAD or set HEADING "XX"
        case  3: // set LINEPAGE
        case  4: // set FORMAT
        case  6: // set FILE
        case  7: // set PATH
        case  9: // set LANGUAGE
        {
            IssueError(95012, keyword_types[keyword_type - 1]);
            break;
        }


        case 5: // set ATTRIBUTES
        {
            NextToken();
            IssueErrorOnTokenMismatch(TOKLPAREN, 517);

            NextToken();

            while( Tkn != TOKRPAREN )
            {
                bool bIsItem = false;

                // allow change of attributes of fields in forms
                if( iAttrNumber >= iAttrMax )
                    IssueError(7060);        // too many symbols

                switch( Tkn )
                {
                    case TOKDICT_PRE80:
                    case TOKFORM:
                    case TOKGROUP:
                        // no restrictions on form fields    // victor Apr 04, 01
                        aAttrSymbols[iAttrNumber] = Tokstindex;
                        break;

                    case TOKVAR:
                        iSymVar = Tokstindex;

                        if( !NPT(iSymVar)->IsA(SymbolType::Variable) || VPT(iSymVar)->SYMTfrm <= 0 )
                            IssueError(7062); // must have a field

                        aAttrSymbols[iAttrNumber] = iSymVar;
                        bIsItem = true;
                        break;

                    default:
                        IssueError(ERROR_RIGHT_PAREN_EXPECTED); // right paren expected
                        break;
                }

                iAttrNumber += 1;

                NextToken();

                if( Tkn == TOKCOMMA )
                    NextToken();
            }

            IssueErrorOnTokenMismatch(TOKRPAREN, ERROR_RIGHT_PAREN_EXPECTED);

            if( iAttrNumber < 1 )
                IssueError(7064); // no symbol specified

            size_t attribute_type = NextKeyword({
                _T("NATIVE"),
                _T("DISPLAY"),
                _T("AUTOSKIP"), _T("RETURN"), _T("PROTECT"),
                _T("HIDDEN"), _T("VISIBLE"),
                _T("REFRESH"),
                _T("ASSISTED")
            });

            if( attribute_type == 0 )
                IssueError(7063); // missing type

            switch( attribute_type )
            {
                case 1:
                    IssueWarning(Logic::ParserMessage::Type::DeprecationMinor, 95021, _T("false"));
                    iAttrType = SET_AT_NATIVE;
                    break;

                case 2:
                    iAttrType = SET_AT_DISPLAY;
                    break;

                case 3:
                    IssueWarning(Logic::ParserMessage::Type::DeprecationMinor, 95015, _T("UseEnterKey"));
                    iAttrType = SET_AT_AUTOSKIP;
                    break;

                case 4:
                    IssueWarning(Logic::ParserMessage::Type::DeprecationMinor, 95015, _T("UseEnterKey"));
                    iAttrType = SET_AT_RETURN;
                    break;

                case 5:
                    IssueWarning(Logic::ParserMessage::Type::DeprecationMinor, 95021, _T("true"));
                    iAttrType = SET_AT_PROTECT;
                    break;

                case 6:
                    iAttrType = SET_AT_HIDDEN;
                    break;

                case 7:
                    iAttrType = SET_AT_VISIBLE;
                    break;

                case 8: // refresh ON/OFF
                {
                    IssueWarning(Logic::ParserMessage::Type::DeprecationMajor, 95014, _T("attributes/refresh"));

                    size_t refresh_type = NextKeyword({ _T("OFF"), _T("ON") });

                    if( refresh_type == 0 )
                        IssueError(7066);

                    else
                        iAttrType = ( refresh_type == 1 ) ? SET_AT_REF_OFF : SET_AT_REF_ON;

                    break;
                }

                case 9: // ASSISTED ON/OFF
                {
                    IssueWarning(Logic::ParserMessage::Type::DeprecationMajor, 95002);

                    size_t assisted_type = NextKeyword({ _T("OFF"), _T("ON"), _T("TOGGLE") });

                    if( assisted_type == 0 )
                        IssueError(7067);

                    iAttrType = ( assisted_type == 1 ) ? SET_AT_CAP_OFF : ( assisted_type == 2 ) ? SET_AT_CAP_ON  : SET_AT_CAP_TOGGLE;

                    NextToken();

                    if( Tkn == TOKLPAREN )
                    {
                        if( iAttrType == SET_AT_CAP_TOGGLE )
                            IssueError(21); // TOGGLE no acepta '('

                        int nopt = 0;

                        while( Tkn != TOKRPAREN && nopt < 2 )
                        {
                            size_t capi_type = NextKeyword({ _T("QUESTION"), _T("VARIABLE"), _T("RESPONSES") });

                            if( capi_type != 0 )
                            {
                                if( capi_type == 1 )
                                    iCapiMode |= DEPRECATED_CAPI_QUESTION_FLAG;

                                else
                                    iCapiMode |= DEPRECATED_CAPI_VARIABLE_FLAG;

                                NextToken(); // For QUESTION: ),
                                             // For VARIABLE: ),(

                                if( Tkn != TOKRPAREN && Tkn != TOKCOMMA && Tkn != TOKLPAREN )
                                    IssueError(ERROR_RIGHT_PAREN_EXPECTED);

                                if( Tkn == TOKLPAREN )
                                {
                                    while( Tkn != TOKRPAREN )
                                    {
                                        size_t window_type = NextKeyword({ _T("TITLE"), _T("LOCKED"), _T("TOP"), _T("CENTER"), _T("BOTTOM"), _T("LEFT"), _T("RIGHT"), _T("DYNAMIC") });

                                        if( window_type == 0 )
                                            IssueError(7069);

                                        if( window_type > 1 ) // the old options are no longer supported
                                            IssueError(7065);

                                        iCapiMode |= DEPRECATED_CAPI_TITLE_FLAG;

                                        NextToken();   // must be a ')'

                                        if( Tkn != TOKRPAREN && Tkn != TOKCOMMA )
                                            IssueError(ERROR_RIGHT_PAREN_EXPECTED);
                                    }

                                    NextToken();
                                }
                            }

                            else
                            {
                                if( nopt == 0 )
                                    IssueError(7068);

                                else
                                    break;
                            }

                            nopt++;
                        }
                    }

                    else
                        bGetNextToken = false;

                    if( iCapiMode == 0 ) // nothing was specified so apply to both questions and variables
                        iCapiMode = DEPRECATED_CAPI_QUESTION_FLAG | DEPRECATED_CAPI_VARIABLE_FLAG;

                    break;
                }
            }

            if( Flagcomp ) {
                LIST_NODE*  pListNode;
                int         iSizeList= sizeof(LIST_NODE);
                int         iListNode=Prognext;

                if( iAttrNumber >= 1 ) {
                    iSizeList += (iAttrNumber - 1) * sizeof(int);
                }

                pListNode = (LIST_NODE*) (PPT(Prognext));

                OC_CreateCompilationSpace(iSizeList / sizeof(int));

                pListNode->iNumElems = iAttrNumber;
                for( int i = 0; i < iAttrNumber; i++ ) {
                    pListNode->iSym[i] = aAttrSymbols[i];
                }

                pset_at->st_code     = SET_ATTR_CODE;
                pset_at->next_st     = -1;
                pset_at->attr_type   = (short)iAttrType;
                pset_at->iListNode   = iListNode;
                pset_at->m_iCapiMode = iCapiMode;
            }

            break;
        }


        case 8: // set BEHAVIOR
        {
            if( CompSetBehavior(pset_ot) )
                bGetNextToken = false;

            if( GetSyntErr() != 0 )
                return 0;

            break;
        }


        case 10: // set ERRMSG (message overrides)
        {
            NextToken();
            IssueErrorOnTokenMismatch(TOKLPAREN, 517);

            int errorMessageCode = 0;
            int keystrokeCode = 0;

            size_t errmsg_type = NextKeyword({ _T("DEFAULT"), _T("SYSTEM"), _T("OPERATOR") });

            if( errmsg_type == 0 )
                IssueError(7100);

            NextToken();

            // we will just use the ACCESS_NODE instead of creating our own node
            if( Flagcomp )
                OC_CreateCompilationSpace(-1); // we are not using the ac member

            if( errmsg_type == 1 ) // message overrides are being turned off
            {
                errorMessageCode = -1; // -1 means turn the message overrides off
                keystrokeCode = -1;
            }

            else if( errmsg_type == 2 ) // message overrides are being turned to system mode
            {
                errorMessageCode = -2; // -2 means system mode
                keystrokeCode = -2;
            }

            else if( errmsg_type == 3 ) // message overrides are being turned on
            {
                errorMessageCode = -3; // -3 means operator mode without this parameter
                keystrokeCode = -3; // -3 means operator mode without this parameter

                for( int i = 0; i < 2; i++ ) // there can be up to two parameters, in either order
                {
                    if( Tkn == TOKRPAREN ) // no more parameters are being passed
                        break;

                    IssueErrorOnTokenMismatch(TOKCOMMA, 528);

                    NextToken();

                    if( errorMessageCode == -3 && IsCurrentTokenString() ) // the error message is being passed
                    {
                        errorMessageCode = CompileStringExpression();
                    }

                    else if( keystrokeCode == -3 )
                    {
                        keystrokeCode = exprlog();
                    }

                    else
                    {
                        IssueError(1);
                    }
                }
            }

            IssueErrorOnTokenMismatch(TOKRPAREN, ERROR_RIGHT_PAREN_EXPECTED);

            if( Flagcomp )
            {
                pset_ac->st_code = FNMESSAGEOVERRDIES_CODE;
                pset_ac->idic = errorMessageCode;
                pset_ac->iidx = keystrokeCode;
            }

            break;
        }
    }

    if( bGetNextToken )
        NextToken();

    IssueErrorOnTokenMismatch(TOKSEMICOLON, 2); // invalid set, ';' expected

    return iIptSet;
}


bool CEngineCompFunc::CompSetBehavior( SETOTHER_NODE* pset_ot ) { // victor Aug 14, 99
    // the 'set behavior' commands are intended to generate compiled nodes
    // to be interpreted by the executor when they arise in the procedures
    // stream:
    //     set behavior() Path                    off|on
    //     set behavior() CanEnter( NOTAPPL )     off|on( NOCONFIRM | CONFIRM )
    //     set behavior() CanEnter( OUTOFRANGE )  off|on( NOCONFIRM | CONFIRM )
    //     set behavior() Messages( DISPLAY )     off|on
    //     set behavior() Messages( ERRMSG )      off|on
    //     set behavior() Mouse                   off|on    // victor Feb 16, 00
    //     set behavior() PushMode                off|on    // removed
    //     set behavior() CanEndGroup             off|on    // victor Feb 16, 00
    //     set behavior() CanEndLevel             off|on    // victor Feb 16, 00
    //     set behavior() MsgNumber( DISPLAY )    off|on    // victor Jun 08, 00
    //     set behavior() MsgNumber( ERRMSG )     off|on    // victor Jun 08, 00
    //     set behavior() CheckRanges             off|on    // victor Dec 07, 00
    //     set behavior() CanEnter( NOTAPPL )     off( NOCONFIRM | CONFIRM )|on( NOCONFIRM | CONFIRM ) // RHF Sep 13, 2001
    //     set behavior() CanEnter( OUTOFRANGE )  off( NOCONFIRM | CONFIRM )|on( NOCONFIRM | CONFIRM ) // RHF Sep 13, 2001
    //     set behavior() Export( DATA );
    //     set behavior() Export( SPSS|SAS|STATS|ALL|CSPRO|TABDELIM|COMMADELIM|SEMICOLONDELIM
    //                              [ItemOnly|SubItemOnly|ItemSubItem]
    //                          );
    //     set behavior() SkipStruc               off|on    // victor Mar 14, 01
    //     set behavior() exit                  off|on; // RHF Aug 04, 2006
    //     set behavior() SpecialValues( ZERO ) off|on; // 20090827

    // RHF INIC May 10, 2001
    int     iSymVar=0;
    int     iAttrMax = MAX_SETSYMBOLS;
    int     iAttrNumber= 0;
    int     aAttrSymbols[MAX_SETSYMBOLS+1];
    bool    bAllowList = false;
    // RHF END May 10, 2001

    bool    bNextTokenReady = false;
    bool    bSetOn = true;
    bool    bConfirm = true;
    short   iBehaviorItem = 0;
    short   iBehaviorOpt=0;
    short   iBehaviorOpt2 = 0;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, 517);

    // collect symbol names affected by this statement
    NextToken();

    while( Tkn != TOKRPAREN )
    {
        // allow change of attributes of fields in forms
        if( iAttrNumber >= iAttrMax )
            IssueError(91109);        // too many symbols

        switch( Tkn ) {
        case TOKDICT_PRE80:
        case TOKFORM:
        case TOKGROUP:
            aAttrSymbols[iAttrNumber] = Tokstindex;
            break;
        case TOKVAR:
            iSymVar = Tokstindex;
            if( !NPT(iSymVar)->IsA(SymbolType::Variable) || VPT(iSymVar)->SYMTfrm <= 0 )
                IssueError(91107);// must have a field
            aAttrSymbols[iAttrNumber] = iSymVar;
            break;
        default:
            IssueError(ERROR_RIGHT_PAREN_EXPECTED); // right parent expected
            break;
        }

        iAttrNumber++;
        // RHF END Sep 23, 2004

        NextToken();

        if( Tkn == TOKCOMMA )
            NextToken();
    }

    IssueErrorOnTokenMismatch(TOKRPAREN, ERROR_RIGHT_PAREN_EXPECTED);

    // looks for specific behavior item
    size_t behavior_type = NextKeyword({
        _T("PATH"), _T("CANENTER"), _T("MESSAGES"),
        _T("MOUSE"), _T("PUSHMODE"), _T("CANENDGROUP"), _T("CANENDLEVEL"),
        _T("MSGNUMBER"),                           // victor Jun 08, 00
        _T("EXPORT"),                              // victor Dec 18, 00
        _T("CHECKRANGES"),                         // victor Dec 07, 00
        _T("SKIPSTRUC"),                           // victor Mar 14, 01
        _T("SKIPSTRUCTURE"),                       // victor Mar 14, 01
        _T("EXIT"),                                // RHF Aug 04, 2006
        _T("SPECIALVALUES") });                    // 20090827


    if( behavior_type == 0 )
        IssueError(91100); // unrecognized behavior item

    bAllowList = (behavior_type==2); // Only CanEnter need a list (the list can be empty)
    if( !bAllowList && iAttrNumber > 0 )  // Has some element but the command doesn't allow a list
        IssueError(91108);

    bool    bIsExport=(behavior_type==9);


    if( behavior_type == 1 )                    // Path
        iBehaviorItem = BEHAVIOR_PATH;

    else if( behavior_type == 2 || behavior_type == 3 || behavior_type == 14 ) { // canEnter, Messages, SpecialValues

        NextToken();
        if( Tkn != TOKLPAREN )
            IssueError(517);

        switch( behavior_type ) {
        case 2:                     // canEnter
            switch( NextKeyword({ _T("NOTAPPL"), _T("OUTOFRANGE") }) )
            {
                case 0:
                    IssueError(91101);   // NotAppl, OutOfRange expected
                    break;

                case 1:
                    IssueWarning(Logic::ParserMessage::Type::DeprecationMinor, 95015, _T("CanEnterNotAppl"));
                    iBehaviorItem = BEHAVIOR_CANENTER_NOTAPPL;
                    break;

                case 2:
                    IssueWarning(Logic::ParserMessage::Type::DeprecationMinor, 95015, _T("CanEnterOutOfRange"));
                    iBehaviorItem = BEHAVIOR_CANENTER_OUTOFRANGE;
                    break;
            }

            break;

        case 3:                     // Messages
            switch( NextKeyword({ _T("DISPLAY"), _T("ERRMSG") }) )
            {
                case 0:
                    IssueError(91102); // Display, Errmsg expected
                    break;
                case 1:
                    iBehaviorItem = BEHAVIOR_MESSAGES_DISPLAY;
                    break;
                case 2:
                    iBehaviorItem = BEHAVIOR_MESSAGES_ERRMSG;
                    break;
            }

            break;

        case 14:                    // SpecialValues
            if( NextKeyword({ _T("ZERO") }) == 0 )
                IssueError(91113);  // Zero expected

            IssueWarning(Logic::ParserMessage::Type::DeprecationMinor, 95015, _T("SpecialValuesZero"));
            iBehaviorItem = BEHAVIOR_SPECIALVALUES;
            break;
        }

        NextToken();
        if( Tkn != TOKRPAREN )
            IssueError(ERROR_RIGHT_PAREN_EXPECTED);
    }
    else if( behavior_type == 4 )               // Mouse
        iBehaviorItem = BEHAVIOR_MOUSE;
    else if( behavior_type == 5 ) {             // PushMode
        IssueError(95012, _T("PUSHMODE"));
    }
    else if( behavior_type == 6 )               // EndGroup
        iBehaviorItem = BEHAVIOR_ENDGROUP;
    else if( behavior_type == 7 )               // EndLevel
        iBehaviorItem = BEHAVIOR_ENDLEVEL;
    else if( behavior_type == 8 ) {             // MsgNumber    // victor Jun 08, 00
        switch( NextKeyword({ _T("DISPLAY"), _T("ERRMSG") }) )
        {
            case 0:
                IssueError(91105); // Display, Errmsg expected
                break;
            case 1:
                iBehaviorItem = BEHAVIOR_MSGNUMBER_DISPLAY;
                break;
            case 2:
                iBehaviorItem = BEHAVIOR_MSGNUMBER_ERRMSG;
                break;
        }
    }
    else if( behavior_type == 9 ) {             // Export       // victor Dec 18, 00
        NextToken();
        if( Tkn != TOKLPAREN )
            IssueError(517);

        const std::vector<const TCHAR*> export_types = { _T("DATA"), _T("SPSS"), _T("SAS"), _T("STATA"),
            _T("ALL"), _T("CSPRO"), _T("TABDELIM"), _T("COMMADELIM"), _T("SEMICOLONDELIM"), _T("ALL4"), _T("R"), _T("ALL5") };

        switch( NextKeyword(export_types) )
        {
        case 0:
            IssueError(91106);
            break;
        case 1:
            iBehaviorItem = BEHAVIOR_EXPORT_DATA;
            break;
        case 2:
            iBehaviorItem = BEHAVIOR_EXPORT_SPSS;
            break;
        case 3:
            iBehaviorItem = BEHAVIOR_EXPORT_SAS;
            break;
        case 4:
            iBehaviorItem = BEHAVIOR_EXPORT_STATA;
            break;
        case 5:
            iBehaviorItem = BEHAVIOR_EXPORT_ALL;
            break;
        case 6:
            iBehaviorItem = BEHAVIOR_EXPORT_CSPRO;
            break;
        case 7:
            iBehaviorItem = BEHAVIOR_EXPORT_TABDELIM;
            break;
        case 8:
            iBehaviorItem = BEHAVIOR_EXPORT_COMMADELIM;
            break;
        case 9:
            iBehaviorItem = BEHAVIOR_EXPORT_SEMICOLONDELIM;
            break;
        case 10:
            iBehaviorItem = BEHAVIOR_EXPORT_ALL4;
            break;
        case 11:
            iBehaviorItem = BEHAVIOR_EXPORT_R;
            break;
        case 12:
            iBehaviorItem = BEHAVIOR_EXPORT_ALL5;
            break;
        }

        bool comma_allowed = true;

        while( true )
        {
            size_t optional_argument_type =
                NextKeyword({
                    _T("ITEMONLY"), _T("SUBITEMONLY"), _T("ITEMSUBITEM"),
                    _T("ANSI"), _T("UNICODE"),
                    _T("COMMADECIMAL"),
                    _T("XMLDDI2"), _T("XMLDDI3"), _T("XMLCSPRO"), _T("WEIGHT"), _T("XMLFREQUENCIES"), _T("XMLFREQUNIVERSE"),
                });

            if( optional_argument_type == 0 )
            {
                NextToken();

                if( comma_allowed && Tkn == TOKCOMMA )
                {
                    comma_allowed = false;
                    continue;
                }

                break;
            }

            switch( optional_argument_type )
            {
                case 1:
                    iBehaviorOpt = BEHAVIOR_EXPORT_ITEMONLY;
                    break;

                case 2:
                    iBehaviorOpt = BEHAVIOR_EXPORT_SUBITEMONLY;
                    break;

                case 3:
                    iBehaviorOpt = BEHAVIOR_EXPORT_ITEMSUBITEM;
                    break;

                case 4: // 20120416
                    break; // ANSI is the default option for the flag

                case 5:
                    iBehaviorOpt2 |= BEHAVIOR_EXPORT_UNICODE;
                    break;

                case 6:
                    iBehaviorOpt2 |= BEHAVIOR_EXPORT_COMMA_DECIMAL;
                    break;

                case 7:
                case 8:
                case 9:
                case 10:
                case 11:
                case 12:
                    IssueError(95027);
                    break;
            }

            comma_allowed = true;
        }

        if( ( iBehaviorOpt2 & BEHAVIOR_EXPORT_COMMA_DECIMAL ) && ( iBehaviorItem == BEHAVIOR_EXPORT_COMMADELIM || iBehaviorItem == BEHAVIOR_EXPORT_CSPRO ) )
        {
            iBehaviorOpt2 &= ~BEHAVIOR_EXPORT_COMMA_DECIMAL;
            IssueWarning(31068);
        }

        if( ( iBehaviorOpt2 & BEHAVIOR_EXPORT_UNICODE ) &&
                ( iBehaviorItem == BEHAVIOR_EXPORT_R || iBehaviorItem == BEHAVIOR_EXPORT_ALL ||
                    iBehaviorItem == BEHAVIOR_EXPORT_ALL4 || iBehaviorItem == BEHAVIOR_EXPORT_ALL5 ) )
        {
            iBehaviorOpt2 &= ~BEHAVIOR_EXPORT_UNICODE;
            IssueWarning(31069);
        }

        if( Tkn != TOKRPAREN ) {
            IssueError(ERROR_RIGHT_PAREN_EXPECTED);
        }
        NextToken();
    }
    else if( behavior_type == 10 ) {            // CheckRanges  // victor Dec 07, 00
        iBehaviorItem = BEHAVIOR_CHECKRANGES;
        m_pEngineSettings->SetHasCheckRanges( true );
    }
    else if( behavior_type == 11 ||             // SkipStruc    // victor Mar 14, 01
             behavior_type == 12 ) {            // SkipStructure// victor Mar 14, 01
        iBehaviorItem = BEHAVIOR_SKIPSTRUC;
        m_pEngineSettings->SetHasSkipStruc( true );
    }
    else if( behavior_type == 13 ) {          // exit
        iBehaviorItem = BEHAVIOR_EXIT;
    }

    if( bIsExport ) { // Always ON
        bSetOn = true;
    }
    else {
        // looks for mandatory keywords ON/OFF
        switch( NextKeyword({ _T("ON"), _T("OFF") }) )
        {
            case 0:
                IssueError(91103);
                break;
            case 1:
                bSetOn = true;
                break;
            case 2:
                bSetOn = false;
                break;
        }

        // is a canEnter ON... looks for optional noConfirm keyword
        NextToken();
    }

    // RHF INIC Nov 09, 2001
    // SKIPSTRUC ON [ (IMPUTE) ]
    if( bSetOn && Tkn == TOKLPAREN && iBehaviorItem == BEHAVIOR_SKIPSTRUC ) {
        NextToken();
        if( Tkn != TOKFUNCTION || CurrentToken.function_details->code != FNIMPUTE_CODE )
            IssueError(91110);
        NextToken();
        if( Tkn != TOKRPAREN )
            IssueError(ERROR_RIGHT_PAREN_EXPECTED);
        NextToken();

        iBehaviorItem = BEHAVIOR_SKIPSTRUCIMPUTE;


        // include( var-list) exlude (var-list)
        bool    bFirst=true;
        bool    bInclude=false;
        bool    bExclude=false;
        while( Tkn == TOKINCLUDE || Tkn == TOKEXCLUDE ) {
            if( Tkn == TOKINCLUDE ) {
                if( bInclude )
                    IssueError(91115);
                bInclude = true;
            }

            if( Tkn == TOKEXCLUDE ) {
                if( bExclude )
                    IssueError(91115);
                bExclude = true;
            }

            bool    bIncludeTkn=(Tkn==TOKINCLUDE);

            // Mark the full dictionary
            if( bFirst ) {
                DICT*   pDicT=DIP(0);      // referred to input dict only
                int     iSymSec = pDicT->SYMTfsec;

                while( iSymSec > 0 ) {
                    SECT*   pSecT= SPT(iSymSec);
                    int     iSymVar = pSecT->SYMTfvar;

                    while( iSymVar > 0 ) {
                        VART*   pVarT = VPT(iSymVar);
                        // Is better to be sure the flag is true when exclude or not clause used.
                        pVarT->SetSkipStrucImpute( bIncludeTkn ? false: true );
                        iSymVar = pVarT->SYMTfwd;
                    }

                    iSymSec = pSecT->SYMTfwd;
                }

                bFirst = false;
            }

            NextToken();
            if( Tkn != TOKLPAREN )
                IssueError(517);
            NextToken();

            int     iNumItems=0;
            while( 1 ) {
                if( Tkn == TOKRPAREN )
                    break;

                if( !CompExpIsValidVar() )
                    IssueError(11);  // Variable expected

                int     iSymVar = Tokstindex;
                VART*   pVarT = VPT(iSymVar);

                if( pVarT->GetSubType() != SymbolSubType::Input )
                    IssueError(91114);

                pVarT->SetSkipStrucImpute( bIncludeTkn ? true : false );
                iNumItems++;

                NextToken();
                while( Tkn == TOKCOMMA )
                    NextToken();
            }

            if( iNumItems == 0 )
                IssueError(31086);

            if( Tkn != TOKRPAREN )
                IssueError(ERROR_RIGHT_PAREN_EXPECTED);

            NextToken();
        }
    }
    // RHF END Nov 09, 2001

    if( bSetOn && ( iBehaviorItem == BEHAVIOR_CANENTER_NOTAPPL  || iBehaviorItem == BEHAVIOR_CANENTER_OUTOFRANGE ) )
    {
        if( Tkn != TOKLPAREN )
            IssueError(517);

        switch( NextKeyword({ _T("CONFIRM"), _T("NOCONFIRM") }) )
        {
            case 0:
                IssueError(91104); // Confirm, noConfirm expected
                break;
            case 1:
                bConfirm = true;
                break;
            case 2:
                bConfirm = false;
                break;
        }

        NextToken();

        if( Tkn != TOKRPAREN )
            IssueError(ERROR_RIGHT_PAREN_EXPECTED);
    }
    else
    {
        // RHF INIC Sep 20, 2004
        // Allow OFF( NoConfirm )
        if( !bSetOn && ( iBehaviorItem == BEHAVIOR_CANENTER_NOTAPPL  || iBehaviorItem == BEHAVIOR_CANENTER_OUTOFRANGE ) && Tkn == TOKLPAREN )
        {
            switch( NextKeyword({ _T("CONFIRM"), _T("NOCONFIRM") }) )
            {
                case 0:
                    IssueError(91104); // Confirm, noConfirm expected
                    break;
                case 1:
                    bConfirm = true;
                    break;
                case 2:
                    bConfirm = false;
                    break;
            }

            NextToken();
            if( Tkn != TOKRPAREN )
                IssueError(ERROR_RIGHT_PAREN_EXPECTED);
        }
        else
            // RHF END Sep 20, 2004
            bNextTokenReady = true;
    }

    // finally... setup node to be interpreted
    if( Flagcomp ) {
        int     size_acce = sizeof(ACCESS_NODE);
        int     size_othe = sizeof(SETOTHER_NODE);


        OC_CreateCompilationSpace(( size_othe - size_acce ) / sizeof(int));
        pset_ot->type           = static_cast<int>(SetAction::Behavior);
        pset_ot->setinfo.behavior_item = (int) ( ( ( iBehaviorOpt << 16 ) & 0x00ff0000 ) |
                                               ( ( iBehaviorOpt2 << 24 ) & 0xff000000 ) |
                                               ( ( iBehaviorItem & 0x0000ffff ) ) );
        pset_ot->len            = bSetOn;   // 0: OFF, 1: ON

        if( iBehaviorItem == BEHAVIOR_CANENTER_NOTAPPL    ||
            iBehaviorItem == BEHAVIOR_CANENTER_OUTOFRANGE ) {
            if( bSetOn && !bConfirm )
                pset_ot->len += 1;      // 2: CanEnter ON without confirmation

            // RHF INIC Sep 23, 2004
            if( !bSetOn && !bConfirm )
                pset_ot->len = -1;
            // RHF END Sep 23, 2004
        }

        // RHF INIC Sep 23, 2004
        if( bAllowList ) { // iAttrNumber can be 0
            LIST_NODE*  pListNode;
            int         iSizeList= sizeof(LIST_NODE);

            if( iAttrNumber >= 1 )
                iSizeList += (iAttrNumber - 1) * sizeof(int);

            pListNode = (LIST_NODE*) (PPT(Prognext));

            OC_CreateCompilationSpace(iSizeList / sizeof(int));

            pListNode->iNumElems = iAttrNumber;
            for( int i = 0; i < iAttrNumber; i++ )
                pListNode->iSym[i] = aAttrSymbols[i];

        }
        // RHF END Sep 23, 2004
    }

    return bNextTokenReady;
}
