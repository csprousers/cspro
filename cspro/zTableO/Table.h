#pragma once

//***************************************************************************
//  File name: Table.h
//
//  Description:
//       Table objects prototypes
//
//  History:    Date       Author     Comment
//              -----------------------------
//              05 Dec 02   BMD       CSPro 3.0
//              Nov 2004    csc+savy  reimplementation
//
//***************************************************************************

#include <zTableO/zTableO.h>
#include <zTableO/Style.h>
#include <engine/ttype.h>

class DictLevel;
class DictRelation;
class DictValue;
class ProgressDlg;


//Savy (R) sampling app 20090211
enum SAMPLING_ANALYSIS_TYPE {ANALYSIS_TYPE_NONE=0, TOTALS_ANALYSIS_TYPE, MEANS_ANALYSIS_TYPE, RATIOS_ANALYSIS_TYPE, PROPORTIONS_ANALYSIS_TYPE };
//Savy (R) sampling app 20081216
enum SAMPLING_METHOD_TYPE {METHOD_NONE=0, TAYLOR_SERIES_METHOD, JACK_KNIFE_METHOD, BRRP_METHOD };
enum SPECTYPE { CROSSTAB_SPEC, FREQ_SPEC};
enum TVARTYPE { VT_ROOT=0, VT_DICT, VT_SOURCE, VT_CUSTOM };
enum TABVAL_TYPE {INVALID_TABVAL=0,DICT_TABVAL,SPECIAL_TABVAL,
        SPECIAL_MISSING_TABVAL,SPECIAL_NOTAPPL_TABVAL,
        SPECIAL_DEFAULT_TABVAL,SPECIAL_UNDEFINED_TABVAL,
        RDRBRK_TABVAL,STAT_TOTAL_TABVAL,STAT_MIN_TABVAL,STAT_MAX_TABVAL,STAT_MEAN_TABVAL,
        STAT_MODE_TABVAL,STAT_STDDEV_TABVAL,STAT_STDERR_TABVAL,STAT_MEDIAN_TABVAL,
        STAT_VARIANCE_TABVAL,STAT_PCT_TABVAL,STAT_NTILE_TABVAL,STAT_NROW_TABVAL,
        STAT_PROPORTION_RATIO_TABVAL, STAT_PROPORTION_PERCENT_TABVAL, STAT_PROPORTION_TOTAL_TABVAL,
        STAT_SAMPLING_R_STABVAL, STAT_SAMPLING_SE_STABVAL, STAT_SAMPLING_N_UNWE_STABVAL, STAT_SAMPLING_N_WEIG_STABVAL, STAT_SAMPLING_SER_STABVAL, STAT_SAMPLING_SD_STABVAL,
        STAT_SAMPLING_DEFT_STABVAL, STAT_SAMPLING_ROH_STABVAL, STAT_SAMPLING_SE_R_STABVAL,
        STAT_SAMPLING_R_N2SE_STABVAL, STAT_SAMPLING_R_P2SE_STABVAL, STAT_SAMPLING_SAMP_BASE_STABVAL,
        STAT_SAMPLING_B_STABVAL, //Savy (R) sampling app 20081128
        SPECIAL_REFUSED_TABVAL };

enum XTABSTMENT_TYPE{XTABSTMENT_ALL =0,XTABSTMENT_WGHT_ONLY =1,XTABSTMENT_UNIV_ONLY =2, XTABSTMENT_UNITS_ONLY=3,
XTABSTMENT_TABLOGIC_ONLY,XTABSTMENT_POSTCALC_ONLY,XTABSTMENT_BASIC};

const CString AREA_TOKEN            = _T("%AreaName%");
const CString WORKVAR_TOTAL_NAME    = _T("SYSTEM_TOTAL");
const CString WORKVAR_TOTAL_LABEL   = _T("System Total");
const CString WORKVAR_RECORD_NAME   = _T("SYSTEM_REC");
const CString WORKVAR_RECORD_LABEL  = _T("System Record");

const CString TOTAL_LABEL           = _T("Total");

const CString WORKVAR_STAT_NAME     = _T("SYSTEM_STAT");
const CString WORKVAR_STAT_LABEL    = _T("System Statistics");

class CustSpecialValSettings {
public:
    CustSpecialValSettings()
        : m_bUseCustomSpecVal(false),
          m_bUseSpecValNotAppl(false),
          m_bUseSpecValMissing(false),
          m_bUseSpecValRefused(false),
          m_bUseSpecValDefault(false),
          m_bUseSpecValUndefined(false)
    {
    }

    bool GetUseCustomSpecVal() const    { return m_bUseCustomSpecVal;}
    bool GetUseSpecValNotAppl() const   { return m_bUseSpecValNotAppl; }
    bool GetUseSpecValMissing() const   { return m_bUseSpecValMissing; }
    bool GetUseSpecValRefused() const   { return m_bUseSpecValRefused; }
    bool GetUseSpecValDefault() const   { return m_bUseSpecValDefault; }
    bool GetUseSpecValUndefined() const { return m_bUseSpecValUndefined; }
    void SetUseSpecValNotAppl(bool b)   { m_bUseSpecValNotAppl = b; }
    void SetUseSpecValMissing(bool b)   { m_bUseSpecValMissing = b; }
    void SetUseSpecValRefused(bool b)   { m_bUseSpecValRefused = b; }
    void SetUseSpecValDefault(bool b)   { m_bUseSpecValDefault = b; }
    void SetUseSpecValUndefined(bool b) { m_bUseSpecValUndefined = b; }
    void SetUseCustomSpecVal(bool b)    { m_bUseCustomSpecVal = b; }

private:
    bool m_bUseCustomSpecVal;
    bool m_bUseSpecValNotAppl;
    bool m_bUseSpecValMissing;
    bool m_bUseSpecValRefused;
    bool m_bUseSpecValDefault;
    bool m_bUseSpecValUndefined;
};

struct CONITEM {
    int     level;      // Index to area name in structure
    int     lower;      // Lower range value (-1 = none)
    int     upper;      // Upper range value (-1 = none)
    int     replace;    // Replacement value (-1 = none - keep each)
};

//Savy for Sampling Error Application
//For holding SMEAN data
struct SMEANACUM2 {
    double  m_dNumWeight;
    double  m_dNumSquare;
    double  m_dDenWeight;
    double  m_dDenSquare;
    double  m_dDenUnWeight;
    double  m_dCumWeight;
    double  m_dCrosProd;
    double  m_dCumCases;
    double m_dMean;

    int     m_iClusters;//Run time values
} ;
class CLASS_DECL_ZTABLEO DOMAINVAR :public CObject
{
DECLARE_DYNAMIC(DOMAINVAR)

public:
    DOMAINVAR(){}
    ~DOMAINVAR(){}
    CMap<CString,LPCTSTR,int,int> m_mapOfVSetValues2ClusterFreqs;
    const CDictItem*              m_pDictItem ;  //This should come from the CDictItem. The VSet to Cluster will use the first VSet to lookup
    CString                       m_sDomainVarName;

    DOMAINVAR& operator=(const DOMAINVAR& var) {
        m_pDictItem = var.m_pDictItem;
        m_sDomainVarName = var.m_sDomainVarName;

        //Copy the VSET
        POSITION pos = var.m_mapOfVSetValues2ClusterFreqs.GetStartPosition();
        while(pos) {
            CString sKey;
            int iValue;

            var.m_mapOfVSetValues2ClusterFreqs.GetNextAssoc( pos, sKey,iValue ) ;
            m_mapOfVSetValues2ClusterFreqs.SetAt(sKey,iValue);
        }

        return *this;
    }
};
//End -Savy for Sampling Error Application


#define XTS_SECT_TABSPEC           _T("[TabSpecs]")
#define XTS_SECT_DICTS             _T("[Dictionaries]")
#define XTS_SECT_TABLE             _T("[Table]")
#define XTS_SECT_LEVEL             _T("[Level]")
#define XTS_SECT_ROWS              _T("[Rows]")
#define XTS_SECT_COLS              _T("[Cols]")
#define XTS_SECT_VAR               _T("[Variable]")
#define XTS_SECT_ENDVAR            _T("[EndVar]")
#define XTS_SECT_VALUE             _T("[Value]")
#define XTS_SECT_SPECIAL           _T("[Special]")
#define XTS_SECT_DATA              _T("[Data]")
#define XTS_SECT_AREA              _T("[Area]")
#define XTS_SECT_CELLS             _T("[Cells]")
#define XTS_SECT_FRQSTATS          _T("[FreqStats]")
#define XTS_SECT_ENDFRQSTATS       _T("[EndFreqStats]")
#define XTS_SECT_FRQNTILES         _T("[NTiles]")
#define XTS_SECT_ENDFRQNTILES      _T("[EndNTiles]")
#define XTS_SECT_UNITSPECS         _T("[UnitSpecs]")
#define XTS_SECT_UNIT              _T("[Unit]")
#define XTS_SECT_ENDUNITSPECS      _T("[EndUnitSpecs]")
#define XTS_SECT_TALLYFMT          _T("[TallyFormat]")
#define XTS_SECT_TITLE             _T("[Title]")
#define XTS_SECT_SUBTITLE          _T("[SubTitle]")
#define XTS_SECT_PAGE_NOTE         _T("[PageNote]")
#define XTS_SECT_END_NOTE          _T("[EndNote]")
#define XTS_SECT_HEADER_LEFT       _T("[HeaderLeft]")
#define XTS_SECT_HEADER_CENTER     _T("[HeaderCenter]")
#define XTS_SECT_HEADER_RIGHT      _T("[HeaderRight]")
#define XTS_SECT_FOOTER_LEFT       _T("[FooterLeft]")
#define XTS_SECT_FOOTER_CENTER     _T("[FooterCenter]")
#define XTS_SECT_FOOTER_RIGHT      _T("[FooterRight]")
#define XTS_SECT_STUBHEAD          _T("[StubHead]")
#define XTS_SECT_STUBHEAD_SEC      _T("[SecondaryStubHead]")
#define XTS_SECT_AREA_CAPTION      _T("[AreaCaption]")
#define XTS_SECT_ONEROWCOL_TOTAL   _T("[OneRowColTotal]")
#define XTS_SECT_AREASTRUCT        _T("[AreaStructure]")
#define XTS_SECT_CONSPEC           _T("[ConSpec]")
#define XTS_SECT_SAMPSPEC          _T("[SamplingSpec]")
#define XTS_SECT_SPECIAL_VALUES    _T("[SpecialValues]")
#define XTS_SECT_POSTCALC          _T("[PostCalc]")

