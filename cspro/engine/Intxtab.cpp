#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include "Ctab.h"

double CIntDriver::exxtab( int iExpr )
{
    double      dWeightValue;
    LIST_NODE*  pListNode; // RHF Jul 09, 2002
    CTAB*       pCtab;

    FN6_NODE*   pFun = (FN6_NODE *) (PPT(iExpr));

    int         iPosList=pFun->iListNode;
    pListNode = (LIST_NODE *) (PPT(iPosList)); // RHF Jul 09, 2002

    pCtab = XPT( pFun->tabl_ind );
    if( pCtab->GetAcumArea() == NULL )
        return( 0 );

    // RHF INIC Jan 23, 2003
    // checks select' expression
    int         iSelectExpr = pFun->iSelectExpr;
    if( iSelectExpr == 0 )
        ;
    else {
        double  dSelectExprValue;
        if( iSelectExpr > 0 )
            dSelectExprValue = evalexpr( iSelectExpr );
        else {
            dSelectExprValue = evalexpr( -iSelectExpr );
        }

        if( dSelectExprValue == 0 || IsSpecial(dSelectExprValue) )
            return( (double) -1 );
    }
    // RHF END Jan 23, 2003

    // RHF INIC Jul 04, 2002
    int      iWeightExpr=pFun->iWeightExpr;

    if( iWeightExpr == 0 )
        dWeightValue = 1;
    else if( iWeightExpr > 0 )
        dWeightValue = evalexpr( iWeightExpr );
    else {
        dWeightValue = evalexpr( -iWeightExpr );
    }

    if( IsSpecial(dWeightValue) )
        return( (double) -1 );
    // RHF END Jul 04, 2002

    return( DoXtab( pCtab, dWeightValue, pCtab->GetTabLogicExpr(), pListNode ) );
}

double CIntDriver::exupdate( int iExpr )
{
    return exxtab(iExpr);
}
