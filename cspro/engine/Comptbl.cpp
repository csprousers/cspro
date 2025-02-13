//-----------------------------------------------------------------------
//
// compTBL.cpp  compiler for Table arithmetics
//
//-----------------------------------------------------------------------

#include "StandardSystemIncludes.h"
#include "Tables.h"
#include "COMPILAD.H"
#include "COMPUTIL.H"
#include "Engine.h"
#include "Ctab.h"

int CEngineCompFunc::rutcpttbl()
  {
    int     icpt, i;

#ifdef GENCODE
    TBL_CPT_NODE    *ptrcpt;
#endif


    bool    bPostZero = (m_pEngineArea->IsLevel( InCompIdx ) && LvlInComp == 0 && ProcInComp == PROCTYPE_POST);// RHF Mar 26, 2002

    bPostZero = bPostZero || (ProcInComp == PROCTYPE_ECALC); // RHF May 06, 2003

    bool    bValid=(bPostZero || NPT(InCompIdx)->IsA(SymbolType::UserFunction) ); // RHF Jan 11, 2003
    // RHF COM Jan 11, 2003 bool    bValid=bPostZero;


    // RHF COM Mar 26, 2002 if( !m_pEngineArea->IsLevel( InCompIdx ) || LvlInComp != 0 || ProcInComp != PROCTYPE_POST )
    if( !bValid )// RHF Mar 26, 2002
        return( SetSyntErr(535), 0 );

    icpt = Prognext;

#ifdef GENCODE
    ptrcpt = (TBL_CPT_NODE *) (PPT(Prognext));
    if( Flagcomp )
      {
        ADVANCE_NODE( TBL_CPT_NODE );
        ptrcpt->st_code = TBLCPT_CODE;
      }
#endif

    i = ctblref();
    if( GetSyntErr() != 0 )
        return 0;

#ifdef GENCODE
    if( Flagcomp )
        ptrcpt->tbl_left = i;
#endif

    if( Tkn != TOKEQOP  )
        return( SetSyntErr(5), 0 );

    NextToken();

    if( Tkn == TOKFUNCTION )
    {
        if( CurrentToken.function_details->compilation_type == Logic::FunctionCompilationType::FN10 )
        {
            i = ( CurrentToken.function_details->code == FNTBLSUM_CODE ) ? ctblsum() :
                ( CurrentToken.function_details->code == FNTBLMED_CODE ) ? ctblmed() :
                                                                           0;
        }

        else
        {
            SetSyntErr(571);
        }
      }

    else
    {
        i = tbl_expr();
    }

    if( GetSyntErr() != 0 )
        return 0;

#ifdef GENCODE
    if( Flagcomp )
        ptrcpt->tbl_expr = i;
#endif

    return( icpt );
  }

int CEngineCompFunc::tbl_expr()
{
    if( GetSyntErr() != 0 )
        return 0;

    int p1 = tbl_term();

    while( GetSyntErr() == 0 && ( Tkn == TOKADDOP || Tkn == TOKMINOP ) )
    {
        TokenCode oper = Tkn;

        NextToken();                      /* arithmetic operator ( *, / ) */
        int p2 = tbl_term();

        p1 = CreateOperatorNode(oper == TOKADDOP ? ADD_CODE : SUB_CODE, p1, p2);
    }

    return p1;
}

int CEngineCompFunc::tbl_term()
{
    if( GetSyntErr() != 0 )
        return 0;

    int p1 = tbl_factor();

    while( GetSyntErr() == 0 && ( Tkn == TOKMULOP || Tkn == TOKDIVOP ) )
    {
        TokenCode oper = Tkn;

        NextToken();                      /* arithmetic operator ( *, / ) */
        int p2 = tbl_factor();

        p1 = CreateOperatorNode(oper == TOKMULOP ? MULT_CODE : DIV_CODE, p1, p2);
    }

    return p1;
}

int CEngineCompFunc::tbl_factor()
  {
    if( GetSyntErr() != 0 )
        return 0;

    int p1 = 0;

    if( Tkn == TOKLPAREN )
      {
        NextToken();
        p1 = tbl_expr();
        if( Tkn == TOKRPAREN )
            NextToken();
        else
            SetSyntErr(19);
      }
    else if( Tkn == TOKMINOP || Tkn == TOKCTE ) // GHM 20120405 added TOKMINOP stuff so that negative values can be used in PostCalc expressions
      {
          bool negateValue = Tkn == TOKMINOP;

          if( negateValue )
          {
              NextToken();

              if( Tkn != TOKCTE )
                  SetSyntErr(21);
          }

          p1 = CreateNumericConstantNode(negateValue ? -Tokvalue : Tokvalue);

          NextToken();                      /* constant */
      }
    else if( Tkn == TOKVAR )
      {
        p1 = varsanal( 'N' );
      }
    else if( Tkn == TOKCROSSTAB ) {
        // JH 4/27/06 Allow use of table expression w. parens as scalar value mixed
        // w. table matrix ops, peek ahead for paren for vs. bracket to determine
        // whether to parse as matrix or scalar.

        int sind  = Tokstindex;
        MarkInputBufferToRestartLater();               // mark input buffer to restart
        NextToken();
        int aux = Tkn;
        Tkn = TOKCROSSTAB;
        Tokstindex = sind;
        RestartFromMarkedInputBuffer();               // restart!

        if( aux == TOKLPAREN ) {
            p1 = tvarsanal(); // parse as scalar
        }
        else {
            p1 = ctblref(); // parse as matrix
        }
    }
    else
        SetSyntErr(21);

    return( p1 );
  }