#define XTS_CMD_SPECTYPE           _T("SpecType")
#define XTS_CMD_FRQ_MEAN           _T("Mean")
#define XTS_CMD_FRQ_MINC           _T("MinCode")
#define XTS_CMD_FRQ_MAXC           _T("MaxCode")
#define XTS_CMD_FRQ_STDDEV         _T("Std.Dev")
#define XTS_CMD_FRQ_VARIANCE       _T("Variance")
#define XTS_CMD_FRQ_MODEC          _T("ModeCode")
#define XTS_CMD_FRQ_MEDIANC        _T("MedianCode")
#define XTS_CMD_FRQ_MEDIANI        _T("MedianInt")
#define XTS_CMD_FRQ_TOTCAT         _T("TotalCategories")
#define XTS_CMD_FRQ_ALPHA_STATS    _T("AlphaStats")
#define XTS_CMD_NTILE              _T("NTile")


#define XTS_CMD_NAME               _T("Name")
#define XTS_CMD_LABEL              _T("Label")
#define XTS_CMD_ITM_OCC            _T("ItemOccurrence")
#define XTS_CMD_FILE               _T("File")
#define XTS_CMD_TABLE              _T("Table")
#define XTS_CMD_NUM                _T("Num")
#define XTS_CMD_BREAKLEVEL         _T("BreakLevel")
#define XTS_CMD_TITLE              _T("Title")
#define XTS_CMD_VARTYPE            _T("VarType")
#define XTS_CMD_TOTTYPE            _T("TotalType")
#define XTS_CMD_TOTPOS             _T("TotalPos")
#define XTS_CMD_STATS              _T("Stats")
#define XTS_CMD_TABLENAME          _T("TableName")
#define XTS_CMD_AREALEVEL          _T("AreaLevel")
#define XTS_CMD_AREANAME           _T("AreaName")
#define XTS_CMD_AREACODES          _T("AreaCodes")
#define XTS_CMD_BREAKKEY           _T("BreakKey")
#define XTS_CMD_SIZE               _T("Size")
#define XTS_CMD_ROW                _T("Row")
#define XTS_CMD_FORMAT             _T("Format")
#define XTS_CMD_PANEL              _T("Panel")
#define XTS_CMD_OFFSETCOL          _T("OffsetCol")
#define XTS_CMD_PRINTERCHANGED     _T("SelectedPrinterChanged")   // csc 12/3/04

#define XTS_CMD_SUBTABLE           _T("SubTable")
#define XTS_CMD_UNIVERSE           _T("Universe")
#define XTS_CMD_WEIGHT             _T("Weight")
#define XTS_CMD_UNITVALUE          _T("Value")
#define XTS_CMD_LOOPINGVAR         _T("LoopingVarName")
#define XTS_CMD_TABLOGIC           _T("TabLogic")
#define XTS_CMD_POSTCALC           _T("PostCalc")
#define XTS_CMD_ANALYSIS_TYPE      _T("AnalysisType") // Savy (R) sampling app 20090211
#define XTS_CMD_ANALYSIS_VAR       _T("AnalysisVariable") // Savy (R) sampling app 20090211
#define XTS_CMD_DENOMINATOR        _T("Denominator") // Savy (R) sampling app 20090211

#define XTS_CMD_COUNTS             _T("Counts")
#define XTS_CMD_PERTYPE            _T("PercentType")
#define XTS_CMD_PERPOS             _T("PercentPos")
#define XTS_CMD_TOTPOS             _T("TotalPos")
#define XTS_CMD_MIN                _T("Min")
#define XTS_CMD_MAX                _T("Max")
#define XTS_CMD_MODE               _T("Mode")
#define XTS_CMD_MEAN               _T("Mean")
#define XTS_CMD_MEDIAN             _T("Median")
#define XTS_CMD_NTILES             _T("nTiles")
#define XTS_CMD_STDDEV             _T("StdDev")
#define XTS_CMD_VARIANCE           _T("Variance")
#define XTS_CMD_STDERR             _T("StdErr")

#define XTS_CMD_NAMES              _T("Names")
#define XTS_CMD_STANDARD           _T("Standard")
#define XTS_CMD_LOWEST_LEVEL       _T("LowestLevel")
#define XTS_CMD_LEVEL              _T("Level")

#define XTS_CMD_SAMP_METHOD        _T("SamplingMethod")
#define XTS_CMD_CLUSTER_VAR        _T("ClusterVariable")
#define XTS_CMD_STRATA_VAR         _T("StrataVariable")
#define XTS_CMD_STRATA_FILE        _T("StrataFile")
#define XTS_CMD_DOMAIN_VAR         _T("DomainVariable")

#define XTS_CMD_SPECVAL_USECUSTOM  _T("UseCustomSpecial")
#define XTS_CMD_SPECVAL_NOTAPPL    _T("UseSpecialValueNotAppl")
#define XTS_CMD_SPECVAL_UNDEFINED  _T("UseSpecialValueUndefined")
#define XTS_CMD_SPECVAL_MISSING    _T("UseSpecialValueMissing")
#define XTS_CMD_SPECVAL_REFUSED    _T("UseSpecialValueRefused")
#define XTS_CMD_SPECVAL_DEFAULT    _T("UseSpecialValueDefault")

#define XTS_CMD_TBL_GENLOGIC       _T("GenerateLogic")
#define XTS_CMD_TBL_EXCLUDE_FOR_RUN _T("ExcludeForRun")

#define XTS_CMD_INPUTDATAFILENAME  _T("InputDataFilename")  // GHM 20090915
#define XTS_DEFAULT_INPUTDATAFILENAME  _T("<data filename>")    // for the &I header and footer option
#define XTS_DEFAULT_INPUTDATAFILENAMEHEADERLENGTH  50


#define XTS_ARG_DICT               _T("Dict")
#define XTS_ARG_SOURCE             _T("Source")
#define XTS_ARG_CUSTOM             _T("Custom")
#define XTS_ARG_COUNT              _T("Count")
#define XTS_ARG_PERCENT            _T("Percent")
#define XTS_ARG_BOTH               _T("Both")
#define XTS_ARG_NONE               _T("None")
#define XTS_ARG_BEFORE             _T("Before")
#define XTS_ARG_AFTER              _T("After")
#define XTS_ARG_RIGHT              _T("Right")
#define XTS_ARG_LEFT               _T("Left")
#define XTS_ARG_ABOVE              _T("Above")
#define XTS_ARG_BELOW              _T("Below")
#define XTS_ARG_TOTAL              _T("Total")
#define XTS_ARG_ROW                _T("Row")
#define XTS_ARG_COL                _T("Col")
#define XTS_ARG_MAX                _T("Max")
#define XTS_ARG_MIN                _T("Min")
#define XTS_ARG_NROW               _T("N-Total")
#define XTS_ARG_MEAN               _T("Mean")
#define XTS_ARG_MEDIAN             _T("Median")
#define XTS_ARG_NTILE              _T("Percentile")

#define XTS_ARG_MODE               _T("Mode")
#define XTS_ARG_STD                _T("Std")
#define XTS_ARG_VAR                _T("Var")
#define XTS_ARG_STDDEV             _T("StdDev")
#define XTS_ARG_STDERR             _T("StdErr")

#define XTS_ARG_TAYLOR             _T("TaylorSeries")
#define XTS_ARG_JACKKNIFE          _T("JackKnife")
#define XTS_ARG_BRRP               _T("BRRP")

//Savy (R) sampling app 20090211
#define XTS_ARG_SAMP_TOTALS         _T("Totals")
#define XTS_ARG_SAMP_MEANS          _T("Means")
#define XTS_ARG_SAMP_RATIOS         _T("Ratios")
#define XTS_ARG_SAMP_PROPORTION     _T("Proportion")

#define XTS_CMD_TABVAL_TYPE        _T("TabValueType")
#define XTS_CMD_TABVAL_STAT_INDEX  _T("TabValueStatIndex")
#define XTS_ARG_DICT_TABVAL        _T("VSetValue")
#define XTS_ARG_RDRBRK_TABVAL      _T("ReaderBreak")
#define XTS_ARG_SPECIAL_TABVAL     _T("Special")
#define XTS_ARG_SPECIAL_MISSING    _T("Missing")
#define XTS_ARG_SPECIAL_REFUSED    _T("Refused")
#define XTS_ARG_SPECIAL_DEFAULT    _T("Default")
#define XTS_ARG_SPECIAL_NOTAPPL    _T("NotAppl")
#define XTS_ARG_SPECIAL_UNDEFINED  _T("Undefined")

#define XTS_ARG_PRNTVSZ            _T("PrintViewSize")
#define XTS_ARG_GRIDVSZ            _T("GridViewSize")
#define XTS_ARG_GENLOGIC           _T("GenerateLogic")

#define XTS_ARG_YES                _T("Yes")
#define XTS_ARG_NO                 _T("No")

#define AREA_NONE_TEXT             _T("X")

#define MAX_XTS_LINE               200
#define CON_NONE                   -987654321

class CTabVar;

/////////////////////////////////////////////////////////////////////////////
//
//                             CGrdViewInfo
//
/////////////////////////////////////////////////////////////////////////////
class CGrdViewInfo
{
// Attributes
protected:
    CSize       m_szCurr;            // current size, pixels
    CSize       m_szMin;             // min size, pixels
    CSize       m_szMax;             // max size, pixels

//Construction
public:
    CGrdViewInfo() : m_szCurr(0,0), m_szMin(0,0), m_szMax(0,0) {}
    CGrdViewInfo(const CGrdViewInfo& g) { *this = g; }
    CGrdViewInfo(const CSize& szCurr,const CSize&  szMin,const CSize&  szMax) {
        m_szCurr = szCurr;
        m_szMin = szMin;
        m_szMax = szMax;
    }

// Operations
public:
    CSize GetCurrSize() const { return m_szCurr; }
    CSize& GetCurrSize() { return m_szCurr; }
    void SetCurrSize(const CSize& szCurr) { m_szCurr = szCurr; }

    CSize GetMinSize() const { return m_szMin; }
    CSize& GetMinSize() { return m_szMin; }
    void SetMinSize(const CSize& szMin) { m_szMin = szMin; }

