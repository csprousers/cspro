/*------------------------------------------------------------------------*/
/*                                                                        */
/*  INTTBL.cpp    interprets table expressions and functions              */
/*                                                                        */
/*------------------------------------------------------------------------*/

#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include "Ctab.h"

static    int     T_idx[3];
static    TBL_NODE *Tbl_left;

/*------------------------------------------------------------------------*/
/*      excpttbl: execute table assignment statement                      */
/*------------------------------------------------------------------------*/
double CIntDriver::excpttbl( int iExpr ) {
    const TBL_CPT_NODE* pcpt = (TBL_CPT_NODE*)PPT(iExpr);

    Tbl_left = (TBL_NODE*)PPT(pcpt->tbl_left);
    const int* pint = PPT(pcpt->tbl_expr);

    if( *pint == FNTBLSUM_CODE )
        extblsum( iExpr  ); // RHF 2/11/99 Old pcpt
    else if( *pint == FNTBLMED_CODE )
        extblmed( iExpr );   // RHF 2/11/99 Old pcpt
    else {
        tblsetlu( Tbl_left );
        if( tblchkexpr( pcpt->tbl_expr ) )
            docpttbl( pcpt->tbl_expr );
    }

    return( 0 );
}

/*------------------------------------------------------------------------*/
/*       docpttbl: do the assignment requested in excpttbl                */
/*------------------------------------------------------------------------*/
void CIntDriver::docpttbl( int iExpr ) {
    int     n[3], i, j, k;

    for( i = 0; i < 3; i++ )
        n[i] = Tbl_left->range[i].up < 0 ?
        1 : Tbl_left->range[i].up - Tbl_left->range[i].lo + 1;

    for( T_idx[0] = 0; T_idx[0] < n[0]; T_idx[0]++ ) {
        i = Tbl_left->range[0].lo + T_idx[0];
        for( T_idx[1] = 0; T_idx[1] < n[1]; T_idx[1]++ ) {
            j = Tbl_left->range[1].lo + T_idx[1];
            for( T_idx[2] = 0; T_idx[2] < n[2]; T_idx[2]++ ) {
                k = Tbl_left->range[2].lo + T_idx[2];
                CTAB*   pCtab= XPT(Tbl_left->tbl_index);

                pCtab->m_pAcum.PutDoubleValue( evaltblexpr( iExpr ), i, j, k );

            }
        }
    }
}

/*------------------------------------------------------------------------*/
/*                  evaltblexpr : computes a table expression             */
/*------------------------------------------------------------------------*/
double CIntDriver::evaltblexpr( int iExpr )  {
    int     idx[3], i;
    double  val1 = 0, val2 = 0;

    const Nodes::Operator* poper = (const Nodes::Operator*)PPT(iExpr);
    const TBL_NODE* tbl = (const TBL_NODE*)PPT(iExpr);

    if( poper->oper == ADD_CODE  || poper->oper == SUB_CODE ||
        poper->oper == MULT_CODE || poper->oper == DIV_CODE ) {
        val1 = evaltblexpr( poper->left_expr );
        val2 = evaltblexpr( poper->right_expr );
        if( IsSpecial(val1) || IsSpecial(val2) )
            return( DEFAULT );
    }

    switch( poper->oper ) {
    case ADD_CODE:
        return( val1 + val2 );
    case SUB_CODE:
        return( val1 - val2 );
    case MULT_CODE:
        return( val1 * val2 );
    case DIV_CODE:
        if( val2 == 0 )
            return( DEFAULT );
        else
            return( val1 / val2 );
    case TBL_CODE:
        {
            for( i = 0; i < 3; i++ ) {
                if( tbl->left_idx[i] < 0 )
                    idx[i] = tbl->range[i].lo;
                else
                    idx[i] = T_idx[tbl->left_idx[i]] + tbl->range[i].lo;
                if( idx[i] < 0 )
                    idx[i] = 0;
            }
            CTAB*   pCtab=XPT(tbl->tbl_index);
            // RHF COM Jul 31, 2001 return( tabvalue( pCtab, idx[0], idx[1], idx[2] ) );
            return( pCtab->m_pAcum.GetDoubleValue( idx[0], idx[1], idx[2] ) );
        }
    default:   /*   VARIABLES AND CONSTANTS    */
        return( evalexpr( iExpr ) );
    }

    return( DEFAULT );
}

