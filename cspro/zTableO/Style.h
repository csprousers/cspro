#pragma once

//***************************************************************************
//  File name: Style.h
//
//  Description:
//       Style objects prototypes
//
//  History:    Date       Author     Comment
//              -----------------------------
//              28 Oct 02   BMD       CSPro 3.0
//              Nov 2004    csc+savy  reimplementation
//
//***************************************************************************

#include <zTableO/zTableO.h>

// commands -- header section
#define TFT_SECT_FORMAT_FILE         _T("[TableFormatFile]")        // heading for a TFT file

#define TFT_SECT_FORMAT_TABSET       _T("[TabSetFormat]")           // indicates a CTabSetFmt section
#define TFT_SECT_FORMAT_TBL          _T("[TableFormat]")            // indicates a CTblFmt section
#define TFT_SECT_FORMAT_TALLY        _T("[TallyFormat]")            // indicates a CTallyFmt section
#define TFT_SECT_TALLY_STAT           _T("[TallyStatistic]")           // start new tally stat section
#define TFT_SECT_FORMAT_TBLPRINT     _T("[TablePrintFormat]")       // indicates a CTbleFmt section
#define TFT_SECT_FORMAT_DATACELL     _T("[DataCellFormat]")         // indicates a CDataCellFmt section
#define TFT_SECT_FORMAT              _T("[Format]")                 // indicates a CFmt section

// commands -- CFmtBase stuff
#define TFT_CMD_NAME                 _T("Name")                     // corresponds to CFmtBase::m_ID
#define TFT_CMD_INDEX                _T("Index")                    // corresponds to CFmtBase::m_iIndex;

// commands -- CFmt stuff
#define TFT_CMD_FONT                 _T("Font")                     // corresponds to CFmt::m_font
#define TFT_CMD_HORZALIGN            _T("HorzAlign")                // corresponds to CFmt::m_eHorzAlign
#define TFT_CMD_VERTALIGN            _T("VertAlign")                // corresponds to CFmt::m_eVertAlign
#define TFT_CMD_COLOR_TEXT           _T("TextColor")                // corresponds to CFmt::m_colorText
#define TFT_CMD_COLOR_FILL           _T("FillColor")                // corresponds to CFmt::m_colorFill
#define TFT_CMD_INDENT_LEFT          _T("IndentLeft")               // corresponds to CFmt::m_indentLeft
#define TFT_CMD_INDENT_RIGHT         _T("IndentRight")              // corresponds to CFmt::m_indentRight
#define TFT_CMD_LINE_TOP             _T("LineTop")                  // corresponds to CFmt::m_eLineTop
#define TFT_CMD_LINE_LEFT            _T("LineLeft")                 // corresponds to CFmt::m_eLineLeft
#define TFT_CMD_LINE_RIGHT           _T("LineRight")                // corresponds to CFmt::m_eLineRight
#define TFT_CMD_LINE_BOTTOM          _T("LineBottom")               // corresponds to CFmt::m_eLineBottom
#define TFT_CMD_HIDDEN               _T("Hidden")                   // corresponds to CFmt::m_eHidden
#define TFT_CMD_SPAN_CELLS           _T("SpanCells")                // corresponds to CFmt::m_eSpanCells
#define TFT_CMD_CUSTOM               _T("Custom")                   // corresponds to CFmt::m_custom
#define TFT_CMD_EXTEND_FONT          _T("ExtendFont")               // corresponds to CFmt::m_bFontExtends
#define TFT_CMD_EXTEND_TEXT_COLOR    _T("ExtendTextColor")          // corresponds to CFmt::m_bTextColorExtends
#define TFT_CMD_EXTEND_FILL_COLOR    _T("ExtendFillColor")          // corresponds to CFmt::m_bFillColorExtends
#define TFT_CMD_EXTEND_LINES         _T("ExtendLines")              // corresponds to CFmt::m_bLinesExtend
#define TFT_CMD_EXTEND_INDENTATION   _T("ExtendIndentation")        // corresponds to CFmt::m_bIndentationExtends

// commands -- CDataCellFmt stuff
#define TFT_CMD_DECIMALS             _T("NumDecimals")              // corresponds to CDataCellFmt::m_eNumDecimals
#define TFT_CMD_HIDE_ZERO_ROW        _T("HideZeroRow")              // corresponds to CDataCellFmt::m_eZeroHidden
#define TFT_CMD_JOIN_SPANNERS        _T("NumJoinSpanners")          //corresponds to CDataCellFmt::m_iNumJoinSpanners

// commands -- CTallyFmt stuff
#define TFT_CMD_TALLY_STAT_TYPE      _T("Type")                     // type of stat (Total, Mean,....)
#define TFT_CMD_TOTALS_POS           _T("TotalsPostion")            // corresponds to CTallyFmt::m_eTotalsPos
#define TFT_CMD_PCT_TYPE             _T("PercentType")              // corresponds to CTallyFmt::m_ePercentType
#define TFT_CMD_PCT_INTERLEAVED      _T("PercentInterleaved")
#define TFT_CMD_PCT_POS              _T("PercentPosition")          // corresponds to CTallyFmt::m_ePercentPos
#define TFT_CMD_COUNTS               _T("Counts")                   // corresponds to CTallyFmt::m_eCounts
#define TFT_CMD_INCLUDE_UNDEF        _T("IncludeUndefined")         // corresponds to CTallyFmt::m_eInclUndef
#define TFT_CMD_DUMP_UNDEF           _T("DumpUndefined")            // corresponds to CTallyFmt::m_eDumpUndef
#define TFT_CMD_STATS_MIN            _T("StatsMin")                 // corresponds to CTallyFmt::m_eMin
#define TFT_CMD_STATS_MAX            _T("StatsMax")                 // corresponds to CTallyFmt::m_eMax
#define TFT_CMD_STATS_STDDEV         _T("StatsStandardDeviation")   // corresponds to CTallyFmt::m_eStdDev
#define TFT_CMD_STATS_VAR            _T("StatsVariance")            // corresponds to CTallyFmt::m_eVariance
#define TFT_CMD_STATS_MEAN           _T("StatsMean")                // corresponds to CTallyFmt::m_eMean
#define TFT_CMD_STATS_NROW           _T("N-Total")                  // corresponds to CTallyFmt::m_eNRow
#define TFT_CMD_STATS_MODE           _T("StatsMode")                // corresponds to CTallyFmt::m_eMode
#define TFT_CMD_STATS_STDERR         _T("StatsStandardError")       // corresponds to CTallyFmt::m_eStdError
#define TFT_CMD_STATS_PROPORTION     _T("StatsProportion")          // corresponds to CTallyFmt::m_eProportion
#define TFT_CMD_STATS_NTILES         _T("StatsNTiles")              // corresponds to CTallyFmt::m_eNTiles
#define TFT_CMD_STATS_MEDIAN         _T("StatsMedian")              // corresponds to CTallyFmt::m_eMedian
#define TFT_CMD_STATS_NUMTILES       _T("NumTiles")                 // number of tiles for n-tiles
#define TFT_CMD_USE_VSET             _T("UseValueSet")              // use vset yes/no for median/ntiles
#define TFT_CMD_INTERVALS_MIN        _T("IntervalsMin")             // lower limit of ranges for median/ntiles
#define TFT_CMD_INTERVALS_MAX        _T("IntervalsMax")             // upper limit of ranges for median/ntiles
#define TFT_CMD_INTERVALS_STEP       _T("IntervalsStep")            // bucket size for ranges for median/ntiles
#define TFT_CMD_MEDIAN_TYPE          _T("MedianType")               // median type, discrete or continuos
#define TFT_CMD_PROP_TYPE            _T("ProportionType")           // type of proportion, percent, percent & total, etc...
#define TFT_CMD_PROP_RANGE           _T("ProportionRange")          // rangelist for proportion

// commands -- CTblFmt stuff
#define TFT_CMD_BORDER_TOP           _T("BorderTop")                // corresponds to CTblFmt::m_eBorderTop
#define TFT_CMD_BORDER_LEFT          _T("BorderLeft")               // corresponds to CTblFmt::m_eBorderLeft
#define TFT_CMD_BORDER_RIGHT         _T("BorderRight")              // corresponds to CTblFmt::m_eBorderRight
#define TFT_CMD_BORDER_BOTTOM        _T("BorderBottom")             // corresponds to CTblFmt::m_eBorderBottom
#define TFT_CMD_LEADER_LEFT          _T("LeftLeadering")            // corresponds to CTblFmt::m_leaderingLeft
#define TFT_CMD_LEADER_RIGHT         _T("RightLeadering")           // corresponds to CTblFmt::m_leaderingRight
#define TFT_CMD_READER_BREAK         _T("ReaderBreak")              // corresponds to CTblFmt::m_eReaderBreak
#define TFT_CMD_INCLUDE_SUBTITLE     _T("IncludeSubTitle")          // corresponds to CTblFmt::m_bIncludeSubTitle
#define TFT_CMD_INCLUDE_PAGENOTE     _T("IncludePageNote")          // corresponds to CTblFmt::m_bIncludePageNote
#define TFT_CMD_INCLUDE_ENDNOTE     _T("IncludeEndNote")          // corresponds to CTblFmt::m_bIncludeEndNote

