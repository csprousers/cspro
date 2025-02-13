// ctstat.cpp: implementation of the CtStat class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ctstat.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


CtStat::CtStat()
{
    SetNumber(-1);
    SetOwnerCtabSymbol(-1);
    SetInGlobal( false );
    RemoveAllStatVar();
    RemoveAllStat();
}

CtStat::~CtStat()
{
    RemoveAllStat();
}

void        CtStat::SetNumber( int iNumber )                    { m_iNumber = iNumber; }
int         CtStat::GetNumber()             { return m_iNumber; }

void        CtStat::SetOwnerCtabSymbol( int iOwnerCtabSymbol )  { m_iOwnerCtabSymbol = iOwnerCtabSymbol; }
int         CtStat::GetOwnerCtabSymbol()    { return m_iOwnerCtabSymbol; }

void        CtStat::SetInGlobal( bool bInGlobal )               { m_bInGlobal = bInGlobal; }
bool        CtStat::InGlobal()              { return m_bInGlobal; }

void        CtStat::AddStatVar( CtStatVar&  rCtStatVar )        { m_aStatVar.Add( rCtStatVar ); }
int         CtStat::GetNumStatVar()                             { return m_aStatVar.GetSize(); }
CtStatVar&  CtStat::GetStatVar( int iStatVar )                  { return m_aStatVar.ElementAt( iStatVar ); }
void        CtStat::RemoveAllStatVar()                          { m_aStatVar.RemoveAll(); }


void        CtStat::AddStat( CtStatBase*  pStatBase )            { m_aStatBase.Add( pStatBase ); }
int         CtStat::GetNumStat()                                { return m_aStatBase.GetSize(); }
CtStatBase* CtStat::GetStat( int i )                            { return m_aStatBase.GetAt(i); }
void        CtStat::RemoveAllStat()                             {
    for( int i=0; i < GetNumStat(); i++ ) {
        CtStatBase* pStatBase=GetStat(i);
        delete pStatBase;
    }

    m_aStatBase.RemoveAll();
 }


// StatBase
CStatType CtStatBase::GetStatType() { return m_eStatType; }
CtStatBase::CtStatBase() {
    m_iSubCells = 1;
    m_eStatType = CTSTAT_NONE;
}

void        CtStatBase::SetSubCells( int iSubCells ) { m_iSubCells = iSubCells; }
int         CtStatBase::GetSubCells() { return m_iSubCells; }

CtStatBase::~CtStatBase() {}
CString CtStatBase::GetStatName() {
    return GetStatName( GetStatType() );
}

CString CtStatBase::GetStatName(CStatType eStatType) {
    CString csName;

    switch( eStatType ) {
    case CTSTAT_NONE:
        csName = _T("None");
        break;
    case CTSTAT_OVERLAPPED:
        csName = _T("Overlapped");
        break;
    case CTSTAT_FREQ:
        csName = _T("Freq");
        break;
    case CTSTAT_TOTAL:
        csName = _T("Total");
        break;
    case CTSTAT_PERCENT:
        csName = _T("Percent");
        break;
    case CTSTAT_PROP:
        csName = _T("Prop");
        break;
    case CTSTAT_MIN:
        csName = _T("Min");
        break;
    case CTSTAT_MAX:
        csName = _T("Max");
        break;
    case CTSTAT_MODE:
        csName = _T("Mode");
        break;
    case CTSTAT_MEAN:
        csName = _T("Mean");
        break;
    case CTSTAT_MEDIAN:
        csName = _T("Median");
        break;
    case CTSTAT_PTILE:
        csName = _T("Ptile");
        break;
    case CTSTAT_STDEV:
        csName = _T("Stddev");
        break;
    case CTSTAT_VARIANCE:
        csName = _T("Variance");
        break;
    case CTSTAT_STDERR:
        csName = _T("StdErr");
        break;
    case CTSTAT_VPCT:
        csName = _T("Vpct");
        break;
    default:
        ASSERT(0);
        break;
    }

    return csName;
}


// Freq
CtStatFreq::CtStatFreq() { m_eStatType = CTSTAT_FREQ; }
CtStatFreq::~CtStatFreq() {}


// Total
CtStatTotal::CtStatTotal() { m_eStatType = CTSTAT_TOTAL; }
CtStatTotal::~CtStatTotal() {}