/*------------------------------------------------------------------------*/
/*       tblchkexpr: check table expression dimensions                    */
/*------------------------------------------------------------------------*/
bool CIntDriver::tblchkexpr( int iExpr ) {
    const Nodes::Operator* poper = (const Nodes::Operator*)PPT(iExpr);
    switch( poper->oper ) {
    case ADD_CODE:
    case SUB_CODE:
    case MULT_CODE:
    case DIV_CODE:
        if( tblchkexpr( poper->left_expr ) )
            return( tblchkexpr( poper->right_expr ) );
        else
            return( false );
    case TBL_CODE:
        tblsetlu( (TBL_NODE *) poper );
        return( tbldimchk( Tbl_left, (TBL_NODE *) poper ) );
    }

    return( true );
}

/*------------------------------------------------------------------------*/
/*       tblsetlu: load l & u with index ranges for given table refer.    */
/*------------------------------------------------------------------------*/
void CIntDriver::tblsetlu( TBL_NODE *tp ) {
    int     i;
    CTAB    *ta;

    ta = XPT(tp->tbl_index);

    for( i = 0; i < 3; i++ )
        tp->range[i].lo = tp->range[i].up = -1;

    for( i = 0; i < ta->GetNumDim(); i++ )
        if( tp->iexpr[i].lo >= 0 ) {
            tp->range[i].lo = evalexpr( tp->iexpr[i].lo );
            tp->range[i].up = evalexpr( tp->iexpr[i].up );
        }
        else if( tp->iexpr[i].lo == -1 ) {
            tp->range[i].lo = 0;
            tp->range[i].up = tp->iexpr[i].up;
        }
        else
            tp->range[i].lo = tp->range[i].up = evalexpr( tp->iexpr[i].up );
}

/*------------------------------------------------------------------------*/
/*       tbldimchk: check index ranges for two table references           */
/*------------------------------------------------------------------------*/
bool CIntDriver::tbldimchk( TBL_NODE *t1, TBL_NODE *t2 ) {
    int     n1, dim1[3], n2, dim2[3], i, d, i1[3];
    int     t;
    CTAB    *ltp, *rtp;

    ltp = XPT(t1->tbl_index);
    rtp = XPT(t2->tbl_index);

    for( i = n1 = 0; i < 3 && t1->range[i].lo >= 0; i++ ) {
        /* RHF 15/7/94 */
        t = ltp->GetTotDim(i);
        if( t != 0 && t1->range[i].up >= t || t1->range[i].lo < 0 ) {
            issaerror( MessageType::Warning, 1031, _T("left"), ltp->GetName().c_str() );
            return( false );
        }
        /* RHF 15/7/94 */

        if( ( d = t1->range[i].up - t1->range[i].lo ) > 0 ) {
            i1[n1] = i;
            dim1[n1++] = d;
        }
        else if( d < 0 ) {
            issaerror( MessageType::Warning, 1031, _T("left"), NPT(t1->tbl_index)->GetName().c_str() );
            return( false );
        }
    }

    for( i = 0; i < 3; i++ )
        t2->left_idx[i] = -1;

    for( i = n2 = 0; i < 3 && t2->range[i].lo >= 0; i++ ) {
        /* RHF 15/7/94 */
        t = rtp->GetTotDim(i);
        if( t != 0 && t2->range[i].up >= t || t2->range[i].lo < 0 ) {
            issaerror( MessageType::Warning, 1031, _T("right"), rtp->GetName().c_str() );
            return( false );
        }
        /* RHF 15/7/94 */

        if( ( d = t2->range[i].up - t2->range[i].lo ) > 0 ) {
            t2->left_idx[i] = i1[n2];
            dim2[n2++] = d;
        }
        else if( d < 0 ) {
            issaerror( MessageType::Warning, 1031, _T("right"), NPT(t2->tbl_index)->GetName().c_str() );
            return( false );
        }
    }

    if( n1 != n2 ) {
        issaerror( MessageType::Warning, 1034, NPT(t1->tbl_index)->GetName().c_str(), NPT(t2->tbl_index)->GetName().c_str() );
        return( false );
    }

    for( i = 0; i < n1; i++ )
        if( dim1[i] != dim2[i] ) {
            issaerror( MessageType::Warning, 1032, NPT(t1->tbl_index)->GetName().c_str(), NPT(t2->tbl_index)->GetName().c_str() );
            return( false );
        }

        return( true );
}