// commands -- CTabSetFmt stuff
#define TFT_CMD_SEP_THOUSANDS        _T("ThousandsSeparator")       // corresponds to CTabSetFmt::m_sThousands
#define TFT_CMD_SEP_DECIMALS         _T("DecimalSeparator")         // corresponds to CTabSetFmt::m_sDecimal
#define TFT_CMD_ZERO_MASK            _T("ZeroMask")                 // corresponds to CTabSetFmt::m_sZeroMask
#define TFT_CMD_ZROUND_MASK          _T("ZeroRoundMask")            // corresponds to CTabSetFmt::m_sZRoundMask
#define TFT_CMD_SUPPRESSED           _T("Suppressed")               // corresponds to CTabSetFmt::m_sSuppressed
#define TFT_CMD_DIGIT_GROUPING       _T("DigitGrouping")            // corresponds to CTabSetFmt::m_eDigitGrouping
#define TFT_CMD_ZERO_BEFORE_DEC      _T("ZeroBeforeDecimal")        // corresponds to CTabSetFmt::m_bZeroBeforeDecimal
#define TFT_CMD_UNITS                _T("MeasurementSystem")        // corresponds to CTabSetFmt::m_eUnits
#define TFT_CMD_FOREIGN_TABLE        _T("WordTable")                // corresponds to CTabSetFmt::m_foreign.m_sTable
#define TFT_CMD_FOREIGN_AND          _T("WordAnd")                  // corresponds to CTabSetFmt::m_foreign.m_sAnd
#define TFT_CMD_FOREIGN_BY           _T("WordBy")                   // corresponds to CTabSetFmt::m_foreign.m_sBy
#define TFT_CMD_FOREIGN_FOR          _T("WordFor")                  // corresponds to CTabSetFmt::m_foreign.m_sFor
#define TFT_CMD_FOREIGN_TO           _T("WordTo")                   // corresponds to CTabSetFmt::m_foreign.m_sTo
#define TFT_CMD_FOREIGN_WEIGHTED     _T("WordWeighted")             // corresponds to CTabSetFmt::m_foreign.m_sWeighted
#define TFT_CMD_FOREIGN_PERCENTS     _T("WordPercents")             // corresponds to CTabSetFmt::m_foreign.m_sPercents
#define TFT_CMD_FOREIGN_PERCENT      _T("WordPercent")              // corresponds to CTabSetFmt::m_foreign.m_sPercent
#define TFT_CMD_FOREIGN_TOTAL        _T("WordTotal")                // corresponds to CTabSetFmt::m_foreign.m_sTotal
#define TFT_CMD_FOREIGN_UNDEFINED    _T("WordUndefined")            // corresponds to CTabSetFmt::m_foreign.m_sUndefined
#define TFT_CMD_FOREIGN_FREQUENCY    _T("WordFrequency")            // corresponds to CTabSetFmt::m_foreign.m_sFrequency
#define TFT_CMD_FOREIGN_CUMULATIVE   _T("WordCumulative")           // corresponds to CTabSetFmt::m_foreign.m_sCumulative
#define TFT_CMD_FOREIGN_NOLABEL      _T("WordNoLabel")              // corresponds to CTabSetFmt::m_foreign.m_sNoLabel
#define TFT_CMD_FOREIGN_KEY          _T("ForeignKey")               // corresponds to CTabSetFmt foreign keys (from 3.2 and on)
#define TFT_CMD_TITLE_TEMPLATE       _T("TitleTemplate")            // corresponds to CTabSetFmt::m_sTitleTemplate
#define TFT_CMD_CONTINUATION         _T("Continuation")             // corresponds to CTabSetFmt::m_sContinuation

// commands -- CTblPrintFmt stuff
#define TFT_CMD_TBL_LAYOUT           _T("TableLayout")              // corresponds to CTblPrintFmt::m_eTblLayout
#define TFT_CMD_PAGE_BREAK           _T("PageBreakBeforeTable")     // corresponds to CTblPrintFmt::m_ePageBreak
#define TFT_CMD_HORZCENTER           _T("TblHorzCenter")            // corresponds to CTblPrintFmt::m_eCtrHorz
#define TFT_CMD_VERTCENTER           _T("TblVertCenter")            // corresponds to CTblPrintFmt::m_eCtrVert
#define TFT_CMD_START_PAGE           _T("StartPage")                // corresponds to CTblPrintFmt::m_iStartPage
#define TFT_CMD_PAPER_TYPE           _T("PaperType")                // corresponds to CTblPrintFmt::m_ePaperType
#define TFT_CMD_PAPER_ORIENT         _T("PageOrientation")          // corresponds to CTblPrintFmt::m_ePageOrientation
#define TFT_CMD_PRINTER_DEVICE       _T("PrinterDevice")            // corresponds to CTblPrintFmt::m_sPrinterDevice
#define TFT_CMD_PRINTER_DRIVER       _T("PrinterDriver")            // corresponds to CTblPrintFmt::m_sPrinterDriver
#define TFT_CMD_PRINTER_OUTPUT       _T("PrinterOutput")            // corresponds to CTblPrintFmt::m_sPrinterOutput
#define TFT_CMD_MARGIN_TOP           _T("MarginTop")                // corresponds to CTblPrintFmt::m_rcPageMargin.top
#define TFT_CMD_MARGIN_BOTTOM        _T("MarginBottom")             // corresponds to CTblPrintFmt::m_rcPageMargin.bottom
#define TFT_CMD_MARGIN_LEFT          _T("MarginLeft")               // corresponds to CTblPrintFmt::m_rcPageMargin.left
#define TFT_CMD_MARGIN_RIGHT         _T("MarginRight")              // corresponds to CTblPrintFmt::m_rcPageMargin.right
#define TFT_CMD_HEADER_FREQUENCY     _T("HeaderFrequency")          // corresponds to CTblPrintFmt::m_eHeaderFrequency


// arguments
#define TFT_ARG_CENTER               _T("Center")
#define TFT_ARG_LEFT                 _T("Left")
#define TFT_ARG_RIGHT                _T("Right")
#define TFT_ARG_TOP                  _T("Top")
#define TFT_ARG_MID                  _T("Middle")
#define TFT_ARG_BOTTOM               _T("Bottom")
#define TFT_ARG_THICK                _T("Thick")
#define TFT_ARG_THIN                 _T("Thin")
#define TFT_ARG_DOUBLE               _T("Double")
#define TFT_ARG_SINGLE               _T("Single")
#define TFT_ARG_NONE                 _T("None")
#define TFT_ARG_YES                  _T("Yes")
#define TFT_ARG_NO                   _T("No")
#define TFT_ARG_BEFORE               _T("Before")    //dflt; not used
#define TFT_ARG_AFTER                _T("After")
#define TFT_ARG_LAY_LEFT_STD         _T("LeftStandard")
#define TFT_ARG_LAY_LEFT_FACING      _T("LeftFacing")
#define TFT_ARG_LAY_BOTH_STD         _T("BothStandard")
#define TFT_ARG_LAY_BOTH_FACING      _T("BothFacing")
#define TFT_ARG_HEADER_TOP_TBL       _T("TopOfTable")
#define TFT_ARG_HEADER_TOP_PAGE      _T("TopOfPage")
#define TFT_ARG_ABOVE                _T("Above")
#define TFT_ARG_BELOW                _T("Below")
#define TFT_ARG_TOTAL                _T("Total")
#define TFT_ARG_ROW                  _T("Row")
#define TFT_ARG_COL                  _T("Column")
#define TFT_ARG_SAME_AS_PCT          _T("SameAsPercent")
#define TFT_ARG_BYROW                _T("ByRow")
#define TFT_ARG_BYCOL                _T("ByColumn")
#define TFT_ARG_CELL                 _T("Cell")
#define TFT_ARG_PERCENT              _T("Percent")
#define TFT_ARG_PERCENT_AND_TOTAL    _T("PercentAndTotal")
#define TFT_ARG_RATIO                _T("Ratio")
#define TFT_ARG_RATIO_AND_TOTAL      _T("RatioAndTotal")
#define TFT_ARG_THOUSANDS            _T("Thousands")
#define TFT_ARG_INDIC                _T("Indic")
#define TFT_ARG_ZERO                 _T("Zero")
#define TFT_ARG_ONE                  _T("One")
#define TFT_ARG_TWO                  _T("Two")
#define TFT_ARG_THREE                _T("Three")
#define TFT_ARG_FOUR                 _T("Four")
#define TFT_ARG_FIVE                 _T("Five")
#define TFT_ARG_SIX                  _T("Six")
#define TFT_ARG_SEVEN                _T("Seven")
#define TFT_ARG_EIGHT                _T("Eight")
#define TFT_ARG_NINE                 _T("Nine")
#define TFT_ARG_TEN                  _T("Ten")
#define TFT_ARG_METRIC               _T("MetricUnits")
#define TFT_ARG_US                   _T("USUnits")
#define TFT_ARG_DOT                  _T("Dot")
#define TFT_ARG_DOT_SPACE            _T("DotSpace")
#define TFT_ARG_DASH                 _T("Dash")
#define TFT_ARG_DASH_SPACE           _T("DashSpace")
#define TFT_ARG_BOTH                 _T("Both")
#define TFT_ARG_COUNTONLY            _T("CountsOnly")
#define TFT_ARG_PCTONLY              _T("PercentsOnly")
#define TFT_ARG_CONTINUOUS           _T("Continuous")
#define TFT_ARG_DISCRETE             _T("Discrete")
#define TFT_ARG_A4                   _T("A4")
#define TFT_ARG_A3                   _T("A3")
#define TFT_ARG_LETTER               _T("Letter")
#define TFT_ARG_LEGAL                _T("Legal")
#define TFT_ARG_TABLOID              _T("Tabloid")
#define TFT_ARG_PORTRAIT             _T("Portrait")
#define TFT_ARG_LANDSCAPE            _T("Landscape")
#define TFT_ARG_DEFAULT              _T("Default")
#define TFT_ARG_NOTAPPL              _T("NotApplicable")

