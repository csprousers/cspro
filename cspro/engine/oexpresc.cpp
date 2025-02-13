//---------------------------------------------------------------------------
//  EXPRESC.cpp   compiler of expressions
//---------------------------------------------------------------------------
#include "StandardSystemIncludes.h"

#ifdef GENCODE
#include "Exappl.h"
#else
#include "Tables.h"
#endif
#include "COMPILAD.H"
#include "Engine.h"

#include "Ctab.h"
#include <zDictO/DDClass.h>
#include "COMPUTIL.H"



// RHF INIC 23/9/96
int CEngineCompFunc::old_do_cfun_fntc( CTAB* ct, int iDimType, void* pvoid ) {
    CTTERM*     pctt;

#ifdef GENCODE
    OLD_FNTC_NODE*  ptrfunc = NULL;

    if( Flagcomp )
        ptrfunc = (OLD_FNTC_NODE*) pvoid;
#endif

    NextToken(); // Eat table name

    // Hay 5 posibilidades:  (T1) (T1,5) (T1,4,expr) (T1,,expr) (T1,expr)

    if( Tkn == TOKRPAREN ) {            // Posibilidad 1
        pctt = default_lastsubtable( ct, iDimType, pvoid );
        if( !default_expr( ct, pctt, iDimType, pvoid ) )
            return 0;
        return 1;
    }
    else if( Tkn != TOKCOMMA ) {
        SetSyntErr( 528 );     // Comma Expected
        return 0;
    }

    NextToken();
    if( Tkn == TOKVAR ) {               // Posibilidad 5
        if( ( pctt = search_subtable( ct, iDimType, pvoid, Tokstindex ) ) == NULL )
            return( SetSyntErr(655) );    // invalid variable spec.

        if( !compile_expr( ct, pctt, iDimType, pvoid ) )
            return 0;
    }
    else if( Tkn == TOKCOMMA ) {        // Posibilidad 4
        NextToken();

        pctt = default_lastsubtable( ct, iDimType, pvoid );
        if( !compile_expr( ct, pctt, iDimType, pvoid ) )
            return 0;
    }

    else if( Tkn == TOKCTE ) {          // Posibilidad 2 o 3
        if( Tokvalue != (int) Tokvalue ) {
            SetSyntErr( 653 );
            return 0;
        }

        int nterm = (int) Tokvalue;

#ifdef GENCODE
        if( Flagcomp ) {
            ptrfunc->ict   = Tokstindex;
            ptrfunc->nterm = nterm;
        }
#endif

        NextToken();
        if( Tkn != TOKCOMMA ) {
            SetSyntErr( 528 );  // Comma Expected
            return 0;
        }

        NextToken();

        pctt = ct->GetTerm(iDimType-1);

        int iterm;

        for( iterm = 1; iterm < nterm && pctt->m_iNodeId != -1; iterm++ )
            pctt++;

        if( iterm < nterm || pctt->m_iNodeId == -1 ) {
            SetSyntErr( 654 );
            return 0;
        }

        if( Tkn == TOKRPAREN ) {        // Posibilidad 2
            if( !default_expr( ct, pctt, iDimType, pvoid ) )
                return 0;
        }
        else {                          // Posibilidad 3
            if( !compile_expr( ct, pctt, iDimType, pvoid ) )
                return 0;
        }
    }

    return 1;
}