/*------------------------------------------------------------------------*/
/*       extblsum: execute tblsum function                                */
/*------------------------------------------------------------------------*/
double CIntDriver::extblsum( int iExpr ) {
    TBL_CPT_NODE* pfun=(TBL_CPT_NODE *)(PPT(iExpr));

    int     errorflg, axis, axis1, axis2,
        nmin, nmin1, nmin2,
        nmax, nmax1, nmax2,
        lindex[3], rindex[3], rl[3], j;
    double  value, s;
    CTAB    *ltp, *rtp;
    TBL_NODE *tleft, *tright;
    TBL_FUN_NODE *right;

    tleft = (TBL_NODE *) (PPT(pfun->tbl_left));
    ltp = XPT(tleft->tbl_index);        /* pointer to left matrix */

    right = (TBL_FUN_NODE *) (PPT(pfun->tbl_expr));
    axis = right->vardim - 1;           /* inner axis */
    tright = (TBL_NODE *) (right + 1);
    rtp = XPT(tright->tbl_index);       /* pointer to right matrix */

    /* RHF 16/1/95 */ lindex[0] = lindex[1]= lindex[2]=0;
    errorflg = tblsummed( tleft, tright, rl, lindex, axis );
    if( errorflg != 0 )
        return( (double) errorflg );

    /* setup remaining axis */
    axis1 = 1;                          /* middle: columns */
    axis2 = 2;                          /* outermost: layers */
    if( axis > 0 ) {                    /* if inner is cols or layers... */
        axis1 = 0;                      /* ... middle: rows */
        axis2 = 3 - axis;               /* ... outermost: layers or cols */
    }

    /* upper limit for each axis */
    nmax2 = tright->range[axis2].up + 1;
    if( nmax2 <= 0 ) nmax2 = 1;
    nmax1 = tright->range[axis1].up + 1;
    if( nmax1 <= 0 ) nmax1 = 1;
    nmax = tright->range[axis].up + 1;
    if( nmax <= 0 ) nmax = 1;

    /* lower limit for each axis */
    nmin2 = tright->range[axis2].lo;
    if( nmin2 < 0 ) nmin2 = 0;
    nmin1 = tright->range[axis1].lo;
    if( nmin1 < 0 ) nmin1 = 0;
    nmin = tright->range[axis].lo;

    /* main loop on the outermost axis */
    for( rindex[axis2] = nmin2; rindex[axis2] < nmax2; rindex[axis2]++ ) {
        /* reinitializes lower bound if needed */
        if( ( j = rl[axis1] ) < 3 )             /* VC Jan 16, 95 */
            lindex[j] = tleft->range[j].lo;     /* VC Jan 16, 95 */

        /* secondary loop on the middle axis */
        for( rindex[axis1] = nmin1; rindex[axis1] < nmax1; rindex[axis1]++ ) {
            /* loop on inner axis to get the sum */
            s = 0;
            for( rindex[axis] = nmin; rindex[axis] < nmax; rindex[axis]++ ) {
                // RHF COM Jul 31, 2001 value = tabvalue( rtp, rindex[0], rindex[1], rindex[2] );
                value = rtp->m_pAcum.GetDoubleValue( rindex[0], rindex[1], rindex[2] );
                if( !IsSpecial(value) )
                    s += value;
            }

            /* store the calculated sum */
            // RHF COM Jul 31, 2001 storetabv( ltp, lindex[0], lindex[1], lindex[2], s );
            ltp->m_pAcum.PutDoubleValue( s, lindex[0], lindex[1], lindex[2] );


            /* advance to next coordinate on the middle axis */
            if( ( j = rl[axis1] ) < 3 )
                lindex[j]++;
        }

        /* advance to next coordinate on outermost axis */
        if( ( j = rl[axis2] ) < 3 )
            lindex[j]++;
    }

    return( (double) 0 );
}

