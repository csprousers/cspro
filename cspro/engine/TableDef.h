#pragma once

class CTableDef {

public:
    enum ETableType {
            Ctab_NoType             = 0,
            Ctab_Crosstab           = 1,
            Ctab_Mean               = 2,
            Ctab_SMean              = 3,
            Ctab_STable             = 4,
            Ctab_Hotdeck            = 5,
            Ctab_Freq               = 6,
            Ctab_FreqDisjoint1Dim   = 7,
            Ctab_FreqDisjoint2Dim   = 8,
            Ctab_FreqDisjoint3Dim   = 9,
            Ctab_MinTable           = 10,
            Ctab_MaxTable           = 11,
            Ctab_Percent            = 12,
            Ctab_Mode               = 13,
            Ctab_Median             = 14,
            Ctab_Percentil          = 15,
            Ctab_StdDev             = 16,
            Ctab_Variance           = 17,
            Ctab_StdErr             = 18,
            Ctab_ValidPct           = 19,
            Ctab_Prop               = 20,
            Ctab_StatMean           = 21,
            Ctab_Total              = 22,
            Ctab_GrandTotal         = 23,
            Ctab_NotAppl            = 24
    };
};