    CSize GetMaxSize() const { return m_szMax; }
    CSize& GetMaxSize() { return m_szMax; }
    void SetMaxSize(const CSize& szMax) { m_szMax = szMax; }

// Operators
public:
    CGrdViewInfo& operator=(const CGrdViewInfo& g) {
        SetCurrSize(g.GetCurrSize());
        SetMinSize(g.GetMinSize());
        SetMaxSize(g.GetMaxSize());
        return *this;
    }
};
/////////////////////////////////////////////////////////////////////////////
//
//                             CPrtViewInfo
//
/////////////////////////////////////////////////////////////////////////////
class CPrtViewInfo
{
// Attributes
protected:
    CSize       m_szCurr;            // current size, twips
    CSize       m_szExtra;           // portion of current size that is "extra" (allocated by the layout heuristics)
    CSize       m_szMin;             // min size, pixels
    CSize       m_szMax;             // max size, pixels
    bool        m_bCustom;           // the current size has been customized by the user
    bool        m_bPageBreak;        // true if layout should force a hard page break below (stubs) or to the right (col headers) of this object

//Construction
public:
    CPrtViewInfo() : m_szCurr(0,0), m_szMin(0,0), m_szMax(0,0), m_szExtra(0,0) { m_bCustom=false; m_bPageBreak=false; }
    CPrtViewInfo(const CPrtViewInfo& p) { *this = p; }
    CPrtViewInfo(const CSize& szCurr,const CSize&  szMin,const CSize&  szMax, const CSize& szExtra, bool bCustom, bool bPageBreak) {
        m_szCurr = szCurr;
        m_szMin = szMin;
        m_szMax = szMax;
        m_szExtra = szExtra;
        m_bCustom = bCustom;
        m_bPageBreak = bPageBreak;
    }

// Operations
public:
    CSize GetCurrSize() const { return m_szCurr; }
    CSize& GetCurrSize() { return m_szCurr; }
    void SetCurrSize(const CSize& szCurr) { m_szCurr = szCurr; }

    CSize GetExtraSize() const { return m_szExtra; }
    CSize& GetExtraSize() { return m_szExtra; }
    void SetExtraSize(const CSize& szExtra) { m_szExtra = szExtra; }

    CSize GetMinSize() const { return m_szMin; }
    CSize& GetMinSize() { return m_szMin; }
    void SetMinSize(const CSize& szMin) { m_szMin = szMin; }

    CSize GetMaxSize() const { return m_szMax; }
    CSize& GetMaxSize() { return m_szMax; }
    void SetMaxSize(const CSize& szMax) { m_szMax = szMax; }

    bool IsCustom() const { return m_bCustom; }
    void SetCustom(bool bCustom=true) { m_bCustom=bCustom; }

    bool IsPageBreak() const { return m_bPageBreak; }
    void SetPageBreak(bool bPageBreak=true) { m_bPageBreak=bPageBreak; }

// Operators
public:
    CPrtViewInfo& operator=(const CPrtViewInfo& p) {
        SetCurrSize(p.GetCurrSize());
        SetMinSize(p.GetMinSize());
        SetMaxSize(p.GetMaxSize());
        SetExtraSize(p.GetExtraSize());
        SetCustom(p.IsCustom());
        SetPageBreak(p.IsPageBreak());
        return *this;
    }
};

/////////////////////////////////////////////////////////////////////////////////
//
//                              CTabUnit
//
/////////////////////////////////////////////////////////////////////////////////
class CLASS_DECL_ZTABLEO CUnitSpec : public CObject {
DECLARE_DYNAMIC (CUnitSpec)

// Data Members
private:
    CString      m_sSubtable;
    CString      m_sValue;
    CString      m_sWeight;
    CString      m_sUniverse;
    CString      m_sLoopingVarName;
    CStringArray m_arrTabLogic;
    CString      m_sAnalysisVariable; //Savy (R) sampling app 20090211
    CString      m_sDenominator; //Savy (R) sampling app 20090211
    SAMPLING_ANALYSIS_TYPE m_eSampAnalysisType; //Savy (R) sampling app 20090211
    bool         m_bMarkedNew;
    bool         m_bUseDefaultLoopingVar;
    CString      m_sAlternateSubTableString; //Kludge for display
    bool         m_bIsSamplingErrApp;//Savy (R) sampling app 20090415

public:
// Construction/destruction
    CUnitSpec () { m_bUseDefaultLoopingVar = true;  m_bMarkedNew =false;m_bIsSamplingErrApp=false;}//Savy (R) sampling app 20090415
    CUnitSpec (const CString& sSubTable){m_sSubtable =sSubTable; m_bMarkedNew =false;}
    ~CUnitSpec (){};
    CUnitSpec(const CUnitSpec& unitCopy){*this = unitCopy;}

//Savy (R) sampling app 20090211
public:

    const CString& GetAnalysisVariable() const                 { return m_sAnalysisVariable; }
    void SetAnalysisVariable(const CString& sAnalysisVariable) { m_sAnalysisVariable = sAnalysisVariable; }

    const CString& GetDenominator() const            { return m_sDenominator; }
    void SetDenominator(const CString& sDenominator) { m_sDenominator = sDenominator; }

    SAMPLING_ANALYSIS_TYPE GetSampAnalysisType()const {return m_eSampAnalysisType;}
    void SetSampAnalysisType(SAMPLING_ANALYSIS_TYPE eSampAnalysisType) { m_eSampAnalysisType = eSampAnalysisType;}
    //Savy (R) sampling app 20090415
    void SetSamplingErrAppFlag(bool bFlag) { m_bIsSamplingErrApp = bFlag; }
    bool GetSamplingErrAppFlag() const     { return m_bIsSamplingErrApp; }
//
public://Methods
    CStringArray&  GetTabLogicArray()              { return m_arrTabLogic; }
    const CString& GetSubTableString () const      { return m_sSubtable; }
    const CString& GetLoopingVarName () const      { return m_sLoopingVarName; }
    const CString& GetUniverse       () const      { return m_sUniverse; }
    const CString& GetWeightExpr     () const      { return m_sWeight; }
    const CString& GetValue          () const      { return m_sValue; }
    bool           GetUseDefaultLoopingVar() const { return m_bUseDefaultLoopingVar; }

    bool           IsMarkedNew() const    { return m_bMarkedNew; }
    void           SetNewMark(bool bMark) { m_bMarkedNew = bMark; }

    void SetSubTableString (const CString& sSubtable) { m_sSubtable = sSubtable;m_sSubtable.Trim(); }
    void SetLoopingVarName (const CString& sLoopingVarName) { m_sLoopingVarName = sLoopingVarName; m_sLoopingVarName.Trim();}
    void SetUniverse (const CString& sUniverse) { m_sUniverse = sUniverse;m_sUniverse.Trim(); }
    void SetWeightExpr (const CString& sWeight) { m_sWeight = sWeight;m_sWeight.Trim(); }
    void SetValue (const CString& sValue) { m_sValue = sValue;m_sValue.Trim(); }
    void SetUseDefaultLoopingVar(bool b) { m_bUseDefaultLoopingVar = b; }
    bool IsUnitPresent() const {
        //Savy (R) sampling app 20090213
        return ( !m_sValue.IsEmpty() || (!m_sLoopingVarName.IsEmpty() && !m_bUseDefaultLoopingVar) || !m_sUniverse.IsEmpty()|| !m_sWeight.IsEmpty() || m_arrTabLogic.GetSize()>0 || !m_sAnalysisVariable.IsEmpty() || !m_sDenominator.IsEmpty());
    }

// Input/Output
    bool Build (CSpecFile& specFile, bool bSilent=false);
    void Save (CSpecFile& specFile);

    void SetAltSubTableString(const CString& sAltSubtable) { m_sAlternateSubTableString = sAltSubtable;m_sAlternateSubTableString.Trim(); }
    const CString& GetAltSubTableString() const            { return m_sAlternateSubTableString; }
// Operators
    void operator= (const CUnitSpec& tabUnit) {
        m_bMarkedNew =tabUnit.IsMarkedNew();
        m_sSubtable = tabUnit.GetSubTableString();
        m_sLoopingVarName = tabUnit.GetLoopingVarName();
        m_sUniverse = tabUnit.GetUniverse();
        m_sWeight = tabUnit.GetWeightExpr();
        m_sValue =tabUnit.GetValue();
        m_bUseDefaultLoopingVar = tabUnit.m_bUseDefaultLoopingVar;
        m_sAlternateSubTableString = tabUnit.m_sAlternateSubTableString;
        m_arrTabLogic.RemoveAll();
        if(tabUnit.m_arrTabLogic.GetSize() >0){
            m_arrTabLogic.Append(tabUnit.m_arrTabLogic);
        }
        //Savy (R) sampling app 20090211
        m_eSampAnalysisType = tabUnit.GetSampAnalysisType();
        m_sAnalysisVariable = tabUnit.m_sAnalysisVariable;
        m_sDenominator      = tabUnit.m_sDenominator;

    };
    bool operator== (const CUnitSpec& tabUnit) const {
        bool bRet = false;
        bRet = (
        m_bMarkedNew ==tabUnit.IsMarkedNew() &&
        m_sSubtable.CompareNoCase(tabUnit.GetSubTableString())==0
        && m_sLoopingVarName.CompareNoCase(tabUnit.GetLoopingVarName())==0
        && m_sUniverse.CompareNoCase(tabUnit.GetUniverse())==0
        && m_sWeight.CompareNoCase(tabUnit.GetWeightExpr())==0
        && m_sValue.CompareNoCase(tabUnit.GetValue())==0
        && m_sAlternateSubTableString.CompareNoCase(tabUnit.m_sAlternateSubTableString)==0
        && m_bUseDefaultLoopingVar==tabUnit.m_bUseDefaultLoopingVar
        //Savy (R) sampling app 20090211
        && m_eSampAnalysisType == tabUnit.m_eSampAnalysisType
        && m_sAnalysisVariable.CompareNoCase(tabUnit.GetAnalysisVariable())==0
        && m_sDenominator.CompareNoCase(tabUnit.GetDenominator())==0
        );
        if (m_arrTabLogic.GetSize()!= tabUnit.m_arrTabLogic.GetSize()){
            return false;
        }
        for(int iIndex =0; iIndex < m_arrTabLogic.GetSize(); iIndex++){
            if(m_arrTabLogic[iIndex].Compare(tabUnit.m_arrTabLogic[iIndex]) !=0)
                return false;
        }
        return bRet;
    };
    bool operator!= (CUnitSpec tabUnit) const  { return !(*this == tabUnit); }

// diagnostic support
public:
    virtual void Dump (CDumpContext& dc) const  {
        CObject::Dump(dc);
        dc << _T("CTabUnit");

    }
    virtual void AssertValid() const {
        CObject::AssertValid();
    }
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CTabData
//
/////////////////////////////////////////////////////////////////////////////
class CLASS_DECL_ZTABLEO CTabData :public CObject
{
DECLARE_DYNAMIC(CTabData)

//  Members
protected:
    CString                        m_sTableName;       // Table name
    CString                        m_sAreaLevel;       // Area level name
    CString                        m_sAreaLabel;       // Area name text
    CArray<UINT, UINT>             m_aAreaCodes;       // Area codes
    int                            m_iRows;            // Number of table data rows
    int                            m_iCols;            // Number of table data cols
    CArray<double, double&>        m_aDataCells;       // Area of data cells.
    CArray<SMEANACUM2,SMEANACUM2&> m_aDataCells4Sampling; //Has the data For Sampling