// format object IDs
#define TFT_FMT_ID_SPANNER           _T("SpannerFmt")         // label for format registry entry for a spanner
#define TFT_FMT_ID_COLHEAD           _T("ColumnHeadFmt")      // label for format registry entry for a column head
#define TFT_FMT_ID_CAPTION           _T("CaptionFmt")         // label for format registry entry for a caption
#define TFT_FMT_ID_STUB              _T("StubFmt")            // label for format registry entry for a stub
#define TFT_FMT_ID_DATACELL          _T("DataCellFmt")        // label for format registry entry for a data cell
#define TFT_FMT_ID_HEADER_LEFT       _T("HeaderLeftFmt")      // label for format registry entry for a header (left)
#define TFT_FMT_ID_HEADER_CENTER     _T("HeaderCenterFmt")    // label for format registry entry for a header (center)
#define TFT_FMT_ID_HEADER_RIGHT      _T("HeaderRightFmt")     // label for format registry entry for a header (right)
#define TFT_FMT_ID_FOOTER_LEFT       _T("FooterLeftFmt")      // label for format registry entry for a footer (left)
#define TFT_FMT_ID_FOOTER_CENTER     _T("FooterCenterFmt")    // label for format registry entry for a footer (center)
#define TFT_FMT_ID_FOOTER_RIGHT      _T("FooterRightFmt")     // label for format registry entry for a footer (right)
#define TFT_FMT_ID_TITLE             _T("TitleFmt")           // label for format registry entry for a title
#define TFT_FMT_ID_SUBTITLE          _T("SubTitleFmt")        // label for format registry entry for a sub title
#define TFT_FMT_ID_PAGENOTE          _T("PageNoteFmt")        // label for format registry entry for a page note
#define TFT_FMT_ID_ENDNOTE           _T("EndNoteFmt")         // label for format registry entry for an end note
#define TFT_FMT_ID_STUBHEAD          _T("StubHead")           // label for format registry entry for primary stubhead
#define TFT_FMT_ID_STUBHEAD_SEC      _T("SecondaryStubHead")  // label for format registry entry for secondary stubhead
#define TFT_FMT_ID_AREA_CAPTION      _T("AreaCaption")        // label for format registry entry for area caption
#define TFT_FMT_ID_TALLY             _T("TallyFmt")           // label for format registry entry for a set of tally info
#define TFT_FMT_ID_TABSET            _T("TabSetFmt")          // label for format registry entry for a tab set
#define TFT_FMT_ID_TABLE             _T("TableFmt")           // label for format registry entry for a set of table info
#define TFT_FMT_ID_TBLPRINT          _T("TblPrintFmt")        // label for format registry entry for table printing info
#define TFT_FMT_ID_TALLY_ROW         _T("RowTallyFmt")        // label for format registry entry for a set of tally info for row variables
#define TFT_FMT_ID_TALLY_COL         _T("ColumnTallyFmt")     // label for format registry entry for a set of tally info for column variables


/////////////////////////////////////////////////////////////////////
// identification of formatting objects ...
enum FMT_ID {
    FMT_ID_INVALID=-1,            // invalid (undefined) registry entry
    FMT_ID_SPANNER=0,             // format registry ID for a spanner
    FMT_ID_COLHEAD,               // format registry ID for a column head
    FMT_ID_CAPTION,               // format registry ID for a caption
    FMT_ID_STUB,                  // format registry ID for a stub
    FMT_ID_DATACELL,              // format registry ID for a data cell
    FMT_ID_HEADER_LEFT,           // format registry ID for a header (left)
    FMT_ID_HEADER_CENTER,         // format registry ID for a header (center)
    FMT_ID_HEADER_RIGHT,          // format registry ID for a header (right)
    FMT_ID_FOOTER_LEFT,           // format registry ID for a footer (left)
    FMT_ID_FOOTER_CENTER,         // format registry ID for a footer (center)
    FMT_ID_FOOTER_RIGHT,          // format registry ID for a footer (right)
    FMT_ID_TITLE,                 // format registry ID for a title
    FMT_ID_SUBTITLE,              // format registry ID for a sub title
    FMT_ID_PAGENOTE,              // format registry ID for a page note
    FMT_ID_ENDNOTE,               // format registry ID for an end note
    FMT_ID_STUBHEAD,              // format registry ID for primary stubhead
    FMT_ID_STUBHEAD_SEC,          // format registry ID for secondary stubhead
    FMT_ID_AREA_CAPTION,          // format registry ID for area caption
    FMT_ID_TALLY,                 // format registry ID for a set of tally info
    FMT_ID_TABSET,                // format registry ID for a tab set
    FMT_ID_TABLE,                 // format registry ID for a set of table info
    FMT_ID_TBLPRINT,              // format registry ID for table printing info
    FMT_ID_TALLY_ROW,             // format registry ID for a set of tally info of row variables
    FMT_ID_TALLY_COL,             // format registry ID for a set of tally info of column variables
};

// for iterating formats ... (change these if we add additional formats to the beginning/end of the above list)
const int FMT_ID_FIRST=(int)FMT_ID_SPANNER;
const int FMT_ID_LAST=(int)FMT_ID_TBLPRINT;

CLASS_DECL_ZTABLEO CIMSAString GetFormatString(FMT_ID id);

/////////////////////////////////////////////////////////////////////
// types and values for formatting objects ...

// font is in CFmtFont below

// color
typedef struct FMT_COLOR {
public:
    COLORREF  m_rgb;
    bool      m_bUseDefault;    //=true if we should get color info from the fmt registry's default

    bool operator==(const struct FMT_COLOR& f) const { return (m_rgb==f.m_rgb && m_bUseDefault==f.m_bUseDefault); }
} FMT_COLOR;

// horizontal alignment
//SAVY to avoid redefinition in VS2010
enum TAB_HALIGN {
    HALIGN_DEFAULT=0,
    HALIGN_LEFT,
    HALIGN_CENTER,
    HALIGN_RIGHT,
};

// vertical alignment
//SAVY to avoid redefinition in VS2010
enum TAB_VALIGN {
    VALIGN_DEFAULT=0,
    VALIGN_TOP,
    VALIGN_MID,
    VALIGN_BOTTOM,
};

// lines
enum LINE {
    LINE_DEFAULT=0,
    LINE_NONE,
    LINE_THIN,
    LINE_THICK,
    LINE_NOT_APPL,   // used by datacellformats only
};

// hidden
enum HIDDEN {
    HIDDEN_DEFAULT=0,
    HIDDEN_YES,
    HIDDEN_NO,
    HIDDEN_NOT_APPL,   // used by datacellformats only
};

// span cells
enum SPAN_CELLS {
    SPAN_CELLS_DEFAULT=0,
    SPAN_CELLS_YES,
    SPAN_CELLS_NO,
    SPAN_CELLS_NOT_APPL,  // used by captions only
};

// custom text
typedef struct CUSTOM {
public:
    CIMSAString     m_sCustomText;        // customized text
    bool            m_bIsCustomized;       //=true if this object is using customized text, =false if the object does not have customized text

    bool operator==(const struct CUSTOM& c) const { return (m_sCustomText==c.m_sCustomText && m_bIsCustomized==c.m_bIsCustomized); }
} CUSTOM;

// indentation (uses an int, but we define a default value)
const int INDENT_DEFAULT=-1;

// number of decimals
enum NUM_DECIMALS {
    NUM_DECIMALS_NOTAPPL=-1,
    NUM_DECIMALS_DEFAULT=0,
    NUM_DECIMALS_ZERO,
    NUM_DECIMALS_ONE,
    NUM_DECIMALS_TWO,
    NUM_DECIMALS_THREE,
    NUM_DECIMALS_FOUR,
    NUM_DECIMALS_FIVE,
};

// table layout (stubs and multi-page)
enum TBL_LAYOUT {
    TBL_LAYOUT_DEFAULT=0,
    TBL_LAYOUT_LEFT_STANDARD,        // stubs on left only, standard page layout (default)
    TBL_LAYOUT_LEFT_FACING,          // stubs on left, facing-pages layout
    TBL_LAYOUT_BOTH_STANDARD,        // stubs on both left and right, standard page layout
    TBL_LAYOUT_BOTH_FACING,          // stubs on both left and right, facing-pages layout
};