// Percent
CtStatPercent::CtStatPercent() {
    m_eStatType = CTSTAT_PERCENT;
    m_iPctType = 0;
}
CtStatPercent::~CtStatPercent() {}

void  CtStatPercent::SetPctType( int iPctType )  { m_iPctType = iPctType; }
int   CtStatPercent::GetPctType()                { return m_iPctType; }


// Prop
CtStatProp::CtStatProp() {
    m_eStatType = CTSTAT_PROP;
    SetPropType( 0 );
    RemoveAllRanges();
}

CtStatProp::~CtStatProp() {
    RemoveAllRanges();
}

CtStatProp::CtStatProp( CtStatProp& rCtStatProp ) {
    Copy( rCtStatProp );
}

void CtStatProp::Copy( CtStatProp& rCtStatProp ) {
    m_eStatType = rCtStatProp.GetStatType();
    m_iPropType = rCtStatProp.GetPropType();

    RemoveAllRanges();
    for( int i=0; i < rCtStatProp.GetNumRanges(); i++ ) {
        CSubRange&      rSubRange=rCtStatProp.GetRange(i);
        AddRange(rSubRange);
    }
}

void CtStatProp::AddRange( CSubRange& rSubRange ) {
    m_aPropRanges.Add( rSubRange );
}
int    CtStatProp::GetNumRanges() { return m_aPropRanges.GetSize(); }
CSubRange& CtStatProp::GetRange( int i ) { return m_aPropRanges.ElementAt(i); }
void       CtStatProp::RemoveAllRanges() {m_aPropRanges.RemoveAll(); }


void           CtStatProp::SetPropType( int iPropType )    { m_iPropType = iPropType; }
int            CtStatProp::GetPropType()                   { return m_iPropType; }


// Min
CtStatMin::CtStatMin() { m_eStatType = CTSTAT_MIN; }
CtStatMin::~CtStatMin() {}

// Max
CtStatMax::CtStatMax() { m_eStatType = CTSTAT_MAX; }
CtStatMax::~CtStatMax() {}

// Mode
CtStatMode::CtStatMode() { m_eStatType = CTSTAT_MODE; }
CtStatMode::~CtStatMode() {}

// JH 5/30/06 base class for stats with intervals (median, ptile)
std::vector<double>& CtStatIntervals::GetIntervals() {return m_aIntervals;}
bool CtStatIntervals::GetHasIntervals()
{
    return !m_aIntervals.empty();
}

// Mean
CtStatMean::CtStatMean() { m_eStatType = CTSTAT_MEAN; }
CtStatMean::~CtStatMean() {}

// Median
CtStatMedian::CtStatMedian() {
    m_eStatType = CTSTAT_MEDIAN;
    SetMedianType(0); // RHF Sep 12, 2005
}
CtStatMedian::~CtStatMedian() {}

void   CtStatMedian::SetMedianType( int iMedianType )   { m_iMedianType = iMedianType; }
int    CtStatMedian::GetMedianType()                    { return m_iMedianType; }


// pTile
CtStatPTile::CtStatPTile() {
    m_eStatType = CTSTAT_PTILE;
    SetNumTiles(0);
    SetInterPol( false );
    SetPTileType(0);
}
CtStatPTile::~CtStatPTile() {}

void   CtStatPTile::SetNumTiles( int iNumTiles )    { m_iNumTiles = iNumTiles; }
int    CtStatPTile::GetNumTiles()               { return m_iNumTiles; }

void   CtStatPTile::SetInterPol( bool bInterPol )    { m_bInterPol =  bInterPol; }
bool   CtStatPTile::GetInterPol()               { return m_bInterPol; }

void   CtStatPTile::SetPTileType( int iPTileType )   { m_iPTileType = iPTileType; }
int    CtStatPTile::GetPTileType()                    { return m_iPTileType; }



// StdDev
CtStatStdDev::CtStatStdDev() { m_eStatType = CTSTAT_STDEV; }
CtStatStdDev::~CtStatStdDev() {}

// Variance
CtStatVariance::CtStatVariance() { m_eStatType = CTSTAT_VARIANCE; }
CtStatVariance::~CtStatVariance() {}

// StdErr
CtStatStdErr::CtStatStdErr() { m_eStatType = CTSTAT_STDERR; }
CtStatStdErr::~CtStatStdErr() {}

// VPct
CtStatVPct::CtStatVPct() { m_eStatType = CTSTAT_VPCT; }
CtStatVPct::~CtStatVPct() {}