    CString                        m_sError;           // Error messages
    CString                        m_sBreakKey;        //Break key of the slice in tbdslice
    long                           m_lRecodedCluster;  //Recoded break key for sampling errors
// Methods
public:
    // Construction / Destruction
    CTabData();
    ~CTabData();

    // Access
    void SetTableName(const CString& sTableName) { m_sTableName = sTableName; }
    const CString& GetTableName() const          { return m_sTableName; }

    void SetAreaLevel(const CString& sAreaLevel) { m_sAreaLevel = sAreaLevel; }
    const CString& GetAreaLevel() const          { return m_sAreaLevel; }

    void SetAreaLabel(const CString& sAreaLabel) { m_sAreaLabel = sAreaLabel; }
    const CString& GetAreaLabel() const          { return m_sAreaLabel; }

    void SetBreakKey(const CString& sBreakKey) { m_sBreakKey = sBreakKey; }
    const CString& GetBreakKey()               { return m_sBreakKey; }

    void SetRecodedCluster(long lRecodedCluster) { m_lRecodedCluster = lRecodedCluster; }
    long GetRecodedCluster() const               { return m_lRecodedCluster; }


    void SetNumRows(int iRows) { m_iRows = iRows; }
    void SetNumCols(int iCols) { m_iCols = iCols; }

    CArray<double, double&>& GetCellArray() { return m_aDataCells; }
    CArray<SMEANACUM2,SMEANACUM2&>& GetCellArray4Sampling() { return m_aDataCells4Sampling; }

    // Build / Save
    bool Build(CSpecFile& specFile, bool bSilent = false);
    void Save(CSpecFile& specFile);
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CTblBase
//
/////////////////////////////////////////////////////////////////////////////
class CLASS_DECL_ZTABLEO CTblBase : public CObject
{
    DECLARE_DYNAMIC(CTblBase)

    //  Attributes
protected:
    // format
    CFmtBase*                       m_pFmt;                // formatting attributes (derived classes implement this polymorphically:
    //     CTblOb       --> CFmt*
    //     CTabVar      --> CFmt*
    //     CTabVal      --> CDataCellFmt*
    //     CTable       --> CTblFmt*
    //     CSpecialCell --> CDataCellFmt*
    //     CTabSet      --> CTabSetFmt*

    CString                     m_sText;                  //Used by CTabVal,CTabvar and CTblOb;
    // Print /Grid  view stuff
    CArray<CGrdViewInfo, CGrdViewInfo&>  m_aGrdViewInfo;  // layout in the grid view (array for repeating stubs/cols)
    CArray<CPrtViewInfo, CPrtViewInfo&>  m_aPrtViewInfo;  // layout in the print view (array for repeating stubs/cols)

public:
    //Created and delete after the run .A place holder of Fmts to avoid recomuptations
    //of evaluated formats . For Large number of rows like village level breaks we have
    //serious performance problems . Will be delete and created by the runtime system
    CArray<CDataCellFmt,CDataCellFmt&> m_arrRunTimeDataCellFmts; //Used for optimization. Especially for Area Processing

    // Construction and initialization
protected:
    CTblBase(){m_pFmt=NULL;}
    CTblBase(CTblBase& t);
    virtual ~CTblBase() {}

    // Serialization
public:

    // Access
public:
    CFmtBase* GetFmt() const    { return m_pFmt; }
    void SetFmt(CFmtBase* pFmt) {  m_pFmt=pFmt;}

    const CString& GetText() const     { return m_sText; }
    void SetText(const CString& sText) { m_sText=sText;}

    bool BuildStateInfo(CSpecFile& specFile, bool bSilent  = false );
    void SaveStateInfo(CSpecFile& specFile) const;

    CGrdViewInfo GetGrdViewInfo(int iIndex) const { return m_aGrdViewInfo.GetAt(iIndex); }
    CGrdViewInfo& GetGrdViewInfo(int iIndex) { return m_aGrdViewInfo.ElementAt(iIndex); }
    void RemoveAllGrdViewInfo() { m_aGrdViewInfo.RemoveAll(); }
    int GetGrdViewInfoSize() const { return m_aGrdViewInfo.GetSize(); }
    void AddGrdViewInfo(CGrdViewInfo& g) { m_aGrdViewInfo.Add(g); }

    CPrtViewInfo GetPrtViewInfo(int iIndex) const { return m_aPrtViewInfo.GetAt(iIndex); }
    CPrtViewInfo& GetPrtViewInfo(int iIndex) { return m_aPrtViewInfo.ElementAt(iIndex); }
    void RemoveAllPrtViewInfo() { m_aPrtViewInfo.RemoveAll(); }
    int GetPrtViewInfoSize() const { return m_aPrtViewInfo.GetSize(); }
    void AddPrtViewInfo(CPrtViewInfo& p) { m_aPrtViewInfo.Add(p); }
    void SetPrtViewInfo(int iIndex, CPrtViewInfo& p) {
        ASSERT(iIndex<m_aPrtViewInfo.GetSize()) ; m_aPrtViewInfo[iIndex]=p; }


    // Operators
public:
    void operator=(CTblBase& t){
        m_pFmt =t.m_pFmt;
        m_sText =t.m_sText;
        m_aGrdViewInfo.RemoveAll();
        m_aPrtViewInfo.RemoveAll();
        for (int i=0 ; i<t.m_aGrdViewInfo.GetSize() ; i++) {
            m_aGrdViewInfo.Add(t.m_aGrdViewInfo[i]);
        }
        for (int i=0 ; i<t.m_aPrtViewInfo.GetSize() ; i++) {
            m_aPrtViewInfo.Add(t.m_aPrtViewInfo[i]);
        }

    };

    // Debug support
public:
    virtual void Dump(CDumpContext& dc) const=0;
    virtual void AssertValid() const {
        CObject::AssertValid();
        if (m_pFmt) {
            ASSERT_VALID(m_pFmt);
        }
    }
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CTblOb
//
// CFmtBase::m_pFmt is a CFmt in this class.
//
/////////////////////////////////////////////////////////////////////////////
class CLASS_DECL_ZTABLEO CTblOb : public CTblBase
{
DECLARE_DYNAMIC(CTblOb)

//  Attributes
protected:
    //no members as of now. CFmt and Text come from the base class

// Construction and initialization
public:
    CTblOb();
    CTblOb(CTblOb& t)   { *this=t; }
    ~CTblOb() {};

// Serialization
public:
    void Save(CSpecFile& specFile, const CString& sSection) const;
    bool Build(CSpecFile& specFile, const CFmtReg& reg, const CString& sVersion);

    CFmt* GetDerFmt() const { return DYNAMIC_DOWNCAST(CFmt,m_pFmt);  } // for convenience, avoids having to dynamic_downcast all the time

// Access
public:
    // Access

// Operators
public:
    void operator=(CTblOb& t){
        CTblBase::operator=(t);
    }

// Debug support
public:
    virtual void Dump(CDumpContext& dc) const {
        CObject::Dump(dc);
        dc << _T("CTblOb text = ") << m_sText;
    }
    virtual void AssertValid() const {
        CTblBase::AssertValid();
    }
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CSpecialCell
//
// Special cells are owned by stub rows only.
// CFmtBase::m_pFmt is a CDataCellFmt in this class.
//
/////////////////////////////////////////////////////////////////////////////
class CLASS_DECL_ZTABLEO CSpecialCell : public CTblBase
{
DECLARE_DYNAMIC(CSpecialCell)

//  Attributes
protected:
    int                             m_iRowPanel;           // row panel occurrence for the row owning this special cell
    int                             m_iCol;                // column where this special cell resides (irrespective of column panels)
    CString                         m_sError;          // Error messages
// Construction and initialization
public:
    CSpecialCell(int iPanel=NONE, int iCol=NONE) : m_iRowPanel(iPanel), m_iCol(iCol) { }
    CSpecialCell(CSpecialCell& d) { *this=d ;}

// Serialization
public:
    bool Build(CSpecFile& specFile, const CFmtReg& reg, bool bSilent = false);
    void Save(CSpecFile& specFile);
// Access
public:
    int GetPanel() const      { return m_iRowPanel; }
    void SetPanel(int iPanel) { m_iRowPanel=iPanel; }

    int GetCol() const    { return m_iCol; }
    void SetCol(int iCol) { m_iCol=iCol; }