/*------------------------------------------------------------------------*/
/*       extblmed: execute tblmed function                                */
/*------------------------------------------------------------------------*/
double CIntDriver::extblmed( int iExpr ) {
    TBL_CPT_NODE* pfun=(TBL_CPT_NODE *)(PPT(iExpr));
    int     med_type,                   /* median type */   // RHF 7/5/97
        errorflg, axis, axis1, axis2,
        nmin, nmin1, nmin2,
        nmax, nmax1, nmax2,
        nlowers,
        lindex[3], rindex[3], rl[3], j,
        *phighest,
        use_intervals, use_lowers, use_highest,         /* VC Jun 26, 97 */
        i_low_cat, i_hig_cat, i_cat, max_used, in_one,  /* VC Jun 26, 97*/
        v_base, v_int0, v_int1, v_intx;                 /* VC Oct 10, 96 */
    double  median, value, max_lower, highest,
        s_tot, s50, s_cum, s_new, proportion = 0;
    CTAB    *ltp, *rtp;
    TBL_NODE *tleft, *tright;
    TBL_FUN_NODE *right;

    tleft = (TBL_NODE *) (PPT(pfun->tbl_left));
    ltp = XPT(tleft->tbl_index);        /* pointer to proper matrix */

    right = (TBL_FUN_NODE *) (PPT(pfun->tbl_expr));
    axis  = right->vardim - 1;          /* inner axis */
    phighest = (int *) ( right + 1 );
    highest  = ( *phighest < 0 ) ? -MAXVALUE : GetNumericConstant( *phighest );
    tright   = (TBL_NODE *) ( phighest + 4 );
    rtp = XPT(tright->tbl_index);       /* pointer to proper matrix */

    /* median calculation type: 0/no-parm equal to 1/continuous, 2/discrete */
    med_type = *(phighest + 3);         // RHF 7/5/97 Ver comptbl.c
    if( med_type < 0 || med_type > 2 )
        med_type = 0;                   /* 0-no parm/1-continuous/2-discrete */

    nlowers = *(phighest + 1);
    const int* plower = PPT(*(phighest + 2));

    use_lowers    = ( nlowers > 0 );
    use_highest   = ( *phighest >= 0 );
    use_intervals = ( use_lowers || use_highest );

    max_lower = ( use_lowers ) ? GetNumericConstant( plower[nlowers - 1] ) : -MAXVALUE;

    lindex[0] = lindex[1] = lindex[2] = 0;                  /* RHF 16/1/95 */
    errorflg = tblsummed( tleft, tright, rl, lindex, axis );
    if( errorflg != 0 )
        return( (double) errorflg );

    /* setup remaining axis */
    axis1 = 1;                          /* middle: columns */
    axis2 = 2;                          /* outermost: layers */
    if( axis > 0 ) {                    /* if inner is cols or layers... */
        axis1 = 0;                      /* ... middle: rows */
        axis2 = 3 - axis;               /* ... outermost: layers or cols */
    }

    /* upper limit for each axis */
    nmax2 = tright->range[axis2].up + 1;
    if( nmax2 <= 0 ) nmax2 = 1;
    nmax1 = tright->range[axis1].up + 1;
    if( nmax1 <= 0 ) nmax1 = 1;
    nmax = tright->range[axis].up + 1;
    if( nmax <= 0 ) nmax = 1;

    /* lower limit for each axis */
    nmin2 = tright->range[axis2].lo;
    if( nmin2 < 0 ) nmin2 = 0;
    nmin1 = tright->range[axis1].lo;
    if( nmin1 < 0 ) nmin1 = 0;
    nmin = tright->range[axis].lo;

    /* main loop on the outermost axis */
    for( rindex[axis2] = nmin2; rindex[axis2] < nmax2; rindex[axis2]++ ) {
        /* reinitializes lower bound if needed */
        if( ( j = rl[axis1] ) < 3 )
            lindex[j] = tleft->range[j].lo;

        /* secondary loop on the middle axis */
        for( rindex[axis1] = nmin1; rindex[axis1] < nmax1; rindex[axis1]++ ) {
            /* loop on inner axis to get the sum */
            value = 0;                  /* category' freq *//* VC Oct 10, 96 */
            s_tot = 0;                  /* cumulated sum */
            for( rindex[axis] = nmin; rindex[axis] < nmax; rindex[axis]++ ) {
                // RHF COM Jul 31, 2001 value = tabvalue( rtp, rindex[0], rindex[1], rindex[2] );
                value = rtp->m_pAcum.GetDoubleValue( rindex[0], rindex[1], rindex[2] );

                if( !IsSpecial(value) )
                    s_tot += value;
            }

            /* again a loop on inner axis to get the median point */
            s50 = s_tot / 2;            /* 50% of cumulated sum */
            i_low_cat = i_hig_cat = -1;
            for( rindex[axis] = nmin, s_cum = s_new = 0;
            rindex[axis] < nmax && s_new < s50;
            rindex[axis]++ ) {
                // RHF COM Jul 31, 2001 value = tabvalue( rtp, rindex[0], rindex[1], rindex[2] );
                value = rtp->m_pAcum.GetDoubleValue( rindex[0], rindex[1], rindex[2] );
                if( !IsSpecial(value) ) {  /* ignore special values */
                    s_cum = s_new;      /* previously cumulated */
                    s_new += value;     /* cumulated up to this point */
                    i_hig_cat = rindex[axis];
                }
            }

            /* calculate proportion */
            in_one = ( s_cum <= 0 && s_new >= s_tot );
            if( in_one ) {                /* median falls into one cat only */
                i_low_cat = i_hig_cat;
                proportion = 0;         /* ... just at that point */
            }
            else if( i_hig_cat >= nmin ) {
                i_low_cat = i_hig_cat - 1;
                proportion = ( s50 - s_cum ) / value;
            }

            /* setup v_intx (category value for coordinate of order intx) */
            v_int0 = v_int1 = v_base = -1;
            max_used = FALSE;           /* assume low-cat hasn't max value */
            if( i_low_cat >= nmin && i_hig_cat >= nmin ) {
                /* cat' value of the low-category is the basic value */
                v_intx = val_coord( rtp->GetNodeExpr(axis), (double) i_low_cat );
                i_cat = i_low_cat - nmin;
                if( !use_lowers )
                    v_int0 = v_intx;
                else {                     /* use lowers, maybe highest */
                    if( i_cat < nlowers )
                        v_int0 = GetNumericConstant( plower[i_cat] );
                    else if( !use_highest ) {
                        v_int0 = max_lower + 1;
                        max_used = TRUE;
                    }
                    else {                /* use highest */
                        v_int0 = highest;
                        max_used = !IsSpecial(highest);
                    }
                }

                /* cat' value of the hig-category is the basic value */
                v_intx = val_coord( rtp->GetNodeExpr(axis), (double) i_hig_cat );
                i_cat = i_hig_cat - nmin;
                if( !use_intervals ) {                     /* don't use highest nor lowers */
                    if( i_cat < nmax - 1 )
                        v_int1 = v_intx;
                    else
                        v_int1 = highest;
                }
                else if( !use_lowers ) {                     /* use highest only */
                    if( i_cat < nmax - 1 )
                        v_int1 = v_intx;
                    else
                        v_int1 = highest;
                }
                else {                     /* use lowers, maybe highest */
                    if( i_cat < nlowers )
                        v_int1 = GetNumericConstant( plower[i_cat] );
                    else if( !use_highest || max_used )
                        v_int1 = v_int0 + ( !in_one );
                    else
                        v_int1 = highest;
                }
            }
            /* if special values... */
            if( v_int0 >= 10000 || v_int1 >= 10000 )
                v_int0 = v_int1 = -1;

            /* final calculation - only if there is 2 points to interpolate */
            median = DEFAULT;
            if( !in_one && v_int0 >= 0 && v_int1 >= 0 ){
                /* setup "at base" point */
                if( med_type < 2 )      /* no-parm, continuous */
                    v_base = v_int1;
                else                    /* discrete */
                    v_base = v_int0;

                median = v_base + proportion * ( v_int1 - v_int0 );
            }

            /* store the calculated median */
            // RHF COM Jul 31, 2001 storetabv( ltp, lindex[0], lindex[1], lindex[2], median );
            ltp->m_pAcum.PutDoubleValue( median, lindex[0], lindex[1], lindex[2] );

            /* advance to next coordinate on the middle axis */
            if( ( j = rl[axis1] ) < 3 )
                lindex[j]++;
          }

          /* advance to next coordinate on outermost axis */
          if( ( j = rl[axis2] ) < 3 )
              lindex[j]++;
      }

      return( (double) 0 );
}