// foreign key words
class CLASS_DECL_ZTABLEO FOREIGN_KEYS {
public:
    LPCTSTR GetKey(LPCTSTR sDefault) const;

    void SetKey(LPCTSTR sDefault, LPCTSTR sAlt);

    bool operator==(const FOREIGN_KEYS& f) const;

    FOREIGN_KEYS& operator=(const FOREIGN_KEYS& f);

    void Save(CSpecFile& f) const;

    void GetAllKeys(CStringArray& aDefaults) const;

protected:
    CIMSAString MakeForeignKeyString(CIMSAString sDefaultString ,CIMSAString sAltString) const;

private:

    typedef CMap<CString, LPCTSTR, CString, LPCTSTR> KeyMap;
    KeyMap m_keys;
};

// totals position
enum TOTALS_POSITION {
    TOTALS_POSITION_DEFAULT=0,
    TOTALS_POSITION_NONE,
    TOTALS_POSITION_BEFORE,
    TOTALS_POSITION_AFTER,
};

// percents type and usage
enum PCT_TYPE {
    PCT_TYPE_DEFAULT=0,
    PCT_TYPE_NONE,
    PCT_TYPE_TOTAL,
    PCT_TYPE_ROW,
    PCT_TYPE_COL,
    PCT_TYPE_CELL,
};

// percent position
enum PCT_POS {
    PCT_POS_DEFAULT=0,
    PCT_POS_ABOVE_OR_LEFT,
    PCT_POS_BELOW_OR_RIGHT,
};

// median type
enum MEDIAN_TYPE {
    MEDIAN_TYPE_DEFAULT=0,
    MEDIAN_TYPE_NONE,
    MEDIAN_TYPE_CONTINUOUS,
    MEDIAN_TYPE_DISCRETE,
};

// statistics
enum TALLY_STATISTIC {
    TALLY_STATISTIC_DEFAULT=0,
    TALLY_STATISTIC_YES,
    TALLY_STATISTIC_NO,
};

// include undefined
enum INCLUDE_UNDEF {
    INCLUDE_UNDEF_DEFAULT=0,
    INCLUDE_UNDEF_YES,
    INCLUDE_UNDEF_NO,
};

// dump undefined values
enum DUMP_UNDEF {
    DUMP_UNDEF_DEFAULT=0,
    DUMP_UNDEF_YES,
    DUMP_UNDEF_NO,
};

// digit grouping  (no default for this)
enum DIGIT_GROUPING {
    DIGIT_GROUPING_NONE,
    DIGIT_GROUPING_THOUSANDS,          // what is usually used: 123,456,789
    DIGIT_GROUPING_INDIC,              // Indian grouping:  12,34,56,789
};

// page break before table
enum PAGE_BREAK {
    PAGE_BREAK_DEFAULT=0,
    PAGE_BREAK_YES,
    PAGE_BREAK_NO,
};

// leadering
enum LEADERING {
    LEADERING_DEFAULT=0,
    LEADERING_NONE,
    LEADERING_DOT,
    LEADERING_DOT_SPACE,
    LEADERING_DASH,
    LEADERING_DASH_SPACE,
};

// reader breaks
enum READER_BREAK {
    READER_BREAK_DEFAULT=0,
    READER_BREAK_NONE,          // no reader breaks
    READER_BREAK_ONE,           // insert reader break after each line
    READER_BREAK_TWO,           // insert reader break after each 2 lines
    READER_BREAK_THREE,
    READER_BREAK_FOUR,
    READER_BREAK_FIVE,
    READER_BREAK_SIX,
    READER_BREAK_SEVEN,
    READER_BREAK_EIGHT,
    READER_BREAK_NINE,
    READER_BREAK_TEN,
};

// center table (horizontally or vertically)
enum CENTER_TBL {
    CENTER_TBL_DEFAULT=0,
    CENTER_TBL_YES,
    CENTER_TBL_NO,
};

// starting page number (uses an int, but we define a default value)
const int START_PAGE_DEFAULT=0;

// paper type
enum PAPER_TYPE {
    PAPER_TYPE_DEFAULT=0,
    PAPER_TYPE_A4,
    PAPER_TYPE_A3,
    PAPER_TYPE_LETTER,
    PAPER_TYPE_LEGAL,
    PAPER_TYPE_TABLOID,      // 11" x 17" size
};

// page orientation
enum PAGE_ORIENTATION {
    PAGE_ORIENTATION_DEFAULT=0,
    PAGE_ORIENTATION_PORTRAIT,
    PAGE_ORIENTATION_LANDSCAPE,
};

// header frequency
enum HEADER_FREQUENCY {
    HEADER_FREQUENCY_DEFAULT=0,
    HEADER_FREQUENCY_TOP_TABLE,
    HEADER_FREQUENCY_TOP_PAGE,
    HEADER_FREQUENCY_NONE,
};

// page margins (uses an CRect, but we define a default value for each of the CRect's components)
const int PAGE_MARGIN_DEFAULT=-1;

// measurement units
enum UNITS {
    UNITS_NOT_APPL = -1,
    UNITS_METRIC,
    UNITS_US,
};

// left/right
enum LEFTRIGHT {
    LEFT=0,
    RIGHT,
};

const int CAPTION_LEFT_INDENT = (int)(0.125*1440);
/////////////////////////////////////////////////////////////////////////////
// forward declarations
/////////////////////////////////////////////////////////////////////////////
class CFmt;
class CFmtReg;


/////////////////////////////////////////////////////////////////////////////
//
// helper functions
//
/////////////////////////////////////////////////////////////////////////////
CLASS_DECL_ZTABLEO float TwipsToInches(long lTwips);
CLASS_DECL_ZTABLEO float TwipsToCm(long lTwips);
CLASS_DECL_ZTABLEO long InchesToTwips(float fInches);
CLASS_DECL_ZTABLEO long CmToTwips(float fCm);
CLASS_DECL_ZTABLEO float Round(float fVal, int iPrecision);
CLASS_DECL_ZTABLEO int PointsToTwips(int iPoints);
CLASS_DECL_ZTABLEO void PointsToTwips(CFmt* pFmt);
CLASS_DECL_ZTABLEO int LPToPoints(int iLP, int iLogPixelsY);
CLASS_DECL_ZTABLEO void LPToPoints(CFmt* pFmt, int iLogPixelsY);
CLASS_DECL_ZTABLEO int TwipsToPoints(int iTwips);
CLASS_DECL_ZTABLEO BOOL GetPrinterDevice(LPTSTR pszPrinterName, HGLOBAL* phDevNames, HGLOBAL* phDevMode);

/////////////////////////////////////////////////////////////////////////////
//
//                             CFmtFont
//
// font object; contains CFont pointers that are used for rendering
// table objects throughout the system.  Handles allocation and freeing
// memory for GDI objects.
//
/////////////////////////////////////////////////////////////////////////////
class CLASS_DECL_ZTABLEO CFmtFont : public CObject
{
DECLARE_DYNAMIC(CFmtFont)

// Attributes
private:
    CFont*                         m_pFont;          //=NULL if we should get font info from the fmt registry's default
    static CMapStringToPtr         m_mapFont;        //contains shared pointers to CFonts used by the grid and print views

// Construction and initialization
public:
    CFmtFont()                      { m_pFont=NULL; }
    CFmtFont(const CFmtFont& f)     { m_pFont=f.GetFont(); }
    virtual ~CFmtFont() {}

// Serialization
public:

// Access
public:
    CFont* GetFont(void) const { return m_pFont; }
    void SetFont(CFont* pFont) { m_pFont=pFont; }   // doesn't allocate memory
    void SetFont(LOGFONT* pLF);

    static void Delete(void) {
        POSITION pos=m_mapFont.GetStartPosition();
        CFont* pFont;
        CString sLF;
        while (NULL!=pos) {
            m_mapFont.GetNextAssoc(pos, sLF, (void*&)pFont);
            ASSERT_KINDOF(CFont,pFont);
            pFont->DeleteObject();
            delete pFont;
        }
        m_mapFont.RemoveAll();
    }

// Operators
public:
    bool operator==(const CFmtFont& f) const    { return (m_pFont==f.GetFont());  }
    bool operator!=(const CFmtFont& f) const    { return !(operator==(f));  }
    void operator=(const CFmtFont& f)           { m_pFont=f.GetFont();  }

// Debug support
public:
    virtual void Dump(CDumpContext& dc) const {
        CObject::Dump(dc);
        dc << _T("CFmtFont");
    }
    virtual void AssertValid() const {
        CObject::AssertValid();
        if (m_pFont) {
            ASSERT_VALID(m_pFont);
        }
    }
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CFmtBase
//
// base class for all formatting objects; abstract
//
/////////////////////////////////////////////////////////////////////////////
class CLASS_DECL_ZTABLEO CFmtBase : public CObject
{
DECLARE_DYNAMIC(CFmtBase)

// Attributes
protected:
    FMT_ID       m_ID;          // object ID (ex: FMT_ID_SPANNER)
    int          m_iIndex;      // ID index (0=app default, 1+ for customized objects)
    UNITS                           m_eUnits;             // units (a copy of CTabSetFmt::m_eUnits); used for converting indentation to/from TWIPS during i/o operations
private:
    bool m_bUsed;
// Construction and initialization
public:
    CFmtBase();
    CFmtBase(const CFmtBase& fb);
    virtual ~CFmtBase() {}
    virtual void Init(void)=0;

// Serialization
public:
    virtual bool Build(CSpecFile& specFile, bool bSilent=false);
    virtual bool Save(CSpecFile& specFile) const;
    virtual const csprochar* GetSectionHeading(void) const=0;

// Access
public:
    const FMT_ID GetID(void) const { return m_ID; }
    CIMSAString GetIDString(void) const;
    void SetID(const FMT_ID ID) { m_ID=ID; }
    bool SetID(const CIMSAString& sID);