    CDataCellFmt* GetDerFmt() const { return DYNAMIC_DOWNCAST(CDataCellFmt,m_pFmt); }  // for convenience, avoids having to dynamic_downcast all the time

private:
//    void FormatData();       //Not yet   SAVY!!!!

// Operators
public:
    void operator=(CSpecialCell& s) {
        CTblBase::operator=(s);
        m_iRowPanel=s.GetPanel();
        m_iCol=s.GetCol();
    }


// Debug support
public:
    virtual void Dump(CDumpContext& dc) const {
        CObject::Dump(dc);
        dc << _T("CSpecialCell text = ") << m_sText;
    }
    virtual void AssertValid() const {
        CTblBase::AssertValid();
    }
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CTabValue
//
// TabValues are stubs or column heads in a table.
// CFmtBase::m_pFmt is a CDataCellFmt in this class.
//
/////////////////////////////////////////////////////////////////////////////
class CLASS_DECL_ZTABLEO CTabValue : public CTblBase
{
DECLARE_DYNAMIC(CTabValue)

//  Attributes
protected:
    CTabVar*                            m_pParent;        // Pointer to parent table var
//    CDataCell                           m_defaultCell;      // Decimal places for the cells "owned" by this row/col
//    CArray<CDataCell*,CDataCell*>       m_aSpecCell;        // Special cell styles and/or formats
 //   CArray<CSource*,CSource*>           m_aSource;        // Source conditions
    CArray<CSpecialCell, CSpecialCell&> m_aSpecialCell;   // special cells owned by this stub (not used by column heads)
    CString                             m_sError;         // Error messages
    TABVAL_TYPE                         m_eTabValType;
    int                                 m_nStatId;       // id of associated statistic, for reconcile

// Construction and initialization
public:
    CTabValue() {m_eTabValType = DICT_TABVAL;m_nStatId=-1;}
    ~CTabValue() {};

// Serialization
public:
    bool Build(CSpecFile& specFile, const CFmtReg& reg, const CString& sVersion, bool bSilent);
    void Save(CSpecFile& specFile);

// Access
public:
    void SetParentVar(CTabVar* pParent) { m_pParent = pParent; }
    CTabVar* GetParentVar() { return m_pParent; }
    void SetStatId(int id)
    {
        m_nStatId = id;
    }
    int GetStatId() const
    {
        return m_nStatId;
    }

    CDataCellFmt* GetDerFmt() const { return DYNAMIC_DOWNCAST(CDataCellFmt,m_pFmt); }  // for convenience, avoids having to dynamic_downcast all the time
    CSpecialCell* FindSpecialCell(int iPanel,int iOffSetCol);
    CArray<CSpecialCell, CSpecialCell&>& GetSpecialCellArr(){return  m_aSpecialCell;}
// Operators
    void operator=(CTabValue& t){
        m_eTabValType = t.m_eTabValType;
        CTblBase::operator=(t);
    }
public:
    TABVAL_TYPE GetTabValType()const  { return m_eTabValType;}
    void SetTabValType(TABVAL_TYPE eTabValType) { m_eTabValType =eTabValType;}
// Debug support
public:
    virtual void Dump(CDumpContext& dc) const {
        CObject::Dump(dc);
        dc << _T("CTabVal text = ") << m_sText;
    }
    virtual void AssertValid() const {
        CTblBase::AssertValid();
    }
};



/////////////////////////////////////////////////////////////////////////////
//
//                             CTabVar
//
// TabVars are captions or spanners in a table; that is, they are variables that contain values
// CFmtBase::m_pFmt is a CFmt in this class.
//
/////////////////////////////////////////////////////////////////////////////

class CLASS_DECL_ZTABLEO CTabVar : public CTblBase
{
DECLARE_DYNAMIC(CTabVar)
//  Attributes
protected:


    CString                         m_sVarName;        // Name (dict item or variable)
    int                             m_iDictItemOcc;    // Occurrence number //This comes from the dictionary if the user drops a particular occ
    TVARTYPE                        m_VarType;          // Variable type (dict, source, custom)

    CArray<CTabValue*,CTabValue*>   m_aTabValue;        // Array of row, col, lay values including total and percent
    CArray<CTabVar*,CTabVar*>       m_aCrossVar;        // Pointer list of crossed variables
    CTabVar*                        m_pParent;          // Parent CTabVar

    //Revisit
    //bool                            m_bDisplayVar;      // Display variable name as spanner or row head

    CString                         m_sError;          // Error messages

    //Display format comes from the base class
    CTallyFmt*                      m_pTallyFmt;        // formatting attributes specific to tallying


// Construction and initialization
public:
    CTabVar();
    CTabVar(const DictValueSet* pVSet, const CTallyFmt& tblTallyFmt);
    CTabVar(const CDictItem* pDictItem, CStringArray& arrVals, const CTallyFmt& tblTallyFmt);
    void Init(const CDictItem* pDictItem, CStringArray& arrVals, const CTallyFmt& tblTallyFmt);
    //Savy (R) sampling app 20081202

    ~CTabVar();

// Serialization
public:
    bool Build(CSpecFile& specFile, const CFmtReg& reg, const CString& sVersion, bool bSilent);
    void Save(CSpecFile& specFile);

// Access
public:
    const CString& GetName() const        { return m_sVarName; }
    void SetName(const CString& sVarName) { m_sVarName = sVarName; }

    TVARTYPE GetType() const       { return m_VarType; }
    void SetType(TVARTYPE VarType) { m_VarType = VarType; }

    int GetTotValues(bool bIgnoreRdrBrks = false); //for supporting datacells
    int GetNumValues(bool bIgnoreRdrBrks = false) const {
        int iNumVals = m_aTabValue.GetSize();
        if(!bIgnoreRdrBrks){
            return iNumVals;
        }
        else {
            iNumVals = 0;
            for (int iIndex =0; iIndex < m_aTabValue.GetSize();iIndex++){
                if(m_aTabValue[iIndex]->GetTabValType() == RDRBRK_TABVAL || m_aTabValue[iIndex]->GetTabValType() == INVALID_TABVAL ){
                    continue;
                }
                iNumVals++;
            }
        }
        return iNumVals;
    }
    CTabValue* GetValue(int i) { return m_aTabValue[i]; }
    CArray<CTabValue*,CTabValue*>& GetArrTabVals() { return m_aTabValue;}

    int GetOcc() const { return m_iDictItemOcc; }
    void SetOcc(int iOcc) {
        m_iDictItemOcc = iOcc;
        if(m_iDictItemOcc > -1){
            CIMSAString sOcc;
            sOcc.Str(m_iDictItemOcc+1);
            SetText(GetText() + _T("(") + sOcc + _T(")"));
        }
    }

    bool IsRoot() const { return (m_pParent == NULL); }
    int GetNumChildren() const { return m_aCrossVar.GetSize(); }
    CTabVar* GetParent() { return m_pParent; }
    void SetParent(CTabVar* pParent) { m_pParent = pParent; }
    CTabVar* GetChild(int iIndex) { return m_aCrossVar[iIndex]; }

//    bool IsDisplayed()const { return m_bDisplayVar; }
 //   void SetDisplayed(bool bDisplay) { m_bDisplayVar = bDisplay; }

    CTallyFmt* GetTallyFmt() const { return m_pTallyFmt;}
    void SetTallyFmt(CTallyFmt* pTallyFmt) { m_pTallyFmt=pTallyFmt; }

    void UpdateFmtFlag();
    CDataCellFmt* GetDerFmt() const { return DYNAMIC_DOWNCAST(CDataCellFmt,m_pFmt); } // for convenience, avoids having to dynamic_downcast all the time
    // Manipulate crossed variables
public:
    void AddChildVar(CTabVar* pTabVar);
    void InsertSibling(CTabVar* pTabVar, bool bAfter = false);
    void Remove();    // Remove me as var be don't delete

    bool ReconcileName(const CString& sOldName, const CString& sNewName);

    CTabVar* CutVar(const int iIndex);
    void RemoveAllPrtViewInfoRecursive();

// Operators
public:

// Debug support
public:
    virtual void Dump(CDumpContext& dc) const {
        CObject::Dump(dc);
        dc << _T("CTabVar text = ") << m_sText;
    }
    virtual void AssertValid() const {
        CTblBase::AssertValid();
        if (m_pTallyFmt != NULL) {
            ASSERT_VALID(m_pTallyFmt);
        }
        // ASSERT_VALID(m_pParent); - this causes infinite loop JH
        for (int i=0 ; i<m_aTabValue.GetSize() ; i++) {
            ASSERT_VALID(m_aTabValue[i]);
        }
        for (int i=0 ; i<m_aCrossVar.GetSize() ; i++) {
            ASSERT_VALID(m_aCrossVar[i]);
        }
    }
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CTable
//
// CFmtBase::m_pFmt is a CTblFmt in this class.
//
/////////////////////////////////////////////////////////////////////////////

class CLASS_DECL_ZTABLEO CTable : public CTblBase
{
DECLARE_DYNAMIC(CTable)

//  Attributes
protected:
    CString                         m_sNum;            // Table Number
    CString                         m_sName;           // Table Name

    // Around the table stuff
    CTblOb                          m_tTitle;           // Table Title
    CTblOb                          m_tSubTitle;        // Subtitle (below title)
    CTblOb                          m_tPageNote;        // Page note (bottom of every page)
    CTblOb                          m_tEndNote;         // End note (on last page of table)
    CTblOb                          m_tHeader[3];       // At top of every page (folio) -- left, center, and right
    CTblOb                          m_tFooter[3];       // At bottom of every page (folio) -- left, center, and right
    CTblOb                          m_tAreaCaption;     // Just below boxheads, used for default placement of area text
    CTblOb                          m_tOneRowColTotal;  // One Row Or Col Total Place Holder

//    CTblOb                          m_tCol;             // default column (boxhead)
//    CTblOb                          m_tColGroup;        // default column group (spanner)
//    CTblOb                          m_tRow;             // default row (stub)
//    CTblOb                          m_tRowGroup;        // default row group (caption)
    CTblOb                          m_tStubhead[2];     // default stub head -- 0 index=primary stub, 1 index=secondary stub (for layout BOTH)

    // Area Structure stuff
    int                             m_iBreakLevel;      // number of area levels (-1 all, 0 if no break)

    // Tabulation stuff
    CTabVar*                        m_pRowRoot;         // Row variables (base)                         SAVY!!!
    CTabVar*                        m_pColRoot;         // Col variables (base)                         SAVY!!!
//  CArray<CTabVar*, CTabVar*>      m_aLayVars;

    CArray<CUnitSpec, CUnitSpec&>  m_aUnitSpec;     //Array of Units for the table
    CStringArray                   m_aPostCalc;     //Array of PostCalc Strings;
//  CArray<CConSpec*,  CConSpec*>   m_aConSpec;         // Consolidations ??? at table level

//  bool                            m_bCodeGenerated;   // True if source conditions present (not saved)
//  CStringArray                    m_aTabLogic;        // Generated from source conditions (not saved)

    // View stuff
    CArray<CTabData*, CTabData*>    m_aTabData;     //has info abt area breaks/layers and Data
    CTabData                        m_TblTabData;   //Table tabdata stores the cumulative SMEANACUM2 for all the breaks

    CString                         m_sError;           // Error messages
    //CTallyFmt                       m_tallyFmt;       //comes from the registry and is used by tabvar
    CUnitSpec                       m_tableUnit;    //Table Unit

    CTblPrintFmt*                   m_pTblPrintFmt;         // formatting specific to rendering this table for printing
    CFmtReg*                        m_pFmtReg;              // for accessing the format registry