int CEngineCompFunc::compile_expr( CTAB* ct, CTTERM* pctt, int ndim, void* pvoid ) {
    CTNODE* pctn;
    int     iterm;
    int ivar;  // RCH March 28, 2000

#ifdef GENCODE
    OLD_FNTC_NODE*  ptrfunc=NULL;

    if( Flagcomp )
        ptrfunc = (OLD_FNTC_NODE*) pvoid;
#endif

    if( pctt->m_iNodeId == 0 ) {           // term is a single variable
        pctn = CTN( pctt->m_iCtNode );
        if( Tkn != TOKVAR ) {
            SetSyntErr(655);
            return 0;
        }
        ivar = VPT(Tokstindex)->GetSymbolIndex();
        ASSERT( pctn->isVarNode() );
        if( ivar != pctn->m_iSymbol ) {
            SetSyntErr(655);
            return 0;
        }

        NextToken();
        if( Tkn != TOKEQOP ) {
#ifdef GENCODE
            iterm = CreateNumericConstantNode( m_pEngineArea->cthighnumvalue( pctt->m_iCtNode ) );
#endif
        }
        else {
            NextToken();
            iterm = exprlog();
            if( GetSyntErr() != 0 )
                return 0;
        }

#ifdef GENCODE
        if( Flagcomp ) {
            ptrfunc->var1 = pctt - ct->GetTerm(ndim-1);
            ptrfunc->expr1 = iterm;
        }
#endif
    }
    else {                              // term is a product term
        // ------ primera variable puede ser opcional ---------
        pctt = ct->GetTerm(ndim-1) + pctt->m_iNodeId;
        pctn = CTN( pctt->m_iCtNode );
        if( Tkn != TOKVAR ) {
            SetSyntErr(655);
            return 0;
        }
        ivar = VPT(Tokstindex)->GetSymbolIndex();

        ASSERT( pctn->isVarNode() );
        if( ivar != pctn->m_iSymbol ) {
            if( !default_expr1( ct, pctt, ndim, pvoid ) )
                return 0;
        }
        else {
            NextToken();
            if( Tkn != TOKEQOP ) {
#ifdef GENCODE
                iterm = CreateNumericConstantNode( m_pEngineArea->cthighnumvalue( pctt->m_iCtNode ) );
#endif
            }
            else {
                NextToken();
                iterm = exprlog();
                if( GetSyntErr() != 0 )
                    return 0;
            }
#ifdef GENCODE
            if( Flagcomp ) {
                ptrfunc->var1 = pctt - ct->GetTerm(ndim-1);
                ptrfunc->expr1 = iterm;
            }
#endif
        }

        // ----------- segunda variable -----------
        if( Tkn == TOKRPAREN ) {
            if( !default_expr2( ct, pctt, ndim, pvoid ) )
                return 0;
        }
        else {
            if( Tkn != TOKVAR ) {
                SetSyntErr(655);
                return 0;
            }
            ivar = VPT(Tokstindex)->GetSymbolIndex();

            ASSERT( CTN(pctt->m_iCtNode)->isVarNode() );
            pctt++;
            while( pctt->m_iNodeId != -1 && CTN(pctt->m_iCtNode)->m_iSymbol != ivar )
                pctt++;
            if( pctt->m_iNodeId == -1 ) {
                SetSyntErr(655);
                return 0;
            }

            pctn = CTN( pctt->m_iCtNode );
            ASSERT( pctn->isVarNode() );
            if( ivar != pctn->m_iSymbol ) {
                SetSyntErr(655);
                return 0;
            }

            NextToken();
            if( Tkn != TOKEQOP ) {
#ifdef GENCODE
                iterm = CreateNumericConstantNode( m_pEngineArea->cthighnumvalue( pctt->m_iCtNode ) );
#endif
            }
            else {
                NextToken();
                iterm = exprlog();
                if( GetSyntErr() != 0 )
                    return 0;
            }
#ifdef GENCODE
            if( Flagcomp ) {
                ptrfunc->var2 = pctt - ct->GetTerm(ndim-1);
                ptrfunc->expr2 = iterm;
            }
#endif
        }
    }

    return TRUE;
}

int CEngineCompFunc::default_expr( CTAB* ct, CTTERM* pctt, int ndim, void* pvoid ) {
    if( pctt->m_iNodeId == 0 ) {
        if( !default_expr1( ct, pctt, ndim, pvoid ) )
            return 0;
    }
    else {
        pctt = ct->GetTerm(ndim-1) + pctt->m_iNodeId;

        if( !default_expr1( ct, pctt, ndim, pvoid ) )
            return 0;

        if( !default_expr2( ct, pctt, ndim, pvoid ) )
            return 0;
    }

    return 1;
}