    const int GetIndex(void) const { return m_iIndex; }
    CIMSAString GetIndexString(void) const;
    void SetIndex(int iIndex) { m_iIndex=iIndex; }
    bool SetIndex(const CIMSAString& sIndex);

    CIMSAString GetIDInfo(void) const {  return GetIDString() + _T(", ") + GetIndexString();    }

    virtual UNITS GetUnits(void) const { return m_eUnits; }
    virtual void SetUnits(UNITS eUnits) { m_eUnits=eUnits; }
    void SetUsedFlag(bool bFlag) { m_bUsed = bFlag;}
    bool IsUsed() {return m_bUsed;}



// Operators
public:
    bool operator==(const CFmtBase& fb) const;
    bool operator!=(const CFmtBase& fb) const;
    void operator=(const CFmtBase& fb);
    virtual void Assign(const CFmtBase& fb)=0;
    virtual CFmtBase* Clone() const = 0; // polymorphic copy
    virtual void CopyNonDefaultValues(CFmtBase* pTarget, const CFmtBase* pDefault) const = 0;
    virtual void CopyNonDefaultValues(const CFmtBase* pDefault)=0;
    virtual bool ContainsDefaultValues(void) const = 0;

// Debug support
protected:
    virtual void Dump(CDumpContext& dc) const=0;
    virtual void AssertValid() const {
        CObject::AssertValid();
        ASSERT(m_ID!=FMT_ID_INVALID);
        ASSERT(m_iIndex>=0);
    }
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CFmt
//
/////////////////////////////////////////////////////////////////////////////
class CLASS_DECL_ZTABLEO CFmt : public CFmtBase
{
DECLARE_DYNAMIC(CFmt)

// Attributes
protected:
    // text attributes
    CFmtFont                        m_fmtFont;            // font (logfont or default)

    // alignment
    TAB_HALIGN                          m_eHorzAlign;         // horizontal alignment (default, left, center, right)
    TAB_VALIGN                          m_eVertAlign;         // vertical alignment (default, top, middle, bottom)

    // color
    FMT_COLOR                       m_colorText;          // text color
    FMT_COLOR                       m_colorFill;          // fill color (aka background)

    // lines, borders and indentation
    int                             m_iIndentLeft;        // left indentation (padding) (in TWIPS) (or INDENT_DEFAULT)
    int                             m_iIndentRight;       // right indentation (padding) (in TWIPS) (or INDENT_DEFAULT)
    LINE                            m_eLineLeft;          // lines on left (default, none, thin, thick)
    LINE                            m_eLineRight;         // lines on right (default, none, thin, thick)
    LINE                            m_eLineTop;           // lines on top (default, none, thin, thick)
    LINE                            m_eLineBottom;        // lines on bottom (default, none, thin, thick)

    // Misc
    SPAN_CELLS                      m_eSpanCells;         // is this a field spanner (applicable for captions only) (default, yes, no, notappl)
    HIDDEN                          m_eHidden;            // should this object be rendered?
    CUSTOM                          m_custom;             // is text for this object customized?

    // extend attributes, for extending properties into associated cells
    bool                            m_bFontExtends;       //=true if the font extends into a stub/colhead's associated cells
    bool                            m_bTextColorExtends;  //=true if the text color extends into a stub/colhead's associated cells
    bool                            m_bFillColorExtends;  //=true if the fill color extends into a stub/colhead's associated cells
    bool                            m_bLinesExtend;       //=true if the lines extend into a stub/colhead's associated cells (applies to vert/horz lines, depending on the object)
    bool                            m_bIndentationExtends;//=true if the indentation (left+right) extends into a stub/colhead's associated cells

// Construction and initialization
public:
    CFmt();
    CFmt(const CFmt& f);
    virtual CFmtBase* Clone() const // polymorphic copy
    {
        return new CFmt(*this);
    }
    virtual ~CFmt() {}
    virtual void Init(void);

// Serialization
public:
    virtual bool Build(CSpecFile& specFile, const CString& sVersion, bool bSilent);
    virtual bool Save(CSpecFile& specFile) const;
    virtual const TCHAR* GetSectionHeading() const { return TFT_SECT_FORMAT; }

// Access
public:
    bool IsFontCustom(void) const { return (m_fmtFont.GetFont()!=NULL); }
    const CFmtFont& GetFmtFont(void) const { return m_fmtFont; }
    CFont* GetFont(void) const { return m_fmtFont.GetFont(); }
    void SetFmtFont(const CFmtFont& fmtFont) { m_fmtFont=fmtFont; }
    void SetFont(CFont* pFont) { m_fmtFont.SetFont(pFont); }
    void SetFont(LOGFONT* pLF) { m_fmtFont.SetFont(pLF); }

    TAB_HALIGN GetHorzAlign(void) const { return m_eHorzAlign; }
    void SetHorzAlign(TAB_HALIGN eHorzAlign) { m_eHorzAlign=eHorzAlign; }

    TAB_VALIGN GetVertAlign(void) const { return m_eVertAlign; }
    void SetVertAlign(TAB_VALIGN eVertAlign) { m_eVertAlign=eVertAlign; }

    bool IsTextColorCustom(void) const { return !m_colorText.m_bUseDefault; }
    const FMT_COLOR& GetTextColor(void) const { return m_colorText; }
    void SetTextColor(const FMT_COLOR& colorText) { m_colorText=colorText; }

    bool IsFillColorCustom(void) const { return !m_colorFill.m_bUseDefault; }
    const FMT_COLOR& GetFillColor(void) const { return m_colorFill; }
    void SetFillColor(const FMT_COLOR& colorFill) { m_colorFill=colorFill; }

    bool IsIndentCustom(LEFTRIGHT lr) const { return !(lr==LEFT?m_iIndentLeft==INDENT_DEFAULT:m_iIndentRight==INDENT_DEFAULT); }
    int GetIndent(LEFTRIGHT lr) const { return (lr==LEFT?m_iIndentLeft:m_iIndentRight); }
    void SetIndent(LEFTRIGHT lr, int iIndent) { if (lr==LEFT) m_iIndentLeft=iIndent; else m_iIndentRight=iIndent; }

    LINE GetLineLeft (void) const { return m_eLineLeft; }
    void SetLineLeft (LINE eLineLeft) { m_eLineLeft=eLineLeft; }

    LINE GetLineRight (void) const { return m_eLineRight; }
    void SetLineRight (LINE eLineRight) { m_eLineRight=eLineRight; }

    LINE GetLineTop (void) const { return m_eLineTop; }
    void SetLineTop (LINE eLineTop) { m_eLineTop=eLineTop; }

    LINE GetLineBottom (void) const { return m_eLineBottom; }
    void SetLineBottom (LINE eLineBottom) { m_eLineBottom=eLineBottom; }

    HIDDEN GetHidden(void) const { return m_eHidden; }
    void SetHidden(HIDDEN eHidden) { m_eHidden=eHidden; }

    SPAN_CELLS GetSpanCells(void) const { return m_eSpanCells; }
    void SetSpanCells(SPAN_CELLS eSpanCells) { m_eSpanCells=eSpanCells; }

    bool IsTextCustom(void) const { return m_custom.m_bIsCustomized; }
    const CUSTOM& GetCustom(void) const { return m_custom; }
    void SetCustom(const CUSTOM& custom) { m_custom=custom; }
    const CIMSAString& GetCustomText() { ASSERT(m_custom.m_bIsCustomized);return m_custom.m_sCustomText; }
    void SetCustomText(const CIMSAString& sTxt) { m_custom.m_sCustomText=sTxt;m_custom.m_bIsCustomized=true; }

    bool GetFontExtends(void) const { return m_bFontExtends; }
    void SetFontExtends(bool bFontExtends=true) { m_bFontExtends=bFontExtends; }

    bool GetTextColorExtends(void) const { return m_bTextColorExtends; }
    void SetTextColorExtends(bool bTextColorExtends=true) { m_bTextColorExtends=bTextColorExtends; }

    bool GetFillColorExtends(void) const { return m_bFillColorExtends; }
    void SetFillColorExtends(bool bFillColorExtends=true) { m_bFillColorExtends=bFillColorExtends; }

    bool GetLinesExtend(void) const { return m_bLinesExtend; }
    void SetLinesExtend(bool bLinesExtend=true) { m_bLinesExtend=bLinesExtend; }