    CString                         m_sErrMsg; //for reconcile stuff
    CustSpecialValSettings          m_custSpecValSettings;
    bool                            m_bGenerateLogic;
    bool                            m_bExcludeForRun;

    CArray<DOMAINVAR,DOMAINVAR&>    m_arrDomainVar; //Array of Domain Variables used at runtime by CRunSamp.
public: //Freq Stats
    double                  m_dFrqMean;
    double                  m_dFrqMinCode;
    double                  m_dFrqMaxCode;
    double                  m_dFrqModeCount;
    double                  m_dFrqModeCode;
    double                  m_dFrqMedianCode;
    double                  m_dFrqMedianInt;
    double                  m_dFrqStdDev;
    double                  m_dFrqVariance;
    CStringArray            m_arrFrqNTiles;
    int                     m_iTotalCategories;
    bool                    m_bAlphaFreqStats;
    bool                   m_bSaveFreqStats;
    bool                   m_bHasFreqStats;
    bool                   m_bHasFreqNTiles;
    bool                   m_bDoSaveinTBW;
    bool                   m_bDirty;

// Construction and initialization
public:
    CTable();
    CTable(const CString& sNum);
    ~CTable();

// Serialization
public:
    bool Build(CSpecFile& specFile, const CFmtReg& reg, const CString& sVersion, bool bSilent);
    bool BuildFrqStats(CSpecFile& specFile, bool bSilent = false);
    void SaveFrqStats(CSpecFile& specFile);

    bool BuildNTiles(CSpecFile& specFile, bool bSilent = false);
    void SaveNTiles(CSpecFile& specFile);

    // Save whole table
    void Save(CSpecFile& specFile);

    bool DoAllSubTablesHaveSameUnit(CString& sUnitName);

    // alternative API to CTable::Save used for saving only select slices
    // call SaveBegin to write table header info, call SaveTabData on each
    // slice you want to save and the call SaveEnd to write footer info.
    void SaveBegin(CSpecFile& specFile);
    void SaveEnd(CSpecFile& specFile);
    void SaveTabData(CSpecFile& specFile,int iTabData);
    bool IsOneColTable(){
        return ( GetColRoot()->GetNumChildren() ==1
            && GetColRoot()->GetChild(0)->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) ==0);
    }//Just "Total" in col
    bool IsOneRowTable(){
        return (GetRowRoot()->GetNumChildren() ==1
            && GetRowRoot()->GetChild(0)->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) ==0);
    }//Just "Total" in Row
    bool IsRowVar(CTabVar* pTabVar){
        bool bRet = false;
        CTabVar* pRowRoot = GetRowRoot();
        while(pTabVar){
              if(pTabVar == pRowRoot){
                  bRet = true;
                  break;
              }
              pTabVar= pTabVar->GetParent();
        }
        return bRet;
    }
// Access
public:
    bool IsDirty() const            { return m_bDirty; }
    void SetDirty(bool bDirty=true) { m_bDirty=bDirty; }

    const CString& GetName() const     { return m_sName; }
    void SetName(const CString& sName) { m_sName = sName; }

    const CString& GetNum() const    { return m_sNum; }
    void SetNum(const CString& sNum) { m_sNum = sNum; }

    int GetBreakLevel() const{ return m_iBreakLevel; }
    void SetBreakLevel(int iBreakLevel) { m_iBreakLevel = iBreakLevel; }

    CTblOb* GetTitle() { return &m_tTitle; }
    // Get title text or custom text if customized
    const CString& GetTitleText();
    CTblOb* GetSubTitle() { return &m_tSubTitle; }
    CTblOb* GetHeader(int iIndex) { return &m_tHeader[iIndex]; }
    CTblOb* GetFooter(int iIndex) { return &m_tFooter[iIndex]; }
    CTblOb* GetPageNote() { return &m_tPageNote; }
    CTblOb* GetEndNote() { return &m_tEndNote; }
    CTblOb* GetStubhead(int iIndex) { return &m_tStubhead[iIndex]; }
    CTblOb* GetAreaCaption() { return &m_tAreaCaption; }
    CTblOb* GetOneRowColTotal() {return &m_tOneRowColTotal;}
//    CDataCell* GetDefaultCell() { return &m_defaultCell; }

//    CTblOb* GetCol() { return &m_tCol; }
//    CTblOb* GetColGroup() { return &m_tColGroup; }
//    CTblOb* GetRow() { return &m_tRow; }
//    CTblOb* GetRowGroup() { return &m_tRowGroup; }

    CTabVar* GetRowRoot() { return m_pRowRoot; }
    CTabVar* GetColRoot() { return m_pColRoot; }
    CArray<CTabData*, CTabData*>& GetTabDataArray() { return m_aTabData; }
    void RemoveAllData();

    CTabData& GetTblTabData() { return m_TblTabData;} //Used for storing cumulative SMEANACUM2 for the table in sampling application
    // Manipulate variables

    // Dimensionality of table
    int GetNumRows(bool bIgnoreRdrBrks = false); //for supporting datacells
    int GetNumCols(bool bIgnoreRdrBrks = false); //for supporting datacells

    // Generate text
    void GenerateTitle();      // SAVY: this should take advantage of CTabSetFmt::m_sTitleTemplate !!!
    CArray<CUnitSpec, CUnitSpec&>& GetUnitSpecArr(){return m_aUnitSpec;}

    void UpdateSubtableList();

    CTblPrintFmt* GetTblPrintFmt() {return m_pTblPrintFmt;}
    void SetTblPrintFmt(CTblPrintFmt* pPrintFmt){m_pTblPrintFmt =pPrintFmt;}

//    CTallyFmt& GetTallyFmt() { return m_tallyFmt;}
    CUnitSpec& GetTableUnit(){return m_tableUnit;}
    void OnTallyAttributesChange(CTabVar* pTabVar,
                                 const CDWordArray& aMapStatNewToOldPos,
                                 const CDataDict* pDict, const CDataDict* pWorkDict);
    void ReconcileTabVar(const CDataDict* pDict, const CDataDict* pWorkDict, CTabVar* pTabVar, CString& sError, bool bSilent =false);
    void ReconcileTabVals(CTabVar* pTabVar, const CDataDict* pDict, const CDataDict* pWorkDict);
    bool ReconcileTallyFmt(CTabVar* pTabVar, const DictValueSet* pDictVSet);
    bool IsParentRowRoot(CTabVar* pTabVar);
    void SetCellDecimalPlaces(CTabVar* pTabVar);
    void InterleavePercents(CTabVar* pTabVar);
    void InterleaveStatTabVals(CArray<CTabValue*,CTabValue*>& aTabVals, int iStat1, int iStat2);
    void AddReaderBreakTabVals(CTabVar* pTabVar);
    void ReconcileReaderBreakTabVals(CTabVar* pTabVar, CArray<CTabValue*, CTabValue*>& arrStatTabVals);
    CString FormatDataCell(const double& dData, CTabSetFmt* pTabSetFmt, CDataCellFmt* pDataCellFmt);



    //Helper function used specifically for reconcile of special tabvals
    CTabValue* GetStatTabVal(CArray<CTabValue*,CTabValue*>&arrTabVals, TABVAL_TYPE eValType);
    CFmtReg* GetFmtRegPtr() { return m_pFmtReg;}
    void SetFmtRegPtr(CFmtReg* pFmtReg) { m_pFmtReg = pFmtReg;}

    CTblFmt* GetDerFmt() const { return DYNAMIC_DOWNCAST(CTblFmt,m_pFmt); }  // for convenience, avoids having to dynamic_downcast all the time

    CustSpecialValSettings& GetCustSpecialValSettings()
    {
        return m_custSpecValSettings;
    }

    const CustSpecialValSettings& GetCustSpecialValSettings() const
    {
        return m_custSpecValSettings;
    }

    void GetOneUnitSubStatement(const CUnitSpec& unitSpec, CString& sUnitStatement , XTABSTMENT_TYPE eStatement);

    void SetGenerateLogic(bool bFlag) { m_bGenerateLogic = bFlag;}
    bool GetGenerateLogic() const     { return m_bGenerateLogic;}

    //Savy For Selective running
    void SetExcludeTable4Run(bool bFlag) { m_bExcludeForRun = bFlag;}
    bool IsTableExcluded4Run() const     { return m_bExcludeForRun;}

    const CString& GetReconcileErrMsg() const    { return m_sErrMsg; }
    void SetReconcileErrMsg(const CString& sMsg) { m_sErrMsg = sMsg; }

    CStringArray&   GetPostCalcLogic(){return m_aPostCalc;}

private:
    CTableDef::ETableType m_eTableType;
public:
     CMap<long,long,CArray<double,double>*,CArray<double,double>*> m_clusterToAux1Map; //RunTime member of sampling erro computation
public: //Methods for sampling error Application
    CArray<DOMAINVAR,DOMAINVAR&>& GetDomainVariablesArray(){ return m_arrDomainVar;}
    CTableDef::ETableType GetTableType() { return m_eTableType;}
    void SetTableType(CTableDef::ETableType eTblType) { m_eTableType =eTblType;}
    //Savy (R) sampling app 20090218
    bool IsValidSMeanTable(CString& sMsg);
    //Savy (R) sampling app 20090219
    CString GetLogic4Samp(XTABSTMENT_TYPE eStatement = XTABSTMENT_ALL);
// Operators
public:

// Debug support
public:
    virtual void Dump(CDumpContext& dc) const {
        CObject::Dump(dc);
        dc << _T("CTable text = ") << m_sText;
    }
    virtual void AssertValid() const {
        CTblBase::AssertValid();
        ASSERT_VALID(m_pRowRoot);
        ASSERT_VALID(m_pColRoot);
        ASSERT_VALID(m_pTblPrintFmt);
        ASSERT_VALID(m_pFmtReg);
        for (int i=0 ; i<m_aTabData.GetSize() ; i++) {
            ASSERT_VALID(m_aTabData[i]);
        }
    }
};

/////////////////////////////////////////////////////////////////////////////
//
//                             CTabLevel
//
/////////////////////////////////////////////////////////////////////////////

class CLASS_DECL_ZTABLEO CTabLevel : public CObject
{
DECLARE_DYNAMIC(CTabLevel)

//  Members
protected:
    CArray<CTable*, CTable*>        m_aTable;   // Tables at this level

// Methods
public:
    // Construction / Destruction
    CTabLevel() {};
    ~CTabLevel() { m_aTable.RemoveAll(); }