/*------------------------------------------------------------------------*/
/*       tblsummed: common checks for tblsum & tblmed functions           */
/*------------------------------------------------------------------------*/
int CIntDriver::tblsummed( TBL_NODE *tleft, TBL_NODE *tright, int *rl, int *lindex, int axis ) {
    int     errorflg, ldim, rdim, t, i, j;
    CTAB    *ltp, *rtp;

    ltp = XPT(tleft->tbl_index);
    rtp = XPT(tright->tbl_index);

    for( i = 0; i < 3; i++ )
        rl[i] = 3;

    tblsetlu( tleft );
    tblsetlu( tright );

    ldim = rdim = 0;

    for( i = 0; i < ltp->GetNumDim(); i++ ) {
        if( tleft->range[i].lo != tleft->range[i].up )
            ldim++;
    }

    for( i = 0; i < rtp->GetNumDim(); i++ ) {
        if( tright->range[i].lo != tright->range[i].up )
            rdim++;
    }

    if( ldim + 1 != rdim ) {
        issaerror( MessageType::Warning, 1033, ltp->GetName().c_str(), rtp->GetName().c_str() );
        errorflg = -3;
        return( errorflg );
    }

    for( i = 0, j = 0; i < 3; i++ ) {
        lindex[i] = tleft->range[i].lo;
        t = ltp->GetTotDim(i);
        if( ltp->GetTotDim(i) != 0 )
            if( tleft->range[i].up >= t ||
                tleft->range[i].lo < 0 ||
                tleft->range[i].lo > tleft->range[i].up ) {
                issaerror( MessageType::Warning, 1031, _T("left"), ltp->GetName().c_str() );
                errorflg = -1;
                return( errorflg );
            }
            t = rtp->GetTotDim(i);
            if( rtp->GetTotDim(i) != 0 )
                if( tright->range[i].up >= t ||
                    tright->range[i].lo < 0 ||
                    tright->range[i].lo > tright->range[i].up ) {
                    issaerror( MessageType::Warning, 1031, _T("right"), rtp->GetName().c_str() );
                    errorflg = -1;
                    return( errorflg );
                }

                if( tright->range[i].lo != tright->range[i].up && i != axis ) {
                    for( ; tleft->range[j].lo == tleft->range[j].up && j < 3; j++ )
                        ;
                    rl[i] = j;
                    if( j < 3 ) /* RHF 16/1/95 */
                        if( tleft->range[j].up - tleft->range[j].lo !=
                            tright->range[i].up - tright->range[i].lo ) {
                            issaerror( MessageType::Warning, 1032, ltp->GetName().c_str(), rtp->GetName().c_str() );
                            errorflg = -2;
                            return( errorflg );
                        }
                        j++;
                }
    }

    return( 0 );
}