int CEngineCompFunc::ctblref()
  {
    int     ndim, i1, i2, ipt;
    CTAB    *tp;
#ifdef GENCODE
    int     pint, i;
    TBL_NODE   *ptrtable;
#endif

    tp = XPT(Tokstindex);
    ndim = tp->GetNumDim();
    ipt = 0;

#ifdef GENCODE
    if( Flagcomp )
      {
        ptrtable = (TBL_NODE *) (PPT(Prognext));
        ipt = Prognext;

        ADVANCE_NODE( TBL_NODE );
        ptrtable->tbl_type  = TBL_CODE;
        ptrtable->tbl_index = Tokstindex;
        for( i = 0; i < 3; i++ )
          {
            ptrtable->iexpr[i].lo = -1;
            ptrtable->iexpr[i].up = tp->GetTotDim(i) - 1;
          }
        pint = 0;
      }
#endif

    NextToken();
    if( Tkn == TOKLBRACK )
      {
        NextToken();
        while( Tkn != TOKRBRACK )
          {
            if( Tkn == TOKMULOP )
              {
                NextToken();
#ifdef GENCODE
                if( Flagcomp )
                    pint++;
#endif
              }
            else
              {
                i1 = exprlog();
                if( GetSyntErr() != 0 )
                    return 0;

                if( Tkn == TOKCOLON )
                  {
                    NextToken();
                    i2 = exprlog();
                    if( GetSyntErr() != 0 )
                        return 0;
#ifdef GENCODE
                    if( Flagcomp )
                      {
                        ptrtable->iexpr[pint].lo = i1;
                        ptrtable->iexpr[pint].up = i2;
                        pint++;
                      }
#endif
                  }
                else
                  {
#ifdef GENCODE
                    if( Flagcomp )
                      {
                        ptrtable->iexpr[pint].lo = -2;
                        ptrtable->iexpr[pint].up = i1;
                        pint++;
                      }
#endif
                  }
              }
            ndim--;
            if( ndim < 0 ) break; /* RHF 13/9/96 Arregla problema de LOOP
                                     cuando falta ] */
            if( Tkn == ',' )
                NextToken();
          }

        NextToken();
        if( ndim != 0 )
            return( SetSyntErr(536), 0 );
      }

    return( ipt );
  }


int CEngineCompFunc::ctblsum()
{
    int iFunction = CurrentToken.function_details->code;
    int ipt = 0;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, 517);

    int iCoord = (int)NextKeyword({ _T("ROW"), _T("COLUMN"), _T("LAYER") });
    if( iCoord == 0 )
        IssueError(79);

    NextToken();
    IssueErrorOnTokenMismatch(TOKCROSSTAB, 80);

    CTAB* tp = XPT(Tokstindex);
    if( iCoord > tp->GetNumDim() )
        IssueError(536);

#ifdef GENCODE
    if( Flagcomp )
    {
        ipt = Prognext;
        TBL_FUN_NODE* ptrfnode = (TBL_FUN_NODE*)PPT(Prognext);
        ADVANCE_NODE(TBL_FUN_NODE);
        ptrfnode->function = iFunction;
        ptrfnode->vardim = iCoord;
      }
#endif

    ctblref();
    if( GetSyntErr() != 0 )
        return 0;

    IssueErrorOnTokenMismatch(TOKRPAREN, ERROR_RIGHT_PAREN_EXPECTED);

    NextToken();

    return ipt;
}