    // Access
    int GetNumTables() const { return m_aTable.GetSize(); }
    CTable* GetTable(const int iIndex) { return m_aTable[iIndex]; }
    CTable* SearchTable(const CString& sTableName);  // RHF May 13, 2003 Few tables and used in compiling time, no necesary to have a map

    // Add / Insert / Delete
    void AddTable(CTable* pTable) { m_aTable.Add(pTable); }
    void InsertTable(const int iIndex, CTable* pTable) { m_aTable.InsertAt(iIndex, pTable); }
    void DeleteTable(const int iIndex) { m_aTable.RemoveAt(iIndex); }

    // Load / Save
    void Save(CSpecFile& specFile);

    // debug support
    virtual void AssertValid() const {
        CObject::AssertValid();
        for (int i=0 ; i<m_aTable.GetSize() ; i++) {
            ASSERT_VALID(m_aTable[i]);
        }
    }
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CTabSet
//
/////////////////////////////////////////////////////////////////////////////
class CConsolidate;
class CLASS_DECL_ZTABLEO CTabSet : public CObject
{
DECLARE_DYNAMIC(CTabSet)

    friend CTabVar;

//Savy (R) sampling app 20081216
private:

    CArray<DOMAINVAR,DOMAINVAR&> m_arrDomVar;
    CString m_sClusterVar;
    CString m_sStrataVariable;
    CString m_sStrataFileName;
    SAMPLING_METHOD_TYPE m_eSampMethodType;


public:
    const CString& GetClusterVariable() const           { return m_sClusterVar; }
    void SetClusterVariable(const CString& sClusterVar) { m_sClusterVar = sClusterVar; }

    const CString& GetStrataVariable() const          { return m_sStrataVariable; }
    void SetStrataVariable(const CString& sStrataVar) { m_sStrataVariable = sStrataVar; }

    const CString& GetStrataFileName() const         { return m_sStrataFileName; }
    void SetStrataFileName(const CString& sFileName) { m_sStrataFileName = sFileName; }

    CArray<DOMAINVAR,DOMAINVAR&>& GetDomainVarArray() { return m_arrDomVar;}

    SAMPLING_METHOD_TYPE GetSampMethodType() const { return m_eSampMethodType; }
    void SetSampMethodType(SAMPLING_METHOD_TYPE eSampMethodType) { m_eSampMethodType = eSampMethodType;}
    //Savy (R) sampling app 20090218
    bool IsValidSamplingStatSettings(CString& sMsg);
    //Savy (R) sampling app 20090219
    CString GetBreakBy4SampApp();
//end
//  Attributes
protected:
    CString                         m_sLabel;           // Spec file label
    CString                         m_sName;            // Spec file name
    CString                         m_sDictFile;        // Dictionary file name
    CString                         m_sSpecFile;        // Spec full path name
    std::shared_ptr<const CDataDict>m_pDataDict;        // Data dictionary

    CString                         m_sInputDataFilename;   // 20090915 GHM

//    CStyleReg                       m_StyleReg;         // Styles
//    CAppFmt*                        m_pAppFmt;          // App-level formats for this set of tables

    // Formats
//  CCell                           m_defaultCell;      // Default cell style and format
//    CString                       m_sThousandSep;     // Thousands separator
//    CString                       m_sDecimalSep;      // Decimal separator
//    bool                          m_bZeroBeforeDec;   // Zero before decimal point
//    CString                       m_sZeroMask;        // Zero mask string
//    CString                       m_sZeroRound;       // Zero round string
 //   CString                       m_sSuppress;        // Suppressed string
//  int                             m_iMeasureUnit;     // Unit of measure of size

    CConsolidate*                   m_pConsolidate;     // Area Processing
    CArray<CTabLevel*, CTabLevel*>  m_aTabLevel;        // Table levels
    CArray<CTable*, CTable*>        m_aPrintTable;      // Order to print tables

    CString                         m_sError;           // Error messages
    SPECTYPE                        m_eSpecType; // Generated by freq / cross tab

    CFmtReg                         m_fmtReg;
    bool                            m_bGenLogic;
    CMap<CTabVar*, CTabVar*, CString, CString&> m_varNameMap;

    CTabSetFmt*                     m_pTabSetFmt;       // tabset specific formatting

    bool                            m_bSelectedPrinterChanged;  // =true if the user has changed printer via file|print setup, for notifying the build routines  csc 12/3/04

    std::shared_ptr<const CDataDict> m_pWorkDict; //Working Storage Dict
    CSpecFile*                       m_pAreaNameFile;//Area names file
    UINT                             m_uAreaFilePos; //file pos to read
    CMapStringToString               m_AreaLabelLookup; //Label lookup
    bool                             m_bOldAreaNameFile;
    //Savy (R) sampling application 20081202
    bool                             m_bIsSamplingErrApp;

// Construction and initialization
public:
    CTabSet();
    CTabSet(std::shared_ptr<const CDataDict> pDataDict);
    ~CTabSet();

// Serialization
public:
    // SAVY: which methods go here?
    CMapStringToString& GetAreaLabelLookup() { return m_AreaLabelLookup; }
    bool IsOldAreaNameFile() const           { return m_bOldAreaNameFile; }
    //Savy (R) sampling app 20081209
    void SetSamplingErrAppFlag(bool bFlag) { m_bIsSamplingErrApp = bFlag; }
    bool GetSamplingErrAppFlag() const     { return  m_bIsSamplingErrApp ; }


// Access
public:
    const CString& GetLabel() const      { return m_sLabel; }
    void SetLabel(const CString& sLabel) { m_sLabel = sLabel; }

    const CString& GetName() const     { return m_sName; }
    void SetName(const CString& sName) { m_sName = sName; }

    const CString& GetDictFile()               { return m_sDictFile; }
    void SetDictFile(const CString& sDictFile) { m_sDictFile = sDictFile; }

    const CString& GetSpecFile() const         { return m_sSpecFile; }
    void SetSpecFile(const CString& sSpecFile) { m_sSpecFile = sSpecFile; }

    const CDataDict* GetDict() const                         { return m_pDataDict.get(); }
    std::shared_ptr<const CDataDict> GetSharedDictionary()   { return m_pDataDict; }
    void SetDict(std::shared_ptr<const CDataDict> pDataDict) { m_pDataDict = std::move(pDataDict); }

    const CString& GetInputDataFilename() const                  { return m_sInputDataFilename; } // 20090915
    void SetInputDataFilename(const CString& sInputDataFilename) { m_sInputDataFilename = sInputDataFilename; }

    CConsolidate* GetConsolidate()                   { return m_pConsolidate; }
    void SetConsolidate (CConsolidate* pConsolidate) { m_pConsolidate = pConsolidate; }

    int GetNumLevels() const { return m_aTabLevel.GetSize(); }
    void SetNumLevels(int levels);
    CTabLevel* GetLevel(int iLevel) { return m_aTabLevel[iLevel]; }

    CTabSetFmt* GetTabSetFmt() const { return m_pTabSetFmt; }
    void SetTabSetFmt(CTabSetFmt* pTabSetFmt) { m_pTabSetFmt = pTabSetFmt; }

    bool GetGenLogic() const      { return m_bGenLogic;}
    void SetGenLogic(bool bLogic) { m_bGenLogic = bLogic; }

    // Table Manipulation
    int GetNumTables() const { return m_aPrintTable.GetSize(); }
    const CTable* GetTable(int iIndex) const { return m_aPrintTable[iIndex]; }
    CTable* GetTable(int iIndex)             { return m_aPrintTable[iIndex]; }
    CTable* SearchTable(const CString& sTableName,int& iCtabLevel ); // RHF May 13, 2003 Few tables and used in compling time, no necesary to have a map
    void AddTable(CTable* pTable, int iLevel = NONE);
    void InsertTable(CTable* pTable, CTable* pRefTable, int iLevel = NONE);
    void DeleteTable(CTable* pTable);
    void UpdateFmtFlag(CTable* pTable);
    void MovePrintTable(CTable* pTable, CTable* pRefTable, bool bAfter = false);
    void MoveLevelTable(CTable* pTable, CTable* pRefTable,  bool bAfter = false);
    void ReconcileLevels4Tbl(CTable* pTable = NULL);
    bool IsSubTableFromWrkStg(CTable* pTable,const CUnitSpec& unitSpec);

    int GetTableLevelFromUnits(CTable* pTable);
    int GetTableLevel(CTable* pTable);
    void GetTableLevel(CTabVar*pTabVar, int& iCurLevel);

    bool ConsistencyCheckSubTblNTblLevel(CString& sMsg, bool bSilent =false);
    int GetSubTblLevelFromVars(CTable* pTable, const CUnitSpec& unitSpec);
    bool DoAllSubTablesHaveSameUnit(CTable* pTable, CString& sUnitName);

    // Reconcile this!

    // should use the one in doc rather than this, one in doc calls this and also checks
    // universe and weights
    bool Reconcile(CString& sError, bool bSilent, bool bAutoFix);
    bool ReconcileCon(CString& sError, bool bSilent = false);


    bool ReconcileName(const CDataDict& dictionary);
    bool ReconcileLabel(const CDataDict& dictionary);

    // Build and Save
    bool Open (const CString& sXTSFilePath, bool bSilent = false);
    bool Build (CSpecFile& specFile, std::shared_ptr<ProgressDlg> pDlgProgress, const CString& sVersion, bool bSilent = false);
    bool BuildSampSpec(CSpecFile& specFile, bool bSilent=false);
    bool Save (const CString& sSpecFilePath, const CString& sDictFilePath, const CString& sInputDataFilename = _T(""));
    void SaveSamplingSection(CSpecFile& specFile);
    void SetDefaultFmts(CTabVar* pVar, CFmt* pFmtDefaultVar, CFmt* pFmtDefaultVal);

    // Table Numbers
    bool IsUniqueTabNum(const CString& sNum);
    CString GetNextTabNum(const CString& sNum);
    void Renumber();

    SPECTYPE GetSpecType() const         { return m_eSpecType;}
    void SetSpecType(SPECTYPE eSpecType) { m_eSpecType = eSpecType;}

    const CFmtReg& GetFmtReg() const { return m_fmtReg; }
    CFmtReg* GetFmtRegPtr()          { return &m_fmtReg; }