    bool GetIndentationExtends(void) const { return m_bIndentationExtends; }
    void SetIndentationExtends(bool bIndentationExtends=true) { m_bIndentationExtends=bIndentationExtends; }

// Operators
public:
    bool operator==(const CFmt& f) const;
    bool operator!=(const CFmt& f) const;
    void operator=(const CFmt& f);
    virtual void Assign(const CFmtBase& fb) {
        ASSERT(fb.IsKindOf(RUNTIME_CLASS(CFmt)));
        operator=((const CFmt&)fb);
    }

    virtual bool Compare(const CFmtBase& fb) {
        if (fb.IsKindOf(RUNTIME_CLASS(CFmt))) {
            return *(DYNAMIC_DOWNCAST(CFmt, &fb)) == *this;
        }
        else {
            return false;
        }
    }

    virtual void CopyNonDefaultValues(CFmtBase* pTarget, const CFmtBase* pDefault) const;
    virtual void CopyNonDefaultValues(const CFmtBase* pDefault) {
        CFmt fmtTarget;
        fmtTarget=*(DYNAMIC_DOWNCAST(CFmt,this));
        CopyNonDefaultValues(&fmtTarget, pDefault);
        *this=fmtTarget;
    }
    virtual bool ContainsDefaultValues(void) const;

// Debug support
public:
    virtual void Dump(CDumpContext& dc) const {
        CObject::Dump(dc);
        dc << _T("CFmt ID=") << GetIDInfo();

    }
    virtual void AssertValid() const {
        CFmtBase::AssertValid();
    }
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CDataCellFmt
//
// formmatting and style info specific to data cells
//
/////////////////////////////////////////////////////////////////////////////
class CLASS_DECL_ZTABLEO CDataCellFmt : public CFmt
{
DECLARE_DYNAMIC(CDataCellFmt)

// Attributes
protected:
    NUM_DECIMALS                      m_eNumDecimals;     // number of decimals to show
    bool                              m_bHideZero;        //Hide Zero Row /Col
    int                               m_iNumJoinSpanners;   //NumJoinSpanners
// Construction and initialization
public:
    CDataCellFmt();
    CDataCellFmt(const CDataCellFmt& f);
    virtual CFmtBase* Clone() const // polymorphic copy
    {
        return new CDataCellFmt(*this);
    }

    virtual ~CDataCellFmt() {}
    virtual void Init(void);

// Serialization
public:
    bool Build(CSpecFile& specFile, const CString& sVersion, bool bSilent) override ;
    bool Save(CSpecFile& specFile) const override ;
    const TCHAR* GetSectionHeading() const override { return TFT_SECT_FORMAT_DATACELL; }

// Access
public:
    NUM_DECIMALS GetNumDecimals(void) const { return m_eNumDecimals; }
    void SetNumDecimals(NUM_DECIMALS eNumDecimals) { m_eNumDecimals=eNumDecimals; }
    bool GetZeroHidden(void) const { return m_bHideZero; }
    void SetZeroHidden(bool bHidden) { m_bHideZero=bHidden; }
    int  GetNumJoinSpanners()const { return m_iNumJoinSpanners;}
    void SetNumJoinSpanners(int iNumJoinSpanners ) {  m_iNumJoinSpanners = iNumJoinSpanners;}

// Operators
public:
    bool operator==(const CDataCellFmt& f) const;
    bool operator!=(const CDataCellFmt& f) const;
    void operator=(const CDataCellFmt& f);
    virtual void Assign(const CFmtBase& fb) {
        ASSERT(fb.IsKindOf(RUNTIME_CLASS(CDataCellFmt)));
        operator=((const CDataCellFmt&)fb);
    }

    virtual bool Compare(const CFmtBase& fb) {
        if (fb.IsKindOf(RUNTIME_CLASS(CDataCellFmt))) {
            return *(DYNAMIC_DOWNCAST(CDataCellFmt, &fb)) == *this;
        }
        else {
            return false;
        }
    }

    virtual void CopyNonDefaultValues(CFmtBase* pTarget, const CFmtBase* pDefault) const;
    virtual void CopyNonDefaultValues(const CFmtBase* pDefault) {
        CDataCellFmt fmtTarget;
        fmtTarget=*(DYNAMIC_DOWNCAST(CDataCellFmt,this));
        CopyNonDefaultValues(&fmtTarget, pDefault);
        *this=fmtTarget;
    }
    virtual bool ContainsDefaultValues(void) const;

// Debug support
public:
    virtual void Dump(CDumpContext& dc) const {
        CObject::Dump(dc);
        dc << _T("CDataCellFmt ID=") << GetIDInfo();

    }
    virtual void AssertValid() const {
        CFmtBase::AssertValid();
    }
};


class CLASS_DECL_ZTABLEO CTallyVarStatFmt; // format stat types - defined in TllyStat.h

/////////////////////////////////////////////////////////////////////////////
//
//                             CTallyFmt
//
// formatting for tallying
//
/////////////////////////////////////////////////////////////////////////////

class CPre32TallyFmt;

class CLASS_DECL_ZTABLEO CTallyFmt : public CFmtBase
{
DECLARE_DYNAMIC(CTallyFmt)

// Attributes
protected:

    // undefined values stuff
    INCLUDE_UNDEF                   m_eInclUndef;          // include undefined values (default, yes, no)
    DUMP_UNDEF                      m_eDumpUndef;          // dump undefined values (default, yes, no)

    CArray<CTallyVarStatFmt*> m_aStats;

// Construction and initialization
public:
    CTallyFmt();
    CTallyFmt(const CTallyFmt& f);
    virtual ~CTallyFmt();
    virtual void Init(void);
    virtual CFmtBase* Clone() const // polymorphic copy
    {
        return new CTallyFmt(*this);
    }

    const CArray<CTallyVarStatFmt*>& GetStats() const
    {
        return m_aStats;
    }

    void AddStat(CTallyVarStatFmt* pStat);
    void MoveStatTo(int iOrigPos, int iNewPos);
    void RemoveStatAt(int iPos);
    int FindFirstStatPos(LPCTSTR sType);

    bool CompareStats(const CArray<CTallyVarStatFmt*>& aStats) const;
    void CopyStats(const CArray<CTallyVarStatFmt*>& aStats);
    void ClearStats();
    bool HasCounts() const;
    bool HasPercents() const;
    struct InterleavedStatPair {
        int m_first;
        int m_second;
    };
    void GetInterleavedStats(CArray<InterleavedStatPair>& aInterleavedStats) const;
    bool Reconcile(const DictValueSet* pVSet);
// Serialization
public:
    virtual bool Build(CSpecFile& specFile, bool bSilent=false);
    bool BuildPre32(CSpecFile& specFile, CPre32TallyFmt& oldDefRowTallyFmt, CPre32TallyFmt& oldDefColTallyFmt, bool bSilent=false);
    virtual bool Save(CSpecFile& specFile) const;
    virtual const csprochar* GetSectionHeading(void) const { return TFT_SECT_FORMAT_TALLY; }

// Access
public:

    INCLUDE_UNDEF GetInclUndef(void) const { return m_eInclUndef; }
    void SetInclUndef(INCLUDE_UNDEF eInclUndef) { m_eInclUndef=eInclUndef; }

    DUMP_UNDEF GetDumpUndef(void) const { return m_eDumpUndef; }
    void SetDumpUndef(DUMP_UNDEF eDumpUndef) { m_eDumpUndef=eDumpUndef; }

    virtual UNITS GetUnits(void) const { return UNITS_NOT_APPL; }
    virtual void SetUnits(UNITS) {}

// Operators
public:
    bool operator==(const CTallyFmt& f) const;
    bool operator!=(const CTallyFmt& f) const;
    void operator=(const CTallyFmt& f);
    virtual void Assign(const CFmtBase& fb) {
        ASSERT(fb.IsKindOf(RUNTIME_CLASS(CTallyFmt)));
        operator=((const CTallyFmt&)fb);
    }

    virtual void CopyNonDefaultValues(CFmtBase* pTarget, const CFmtBase* pDefault) const;
    virtual void CopyNonDefaultValues(const CFmtBase* pDefault) {
        CTallyFmt fmtTarget;
        fmtTarget=*(DYNAMIC_DOWNCAST(CTallyFmt,this));
        CopyNonDefaultValues(&fmtTarget, pDefault);
        *this=fmtTarget;
    }
    virtual bool ContainsDefaultValues(void) const;

// Debug support
public:
    virtual void Dump(CDumpContext& dc) const {
        CObject::Dump(dc);
        dc << _T("CTallyFmt ID=") << GetIDInfo();
    }
    virtual void AssertValid() const {
        CFmtBase::AssertValid();
    }
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CTblFmt
//
/////////////////////////////////////////////////////////////////////////////
class CLASS_DECL_ZTABLEO CTblFmt : public CFmtBase
{
DECLARE_DYNAMIC(CTblFmt)

// Attributes
protected:
    // borders
    LINE                            m_eBorderLeft;          // left table border (default, none, thin, thick)
    LINE                            m_eBorderRight;         // right table border (default, none, thin, thick)
    LINE                            m_eBorderTop;           // top table border (default, none, thin, thick)
    LINE                            m_eBorderBottom;        // bottom table border (default, none, thin, thick)