int CEngineCompFunc::default_expr1( CTAB* ct, CTTERM* pctt, int ndim, void* pvoid ) {
#ifdef GENCODE
    if( Flagcomp ) {
        int         iterm = CreateNumericConstantNode( m_pEngineArea->cthighnumvalue( pctt->m_iCtNode ) );

        if( iterm < 0 )
            return 0;

        OLD_FNTC_NODE*  ptrfunc = (OLD_FNTC_NODE*) pvoid;

        ptrfunc->var1 = pctt - ct->GetTerm(ndim-1);
        ptrfunc->expr1 = iterm;
    }
#endif

    return 1;
}

int CEngineCompFunc::default_expr2( CTAB* ct, CTTERM* pctt, int ndim, void* pvoid ) {
#ifdef GENCODE
    pctt++;
    while( pctt->m_iNodeId != -1 )
        pctt++;
    pctt--;

    if( Flagcomp ) {
        int     iterm = CreateNumericConstantNode( m_pEngineArea->cthighnumvalue( pctt->m_iCtNode ) );

        if( iterm < 0 )
            return 0;

        OLD_FNTC_NODE*  ptrfunc = (OLD_FNTC_NODE*) pvoid;

        ptrfunc->var2 = pctt - ct->GetTerm(ndim-1);
        ptrfunc->expr2 = iterm;
    }
#endif

    return 1;
}

CTTERM* CEngineCompFunc::default_lastsubtable( CTAB* ct, int ndim, void* pvoid ) {
    CTTERM* pctt  = ct->GetTerm(ndim-1);
    int     nterm = 0;

    while( pctt->m_iNodeId != -1 ) {
        pctt++;
        nterm++;
    }
    pctt--;

#ifdef GENCODE
    if( Flagcomp ) {
        OLD_FNTC_NODE*  ptrfunc = (OLD_FNTC_NODE*) pvoid;

        ptrfunc->ict   = ct->GetSymbolIndex();
        ptrfunc->nterm = nterm;
    }
#endif

    return pctt;
}


CTTERM* CEngineCompFunc::search_subtable( CTAB* ct, int ndim, void* pvoid, int ivar ) {
    CTNODE* pctn;
    CTTERM* pctt;
    CTTERM* pctt2;
    int     lastterm;
    bool    bFound = false;
    CTTERM* pctt1 = ct->GetTerm(ndim-1);
    int     nterm = 1;

    while( pctt1->m_iNodeId != -1 ) {
        if( pctt1->m_iNodeId == 0 ) {
            pctn = CTN( pctt1->m_iCtNode );
            ASSERT( pctn->isVarNode() );
            if( ivar == pctn->m_iSymbol ) {  // found
                bFound   = true;
                lastterm = nterm;
                pctt     = pctt1;
            }
        }
        else {
            pctt2 = ct->GetTerm(ndim-1) + pctt1->m_iNodeId;

            // para cada variable. OJO siempre tiene nodeid=0
            // pues no hay mas niveles de multiplicacion
            while( pctt2->m_iNodeId != -1 ) {
                pctn = CTN( pctt2->m_iCtNode );
                ASSERT( pctn->isVarNode() );
                if( ivar == pctn->m_iSymbol ) { // found
                    bFound   = true;
                    lastterm = nterm;
                    pctt     = pctt1;
                }
                pctt2++;
            }
        }
        pctt1++;
        nterm++;
    }

    if( !bFound )
        return NULL;

#ifdef GENCODE
    if( Flagcomp ) {
        OLD_FNTC_NODE*  ptrfunc = (OLD_FNTC_NODE*) pvoid;

        ptrfunc->ict   = ct->GetSymbolIndex();
        ptrfunc->nterm = lastterm;
    }
#endif

    return pctt;
}