    void MakeCrossTabStatement(CTable* pRefTable , CString& sCrossTabStatement , XTABSTMENT_TYPE eStatement = XTABSTMENT_ALL) ;
    //Savy (R) sampling app 20090213
    void MakeSMeanStatement(CTable* pRefTable , CString& sCrossSTabStatement , XTABSTMENT_TYPE eStatement = XTABSTMENT_ALL) ;
    CString GetLoopingVar4Samp(CTable* pTable);
    void MakeNameMap(CTable* pTable);
    void MakeNameMap(CTabVar* pTabVar,CMapStringToString& arrNames);
    bool GetStatString(CTable* pTable,CString& sStatString,bool bRow =true);
    CString GetIncludeExcludeString(CTable* pTable);
    bool GetVarList(CTabVar* pTabVar , CString& sVarList);
    bool GetSVarList(CTabVar* pTabVar , CString& sVarList);//Savy (R) sampling app 20090213 generatelogic
    CString MakeVarList4DummyVSet(const CDictItem* pDictItem) ;
    bool GetUnitStatement(CTable* pTable,CString& sUnitStatement, XTABSTMENT_TYPE eStatement = XTABSTMENT_ALL);
   // void GetOneUnitSubStatement(const CUnitSpec& unitSpec, CString& sUnitStatement , XTABSTMENT_TYPE eStatement);
    void GetPercentString(PCT_TYPE ePerType,TOTALS_POSITION eTotals,CString& sVarStatString);
    void GetFreqString(TALLY_STATISTIC eCountType,TOTALS_POSITION eTotals,CString& sVarStatString);
    void ReconcileUnitLoopingVar(CUnitSpec& unit,
                                 const CStringArray& asValidLoopingVars,
                                 const CString& sSubtableName,
                                 CString& sMsg);
    int UpdateSubtableList(CTable* pRefTable, CArray<CStringArray,CStringArray&>& arrValidSubtableUnits,
                            CStringArray& validTblUnits,
                            bool bReconcile = false,
                            CString* pReconcileMsg = NULL,
                            CTabVar* pRowVar =NULL
                            ,CTabVar* pColVar =NULL);

    //if drop TargetVar is row then pass the RowVar to get all the subtables along the row marked as new
    //if drop TargetVar is col then pass the ColVar to get all the subtables along the col marked as new
    void UpdateSubtableListNMarkNewSubTables(CTable* pRefTable,CTabVar* pRowVar =NULL,CTabVar* pColVar =NULL);

    void ResetTableUnits(CTable* pRefTable);
    void GetAllRecordRelations(const CDictRecord& dict_record, CArray<const DictRelation*>& aRelations);
    void GetAllRecordRelations(const CDictRecord& dict_record1, const CDictRecord& dict_record2, CArray<const DictRelation*>& aRelations);
    bool HasRecordRelation(const CDictRecord& dict_record1, const CDictRecord& dict_record2);
    void ComputeSubtableUnitNames(CStringArray& asUnitNames, const CStringArray& asVarNames);
    bool SaveTBWEnd(CSpecFile& specFile);
    bool SaveTBWTables(CSpecFile& specFile);
    bool SaveTBWBegin(CSpecFile& specFile, const CString& sDictFilePath);

    const CDataDict* GetWorkDict() const                         { return m_pWorkDict.get(); }
    void SetWorkDict(std::shared_ptr<const CDataDict> pWorkDict) { m_pWorkDict = pWorkDict;}

    const CDataDict* LookupName(const CString& name, const DictLevel** dict_level, const CDictRecord** dict_record, const CDictItem** dict_item, const DictValueSet** dict_value_set) const;
    bool LookupName(const CString& csName, int* iLevel, int* iRecord, int* iItem, int* iVSet) const;

// Misc
public:
    bool HasSelectedPrinterChanged() const { return m_bSelectedPrinterChanged; }
    void SetSelectedPrinterChanged(bool bSelectedPrinterChanged=true) { m_bSelectedPrinterChanged=bSelectedPrinterChanged; }

    bool DoReconcileMultiRecordChk(CTable* pTbl,CString& sMsg);
    bool DoMultiRecordChk(CTabVar* pTabVar,const CDictRecord* pMultRecord);
    bool DoMultiRecordChk(CTable* pTable,const CDictRecord* pMultRecord,CTabVar* pTargetVar,bool bFromRow,bool bIsPlusOper);
    void DoRecursiveMultRecChk(CTable* pTbl,CTabVar* pTabVar,bool bRow,bool bIsPlusOper,CString& sMsg);

    bool DoReconcileMultiItemChk(CTable* pTbl,CString& sMsg);
    bool DoMultiItemChk(CTable* pTable,const CDictItem* pOccDictItem,CTabVar* pTargetVar,bool bFromRow,bool bIsPlusOper);
    bool DoMultiItemChk(CTabVar* pTabVar,const CDictItem* pOccDictItem);
    void DoRecursiveMultiItemChk(CTable* pTbl,CTabVar* pTabVar,bool bRow,bool bIsPlusOper,CString& sMsg);
    void AddSystemTotalVar(CTable* pTable);
    //Savy (R) sampling app 20081201
    void AddStatVar(CTable* pTable);


   void  ReconcileName(CTable* pTbl, CString sOldName, CString sNewName);

   bool OpenAreaNameFile(const CString& sAreaNameFile);
   bool BuildAreaLookupMap();
   void CloseAreaNameFile();
   void Reconcile30TallyFmts();
   void Number3031TabVals();
   void Number3031TabVals(CTable* pTable, CTabVar* pTabVar);
   void FixSysTotalVarTallyFmts();
   void FixSysTotalVarTallyFmts(CTabVar* pTabVar, FMT_ID eFmtID);

// Operators
public:

// Debug support
public:
    virtual void Dump(CDumpContext& dc) const {
        CObject::Dump(dc);
        dc << _T("CTabSet text = ") << m_sLabel;
    }
    virtual void AssertValid() const {
        CObject::AssertValid();
//        ASSERT_VALID(m_pConsolidate);
        ASSERT_VALID(m_pTabSetFmt);
        for (int i=0 ; i<m_aTabLevel.GetSize() ; i++) {
            ASSERT_VALID(m_aTabLevel[i]);
        }
        for (int i=0 ; i<m_aPrintTable.GetSize() ; i++) {
            ASSERT_VALID(m_aPrintTable[i]);
        }

        for (int i=0 ; i<m_aTabLevel.GetSize() ; i++) {
            ASSERT_VALID(m_aTabLevel[i]);
        }
        for (int i=0 ; i<m_aTabLevel.GetSize() ; i++) {
            ASSERT_VALID(m_aTabLevel[i]);
        }

    }
};

/////////////////////////////////////////////////////////////////////////////
//
//                             CSource
//
/////////////////////////////////////////////////////////////////////////////

class CLASS_DECL_ZTABLEO CSource : public CObject
{
DECLARE_DYNAMIC(CSource)

//  Members
protected:
    CString                     m_sItemVarName;     // CSPro language name
    int                         m_iItemType;        // Type (dict vset, var)
    const DictValueSet*         m_pVSet;            // Pointer to dict value set
    CString                     m_sRel;             // Relation (=, >, >=, etc.)
    CString                     m_sValueLabel;      // Dict value label
    CString                     m_sValueNumbers;    // Value string
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CConSpec
//
/////////////////////////////////////////////////////////////////////////////

class CLASS_DECL_ZTABLEO CConSpec : public CObject
{
DECLARE_DYNAMIC(CConSpec)

//  Members
protected:
    CString                 m_sAreaLevel;   // Consolidation level name
    CArray<CONITEM,CONITEM> m_aActions;     // Array of actions

    // Methods
public:
    // Constructors
    CConSpec(CConSpec* c);
    CConSpec(int iNum);
    ~CConSpec();

    // Access
    const CString& GetAreaLevel() const         { return m_sAreaLevel; }
    void SetAreaLevel(const CString&sAreaLevel) { m_sAreaLevel = sAreaLevel; }

    int GetNumActions() const { return m_aActions.GetSize(); }
    CONITEM GetAction(int index) const { return m_aActions[index]; }
    void SetAction(int index, CONITEM item) { m_aActions.SetAtGrow(index,item); }

    // Serialization
    bool Build(CSpecFile& specFile, CStringArray* pAreaStruct, bool bSilent = false);
    void Save(CSpecFile& specFile, CStringArray* pAreaStruct);
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CConsolidate
//
/////////////////////////////////////////////////////////////////////////////

class CLASS_DECL_ZTABLEO CConsolidate : public CObject
{
DECLARE_DYNAMIC(CConsolidate)

//  Members
protected:
    CStringArray                    m_aStructure;   // Area Structure
    bool                            m_bStandard;    // Is standard colsolidation being used
    int                             m_iStandard;    // Lowest Level of Standard Consolidation
    CArray<CConSpec*, CConSpec*>    m_aConSpec;    // Consolidations

// Methods
public:
    // Constructors
    CConsolidate();
    CConsolidate(CConsolidate& c);
    ~CConsolidate();

    // Access
    const CString& GetArea(int index) const       { return m_aStructure[index]; }
    void SetArea(int index, const CString& sArea) { m_aStructure.SetAtGrow(index, sArea); }
    int GetNumAreas() const { return m_aStructure.GetSize(); }
    void RemoveArea(int index){ASSERT(index < m_aStructure.GetSize()); m_aStructure.RemoveAt(index); }
    bool IsStandard() const { return m_bStandard; }
    void SetStandard(bool bStandard) { m_bStandard = bStandard; }

    int GetStandardLevel() const         { return m_iStandard; }
    void SetStandardLevel(int iStandard) { m_iStandard = iStandard; }

    int GetNumConSpecs() const { return m_aConSpec.GetSize(); }
    CConSpec* GetConSpec(int index) { return m_aConSpec[index]; }
    void SetConSpec(int index, CConSpec* pConSpec) { m_aConSpec.SetAtGrow(index,pConSpec); }

    // Generation
    void Reset();
    void RemoveAllConSpecs();
    void GenerateStandard();

    // Reconcile
    void Reconcile();

    // Serialization
    bool Build(CSpecFile& specFile, bool bSilent = false);
    void Save(CSpecFile& specFile);

    // Operators
    void operator= (CConsolidate& c);

    // Debug support
    virtual void AssertValid() const {
        CObject::AssertValid();
        for (int i=0 ; i<m_aConSpec.GetSize() ; i++) {
            ASSERT_VALID(m_aConSpec[i]);
        }
    }
};