/*------------------------------------------------------------------------*/
/*  Crosstab coordinate functions: tblrow, tblcol, tlblay                 */
/*------------------------------------------------------------------------*/

double CIntDriver::tblcoord( int iExpr, int iDim ) {
    FNTC_NODE*  pNode=(FNTC_NODE *) (PPT(iExpr));
    CTAB* pCtab= XPT(pNode->iCtab);

    if( pNode->iNewMode == 0 )
        return oldtblcoord( iExpr, iDim );

    ASSERT( pNode->iSubTableNum >= 1 );

    CSubTable&  cSubTable=pCtab->GetSubTable( pNode->iSubTableNum - 1 );

    ASSERT( pNode->iCatValues[0] > 0 );

    CCoordValue cCoordValue;

    cCoordValue.m_iCtNodeLeft = cSubTable.m_iCtTree[iDim][0];
    cCoordValue.m_iCtNodeRight = cSubTable.m_iCtTree[iDim][1];
    cCoordValue.m_iSeqLeft = pNode->iCatValuesSeq[0];
    cCoordValue.m_iSeqRight = pNode->iCatValuesSeq[1];

    if( pNode->iCatValues[1] == 0 ) {
        cCoordValue.m_dValueLeft = evalexpr( pNode->iCatValues[0] );
        cCoordValue.m_dValueRight = NOTAPPL;
    }
    else {
        cCoordValue.m_dValueLeft = evalexpr( pNode->iCatValues[0] );
        cCoordValue.m_dValueRight = evalexpr( pNode->iCatValues[1] );
    }

    int         ctvector[MAX_TALLYCELLS+1];

    CtPos( pCtab, pCtab->GetNodeExpr(iDim), ctvector, NULL, &cCoordValue, false );
    //ASSERT( ctvector[0] == 0 || ctvector[0] == 1 );

    // Only 1 value must be found
    int     iFound=-1;
    for( int i=0; i < ctvector[0]; i++ ) {
        if( ctvector[i+1] >= 0 ) {
            ASSERT( iFound == -1 );
            iFound = i + 1;
#ifdef _DEBUG
                ;
#else
            break;
#endif
        }
    }


    //return( (double) (ctvector[0] >= 1 && ctvector[1] >= 0) ? ctvector[1] : DEFAULT );
    return( (double) (iFound>=0) ?  ctvector[iFound] : DEFAULT );
}