    // leadering
    LEADERING                       m_eLeaderingLeft;       // left-side stub leadering (default, none, dot, dot/space, dash, dash/space)
    LEADERING                       m_eLeaderingRight;      // right-side stub leadering (default, none, dot, dot/space, dash, dash/space)
    READER_BREAK                    m_eReaderBreak;         // reader breaks (default, none, 1, 2, ..., 10)

    bool                            m_bIncludeSubTitle;
    bool                            m_bIncludePageNote;
    bool                            m_bIncludeEndNote;
// Construction and initialization
public:
    CTblFmt();
    CTblFmt(const CTblFmt& f);
    virtual ~CTblFmt() {}
    virtual void Init(void);
    virtual CFmtBase* Clone() const // polymorphic copy
    {
        return new CTblFmt(*this);
    }

// Serialization
public:
    virtual bool Build(CSpecFile& specFile, bool bSilent=false);
    virtual bool Save(CSpecFile& specFile) const;
    virtual const csprochar* GetSectionHeading(void) const { return TFT_SECT_FORMAT_TBL; }

// Access
public:
    LINE GetBorderLeft (void) const { return m_eBorderLeft; }
    void SetBorderLeft (LINE eBorderLeft) { m_eBorderLeft=eBorderLeft; }

    LINE GetBorderTop (void) const { return m_eBorderTop; }
    void SetBorderTop (LINE eBorderTop) { m_eBorderTop=eBorderTop; }

    LINE GetBorderRight (void) const { return m_eBorderRight; }
    void SetBorderRight (LINE eBorderRight) { m_eBorderRight=eBorderRight; }

    LINE GetBorderBottom (void) const { return m_eBorderBottom; }
    void SetBorderBottom (LINE eBorderBottom) { m_eBorderBottom=eBorderBottom; }

    LEADERING GetLeadering(LEFTRIGHT lr) const { return (lr==LEFT?m_eLeaderingLeft:m_eLeaderingRight); }
    void SetLeadering(LEFTRIGHT lr, LEADERING eLeadering) { if (lr==LEFT) m_eLeaderingLeft=eLeadering; else m_eLeaderingRight=eLeadering; }

    READER_BREAK GetReaderBreak(void) const { return m_eReaderBreak; }
    void SetReaderBreak(READER_BREAK eReaderBreak) { m_eReaderBreak=eReaderBreak; }

    bool HasSubTitle(void) const { return m_bIncludeSubTitle; }
    void SetIncludeSubTitle(bool bIncludeSubTitle) { m_bIncludeSubTitle=bIncludeSubTitle; }

    bool HasPageNote(void) const { return m_bIncludePageNote; }
    void SetIncludePageNote(bool bIncludePageNote) { m_bIncludePageNote=bIncludePageNote; }

    bool HasEndNote(void) const { return m_bIncludeEndNote; }
    void SetIncludeEndNote(bool bIncludeEndNote) { m_bIncludeEndNote=bIncludeEndNote; }

    virtual UNITS GetUnits(void) const { return UNITS_NOT_APPL; }
    virtual void SetUnits(UNITS) {}

// Operators
public:
    bool operator==(const CTblFmt& t) const;
    bool operator!=(const CTblFmt& t) const;
    void operator=(const CTblFmt& t);
    virtual void Assign(const CFmtBase& fb) {
        ASSERT(fb.IsKindOf(RUNTIME_CLASS(CTblFmt)));
        operator=((const CTblFmt&)fb);
    }

    virtual void CopyNonDefaultValues(CFmtBase* pTarget, const CFmtBase* pDefault) const;
    virtual void CopyNonDefaultValues(const CFmtBase* pDefault) {
        CTblFmt fmtTarget;
        fmtTarget=*(DYNAMIC_DOWNCAST(CTblFmt,this));
        CopyNonDefaultValues(&fmtTarget, pDefault);
        *this=fmtTarget;
    }
    virtual bool ContainsDefaultValues(void) const;

// Debug support
public:
    virtual void Dump(CDumpContext& dc) const {
        CObject::Dump(dc);
        dc << _T("CTblFmt ID=") << GetIDInfo();
    }
    virtual void AssertValid() const {
        CFmtBase::AssertValid();
    }
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CTabSetFmt
//
/////////////////////////////////////////////////////////////////////////////
class CLASS_DECL_ZTABLEO CTabSetFmt : public CFmtBase
{
DECLARE_DYNAMIC(CTabSetFmt)

// Attributes
protected:
    // separators
    CIMSAString                     m_sThousands;          // thousands separator string
    CIMSAString                     m_sDecimal;            // decimal separator string

    // presentation of zero and rounded zero
    CIMSAString                     m_sZeroMask;           // zero mask string (ex: "-")
    CIMSAString                     m_sZRoundMask;         // rounded zero mask string (ex: "*")

    // suppression string
    CIMSAString                     m_sSuppressed;         // suppressed cell mask (ex: "SSS")

    // digit presentation
    DIGIT_GROUPING                  m_eDigitGrouping;       // digit grouping (none, 3-digits, 2-digits for chez Savy)
    bool                            m_bZeroBeforeDecimal;   // include zeros before decimals

    // foreign language
    FOREIGN_KEYS                    m_foreignAlt;              // customized foreign-language text

    // titles stuff
    CIMSAString                     m_sTitleTemplate;      // title template       SAVY!!!
    CIMSAString                     m_sContinuationStr;    // continuation string   SAVY!!!

// Construction and initialization
public:
    CTabSetFmt();
    CTabSetFmt(const CTabSetFmt& t);
    virtual ~CTabSetFmt() {}
    virtual void Init(void);
    virtual CFmtBase* Clone() const // polymorphic copy
    {
        return new CTabSetFmt(*this);
    }

// Serialization
public:
    virtual bool Build(CSpecFile& specFile, bool bSilent=false);
    virtual bool Save(CSpecFile& specFile) const;
    virtual const csprochar* GetSectionHeading(void) const { return TFT_SECT_FORMAT_TABSET; }

// Access
public:
    const CIMSAString& GetThousandsSep(void) const { return m_sThousands; }
    void SetThousandsSep(const CIMSAString& sThousands) { m_sThousands=sThousands; }

    const CIMSAString& GetDecimalSep(void) const { return m_sDecimal; }
    void SetDecimalSep(const CIMSAString& sDecimal) { m_sDecimal=sDecimal; }

    const CIMSAString& GetZeroMask(void) const { return m_sZeroMask; }
    void SetZeroMask(const CIMSAString& sZeroMask) { m_sZeroMask=sZeroMask; }

    const CIMSAString& GetZRoundMask(void) const { return m_sZRoundMask; }
    void SetZRoundMask(const CIMSAString& sZRoundMask) { m_sZRoundMask=sZRoundMask; }

    bool GetZeroBeforeDecimal(void) const { return m_bZeroBeforeDecimal; }
    void SetZeroBeforeDecimal(bool bZeroBeforeDecimal) { m_bZeroBeforeDecimal=bZeroBeforeDecimal; }

    const CIMSAString& GetSuppressed(void) const { return m_sSuppressed; }
    void SetSuppressed(const CIMSAString& sSuppressed) { m_sSuppressed=sSuppressed; }

    DIGIT_GROUPING GetDigitGrouping(void) const { return m_eDigitGrouping; }
    void SetDigitGrouping(DIGIT_GROUPING eDigitGrouping) { m_eDigitGrouping=eDigitGrouping; }

    const FOREIGN_KEYS& GetAltForeignKeys(void) const { return m_foreignAlt; }
    void SetAltForeignKeys(const FOREIGN_KEYS& altforeign) { m_foreignAlt=altforeign; }

    const CIMSAString& GetTitleTemplate(void) const { return m_sTitleTemplate; }
    void SetTitleTemplate(const CIMSAString& sTitleTemplate) { m_sTitleTemplate=sTitleTemplate; }

    const CIMSAString& GetContinuationStr(void) const { return m_sContinuationStr; }
    void SetContinuationStr(const CIMSAString& sContinuationStr) { m_sContinuationStr=sContinuationStr; }

// Operators
public:
    bool operator==(const CTabSetFmt& t) const;
    bool operator!=(const CTabSetFmt& t) const;
    void operator=(const CTabSetFmt& t);
    virtual void Assign(const CFmtBase& fb) {
        ASSERT(fb.IsKindOf(RUNTIME_CLASS(CTabSetFmt)));
        operator=((const CTabSetFmt&)fb);
    }

    virtual void CopyNonDefaultValues(CFmtBase* pTarget, const CFmtBase* pDefault) const;
    virtual void CopyNonDefaultValues(const CFmtBase* pDefault) {
        CTabSetFmt fmtTarget;
        fmtTarget=*(DYNAMIC_DOWNCAST(CTabSetFmt,this));
        CopyNonDefaultValues(&fmtTarget, pDefault);
        *this=fmtTarget;
    }
    virtual bool ContainsDefaultValues(void) const;

// Debug support
public:
    virtual void Dump(CDumpContext& dc) const {
        CObject::Dump(dc);
        dc << _T("CTabSetFmt ID=") << GetIDInfo();
    }
    virtual void AssertValid() const {
        CFmtBase::AssertValid();
    }
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CTblPrintFmt
//
// Notes: for 3.0 release, the user cannot change page orientation, paper
// type, and printer name between tables.  We're leaving them in CTblPrintFmt
// (instead of moving to CTabSetFmt, since they're really app-level) since
// this is a feature that we might change in a future release.
//
/////////////////////////////////////////////////////////////////////////////
class CLASS_DECL_ZTABLEO CTblPrintFmt : public CFmtBase
{
DECLARE_DYNAMIC(CTblPrintFmt)

// Attributes
protected:
    // layout
    TBL_LAYOUT                      m_eTblLayout;           // table layout (default, left std, left facing, both std, both facing)
    PAGE_BREAK                      m_ePageBreak;           // page break before this table (default, yes, no)
    CENTER_TBL                      m_eCtrHorz;             // center table horizontally (default, yes, no)
    CENTER_TBL                      m_eCtrVert;             // center table vertically (default, yes, no)

