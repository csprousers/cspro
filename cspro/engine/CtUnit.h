#pragma once

//---------------------------------------------------------------------------
//  File name: CtUnit.h
//
//  Description:
//          Header for Crosstab Unit
//
//  History:    Date       Author   Comment
//              ---------------------------
//              10 Jun 02  RHF     Creation
//              29 Nov 04  rcl     Extensions for 3d handling
//
//---------------------------------------------------------------------------

#include <engine/Defines.h>
#include <ZTBDO/cttree.h>
class CtStat;

#include <ZTBDO/ctstadef.h>


class CtUnit {
private:
    int                         m_iNumber;
    int                         m_iOwnerCtabSymbol;
    bool                        m_bInGlobal;

    CArray<int,int>             m_aSubTableIndex;
    int                         m_iUnitSymbol;
    int                         m_iLoopSymbol;
    int                         m_iCurrentIndex;
    int                         m_iSelectExpr;
    int                         m_iWeightExpr;
    int                         m_iTabLogicExpr; // RHF Jul 14, 2005
    int                         m_iUnitType;
    bool                        m_bDefaultUnit; // RHF Aug 06, 2005

private:
    CMap<int,int,int,int>       m_aCtCoordNumber;

public:
    CtUnit() {
        Init();
    }


    CtUnit( CtUnit& ctUnit ) {
        Copy( ctUnit );
    }

    void operator=( CtUnit& ctUnit ) {
        if( &ctUnit != this )  // rcl, Nov 2004
            Copy( ctUnit );
    }

    void Copy( CtUnit& ctUnit );
    void Init();

    void        SetDefaultUnit( bool bDefaultUnit )         { m_bDefaultUnit = bDefaultUnit; }
    bool        GetDefaultUnit()                            { return m_bDefaultUnit; }
    void        SetNumber( int iNumber )                    { m_iNumber = iNumber; }
    void        SetOwnerCtabSymbol( int iOwnerCtabSymbol )  { m_iOwnerCtabSymbol = iOwnerCtabSymbol; }
    void        SetInGlobal( bool bInGlobal )               { m_bInGlobal = bInGlobal; }



    void        SetUnitSymbol( int iUnitSymbol, int iUnitType ) {
        m_iUnitSymbol = iUnitSymbol;
        m_iUnitType = iUnitType;
    }

    void        SetLoopSymbol( int iLoopSymbol )            { m_iLoopSymbol = iLoopSymbol; }
    void        SetCurrentIndex( int iIndex )               { m_iCurrentIndex = iIndex; }
    void        SetSelectExpr( int iSelectExpr )               { m_iSelectExpr = iSelectExpr; }
    void        SetWeightExpr( int iWeightExpr )               { m_iWeightExpr = iWeightExpr; }
    void        SetTabLogicExpr( int iTabLogicExpr )            { m_iTabLogicExpr = iTabLogicExpr; }

    int         GetNumber()             { return m_iNumber; }
    int         GetOwnerCtabSymbol()    { return m_iOwnerCtabSymbol; }
    bool        InGlobal()              { return m_bInGlobal; }
    int         GetUnitSymbol()         { return m_iUnitSymbol; }
    int         GetLoopSymbol()         { return m_iLoopSymbol; }
    int         GetCurrentIndex()       { return m_iCurrentIndex; }
    int         GetSelectExpr()         { return m_iSelectExpr; }
    int         GetWeightExpr()         { return m_iWeightExpr; }
    int         GetTabLogicExpr()       { return m_iTabLogicExpr; }
    int         GetUnitType()           { return m_iUnitType; }


    // m_aSubTableIndex
    void        AddSubTableIndex( int iSubTableIndex )          { m_aSubTableIndex.Add( iSubTableIndex ); }
    int         GetNumSubTableIndex()                           { return m_aSubTableIndex.GetSize(); }
    int         GetSubTableIndex( int iSubTableIndex )          { return m_aSubTableIndex.ElementAt( iSubTableIndex); }

    // m_aCtCoordNumber
    void        MakeCoordNumberMap( CTAB* pCTab, int* pNodeBase[TBD_MAXDIM] );
    bool        UseCoordNumber( int iCoordNumber );

    void        CleanVectorItems( CTAB* pCtab, CEngineArea* m_pEngineArea);

    bool        HasSubTable( int iSubTable );

//////////////////////////////////////////////////////////////////////////
// 3d Extensions
// rcl, Nov 2004

private:
    bool        m_bUseExtraInfo;
    int         m_iExtraInfoNode; // MVAR_NODE, GRP_NODE
public:
    bool        isUsingExtraInfo() { return m_bUseExtraInfo; }
    int         getExtraInfoNode() { return m_iExtraInfoNode; }
    void        setExtraInfo( int iExtraInfoNode )
    {
        m_iExtraInfoNode = iExtraInfoNode;
        m_bUseExtraInfo = true;
    }
    void        removeExtraInfo() { m_bUseExtraInfo = false; }
};