double CIntDriver::oldtblcoord( int iExpr, int iDim ) {
    OLD_FNTC_NODE*  pnode=(OLD_FNTC_NODE *) (PPT(iExpr));
    int     relcoord, relc1, relc2;
    CTAB    *ct;
    CTTERM  *pctt, *pctterm, *pctv1, *pctv2;
    CTNODE  *pctn;

    ct = XPT(pnode->ict);
    pctt = ct->GetTerm(iDim);

    if( pnode->var2 == -1 ) {             // Single variable term
        pctterm = pctt + pnode->var1;
        pctn = CTN( pctterm->m_iCtNode );
        relcoord = ctvarpos( pctterm->m_iCtNode, evalexpr( pnode->expr1 ) );
        if( relcoord < 0 )
            return( DEFAULT );
    }
    else {  // Compound term var * ( var + ... + var )
        pctterm = pctt + pnode->nterm - 1;
        pctv1 = pctt + pnode->var1;
        pctv2 = pctt + pnode->var2;
        relc1 = ctvarpos( pctv1->m_iCtNode, evalexpr( pnode->expr1 ) );
        relc2 = ctvarpos( pctv2->m_iCtNode, evalexpr( pnode->expr2 ) );
        if( relc1 < 0 || relc2 < 0 )
            return( DEFAULT );
        relcoord = relc2 + pctv2->m_iPrevCells +
            relc1 * CTN( CTN( pctterm->m_iCtNode )->m_iCtRight )->m_iNumCells;
    }

    return( (double) relcoord + pctterm->m_iPrevCells );
}