int CEngineCompFunc::ctblmed()
{
    int     iFunction, iCoord, ipt, iMedianType;
    CTAB    *tp;
    int*    pipl=NULL;
#ifdef GENCODE
    int     iplowest;
    TBL_FUN_NODE *ptrfnode;
#endif

    iFunction = CurrentToken.function_details->code;
    ipt = 0;

    NextToken();
    if( Tkn != TOKLPAREN )
        return( SetSyntErr(517), 0 );

    size_t type = NextKeyword({ _T("ROW"), _T("COLUMN"), _T("LAYER"), _T("CONTINUOUS"), _T("DISCRETE") });
    if( type == 0 )
        return( SetSyntErr(79), 0 );
    iMedianType = 0; // Default old behavior
    if( type == 4 || type == 5 ) {
        iMedianType = ( type == 4 ) ? 1 : 2;
        type = NextKeyword({ _T("ROW"), _T("COLUMN"), _T("LAYER") });
        if( type == 0 )
            return( SetSyntErr(79), 0 );
    }

    iCoord = type;

    NextToken();
    if( Tkn != TOKCROSSTAB )
        return( SetSyntErr(80), 0 );

    tp = XPT( Tokstindex );
    if( iCoord > tp->GetNumDim() )
        return( SetSyntErr(536), 0 );

#ifdef GENCODE
    ptrfnode = (TBL_FUN_NODE *) (PPT(Prognext));
    if( Flagcomp ) {
        ipt = Prognext;

        ADVANCE_NODE( TBL_FUN_NODE );

        ptrfnode->function = iFunction;
        ptrfnode->vardim = iCoord;
        iplowest = Prognext;
        pipl = PPT(iplowest);
        *pipl = -1;            // Highest
        *(pipl + 1) = 0;       // nlowers
        *(pipl + 2) = -1;      // Pointer to lowers area
        *(pipl + 3) = iMedianType;   // Old(00)/Continuos(01)/Discrete(02)

        OC_CreateCompilationSpace(4);
    }
#endif

    ctblref();
    if( GetSyntErr() != 0 )
        return 0;

#ifdef GENCODE
    if( Flagcomp )
        *(pipl + 2) = Prognext;
#endif

    // RHF INIC Sep 12, 2002
    if( Tkn == TOKINTERVAL ) {
        CompIntervals( pipl, NULL );
        if( GetSyntErr() != 0 )
            return 0;

        if( Tkn != TOKRPAREN )
            return( SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED), 0 );
        NextToken();
    }
    // RHF END Sep 12, 2002

    if( Tkn != TOKRPAREN )
        return( SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED), 0 );
    NextToken();

    return( ipt );
}


int CEngineCompFunc::CompIntervals( int* pipl, std::vector<double>* pIntervals ) {
    bool    bLikeTable=(pIntervals==NULL);

#ifdef _DEBUG
    if( !bLikeTable )
        ASSERT( pipl == NULL );
#endif

    if( Tkn == TOKINTERVAL ) {
        bool                    bHasLowers=false, bHasHighest=false;
        double                  dHigh=-MAXVALUE;
        std::vector<double>     aLowers;

        NextToken();
        if( Tkn != TOKLPAREN )
            return( SetSyntErr(517), 0 );
        NextToken();

        while( Tkn == TOKHIGHEST || Tkn == TOKLOWER )  {
            if( Tkn == TOKHIGHEST ) {
                if( bHasHighest )
                    return( SetSyntErr(573), 0 ); // repeated 'highest'
                bHasHighest = true;
                NextToken();
                if( Tkn != TOKCTE )
                    return( SetSyntErr(82), 0 );
#ifdef GENCODE
                if( Flagcomp ) {
                    if( bLikeTable )
                        *pipl = ConserveConstant(Tokvalue);
                    else
                        dHigh = Tokvalue;
                }
                if( GetSyntErr() != 0 )
                    return 0;
#endif
                NextToken();
            }

            if( Tkn == TOKLOWER ) {
                if( bHasLowers )
                    return( SetSyntErr(573), 0 ); // repeated 'lowers'
                bHasLowers = true;
                NextToken();
                while( 1 ) {
                    if( Tkn != TOKCTE )
                        return( SetSyntErr(82), 0 );
#ifdef GENCODE
                    if( Flagcomp ) {
                        if( bLikeTable ) {
                            *(pipl + 1) += 1;
                            *(PPT(Prognext)) = ConserveConstant(Tokvalue);
                            if( GetSyntErr() != 0 )
                                return 0;

                            OC_CreateCompilationSpace(1);
                        }
                        else
                            aLowers.emplace_back( Tokvalue );
                    }
#endif
                    NextToken();
                    if( Tkn == TOKCOMMA )
                        NextToken();
                    else if( Tkn == TOKRPAREN )
                        break;
                    else
                        return( SetSyntErr(572), 0 );
                }
            }
        }


// JH 5/30/06 only generate intervals for GENCODE, avoids assert later on
#ifdef GENCODE
        if( Flagcomp ) {
        if( !bLikeTable ) {
            pIntervals->emplace_back( dHigh );
            for( size_t i = 0; i < aLowers.size(); i++ )
                pIntervals->emplace_back(aLowers[i]);
        }
    }
#endif
    }

    return 0;
}
