#pragma once

//---------------------------------------------------------------------------
//  File name: DefSlot.h
//
//  Description:
//          Header for Slot definition
//
//  History:    Date       Author   Comment
//              ---------------------------
//              16 Jul 01   DVB     Created
//
// -------------------------------------------------------------------------------------
// Below are the rule followed by the TableAcum operators and type equivalent
//
// Ord  Name                    Opera   Struct                  Validate
// ---  -------                 -----   ---------               -----------------
//  1   Crosstab                Sum     CSlotCTab               Same Dimension and break
//  2   Ctab_Mean               Sum     CSlotMean               Same Dimension and break
//  3   Ctab_Smean              Sum     CSlotSMean              Same Dimension and break
//  4   Ctab_Stable             Sum     CSlotSTable             Same Dimension and break
//  5   Ctab_Hotdeck            None    CSlotHotdeck            None
//  6   Ctab_Freq               Merge   CSlotFreq               Same break
//  7   Ctab_FreqDisjoint1Dim   Merge   CSlotFreqDisjoint1Dim   Same break
//  8   Ctab_FreqDisjoint2Dim   Merge   CSlotFreqDisjoint2Dim   Same break
//  9   Ctab_FreqDisjoint3Dim   Merge   CSlotFreqDisjoint3Dim   Same break
//  10  Ctab_MinTable           Min     CStatSlotMin            Same Dimension and break
//  11  Ctab_MaxTable           Max     CStatSlotMax            Same Dimension and break
//  12  Ctab_Percent            Sum     CStatSlotPct            Same Dimension and break
//  13  Ctab_Mode               Sum     CStatSlotMode           Same Dimension and break
//  14  Ctab_Median             Sum     CStatSlotMedian         Same Dimension and break
//  15  Ctab_Percentil          Sum     CStatSlotPercentil      Same Dimension and break
//  16  Ctab_StdDev             Sum     CStatSlotStdDev         Same Dimension and break
//  17  Ctab_Variance           Sum     CStatSlotVariance       Same Dimension and break
//  18  Ctab_StdErr             Sum     CStatSlotStdErr         Same Dimension and break
//  19  Ctab_ValidPct           Sum     CStatSlotValidPct       Same Dimension and break
//  20  Ctab_Prop               Sum     CStatSlotProp           Same Dimension and break
//  21  Ctab_StatMean           Sum     CStatSlotMean           Same Dimension and break
// -------------------------------------------------------------------------------------

#pragma pack( push, defslot_include ) // rcl, Jun 18 2004

#pragma pack(1)

// Normal Slots
typedef struct {
    double  m_dFreq;
} CSlotCTab;            // Type 1


// Special Slots
typedef struct {                        // Acumulator structure for MEAN table
    double  sumx;                       // Sum of dependent variable
    double  sumw;                       // Frequency
    double  sumxw;                      // Weighted sum of dependent variable
    double  sumxw2;                     // Sum of dependent variable raised to 2
} CSlotMean;            // Type 2


typedef struct {                        // Accumulator structure for SMEAN table
    double  num_weight;
    double  num_square;
    double  den_weight;
    double  den_square;
    double  den_unweight;
    double  cum_weight;
    double  cros_prod;
    double  cum_cases;
} CSlotSMean;           // Type 3


typedef struct {
    double  m_dFreq;
} CSlotSTable;          // Type 4


typedef struct {
    double  m_dFreq;
} CSlotHotdeck;         // Type 5


// Table Type 6
typedef struct {
    void*   m_pCode;                    // See max code length
    double  m_dFreq;
} CSlotFreq;            // Type 6


typedef struct {
    void*   m_pCode;                    // See max code length
    double  m_dFreq;
    int     aIndex[1];
} CSlotFreqDisjoint1Dim;  // Type 7


typedef struct {
    void*   m_pCode;                    // See max code length
    double  m_dFreq;
    int     aIndex[2];
} CSlotFreqDisjoint2Dim;  // Type 8


typedef struct {
    void*   m_pCode;                    // See max code length
    double  m_dFreq;
    int     aIndex[3];
} CSlotFreqDisjoint3Dim;  // Type 9


// Stat Slots MIN
typedef struct {        // Accumulator structure for MIN table
    double  m_dFreq;
} CStatSlotMin,         // Type 10
  CStatSlotMax,         // Type 11
  CStatSlotPct,         // Type 12
  CStatSlotMode,        // Type 13
  CStatSlotMedian,      // Type 14
  CstatSlotPercentil;   // Type 15

// Stat
typedef struct {
    double  m_dSumX;
    double  m_dSumX2;
    double  m_dNumCases;
} CStatSlotStdDev,      // Type 16
  CStatSlotVariance,    // Type 17
  CStatSlotStdErr;      // Type 18

typedef struct {
    double  m_dFreq;
    double  m_dNumCases;
} CStatSlotValidPct,    // Type 19
  CStatSlotProp;        // Type 20


// Stat Slot Mean
typedef struct {
        double  m_dSumX;
        double  m_dNumCases;
} CStatSlotMean;        // Type 21

#pragma pack( pop, defslot_include ) // rcl, Jun 18 2004