/*------------------------------------------------------------------------*/
/*  Crosstab value-of-coordinates: support to tblmed        VC Oct 10, 96 */
/*------------------------------------------------------------------------*/
/* val_coord: setup v_intx (category value for coordinate of order intx)  */
/* - call with                                                            */
/*            val_coord( ct->nodexpr[dim], i_coord );                     */
/*      where                                                             */
/*            dim    : 0-row, 1-col, 2-layer                              */
/*            i_coord: coordenada para la que se busca el valor           */
/* - private data:                                                        */
static  double  look_Coord;             /* coordinate to search for       */
static  int     prev_Cats,              /* categories already examined    */
                *ct_Tree;               /* a copy of CtNodebase           */
static double   ret_Value;              // value of searched category
static double   hi_Value;               // high value of most recent interval    // BMD 13 Oct 2005
/* - private method:                                                      */
//static  void    _val_coord( int );

double CIntDriver::val_coord( int i_node, double i_coord ) {
    ret_Value = -1;                     /* -1: not found */
    if( i_coord >= 0 && i_coord < 10000 ) {
        look_Coord = i_coord;
        prev_Cats = 0;

        ct_Tree = CtNodebase;
        _val_coord( i_node );
    }

    return( ret_Value );
}

double CIntDriver::val_high( void ) {     // BMD 13 Oct 2005
    return (hi_Value);
}

void CIntDriver::_val_coord( int i_node ) {
    int     range_cats, ini_range, end_range, i;
    CTNODE  *pnode, *pnodeleft;
    CTRANGE *prange;

    pnode = (CTNODE *)(ct_Tree + i_node);

    if( pnode->isVarNode() ) // look inside ranges of this member (the coordinate-variable)
    {
        ini_range = i_node + CTNODE_SLOTS;
        end_range = ini_range + CTRANGE_SLOTS * pnode->m_iCtNumRanges;
        for( i = ini_range; ret_Value < 0 && i < end_range; i += CTRANGE_SLOTS ) {
            prange = (CTRANGE *) (ct_Tree + i);

            /* # of categories in this range */
            // RHF COM Jul 11, 2002 range_cats = prange->m_iRangeHigh - prange->m_iRangeLow + 1;

            // RHF INIC Jul 11, 2002  // modified by RCL, Jun 2005
            range_cats = prange->getNumCells();
            // RHF END Jul 11, 2002

            if( look_Coord >= prev_Cats &&
                look_Coord < prev_Cats + range_cats ) {
                ret_Value = prange->m_iRangeLow + ( look_Coord - prev_Cats );
                hi_Value = prange->m_iRangeHigh + ( look_Coord - prev_Cats );   // 13 Oct 2005
            }

            /* PENDIENTE: si el valor de la categoria es special...   */
            /*                10001 para MISSING                      */
            /*                10002 para NOTAPPL                      */
            /*                10003 para DEFAULT                      */
            /* - referencia: ct_cattext en 'ct_struc.c'               */

            prev_Cats += range_cats;/* categories already examined    */
        }
    }
    else
    {
        ASSERT( pnode->isOperNode() );
        switch( pnode->getOperator() )
        {
        case CTNODE_OP_ADD:
            /* left-leaf is a multiplier, and it is not searched for */
            pnodeleft = (CTNODE *) (ct_Tree + pnode->m_iCtLeft);

            /* look inside each real leaf of this node */
            for( i = 0; ret_Value < 0 && i < pnodeleft->m_iNumCells; i++ )
                _val_coord( pnode->m_iCtRight );
            break;
        case CTNODE_OP_MUL:
            /* left and right leafs are searched for */
            if( ret_Value < 0 )
                _val_coord( pnode->m_iCtLeft );
            if( ret_Value < 0 )
                _val_coord( pnode->m_iCtRight );
            break;
        default:
            break;
        }
    }
}
