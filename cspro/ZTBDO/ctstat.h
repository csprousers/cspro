#pragma once
//---------------------------------------------------------------------------
//  File name: CtStat.h
//
//  Description:
//          Header for CtStat class
//          This class keep a crosstab Stat
//
//  History:    Date       Author   Comment
//              ---------------------------
//              01 Jul 02   RHF     Created
//
//---------------------------------------------------------------------------
#include <ZTBDO/zTbdO.h>
#include <ZTBDO/ctstatv.h>

#ifndef USE_BINARY // IGNORE_CTAB

#include <ZTBDO/subrange.h>
#include <ZTBDO/cttree.h>
#include <ZTBDO/ctstadef.h>


class CLASS_DECL_ZTBDO CtStatBase {
protected:
    CStatType           m_eStatType;
    int                 m_iSubCells;

public:
    CtStatBase();
    virtual ~CtStatBase();

    // JH 5/30/06 if this stat uses intervals, no by default
    // override in derived classes (median and ptile)
    virtual bool GetHasIntervals() {return false;}
    CStatType   GetStatType();
    CString     GetStatName();
    void        SetSubCells( int iSubCells );
    int         GetSubCells();
    static CString   GetStatName(CStatType iStatType);
};

class CLASS_DECL_ZTBDO CtStatTotal : public CtStatBase {
public:
    CtStatTotal();
    ~CtStatTotal();
};

class CLASS_DECL_ZTBDO CtStatFreq : public CtStatBase {
public:
     CtStatFreq();
     ~CtStatFreq();
};


class CLASS_DECL_ZTBDO CtStatPercent : public CtStatBase {
private:
    int                 m_iPctType; // CTNODE_STAT_PERCENT Bits
public:
     CtStatPercent();
     ~CtStatPercent();

     void           SetPctType( int iPctType );
     int            GetPctType();
};

class CLASS_DECL_ZTBDO CtStatProp : public CtStatBase {
private:
    int                         m_iPropType; // CTNODE_STAT_PROP Bits
    CArray<CSubRange,CSubRange> m_aPropRanges; // No elements if not used.
public:
     CtStatProp();
     CtStatProp( CtStatProp& rCtStatProp );
     ~CtStatProp();
     void Copy( CtStatProp& rCtStatProp );
     void AddRange( CSubRange& rSubRange );
     int    GetNumRanges();
     CSubRange& GetRange( int i );
     void       RemoveAllRanges();

     void           SetPropType( int iPropType );
     int            GetPropType();
};

class CLASS_DECL_ZTBDO CtStatMin : public CtStatBase {
public:
     CtStatMin();
     ~CtStatMin();
};

class CLASS_DECL_ZTBDO CtStatMax : public CtStatBase {
public:
     CtStatMax();
     ~CtStatMax();
};

class CLASS_DECL_ZTBDO CtStatMode : public CtStatBase {
public:
     CtStatMode();
     ~CtStatMode();
};

class CLASS_DECL_ZTBDO CtStatMean : public CtStatBase {
public:
     CtStatMean();
     ~CtStatMean();
};

// JH 5/30/06
// Base class for stats that use intervals (median and ptile)
class CLASS_DECL_ZTBDO CtStatIntervals : public CtStatBase {
private:
    std::vector<double> m_aIntervals;
public:
     std::vector<double>& GetIntervals();

    // if this stat uses intervals, no by default
    // override in derived classes (median and ptile)
    virtual bool GetHasIntervals();
};
class CLASS_DECL_ZTBDO CtStatMedian : public CtStatIntervals {
private:
    //int     m_iMedianType; //0:Normal, 1:Continuos, 2:Discrete
    int     m_iMedianType; //0:Normal, 1:Lower, 2:Upper
public:
     CtStatMedian();
     ~CtStatMedian();

     void   SetMedianType( int iMedianType );
     int    GetMedianType();

};

class CLASS_DECL_ZTBDO CtStatPTile : public CtStatIntervals {
private:
    int     m_iNumTiles;
    int     m_iPTileType; //0:Normal, 1:Lower, 2:Upper
    bool    m_bInterPol;

public:
     CtStatPTile();
     ~CtStatPTile();
     void   SetNumTiles( int iNumTiles );
     int    GetNumTiles();
     void   SetInterPol( bool bInterPol );
     bool   GetInterPol();

     void   SetPTileType( int iPTileType );
     int    GetPTileType();


};

class CLASS_DECL_ZTBDO CtStatStdDev : public CtStatBase {
public:
     CtStatStdDev();
     ~CtStatStdDev();
};

class CLASS_DECL_ZTBDO CtStatVariance : public CtStatBase {
public:
    CtStatVariance();
    ~CtStatVariance();
};

class CLASS_DECL_ZTBDO CtStatStdErr : public CtStatBase {
public:
    CtStatStdErr();
    ~CtStatStdErr();
};

class CLASS_DECL_ZTBDO CtStatVPct : public CtStatBase {
public:
    CtStatVPct();
    ~CtStatVPct();
};

// Stat class
class CLASS_DECL_ZTBDO CtStat {
private:
    int         m_iNumber;
    int         m_iOwnerCtabSymbol;
    bool        m_bInGlobal;

    CArray<CtStatVar,CtStatVar>                     m_aStatVar; // Example: A*(B+C) generate 2 elements when we are referring to A.
    CArray<CtStatBase*,CtStatBase*>                 m_aStatBase;
public:
    CtStat();
    ~CtStat();

    void        SetNumber( int iNumber );
    int         GetNumber();

    void        SetOwnerCtabSymbol( int iOwnerCtabSymbol );
    int         GetOwnerCtabSymbol();

    void        SetInGlobal( bool bInGlobal );
    bool        InGlobal();

    //m_aStatVar
    void        AddStatVar( CtStatVar&  rCtStatVar );
    int         GetNumStatVar();
    CtStatVar&  GetStatVar( int iStatVar );
    void        RemoveAllStatVar();


    //m_aStatBase
    void        AddStat( CtStatBase*  pStatBase );
    int         GetNumStat();
    CtStatBase*  GetStat( int iStat );
    void        RemoveAllStat();
};

#endif