    // printing options
    PAPER_TYPE                      m_ePaperType;           // paper type (default, A4, A3, letter, legal)
    PAGE_ORIENTATION                m_ePageOrientation;     // page orientation (default, portrait, landscape)
    CString                         m_sPrinterDevice;       // printer device
    CString                         m_sPrinterDriver;       // printer driver
    CString                         m_sPrinterOutput;       // printer output

    // page options
    int                             m_iStartPage;           // starting page number, START_PAGE_DEFAULT (-1) means use default
    CRect                           m_rcPageMargin;         // page margins (in TWIPS; thus 1 inch=1440)

    // misc
    HEADER_FREQUENCY                m_eHeaderFrequency;     // header frequency (default, top of each table only, top of each page, none)


// Construction and initialization
public:
    CTblPrintFmt();
    CTblPrintFmt(const CTblPrintFmt& t);
    virtual ~CTblPrintFmt() {}
    virtual void Init(void);
    virtual CFmtBase* Clone() const // polymorphic copy
    {
        return new CTblPrintFmt(*this);
    }

// Serialization
public:
    virtual bool Build(CSpecFile& specFile, bool bSilent=false);
    virtual bool Save(CSpecFile& specFile) const;
    virtual const csprochar* GetSectionHeading(void) const { return TFT_SECT_FORMAT_TBLPRINT; }

// Operations
public:
    TBL_LAYOUT GetTblLayout(void) const { return m_eTblLayout; }
    void SetTblLayout(TBL_LAYOUT eTblLayout) { m_eTblLayout=eTblLayout; }

    PAGE_BREAK GetPageBreak(void) const { return m_ePageBreak; }
    void SetPageBreak(PAGE_BREAK ePageBreak) { m_ePageBreak=ePageBreak; }

    CENTER_TBL GetCtrHorz(void) const { return m_eCtrHorz; }
    void SetCtrHorz(CENTER_TBL eCtrHorz) { m_eCtrHorz=eCtrHorz; }

    CENTER_TBL GetCtrVert(void) const { return m_eCtrVert; }
    void SetCtrVert(CENTER_TBL eCtrVert) { m_eCtrVert=eCtrVert; }

    int GetStartPage(void) const { return m_iStartPage; }
    void SetStartPage(int iStartPage) { m_iStartPage=iStartPage; }

    PAPER_TYPE GetPaperType(void) const { return m_ePaperType; }
    void SetPaperType(PAPER_TYPE ePaperType) { m_ePaperType=ePaperType; }

    PAGE_ORIENTATION GetPageOrientation(void) const { return m_ePageOrientation; }
    void SetPageOrientation(PAGE_ORIENTATION ePageOrientation) { m_ePageOrientation=ePageOrientation; }

    const CString& GetPrinterDevice(void) const { return m_sPrinterDevice; }
    void SetPrinterDevice(const CString& sPrinterDevice) { m_sPrinterDevice=sPrinterDevice; }

    const CString& GetPrinterDriver(void) const { return m_sPrinterDriver; }
    void SetPrinterDriver(const CString& sPrinterDriver) { m_sPrinterDriver=sPrinterDriver; }

    const CString& GetPrinterOutput(void) const { return m_sPrinterOutput; }
    void SetPrinterOutput(const CString& sPrinterOutput) { m_sPrinterOutput=sPrinterOutput; }

    const CRect& GetPageMargin(void) const { return m_rcPageMargin; }
//    CRect& GetPageMargin(void) { return m_rcPageMargin; }  //lvalue
    void SetPageMargin(const CRect& rcPageMargin) { m_rcPageMargin=rcPageMargin; }

    HEADER_FREQUENCY GetHeaderFrequency(void) const { return m_eHeaderFrequency; }
    void SetHeaderFrequency(HEADER_FREQUENCY eHeaderFrequency) { m_eHeaderFrequency=eHeaderFrequency; }

// Operators
public:
    bool operator==(const CTblPrintFmt& t) const;
    bool operator!=(const CTblPrintFmt& t) const;
    void operator=(const CTblPrintFmt& t);
    virtual void Assign(const CFmtBase& fb) {
        ASSERT(fb.IsKindOf(RUNTIME_CLASS(CTblPrintFmt)));
        operator=((const CTblPrintFmt&)fb);
    }

    virtual void CopyNonDefaultValues(CFmtBase* pTarget, const CFmtBase* pDefault) const;
    virtual void CopyNonDefaultValues(const CFmtBase* pDefault) {
        CTblPrintFmt fmtTarget;
        fmtTarget=*(DYNAMIC_DOWNCAST(CTblPrintFmt,this));
        CopyNonDefaultValues(&fmtTarget, pDefault);
        *this=fmtTarget;
    }
    virtual bool ContainsDefaultValues(void) const;

// Debug support
public:
    virtual void Dump(CDumpContext& dc) const {
        CObject::Dump(dc);
        dc << _T("CTblPrintFmt ID=") << GetIDInfo();
    }
    virtual void AssertValid() const {
        CFmtBase::AssertValid();
    }
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CFmtReg
//
/////////////////////////////////////////////////////////////////////////////
class CLASS_DECL_ZTABLEO CFmtReg : public CObject
{
DECLARE_DYNAMIC(CFmtReg)

// Attributes
protected:
    CArray<CFmtBase*, CFmtBase*>    m_aFmt;         // repository of CFmtBase-derived objects
    CIMSAString                     m_sErrorMsg;    // error message (if found)
// Construction and initialization
public:
    CFmtReg(void);
    virtual ~CFmtReg();

// Access
public:
    CFmtBase* GetFmt(FMT_ID ID, int iIndex=0);
    const CFmtBase* GetFmt(FMT_ID ID, int iIndex=0) const;
    const CFmtBase* GetFmt(const CIMSAString& sID, const CIMSAString& sIndex) const;  // returns NULL if not found

    bool AddFmt(CFmtBase* pFmtBase);//Savy (R)  20090406

    void RemoveAll(void);
    void Remove(FMT_ID ID, int iIndex);

    // sets units in all CFmts in registry.
    // they should all be in synch with CTabSet units (others are just copies) so use this rather
    // than calling CTabSet method directly.
    void SetUnits(UNITS eUnits);

    // copy only default values from another format registry
    void CopyDefaults(const CFmtReg& source);
    void ReconcileFmtsForPaste(CFmtReg& fmtClipBoard,CMap<CFmtBase*,CFmtBase*,CFmtBase*,CFmtBase*>& aMapOldFmts2NewFmts);

// Querying
public:
    static bool IsCustomFmtID(const CFmtBase& fmt) { return fmt.GetIndex()!=0; }
    int GetNextCustomFmtIndex(const CFmtBase& fmtSearch) const;
    int GetNextCustomFmtIndex(FMT_ID id) const;
    int GetNextCustomFmtIndex(CFmtBase* pFmtSearch) const { return GetNextCustomFmtIndex(*pFmtSearch); }

    const CIMSAString& GetErrorMsg(void) const { return m_sErrorMsg; }
    void SetErrorMsg(const CIMSAString& sErrorMsg) { m_sErrorMsg=sErrorMsg; }
    void ResetErrorMsg(void) { m_sErrorMsg.Empty(); }

// Serialization
public:
    bool Build(CSpecFile& specFile, const CIMSAString& sVersion, bool bSilent=false);  // does not read headers; use with XTS
    bool Build(const CIMSAString& sTFTFile);                // reads header; use with TFT
    bool Save(CSpecFile& specFile, bool bDefaultOnly = false) const; // does not save out headers; use with XTS
    bool Save(const CIMSAString& sTFTFile, bool bDefaultOnly) const; // saves out headers; use with TFT
    //Savy (R) to clear the unused format 20090401
    void ClearUnUsedFmts();
    //Savy- Reset Fmt flag for Non-Default Formats
    void ResetFmtUsedFlag() ;
// debug support
public:
    void Debug(void) const {
        TRACE(_T("\n\nformat registry contents:\n"));
        for (int i=0 ; i<m_aFmt.GetSize() ; i++) {
            TRACE(m_aFmt[i]->GetIDInfo()+_T("\n"));
        }
        TRACE(_T("\n"));
    }
    virtual void AssertValid() const {
        CObject::AssertValid();
        for (int i=0 ; i<m_aFmt.GetSize() ; i++) {
            ASSERT_VALID(m_aFmt[i]);
        }
    }
};
