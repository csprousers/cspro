//***************************************************************************
//  File name: TllyStat.cpp
//
//  Description:
//       Stat classes for tally format.
//
//
//***************************************************************************
#include "StdAfx.h"
#include "TllyStat.h"
#include <zUtilO/ArrUtil.h>
#include <math.h>

//
// Helper functions
//
//
//
namespace {

// find special value in vset if it exists
const DictValue* FindSpecialValueInVSet(const DictValueSet& dict_value_set, double special_value)
{
    for( const auto& dict_value : dict_value_set.GetValues() )
    {
        if( dict_value.IsSpecialValue(special_value) )
            return &dict_value;
    }

    return nullptr;
}

// find index of first tab val in array that matches particular type
int FindTabVal(const CArray<CTabValue*,CTabValue*>&arrTabVals , TABVAL_TYPE eValType)
{
    for (int iTV = 0; iTV < arrTabVals.GetCount(); ++iTV) {
        CTabValue* pTV = arrTabVals.GetAt(iTV);
        if (pTV && pTV->GetTabValType() == eValType) {
            return iTV;
        }
    }
    return NONE;
}

// find index of first tab val in array that matches particular label
int FindTabVal(const CArray<CTabValue*,CTabValue*>&arrTabVals , LPCTSTR sLabel)
{
    for (int iTV = 0; iTV < arrTabVals.GetCount(); ++iTV) {
        CTabValue* pTV = arrTabVals.GetAt(iTV);
        if (pTV && pTV->GetText().Compare(sLabel) == 0) {
            return iTV;
        }
    }
    return NONE;
}

// interface for label transforming functor
struct LabelTransformer
{
    virtual CIMSAString operator()(LPCTSTR sLabel) = 0;
};

// functor to add % to label
struct LabelPrependPercent : public LabelTransformer
{
    virtual CIMSAString operator()(LPCTSTR sLabel)
    {
        return CIMSAString(_T("% ")) + sLabel;
    }
};

// compute number of tabvals for a vset taking into account custom special value settings
int GetNumValsWithCustSpecials(const DictValueSet& dict_value_set, const CustSpecialValSettings* custSpecials)
{
    int iNumValues = 0;
    if (custSpecials && custSpecials->GetUseCustomSpecVal()) {
        // count only the non-special vals
        for( const auto& dict_value : dict_value_set.GetValues() ) {
            if( !dict_value.IsSpecial() ) {
                ++iNumValues;
            }
        }

        // add in specials based on custom settings
        if (custSpecials->GetUseSpecValMissing()) {
            ++iNumValues;
        }
        if (custSpecials->GetUseSpecValRefused()) {
            ++iNumValues;
        }
        if (custSpecials->GetUseSpecValDefault()) {
            ++iNumValues;
        }
        if (custSpecials->GetUseSpecValNotAppl()) {
            ++iNumValues;
        }
        if (custSpecials->GetUseSpecValUndefined()) {
            ++iNumValues;
        }
    }
    else {
        // no custom specials - use all values in vset
        iNumValues = dict_value_set.GetNumValues();
    }

    return iNumValues;
}

void AddCustSpecialVal(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                       const DictValueSet* pVSet,
                       std::optional<double> special_value, TABVAL_TYPE eTabValType, LPCTSTR sDefaultLabel,
                       LabelTransformer* pTransformer)
{
    CTabValue* pTabVal = new CTabValue();
    CIMSAString sLabel(sDefaultLabel);

    if( const DictValue* dict_value; special_value.has_value() && ( dict_value = FindSpecialValueInVSet(*pVSet, *special_value) ) != nullptr )
        sLabel = dict_value->GetLabel();

    pTabVal->SetText(pTransformer ? (*pTransformer)(sLabel) : sLabel);
    pTabVal->SetTabValType(eTabValType);
    pTabVal->SetStatId(iStatIndex);
    aTabValue.Add(pTabVal);
}

void ReconcileCustSpecialVal(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                             CArray<CTabValue*,CTabValue*>& aExistingTabValue, const DictValueSet* pVSet,
                             std::optional<double> special_value, TABVAL_TYPE eTabValType, LPCTSTR sDefaultLabel,
                             LabelTransformer* pTransformer)
{
    CIMSAString sLabel(sDefaultLabel);

    if( const DictValue* dict_value; special_value.has_value() && ( dict_value = FindSpecialValueInVSet(*pVSet, *special_value) ) != nullptr )
        sLabel = dict_value->GetLabel();

    if (pTransformer) {
        sLabel = (*pTransformer)(sLabel);
    }
    int iTV = FindTabVal(aExistingTabValue, eTabValType);
    CTabValue* pTabVal;
    if (iTV != NONE) {
        // found existing tab val - use it
        pTabVal = aExistingTabValue[iTV];
        aExistingTabValue[iTV] = NULL;
        CDataCellFmt* pDataCellFmt =  pTabVal->GetDerFmt();
        if((pDataCellFmt && !pDataCellFmt->IsTextCustom())|| !pDataCellFmt ){
            pTabVal->SetText(sLabel);
        }
    }
    else {
        // none existing - create new one
        pTabVal = new CTabValue();
        pTabVal->SetText(sLabel);
        pTabVal->SetTabValType(eTabValType);
    }
    pTabVal->SetStatId(iStatIndex);
    aTabValue.Add(pTabVal);
}

// get tab vals that correspond to a vset
void GetTabValsVSet(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex, const DictValueSet* dict_value_set,
                    TABVAL_TYPE eTabValType, const CustSpecialValSettings* custSpecials,
                    LabelTransformer* pTransformer = NULL)
{
    if (dict_value_set != nullptr) {
        for( const auto& dict_value : dict_value_set->GetValues() ) {
            if (custSpecials && custSpecials->GetUseCustomSpecVal() && dict_value.IsSpecial()) {
                continue;
            }
            CTabValue* pTabVal = new CTabValue();
            CIMSAString sLabel = pTransformer ? (*pTransformer)(dict_value.GetLabel()) : dict_value.GetLabel();
            pTabVal->SetText(sLabel);
            pTabVal->SetStatId(iStatIndex);
            pTabVal->SetTabValType(eTabValType);
            aTabValue.Add(pTabVal);
        }

        // add in custom special values
        if (custSpecials && custSpecials->GetUseCustomSpecVal()) {
            if (custSpecials->GetUseSpecValMissing()) {
                AddCustSpecialVal(aTabValue, iStatIndex, dict_value_set, MISSING, SPECIAL_MISSING_TABVAL, _T("Missing"), pTransformer);
            }
            if (custSpecials->GetUseSpecValRefused()) {
                AddCustSpecialVal(aTabValue, iStatIndex, dict_value_set, REFUSED, SPECIAL_REFUSED_TABVAL, _T("Refused"), pTransformer);
            }
            if (custSpecials->GetUseSpecValDefault()) {
                AddCustSpecialVal(aTabValue, iStatIndex, dict_value_set, DEFAULT, SPECIAL_DEFAULT_TABVAL, _T("Default"), pTransformer);
            }
            if (custSpecials->GetUseSpecValNotAppl()) {
                AddCustSpecialVal(aTabValue, iStatIndex, dict_value_set, NOTAPPL, SPECIAL_NOTAPPL_TABVAL, _T("NotApplicable"), pTransformer);
            }
            if (custSpecials->GetUseSpecValUndefined()) {
                AddCustSpecialVal(aTabValue, iStatIndex, dict_value_set, std::nullopt, SPECIAL_UNDEFINED_TABVAL, _T("Undefined"), pTransformer);
            }
        }
    }
}

// reconcile tab vals to vset on dict/table change
bool ReconcileTabValsVSet(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                          CArray<CTabValue*,CTabValue*>& aExistingTabValue, const DictValueSet* pVSet,
                          TABVAL_TYPE eTabValType, const CustSpecialValSettings* custSpecials,
                          LabelTransformer* pTransformer = NULL)
{
    bool bChanged = false;
    if (pVSet != NULL) {
        for( const auto& dict_value : pVSet->GetValues() ) {
            // ignore specials - deal with them later
            if (custSpecials && custSpecials->GetUseCustomSpecVal() && dict_value.IsSpecial()) {
                continue;
            }

            // look for an existing tab val that matches the vset value
            CIMSAString sTransformedLabel = pTransformer ? (*pTransformer)(dict_value.GetLabel()) : dict_value.GetLabel();

            int iTV = FindTabVal(aExistingTabValue, sTransformedLabel);
            if (iTV != NONE) {
                CTabValue* pExistTabVal = aExistingTabValue[iTV];
                aExistingTabValue[iTV] = NULL; // remove from old list
                pExistTabVal->SetStatId(iStatIndex);
                aTabValue.Add(pExistTabVal); // add to new list
            }
            else {
                // couldn't match to an existing name, create a new one
                // FIXMEJH: try to match based on index
                CTabValue* pTabVal = new CTabValue();
                pTabVal->SetText(sTransformedLabel);
                pTabVal->SetStatId(iStatIndex);
                pTabVal->SetTabValType(eTabValType);
                aTabValue.Add(pTabVal);
                bChanged = true;
            }
        }

        // add in custom specials
        if (custSpecials && custSpecials->GetUseCustomSpecVal()) {
            if (custSpecials->GetUseSpecValMissing()) {
                ReconcileCustSpecialVal(aTabValue, iStatIndex, aExistingTabValue,
                                        pVSet, MISSING, SPECIAL_MISSING_TABVAL, _T("Missing"), pTransformer);
            }
            if (custSpecials->GetUseSpecValRefused()) {
                ReconcileCustSpecialVal(aTabValue, iStatIndex, aExistingTabValue,
                                        pVSet, REFUSED, SPECIAL_REFUSED_TABVAL, _T("Refused"), pTransformer);
            }
            if (custSpecials->GetUseSpecValDefault()) {
                ReconcileCustSpecialVal(aTabValue, iStatIndex, aExistingTabValue,
                                        pVSet, DEFAULT, SPECIAL_DEFAULT_TABVAL, _T("Default"), pTransformer);
            }
            if (custSpecials->GetUseSpecValNotAppl()) {
                ReconcileCustSpecialVal(aTabValue, iStatIndex, aExistingTabValue,
                                        pVSet, NOTAPPL, SPECIAL_NOTAPPL_TABVAL, _T("NotApplicable"), pTransformer);
            }
            if (custSpecials->GetUseSpecValUndefined()) {
                ReconcileCustSpecialVal(aTabValue, iStatIndex, aExistingTabValue,
                                        pVSet, std::nullopt, SPECIAL_UNDEFINED_TABVAL, _T("Undefined"), pTransformer);
            }
        }

        // check if there are any old ones that will be deleted
        for (int i = 0; i < aExistingTabValue.GetCount(); ++i) {
            if (aExistingTabValue.GetAt(i) != NULL) {
                bChanged = true;
            }
        }
    }

    return bChanged;
}

// find index of first stat of this type in array of stats or return -1 if not found
int FindFirstStatIndex(const CArray<CTallyVarStatFmt*>& aStats, LPCTSTR sType)
{
    for (int iStat = 0; iStat < aStats.GetCount(); ++iStat) {
        if (!_tcscmp(aStats.GetAt(iStat)->GetType(), sType)) {
            return iStat;
        }
    }
    return -1;
}

}; // anon namespace

//
// CTallyVarStatFmt
//
// Base class for all stats.
//

bool CTallyVarStatFmt::Build(CSpecFile& specFile, bool bSilent)
{
    UNREFERENCED_PARAMETER(specFile);
    UNREFERENCED_PARAMETER(bSilent);
    return true;
}

void CTallyVarStatFmt::Save(CSpecFile& specFile)
{
    specFile.PutLine(TFT_SECT_TALLY_STAT);
    specFile.PutLine(TFT_CMD_TALLY_STAT_TYPE, GetType());
}

bool CTallyVarStatFmt::operator==(const CTallyVarStatFmt& rhs) const
{
    return rhs.GetType() == GetType();
}

// reconcile stat ranges on vset change
bool CTallyVarStatFmt::ReconcileStatRanges(const DictValueSet* pVSet)
{
    UNREFERENCED_PARAMETER(pVSet);
    return false;
}

///////////////////////////////////////////////////////////
//
// CTallyVarStatFmt::GetTabVals
//
// copy categories for the stat into array of tab vars
///////////////////////////////////////////////////////////
void CTallyVarStatFmt::GetTabVals(CArray<CTabValue*,
                                        CTabValue*>& aTabValue,
                                        int iStatIndex,
                                        const DictValueSet* pVSet,
                                        const CustSpecialValSettings* custSpecials,
                                        const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                        const FOREIGN_KEYS* pForeignKeys)
{
    UNREFERENCED_PARAMETER(pVSet);
    UNREFERENCED_PARAMETER(custSpecials);
    UNREFERENCED_PARAMETER(aNeighborStats);

    CTabValue* pTabVal = new CTabValue();
    CIMSAString sLabel = GetType();
    if (pForeignKeys) {
        sLabel = pForeignKeys->GetKey(sLabel);
    }
    pTabVal->SetText(GetType());
    pTabVal->SetTabValType(GetTabValType());
    pTabVal->SetStatId(iStatIndex);
    aTabValue.Add(pTabVal);
}

///////////////////////////////////////////////////////////
//
// CTallyVarStatFmt::ReconcileTabVals
//
// reconcile tab vals on stat or dictionary change
///////////////////////////////////////////////////////////
bool CTallyVarStatFmt::ReconcileTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                                             CArray<CTabValue*,CTabValue*>& aExistingTabValue, const DictValueSet* pVSet,
                                             const CustSpecialValSettings* custSpecials,
                                             const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                             const FOREIGN_KEYS* pForeignKeys)
{
    UNREFERENCED_PARAMETER(pVSet);
    UNREFERENCED_PARAMETER(custSpecials);
    UNREFERENCED_PARAMETER(aNeighborStats);

    ASSERT(aExistingTabValue.GetSize() == 1);
    bool bChanged = false;
    CTabValue *pTabVal = aExistingTabValue.GetAt(0);
    CDataCellFmt* pDataCellFmt =  pTabVal->GetDerFmt();
    if((pDataCellFmt && !pDataCellFmt->IsTextCustom())|| !pDataCellFmt ){
        CIMSAString sLabel = GetType();
        if (pForeignKeys) {
            sLabel = pForeignKeys->GetKey(sLabel);
        }
        if (pTabVal->GetText() != sLabel) {
            bChanged = true;
            pTabVal->SetText(sLabel);
        }
    }
    pTabVal->SetStatId(iStatIndex);
    aTabValue.Add(pTabVal);
    aExistingTabValue.RemoveAt(0);

    return bChanged;
}


//
// CTallyVarStatFmtCounts
//


// copy categories for the stat into array of tab vars
void CTallyVarStatFmtCounts::GetTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex, const DictValueSet* pVSet,
                                        const CustSpecialValSettings* custSpecials,
                                        const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                        const FOREIGN_KEYS* pForeignKeys)
{
    UNREFERENCED_PARAMETER(aNeighborStats);
    UNREFERENCED_PARAMETER(pForeignKeys);
    GetTabValsVSet(aTabValue, iStatIndex, pVSet, GetTabValType(), custSpecials);
}

// reconcile tab vals on stat or dictionary change
bool CTallyVarStatFmtCounts::ReconcileTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                                              CArray<CTabValue*,CTabValue*>& aExistingTabValue, const DictValueSet* pVSet,
                                              const CustSpecialValSettings* custSpecials,
                                              const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                              const FOREIGN_KEYS* pForeignKeys)
{
    UNREFERENCED_PARAMETER(aNeighborStats);
    UNREFERENCED_PARAMETER(pForeignKeys);
    return ReconcileTabValsVSet(aTabValue, iStatIndex, aExistingTabValue, pVSet, GetTabValType(), custSpecials);
}

// gen code for crosstab statement (stat portion)
CIMSAString CTallyVarStatFmtCounts::GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
{
    UNREFERENCED_PARAMETER(aNeighborStats);
    return CIMSAString(_T("FREQ, "));
}

//
// CTallyVarStatFmtTotal
//
//

///////////////////////////////////////////////////////////
//
// CTallyVarStatFmtTotal::GetTabVals
//
// copy categories for the stat into array of tab vars
///////////////////////////////////////////////////////////
void CTallyVarStatFmtTotal::GetTabVals(CArray<CTabValue*,
                                        CTabValue*>& aTabValue,
                                        int iStatIndex,
                                        const DictValueSet* pVSet,
                                        const CustSpecialValSettings* custSpecials,
                                        const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                        const FOREIGN_KEYS* pForeignKeys)
{
    UNREFERENCED_PARAMETER(pVSet);
    UNREFERENCED_PARAMETER(custSpecials);

    // check for percents only - in that case we use Number instead of Total
    bool bPercentsOnly = (FindFirstStatIndex(aNeighborStats, _T("Counts")) == NONE) &&
                         (FindFirstStatIndex(aNeighborStats, _T("Percents")) != NONE);

    CTabValue* pTabVal = new CTabValue();
    CIMSAString sLabel = bPercentsOnly ? _T("Number") : GetType();
    if (pForeignKeys) {
        sLabel = pForeignKeys->GetKey(sLabel);
    }
    pTabVal->SetText(sLabel);
    pTabVal->SetTabValType(GetTabValType());
    pTabVal->SetStatId(iStatIndex);
    aTabValue.Add(pTabVal);
}

///////////////////////////////////////////////////////////
//
// CTallyVarStatFmt::ReconcileTabVals
//
// reconcile tab vals on stat or dictionary change
///////////////////////////////////////////////////////////
bool CTallyVarStatFmtTotal::ReconcileTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                                             CArray<CTabValue*,CTabValue*>& aExistingTabValue, const DictValueSet* pVSet,
                                             const CustSpecialValSettings* custSpecials,
                                             const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                             const FOREIGN_KEYS* pForeignKeys)
{
    UNREFERENCED_PARAMETER(pVSet);
    UNREFERENCED_PARAMETER(custSpecials);

    ASSERT(aExistingTabValue.GetSize() == 1);

    // check for percents only - in that case we use Number instead of Total
    bool bPercentsOnly = (FindFirstStatIndex(aNeighborStats, _T("Counts")) == NONE) &&
                         (FindFirstStatIndex(aNeighborStats, _T("Percents")) != NONE);

    bool bChanged = false;
    CTabValue *pTabVal = aExistingTabValue.GetAt(0);
    CDataCellFmt* pDataCellFmt =  pTabVal->GetDerFmt();
    if((pDataCellFmt && !pDataCellFmt->IsTextCustom())|| !pDataCellFmt ){
        CIMSAString sLabel = bPercentsOnly ? _T("Number") : GetType();
        if (pForeignKeys) {
            sLabel = pForeignKeys->GetKey(sLabel);
        }
        if (pTabVal->GetText() != sLabel) {
            bChanged = true;
            pTabVal->SetText(sLabel);
        }
    }
    pTabVal->SetStatId(iStatIndex);
    aTabValue.Add(pTabVal);
    aExistingTabValue.RemoveAt(0);

    return bChanged;
}

//
// CTallyVarStatFmtPercent
//
//

// convert pcttype enum to string
PctType PctTypeFromString(const CIMSAString& s)
{
    if (s.CompareNoCase(_T("ROW")) == 0) {
        return PT_ROW;
    }
    else if (s.CompareNoCase(_T("COLUMN")) == 0) {
        return PT_COL;
    }
    else if (s.CompareNoCase(_T("TOTAL")) == 0) {
        return PT_TOTAL;
    }

    return (PctType) -1;
}


// return pct type as string for code generation
CIMSAString PctTypeToString(PctType pt)
{
    switch (pt) {
        case PT_ROW:
            return CIMSAString(_T("Row"));
        case PT_COL:
            return CIMSAString(_T("Column"));
        case PT_TOTAL:
            return CIMSAString(_T("Total"));
    }
    ASSERT(!_T("Invalid percent type"));
    return CIMSAString();
}

// constructor
CTallyVarStatFmtPercent::CTallyVarStatFmtPercent()
: m_pctType(PT_ROW),
  m_bInterleaved(true)
{
}

// copy constructor
CTallyVarStatFmtPercent::CTallyVarStatFmtPercent(const CTallyVarStatFmtPercent& rhs)
: m_pctType(rhs.m_pctType),
  m_bInterleaved(rhs.m_bInterleaved)
{
}

// display string to show in dialog box (e.g. "Percent (Column)")
CIMSAString CTallyVarStatFmtPercent::GetDisplayString() const
{
    return CIMSAString(GetType()) + _T("(") +
        PctTypeToString(m_pctType) + _T("; ") + (m_bInterleaved ? _T("Interleaved") : _T("Separate")) + _T(")");
}

// copy categories for the stat into array of tab vars
void CTallyVarStatFmtPercent::GetTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex, const DictValueSet* pVSet,
                                         const CustSpecialValSettings* custSpecials,
                                         const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                         const FOREIGN_KEYS* pForeignKeys)
{
    // For interleaved percents, we label tab vals "Percent", for separate, we label them same as for counts (vset labels)
    if (m_bInterleaved) {
        if (pVSet != NULL) {
            CIMSAString sLabel = _T("Percent");
            if (pForeignKeys) {
                sLabel = pForeignKeys->GetKey(sLabel);
            }
            // compute # values we need
            int iNumValues = GetNumValsWithCustSpecials(*pVSet, custSpecials);
            // now add in that many values
            for (int v = 0 ; v < iNumValues ; v++) {
                CTabValue* pTabVal = new CTabValue();
                pTabVal->SetText(sLabel);
                pTabVal->SetStatId(iStatIndex);
                pTabVal->SetTabValType(GetTabValType());
                aTabValue.Add(pTabVal);
            }
        }
    }
    else {
        bool bPercentsOnly = (FindFirstStatIndex(aNeighborStats, _T("Counts")) == NONE);
        if (bPercentsOnly) {
            // percents only table use vset labels directly
           GetTabValsVSet(aTabValue, iStatIndex, pVSet, GetTabValType(), custSpecials);
        }
        else {
            // percents and counts but not interleaved - add "% " in front of each vset label
            LabelPrependPercent trans;
            GetTabValsVSet(aTabValue, iStatIndex, pVSet, GetTabValType(), custSpecials, &trans);
        }
    }
}

// reconcile tab vals on stat or dictionary change
bool CTallyVarStatFmtPercent::ReconcileTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                                               CArray<CTabValue*,CTabValue*>& aExistingTabValue, const DictValueSet* pVSet,
                                               const CustSpecialValSettings* custSpecials,
                                               const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                               const FOREIGN_KEYS* pForeignKeys)
{
    bool bChanged = false;

    // For interleaved percents, we label tab vals "Percent", for separate, we label them same as for counts (vset labels)
    if (m_bInterleaved) {
        CIMSAString sLabel = _T("Percent");
        if (pForeignKeys) {
            sLabel = pForeignKeys->GetKey(sLabel);
        }
        int iNumValues = GetNumValsWithCustSpecials(*pVSet, custSpecials);
        for (int iVS = 0 ; iVS < iNumValues ; iVS++) {
            if (aExistingTabValue.GetCount() > 0) {
                // reuse existing val
                CTabValue* pExistTabVal = aExistingTabValue.GetAt(0);
                aExistingTabValue.RemoveAt(0); // remove from old list
                pExistTabVal->SetStatId(iStatIndex);
                CDataCellFmt* pDataCellFmt =  pExistTabVal->GetDerFmt();
                if((pDataCellFmt && !pDataCellFmt->IsTextCustom())|| !pDataCellFmt ){
                    if (pExistTabVal->GetText() != sLabel) {
                        pExistTabVal->SetText(sLabel);
                        bChanged = true;
                    }
                }
                aTabValue.Add(pExistTabVal); // add to new list
            }
            else {
                // couldn't match to an existing name, create a new one
                CTabValue* pTabVal = new CTabValue();
                pTabVal->SetText(sLabel);
                pTabVal->SetStatId(iStatIndex);
                pTabVal->SetTabValType(GetTabValType());
                aTabValue.Add(pTabVal);
                bChanged = true;
            }
        }
    }
    else {
        bool bPercentsOnly = (FindFirstStatIndex(aNeighborStats, _T("Counts")) == NONE);
        if (bPercentsOnly) {
            // percents only - use vset labels directly
            bChanged = ReconcileTabValsVSet(aTabValue, iStatIndex, aExistingTabValue, pVSet, GetTabValType(), custSpecials);
        }
        else {
             // percents and counts but not interleaved - add "% " in front of each vset label
            LabelPrependPercent trans;
            bChanged = ReconcileTabValsVSet(aTabValue, iStatIndex, aExistingTabValue, pVSet, GetTabValType(), custSpecials, &trans);
       }
    }

    return bChanged;
}

// load from file
bool CTallyVarStatFmtPercent::Build(CSpecFile& specFile, bool bSilent)
{
    // base reads header info
    CTallyVarStatFmt::Build(specFile, bSilent);

    // now read line by line ...
    CIMSAString sCmd, sArg;
    bool bLineOK = false;

    while (specFile.GetState() == SF_OK)  {
        specFile.GetLine(sCmd, sArg);
        ASSERT (!sCmd.IsEmpty());
        if (sCmd[0] == '[')  {
            // we are starting a new section ... put back the line and quit
            specFile.UngetLine();
            break;
        }
        bLineOK = false;

        // percent type
        if (!sCmd.CompareNoCase(TFT_CMD_PCT_TYPE)) {
            PctType pt = PctTypeFromString(sArg);
            if (pt != -1) {
                bLineOK=true;
                SetPctType(pt);
            }
            else {
                CIMSAString sMsg;
                sMsg.Format(_T("Invalid percent type at line %d:"), specFile.GetLineNumber());
                sMsg += _T("\n") + sCmd + _T("=") + sArg;
                AfxMessageBox(sMsg,MB_ICONEXCLAMATION);
            }
        } else if (!sCmd.CompareNoCase(TFT_CMD_PCT_INTERLEAVED)) {
            // interleaved
            if (!sArg.CompareNoCase(TFT_ARG_YES)) {
                bLineOK=true;
                SetInterleaved(true);
            }
            else if (!sArg.CompareNoCase(TFT_ARG_NO)) {
                bLineOK=true;
                SetInterleaved(false);
            }
           else {
                CIMSAString sMsg;
                sMsg.Format(_T("Invalid percent interleaved at line %d:"), specFile.GetLineNumber());
                sMsg += _T("\n") + sCmd + _T("=") + sArg;
                AfxMessageBox(sMsg,MB_ICONEXCLAMATION);
            }
        }
        else {
            specFile.UngetLine();
            break;
        }
    }

    return bLineOK;
}

// save to file
void CTallyVarStatFmtPercent::Save(CSpecFile& specFile)
{
    // base writes out header info
    CTallyVarStatFmt::Save(specFile);

    // save params specific to percent
    specFile.PutLine(TFT_CMD_PCT_TYPE, GetPctTypeString());
    specFile.PutLine(TFT_CMD_PCT_INTERLEAVED, m_bInterleaved ? TFT_ARG_YES : TFT_ARG_NO);
}

// comparison
bool CTallyVarStatFmtPercent::operator==(const CTallyVarStatFmt& rhs) const
{
    if (!CTallyVarStatFmt::operator==(rhs)) {
        return false;
    }
    const CTallyVarStatFmtPercent& rhst = static_cast<const CTallyVarStatFmtPercent&>(rhs);
    return rhst.GetPctType() == GetPctType() && rhst.GetInterleaved() == GetInterleaved();
}

// gen code for crosstab statement (stat portion)
CIMSAString CTallyVarStatFmtPercent::GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
{
    UNREFERENCED_PARAMETER(aNeighborStats);
    return CIMSAString(_T("PCT(")) + GetPctTypeString() + CIMSAString(_T("), "));
}

//
// CTallyVarStatFmtPercentTotal
//
//

// constructor
CTallyVarStatFmtTotalPercent::CTallyVarStatFmtTotalPercent()
: m_pctType(PT_ROW),
  m_bSameTypeAsPercents(true)
{
}

// copy constructor
CTallyVarStatFmtTotalPercent::CTallyVarStatFmtTotalPercent(const CTallyVarStatFmtTotalPercent& rhs)
: m_pctType(rhs.m_pctType),
  m_bSameTypeAsPercents(rhs.m_bSameTypeAsPercents)

{
}

// display string to show in dialog box (e.g. "Percent (Column)")
CIMSAString CTallyVarStatFmtTotalPercent::GetDisplayString() const
{
    CIMSAString sTypeStr;
    if (m_bSameTypeAsPercents) {
        sTypeStr = _T("Same as Percents");
    }
    else {
        sTypeStr = PctTypeToString(m_pctType);
    }
    return CIMSAString(GetType()) + _T("(") + sTypeStr +  _T(")");
}

// load from file
bool CTallyVarStatFmtTotalPercent::Build(CSpecFile& specFile, bool bSilent)
{
    // base reads header info
    CTallyVarStatFmt::Build(specFile, bSilent);

    // now read line by line ...
    CIMSAString sCmd, sArg;
    bool bLineOK = false;

    while (specFile.GetState() == SF_OK)  {
        specFile.GetLine(sCmd, sArg);
        ASSERT (!sCmd.IsEmpty());
        if (sCmd[0] == '[')  {
            // we are starting a new section ... put back the line and quit
            specFile.UngetLine();
            break;
        }
        bLineOK = false;

        // percent type
        if (!sCmd.CompareNoCase(TFT_CMD_PCT_TYPE)) {
            if (!sArg.CompareNoCase(TFT_ARG_SAME_AS_PCT)) {
                m_bSameTypeAsPercents = true;
                bLineOK = true;
            }
            else {
                m_bSameTypeAsPercents = false;
                PctType pt = PctTypeFromString(sArg);
                if (pt != -1) {
                    bLineOK=true;
                    SetPctType(pt);
                }
            }
            if (!bLineOK) {
                CIMSAString sMsg;
                sMsg.Format(_T("Invalid percent type at line %d:"), specFile.GetLineNumber());
                sMsg += _T("\n") + sCmd + _T("=") + sArg;
                AfxMessageBox(sMsg,MB_ICONEXCLAMATION);
            }
        }
        else {
            specFile.UngetLine();
            break;
        }

    }

    return bLineOK;
}

// save to file
void CTallyVarStatFmtTotalPercent::Save(CSpecFile& specFile)
{
    // base writes out header info
    CTallyVarStatFmt::Save(specFile);

    // save params specific to percent
    if (m_bSameTypeAsPercents) {
        specFile.PutLine(TFT_CMD_PCT_TYPE, TFT_ARG_SAME_AS_PCT);
    }
    else {
        specFile.PutLine(TFT_CMD_PCT_TYPE, GetPctTypeString());
    }
}

// comparison
bool CTallyVarStatFmtTotalPercent::operator==(const CTallyVarStatFmt& rhs) const
{
    if (!CTallyVarStatFmt::operator==(rhs)) {
        return false;
    }
    const CTallyVarStatFmtPercent& rhst = static_cast<const CTallyVarStatFmtPercent&>(rhs);
    return rhst.GetPctType() == GetPctType();
}

// gen code for crosstab statement (stat portion)
CIMSAString CTallyVarStatFmtTotalPercent::GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
{
    CString sType;
    if (m_bSameTypeAsPercents) {

        // find a percent stat nearby and use it's setting
        int iClosest = FindClosestPercent(aNeighborStats);

        if (iClosest == -1) {
            // didn't find any - default to row
            sType = _T("ROW");
        }
        else {
            // use type from closest
            CTallyVarStatFmtPercent const* pStatPct = static_cast<CTallyVarStatFmtPercent const*>(aNeighborStats.GetAt(iClosest));
            sType = pStatPct->GetPctTypeString();
        }
    }
    else {
        // use our own setting
        sType = GetPctTypeString();
    }
    return CIMSAString(_T("TOTAL(PCT(")) +  sType + CIMSAString(_T(")), "));
}

// Find index of closest percent, return none
int CTallyVarStatFmtTotalPercent::FindClosestPercent(const CArray<CTallyVarStatFmt*>& aNeighborStats)
{
    int iTotPct = FindInArray(aNeighborStats, this);
    int iPrev;
    for (iPrev = iTotPct-1; iPrev >= 0; --iPrev) {
        if (!_tcscmp(aNeighborStats.GetAt(iPrev)->GetType(), _T("Percents"))) {
            break;
        }
    }
    int iAfter;
    for (iAfter = iTotPct+1; iAfter < aNeighborStats.GetCount(); ++iAfter) {
        if (!_tcscmp(aNeighborStats.GetAt(iAfter)->GetType(), _T("Percents"))) {
            break;
        }
    }
    int iClosest = -1;
    if (iPrev >= 0 && iAfter < aNeighborStats.GetCount()) {
        if (abs(iTotPct-iPrev) < abs(iTotPct-iAfter)) {
            iClosest = iPrev;
        }
        else {
            iClosest = iAfter;
        }
    }
    else if (iPrev >= 0 && iAfter >= aNeighborStats.GetCount()) {
        iClosest = iPrev;
    }
    else if (iPrev < 0 && iAfter < aNeighborStats.GetCount()) {
        iClosest = iAfter;
    }

    return iClosest;
}

///////////////////////////////////////////////////////////
//
// CTallyVarStatFmtTotalPercent::GetTabValLabel
//
// Determine label for tab val based on neighbor stats
///////////////////////////////////////////////////////////
CIMSAString CTallyVarStatFmtTotalPercent::GetTabValLabel(const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                                         const FOREIGN_KEYS* pForeignKeys)
{
    // if we have percents only, should use "Total" as label, for interleaved
    // percents with total percent next to total then use "Percent", otherwise
    // use "Total %"
    bool bPercentsOnly = (FindFirstStatIndex(aNeighborStats, _T("Counts")) == NONE) &&
                         (FindFirstStatIndex(aNeighborStats, _T("Percents")) != NONE);
    CIMSAString sLabel = _T("% Total"); // default
    if (bPercentsOnly) {
        sLabel = _T("Total");
        if (pForeignKeys) {
            sLabel = pForeignKeys->GetKey(sLabel);
        }
    }
    else {
        int iPercentIndex = FindClosestPercent(aNeighborStats);
        if (iPercentIndex != NONE) {
            CTallyVarStatFmtPercent* pPercentStat = static_cast<CTallyVarStatFmtPercent*>(aNeighborStats.GetAt(iPercentIndex));
            if (pPercentStat->GetInterleaved()) {
                bool bNextToTotal = false;
                int iThisIndex = FindInArray(aNeighborStats, this);
                if (iThisIndex > 0 && (!_tcscmp(aNeighborStats.GetAt(iThisIndex - 1)->GetType(), _T("Total")))) {
                    bNextToTotal = true;
                }
                else if (iThisIndex < aNeighborStats.GetCount()-1 && (!_tcscmp(aNeighborStats.GetAt(iThisIndex + 1)->GetType(), _T("Total")))) {
                    bNextToTotal = true;
                }
                if (bNextToTotal) {
                    sLabel = _T("Percent");
                    if (pForeignKeys) {
                        sLabel = pForeignKeys->GetKey(sLabel);
                    }
                }
            }
        }
    }

    return sLabel;
}

///////////////////////////////////////////////////////////
//
// CTallyVarStatFmtTotalPercent::GetTabVals
//
// copy categories for the stat into array of tab vars
///////////////////////////////////////////////////////////
void CTallyVarStatFmtTotalPercent::GetTabVals(CArray<CTabValue*,
                                        CTabValue*>& aTabValue,
                                        int iStatIndex,
                                        const DictValueSet* pVSet,
                                        const CustSpecialValSettings* custSpecials,
                                        const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                        const FOREIGN_KEYS* pForeignKeys)
{
    UNREFERENCED_PARAMETER(pVSet);
    UNREFERENCED_PARAMETER(custSpecials);

    CTabValue* pTabVal = new CTabValue();
    pTabVal->SetText(GetTabValLabel(aNeighborStats, pForeignKeys));
    pTabVal->SetTabValType(GetTabValType());
    pTabVal->SetStatId(iStatIndex);
    aTabValue.Add(pTabVal);
}

///////////////////////////////////////////////////////////
//
// CTallyVarStatFmtTotalPercent::ReconcileTabVals
//
// reconcile tab vals on stat or dictionary change
///////////////////////////////////////////////////////////
bool CTallyVarStatFmtTotalPercent::ReconcileTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                                             CArray<CTabValue*,CTabValue*>& aExistingTabValue, const DictValueSet* pVSet,
                                             const CustSpecialValSettings* custSpecials,
                                             const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                             const FOREIGN_KEYS* pForeignKeys)
{
    UNREFERENCED_PARAMETER(pVSet);
    UNREFERENCED_PARAMETER(custSpecials);

    ASSERT(aExistingTabValue.GetSize() == 1);

    bool bChanged = false;
    CTabValue *pTabVal = aExistingTabValue.GetAt(0);
    CDataCellFmt* pDataCellFmt =  pTabVal->GetDerFmt();
    if((pDataCellFmt && !pDataCellFmt->IsTextCustom())|| !pDataCellFmt ){
        CIMSAString sLabel = GetTabValLabel(aNeighborStats, pForeignKeys);
        if (pTabVal->GetText() != sLabel) {
            bChanged = true;
            pTabVal->SetText(sLabel);
        }
    }
    pTabVal->SetStatId(iStatIndex);
    aTabValue.Add(pTabVal);
    aExistingTabValue.RemoveAt(0);

    return bChanged;
}

//
// CStatRangeProps
//
//


// constructor
CStatRangeProps::CStatRangeProps(double dVarMin, double dVarMax)
: m_bUseValueSet(true),
  m_dVarMin(dVarMin),
  m_dVarMax(dVarMax)
{
    SetDefaults(dVarMin, dVarMax);
}

CStatRangeProps::CStatRangeProps(const CStatRangeProps& rhs)
: m_bUseValueSet(rhs.m_bUseValueSet),
  m_dMin(rhs.m_dMin),
  m_dMax(rhs.m_dMax),
  m_dStep(rhs.m_dStep),
  m_dVarMin(rhs.m_dVarMin),
  m_dVarMax(rhs.m_dVarMax)
{
}

void CStatRangeProps::SetDefaults(double dVarMin, double dVarMax)
{
  m_dMin = dVarMin;
  m_dMax = dVarMax;
  m_dStep = ComputeDefaultInterval(dVarMin, dVarMax);
}

double CStatRangeProps::ComputeDefaultInterval(double dVarMin, double dVarMax)
{
  double dRange = dVarMax - dVarMin;
  int i;
  for (i = 0; dRange > 1; ++i) {
      dRange = dRange/10;
  }
  return pow(10.0,std::max(0, i-1));
}

// comparison
bool CStatRangeProps::operator==(const CStatRangeProps& rhs) const
{
    if (m_bUseValueSet) {
        return m_bUseValueSet == rhs.m_bUseValueSet;
    }

    return m_bUseValueSet == rhs.m_bUseValueSet && m_dMin == rhs.m_dMin && m_dMax == rhs.m_dMax && m_dStep == rhs.m_dStep;
}

// display string to show in dialog box
CIMSAString CStatRangeProps::GetDisplayString() const
{
    CIMSAString sDisp;
    if (m_bUseValueSet) {
        sDisp = _T("Use value set");
    }
    else {
        sDisp = DblToStringAsIntIfPossible(m_dMin) + CString(_T(" to ")) +
                DblToStringAsIntIfPossible(m_dMax) + CString(_T(" by ")) +
                DblToStringAsIntIfPossible(m_dStep);
    }
    return sDisp;
}

// return intervals string for code gen
CString CStatRangeProps::GetIntervalsString()
{
    CString sIntervals, s;
    if (!m_bUseValueSet) {

        // calculate interval boundaries
        CArray<double> aBuckets;

        // if the vset has lower boundary than what user set, add in an extra interval
        // at the bottom to catch vals that fall outside lower range
        if (m_dVarMin < GetIntervalsMin()) {
            aBuckets.Add(m_dVarMin);
        }
        double d = GetIntervalsMin();
        do {
            aBuckets.Add(d);
            d += GetIntervalsStep();

        } while (d < GetIntervalsMax());
        aBuckets.Add(GetIntervalsMax());

        // if the vset has higher upper boundary than what user set, add in an extra interval
        // at the top to catch vals that fall outside upper range
        if (m_dVarMax > GetIntervalsMax()) {
            aBuckets.Add(m_dVarMax);
        }

        // build string from boundaries (CSPro expects intervals to go from high to low).
        sIntervals.Format(_T(" INTERVALS(HIGHEST %f LOWERS "), aBuckets.GetAt(aBuckets.GetCount()-1));
        for (int i = aBuckets.GetCount() - 2; i >= 0; --i) {
            s.Format(_T("%f"), aBuckets.GetAt(i));
            sIntervals += s;
            if (i > 0) {
                sIntervals += _T(",");
            }

            // add in newlines to get around the line length limit
            if (i % 10 == 0) {
                sIntervals += _T("\n");
            }
        }
        sIntervals += _T(")");
    }
    return sIntervals;
}

// reconcile on vset change
bool CStatRangeProps::Reconcile(const DictValueSet* pVSet)
{
    bool bChanged = false;
    double dVSetMin, dVSetMax;
    std::tie(dVSetMin, dVSetMax) = pVSet->GetMinMax();
    m_dVarMin = dVSetMin;
    m_dVarMax = dVSetMax;
    if (GetIntervalsMax() <= GetIntervalsMin()) {
        // totally invalid, never properly intialized, reset to defaults
        SetDefaults(dVSetMin, dVSetMax);
        bChanged = true;
    } else {
        if (GetIntervalsMax() > dVSetMax) {
            SetIntervalsMax(dVSetMax);
            bChanged = true;
        }
        if (GetIntervalsMin() < dVSetMin) {
            SetIntervalsMin(dVSetMin);
            bChanged = true;
        }

        if (GetIntervalsStep() > dVSetMax - dVSetMin) {
            SetIntervalsStep(ComputeDefaultInterval(dVSetMin, dVSetMax));
            bChanged = true;
        }
    }

    return bChanged;
}

// Write to file
void CStatRangeProps::Save(CSpecFile& specFile)
{
    specFile.PutLine(TFT_CMD_USE_VSET, GetUseValueSet() ? TFT_ARG_YES : TFT_ARG_NO);
    specFile.PutLine(TFT_CMD_INTERVALS_MIN, GetIntervalsMin());
    specFile.PutLine(TFT_CMD_INTERVALS_MAX, GetIntervalsMax());
    specFile.PutLine(TFT_CMD_INTERVALS_STEP, GetIntervalsStep());
}

// Read line from file, return true if line is read
bool CStatRangeProps::BuildLine(const CIMSAString& sCmd, const CIMSAString& sArg, int lineNumber)
{
    if (!sCmd.CompareNoCase(TFT_CMD_USE_VSET)) {
        if (!sArg.CompareNoCase(TFT_ARG_YES)) {
            SetUseValueSet(true);
        }
        else if (!sArg.CompareNoCase(TFT_ARG_NO)) {
            SetUseValueSet(false);
        }
        else {
            CIMSAString sMsg;
            sMsg.Format(_T("Invalid argument for use value set on line %d:"), lineNumber);
            sMsg += _T("\n") + sCmd + _T("=") + sArg;
            AfxMessageBox(sMsg,MB_ICONEXCLAMATION);
        }
        return true;
    }
    else if (!sCmd.CompareNoCase(TFT_CMD_INTERVALS_MIN)) {
        if (sArg.IsNumeric()) {
            SetIntervalsMin((double) sArg.fVal());
        }
        else {
            CIMSAString sMsg;
            sMsg.Format(_T("Invalid argument for median/ntile intervals on line %d:"), lineNumber);
            sMsg += _T("\n") + sCmd + _T("=") + sArg;
            AfxMessageBox(sMsg,MB_ICONEXCLAMATION);
        }
        return true;
    }
    else if (!sCmd.CompareNoCase(TFT_CMD_INTERVALS_MAX)) {
        if (sArg.IsNumeric()) {
            SetIntervalsMax((double) sArg.fVal());
        }
        else {
            CIMSAString sMsg;
            sMsg.Format(_T("Invalid argument for median/ntile intervals on line %d:"), lineNumber);
            sMsg += _T("\n") + sCmd + _T("=") + sArg;
            AfxMessageBox(sMsg,MB_ICONEXCLAMATION);
        }
        return true;
    }
    else if (!sCmd.CompareNoCase(TFT_CMD_INTERVALS_STEP)) {
        if (sArg.IsNumeric()) {
            SetIntervalsStep((double) sArg.fVal());
        }
        else {
            CIMSAString sMsg;
            sMsg.Format(_T("Invalid argument for median/ntile intervals on line %d:"), lineNumber);
            sMsg += _T("\n") + sCmd + _T("=") + sArg;
            AfxMessageBox(sMsg,MB_ICONEXCLAMATION);
        }
        return true;
    }

    return false;
}

// convert double to string, but if the double is really an integer, then display it
// as an integer (with no dec places)
CIMSAString CStatRangeProps::DblToStringAsIntIfPossible(double d)
{
    const int nDefaultDecPlaces = 6;
    CIMSAString sNum;
    sNum.Format(_T("%.*f"), ((int) d == d) ? 0 : nDefaultDecPlaces, d);
    return sNum;
}

//
// CTallyVarStatFmtMedian
//
//

// constructor
CTallyVarStatFmtMedian::CTallyVarStatFmtMedian(double dVarMin, double dVarMax)
: m_bContinuous(true),
  m_rangeProps(dVarMin, dVarMax)
{
}

// copy constructor
CTallyVarStatFmtMedian::CTallyVarStatFmtMedian(const CTallyVarStatFmtMedian& rhs)
: m_rangeProps(rhs.m_rangeProps),
  m_bContinuous(rhs.m_bContinuous)
{
}

// display string to show in dialog box (e.g. "Percent (Column)")
CIMSAString CTallyVarStatFmtMedian::GetDisplayString() const
{
    CIMSAString sTypeStr = m_bContinuous ? _T("Continuous") : _T("Discrete");
    return CIMSAString(GetType()) + _T("(") + sTypeStr + _T(";") + m_rangeProps.GetDisplayString() + _T(")");
}

// gen code for crosstab statement (stat portion)
CIMSAString CTallyVarStatFmtMedian::GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
{
    UNREFERENCED_PARAMETER(aNeighborStats);

    CIMSAString sStat = _T("MEDIAN(");
    if (GetContinuous()) {
        sStat += _T("CONTINUOUS");
    }
    else {
        sStat += _T("DISCRETE");
    }

    if (!GetRangeProps().GetUseValueSet()) {

        sStat += GetRangeProps().GetIntervalsString();
    }

    return sStat + _T("), ");
}

// load from file
bool CTallyVarStatFmtMedian::Build(CSpecFile& specFile, bool bSilent)
{
    // base reads header info
    CTallyVarStatFmt::Build(specFile, bSilent);
    bool bLineOK = false;

    // now read line by line ...
    CIMSAString sCmd, sArg;

    while (specFile.GetState() == SF_OK)  {
        specFile.GetLine(sCmd, sArg);
        ASSERT (!sCmd.IsEmpty());
        if (sCmd[0] == '[')  {
            // we are starting a new section ... put back the line and quit
            specFile.UngetLine();
            break;
        }
        bLineOK = false;

        // num tiles
        if (!sCmd.CompareNoCase(TFT_CMD_MEDIAN_TYPE)) {
            if (!sArg.CompareNoCase(TFT_ARG_CONTINUOUS)) {
                m_bContinuous = true;
                bLineOK = true;
            }
            else if (!sArg.CompareNoCase(TFT_ARG_DISCRETE)) {
                m_bContinuous = false;
                bLineOK = true;
            }
            if (!bLineOK) {
                CIMSAString sMsg;
                sMsg.Format(_T("Invalid median type line %d:"), specFile.GetLineNumber());
                sMsg += _T("\n") + sCmd + _T("=") + sArg;
                AfxMessageBox(sMsg,MB_ICONEXCLAMATION);
            }
        }
        else if (m_rangeProps.BuildLine(sCmd, sArg, specFile.GetLineNumber())) {
        }
        else {
            specFile.UngetLine();
            break;
        }

    }

    return bLineOK;
}

// save to file
void CTallyVarStatFmtMedian::Save(CSpecFile& specFile)
{
    // base writes out header info
    CTallyVarStatFmt::Save(specFile);

    // save params specific to median
    specFile.PutLine(TFT_CMD_MEDIAN_TYPE, this->m_bContinuous ? TFT_ARG_CONTINUOUS : TFT_ARG_DISCRETE);
    m_rangeProps.Save(specFile);
}

// comparison
bool CTallyVarStatFmtMedian::operator==(const CTallyVarStatFmt& rhs) const
{
    if (!CTallyVarStatFmt::operator==(rhs)) {
        return false;
    }
    const CTallyVarStatFmtMedian& rhst = static_cast<const CTallyVarStatFmtMedian&>(rhs);
    if (GetContinuous() != rhst.GetContinuous()) {
        return false;
    }

    return rhst.GetRangeProps() == GetRangeProps();
}

//
// CTallyVarStatFmtNTiles
//
//

// constructor
CTallyVarStatFmtNTiles::CTallyVarStatFmtNTiles(double dVarMin, double dVarMax)
: m_numTiles(4),
  m_rangeProps(dVarMin, dVarMax)
{
}

// copy constructor
CTallyVarStatFmtNTiles::CTallyVarStatFmtNTiles(const CTallyVarStatFmtNTiles& rhs)
: m_rangeProps(rhs.m_rangeProps),
  m_numTiles(rhs.m_numTiles)
{
}

// display string to show in dialog box (e.g. "Percent (Column)")
CIMSAString CTallyVarStatFmtNTiles::GetDisplayString() const
{
    CIMSAString sNumTiles = IntToString(m_numTiles);
    return CIMSAString(GetType()) + _T("(") + sNumTiles + _T(" tiles;") + m_rangeProps.GetDisplayString() + _T(")");
}

// gen code for crosstab statement (stat portion)
CIMSAString CTallyVarStatFmtNTiles::GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
{
    UNREFERENCED_PARAMETER(aNeighborStats);

    CIMSAString sStat;
    sStat.Format(_T("PTILE %d "), m_numTiles);

    if (!GetRangeProps().GetUseValueSet()) {

        sStat += _T("(") + GetRangeProps().GetIntervalsString() + _T(")");
    }

    return sStat;
}

// load from file
bool CTallyVarStatFmtNTiles::Build(CSpecFile& specFile, bool bSilent)
{
    // base reads header info
    CTallyVarStatFmt::Build(specFile, bSilent);
    bool bLineOK = false;

    // now read line by line ...
    CIMSAString sCmd, sArg;

    while (specFile.GetState() == SF_OK)  {
        specFile.GetLine(sCmd, sArg);
        ASSERT (!sCmd.IsEmpty());
        if (sCmd[0] == '[')  {
            // we are starting a new section ... put back the line and quit
            specFile.UngetLine();
            break;
        }
        bLineOK = false;

        // num tiles
        if (!sCmd.CompareNoCase(TFT_CMD_STATS_NUMTILES)) {
            if (sArg.IsNumeric()) {
                int numTiles = (int) sArg.Val();
                if (numTiles >= 2 && numTiles <= 10) {
                    SetNumTiles(numTiles);
                    bLineOK = true;
                }
            }
            if (!bLineOK) {
                CIMSAString sMsg;
                sMsg.Format(_T("Invalid number of ntiles at line %d:"), specFile.GetLineNumber());
                sMsg += _T("\n") + sCmd + _T("=") + sArg;
                AfxMessageBox(sMsg,MB_ICONEXCLAMATION);
            }
        }
        else if (m_rangeProps.BuildLine(sCmd, sArg, specFile.GetLineNumber())) {
        }
        else {
            specFile.UngetLine();
            break;
        }

    }

    return bLineOK;
}

// save to file
void CTallyVarStatFmtNTiles::Save(CSpecFile& specFile)
{
    // base writes out header info
    CTallyVarStatFmt::Save(specFile);

    // save params specific to ntiles
    specFile.PutLine(TFT_CMD_STATS_NUMTILES, GetNumTiles());
    m_rangeProps.Save(specFile);
}

// comparison
bool CTallyVarStatFmtNTiles::operator==(const CTallyVarStatFmt& rhs) const
{
    if (!CTallyVarStatFmt::operator==(rhs)) {
        return false;
    }
    const CTallyVarStatFmtNTiles& rhst = static_cast<const CTallyVarStatFmtNTiles&>(rhs);
    if (GetNumTiles() != rhst.GetNumTiles()) {
        return false;
    }

    return rhst.GetRangeProps() == GetRangeProps();
}


///////////////////////////////////////////////////////////
//
// CTallyVarStatFmtNTiles::GetTabVals
//
// copy categories for the stat into array of tab vars
///////////////////////////////////////////////////////////
void CTallyVarStatFmtNTiles::GetTabVals(CArray<CTabValue*,CTabValue*>& aTabValue,
                                        int iStatIndex,
                                        const DictValueSet* pVSet,
                                        const CustSpecialValSettings* custSpecials,
                                        const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                        const FOREIGN_KEYS* pForeignKeys)
{
    UNREFERENCED_PARAMETER(pVSet);
    UNREFERENCED_PARAMETER(custSpecials);
    UNREFERENCED_PARAMETER(aNeighborStats);

    CIMSAString sPercentile = _T("Percentile");
    if (pForeignKeys) {
        sPercentile = pForeignKeys->GetKey(sPercentile);
    }

    int iNumTiles= GetNumTiles();
    ASSERT(iNumTiles >=2  && iNumTiles <=10);
    for(int iTile =0; iTile  < iNumTiles -1 ;iTile++) {
        CTabValue* pTabVal = new CTabValue;
        int iPercentile = (100 * (iTile+1)) / iNumTiles;
        CIMSAString sLabel;
        sLabel.Str(iPercentile);
        sLabel += _T(" ") + sPercentile;
        pTabVal->SetText(sLabel);
        pTabVal->SetTabValType(GetTabValType());
        pTabVal->SetStatId(iStatIndex);
        aTabValue.Add(pTabVal);
    }
}

///////////////////////////////////////////////////////////
//
// CTallyVarStatFmtNTiles::ReconcileTabVals
//
// reconcile tab vals on stat or dictionary change
///////////////////////////////////////////////////////////
bool CTallyVarStatFmtNTiles::ReconcileTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                                             CArray<CTabValue*,CTabValue*>& aExistingTabValue, const DictValueSet* pVSet,
                                             const CustSpecialValSettings* custSpecials,
                                             const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                             const FOREIGN_KEYS* pForeignKeys)
{
    UNREFERENCED_PARAMETER(pVSet);
    UNREFERENCED_PARAMETER(custSpecials);
    UNREFERENCED_PARAMETER(aNeighborStats);

    bool bChanged = false;

    CIMSAString sPercentile = _T("Percentile");
    if (pForeignKeys) {
        sPercentile = pForeignKeys->GetKey(sPercentile);
    }

    int iNumTiles= GetNumTiles();
    ASSERT(iNumTiles >=2  && iNumTiles <=10);
    for(int iTile =0; iTile  < iNumTiles -1 ;iTile++){
        CTabValue *pTabVal;
        int iTV = FindTabVal(aExistingTabValue,STAT_NTILE_TABVAL);
        if (iTV != NONE) {
            pTabVal =  aExistingTabValue[iTV];
            aExistingTabValue[iTV] = NULL;
        }
        else {
            pTabVal = new CTabValue();
            bChanged = true;
        }
        CDataCellFmt* pDataCellFmt =  pTabVal->GetDerFmt();
        if((pDataCellFmt && !pDataCellFmt->IsTextCustom())|| !pDataCellFmt ){
            int iPercentile = (100 * (iTile+1)) / iNumTiles;
            CIMSAString sLabel;
            sLabel.Str(iPercentile);
            sLabel += _T(" ") + sPercentile;
            if (pTabVal->GetText() != sLabel) {
                pTabVal->SetText(sLabel);
                bChanged = true;
            }
        }
        pTabVal->SetTabValType(GetTabValType());
        pTabVal->SetStatId(iStatIndex);
        aTabValue.Add(pTabVal);
    }

    return false;
}


//
// CTallyVarStatFmtProportion
//
//

// constructor
CTallyVarStatFmtProportion::CTallyVarStatFmtProportion()
: m_propType(PRT_PERCENT),
  m_sRange(_T(""))
{
}

// copy constructor
CTallyVarStatFmtProportion::CTallyVarStatFmtProportion(const CTallyVarStatFmtProportion& rhs)
: m_propType(rhs.m_propType),
  m_sRange(rhs.m_sRange)

{
}

// load from file
bool CTallyVarStatFmtProportion::Build(CSpecFile& specFile, bool bSilent)
{
    // base reads header info
    CTallyVarStatFmt::Build(specFile, bSilent);

    // now read line by line ...
    CIMSAString sCmd, sArg;
    bool bLineOK = false;

    while (specFile.GetState() == SF_OK)  {
        specFile.GetLine(sCmd, sArg);
        ASSERT (!sCmd.IsEmpty());
        if (sCmd[0] == '[')  {
            // we are starting a new section ... put back the line and quit
            specFile.UngetLine();
            break;
        }
        bLineOK = false;

        // proportion type
        if (!sCmd.CompareNoCase(TFT_CMD_PROP_TYPE)) {
            if (!sArg.CompareNoCase(TFT_ARG_PERCENT)) {
                m_propType = PRT_PERCENT;
                bLineOK = true;
            }
            else if (!sArg.CompareNoCase(TFT_ARG_PERCENT_AND_TOTAL)) {
                m_propType = PRT_PERCENT_AND_TOTAL;
                bLineOK = true;
            }
            else if (!sArg.CompareNoCase(TFT_ARG_RATIO)) {
                m_propType = PRT_RATIO;
                bLineOK = true;
            }
            else if (!sArg.CompareNoCase(TFT_ARG_RATIO_AND_TOTAL)) {
                m_propType = PRT_RATIO_AND_TOTAL;
                bLineOK = true;
            }
            if (!bLineOK) {
                CIMSAString sMsg;
                sMsg.Format(_T("Invalid proportion type at line %d:"), specFile.GetLineNumber());
                sMsg += _T("\n") + sCmd + _T("=") + sArg;
                AfxMessageBox(sMsg,MB_ICONEXCLAMATION);
            }
        }
        // proportion range
        else if (!sCmd.CompareNoCase(TFT_CMD_PROP_RANGE)) {
            m_sRange = sArg;
        }
        else {
            specFile.UngetLine();
            break;
        }

    }

    return bLineOK;
}

// save to file
void CTallyVarStatFmtProportion::Save(CSpecFile& specFile)
{
    // base writes out header info
    CTallyVarStatFmt::Save(specFile);

    // save params specific to proportion
    switch (m_propType) {
        case PRT_PERCENT:
            specFile.PutLine(TFT_CMD_PROP_TYPE, TFT_ARG_PERCENT);
            break;
        case PRT_PERCENT_AND_TOTAL:
            specFile.PutLine(TFT_CMD_PROP_TYPE, TFT_ARG_PERCENT_AND_TOTAL);
            break;
        case PRT_RATIO:
            specFile.PutLine(TFT_CMD_PROP_TYPE, TFT_ARG_RATIO);
            break;
        case PRT_RATIO_AND_TOTAL:
            specFile.PutLine(TFT_CMD_PROP_TYPE, TFT_ARG_RATIO_AND_TOTAL);
            break;
    }
    specFile.PutLine(TFT_CMD_PROP_RANGE, m_sRange);
}

// comparison
bool CTallyVarStatFmtProportion::operator==(const CTallyVarStatFmt& rhs) const
{
    if (!CTallyVarStatFmt::operator==(rhs)) {
        return false;
    }
    const CTallyVarStatFmtProportion& rhst = static_cast<const CTallyVarStatFmtProportion&>(rhs);
    return rhst.GetProportionType() == GetProportionType() && rhst.GetRange() == GetRange();
}

// return default num decimal places for this type of stat
NUM_DECIMALS CTallyVarStatFmtProportion::GetDefaultNumDecimals(CTabValue* pTabVal) const
{
    switch (pTabVal->GetTabValType()) {
        case STAT_PROPORTION_RATIO_TABVAL:
            return NUM_DECIMALS_TWO; // must be ratio, 2 decimal places
        case STAT_PROPORTION_PERCENT_TABVAL:
            return NUM_DECIMALS_ONE; // percent, 1 decimal place
        case STAT_PROPORTION_TOTAL_TABVAL:
            return NUM_DECIMALS_DEFAULT; // total, use default (0) dec places
    }

    ASSERT(!_T("Invalid tab val type for proportion"));
    return NUM_DECIMALS_DEFAULT;
}

// display string to show in dialog box (e.g. "Percent (Column)")
CIMSAString CTallyVarStatFmtProportion::GetDisplayString() const
{
    CIMSAString sRange;
    CIMSAString sType;
    switch (m_propType) {
        case PRT_PERCENT:
            sType = _T("Percent");
            break;
        case PRT_PERCENT_AND_TOTAL:
            sType = _T("Percent&Total");
            break;
        case PRT_RATIO:
            sType = _T("Ratio");
            break;
        case PRT_RATIO_AND_TOTAL:
            sType = _T("Ratio&Total");
            break;
    }

    return CIMSAString(GetType()) + _T("(") + m_sRange + _T(";") + sType + _T(")");
}

// gen code for crosstab statement (stat portion)
CIMSAString CTallyVarStatFmtProportion::GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
{
    UNREFERENCED_PARAMETER(aNeighborStats);

     CString sType;
     switch (GetProportionType()) {
         case PRT_PERCENT:
             sType = _T("PCT");
             break;
         case PRT_PERCENT_AND_TOTAL:
             sType = _T("PCT, FREQ");
             break;
         case PRT_RATIO:
             sType = _T("");
             break;
         case PRT_RATIO_AND_TOTAL:
             sType = _T("FREQ");
             break;
     }

     return CIMSAString(_T("PROP(") + sType + _T(")(")+GetRange()+_T("), "));
}

// Return labels for tab vals based on proportion type and range
void CTallyVarStatFmtProportion::GetTabValLabelsAndTypes(CStringArray& aTabValLabels,
                                                         CArray<TABVAL_TYPE>& aTabValTypes,
                                                         const DictValueSet* pVSet,
                                                         const FOREIGN_KEYS* pForeignKeys)
{
    aTabValLabels.RemoveAll();
    CIMSAString sPercent = _T("Percent");
    CIMSAString sProportion = _T("Proportion");
    CIMSAString sNumber = _T("Number");
    if (pForeignKeys) {
        sPercent = pForeignKeys->GetKey(sPercent);
        sNumber = pForeignKeys->GetKey(sNumber);
        sProportion = pForeignKeys->GetKey(sProportion);
    }
   switch (GetProportionType()) {
         case PRT_PERCENT:
             aTabValLabels.Add(pVSet->GetLabel());
             aTabValTypes.Add(STAT_PROPORTION_PERCENT_TABVAL);
             break;
         case PRT_PERCENT_AND_TOTAL:
             aTabValLabels.Add(sPercent);
             aTabValLabels.Add(sNumber);
             aTabValTypes.Add(STAT_PROPORTION_PERCENT_TABVAL);
             aTabValTypes.Add(STAT_PROPORTION_TOTAL_TABVAL);
             break;
         case PRT_RATIO:
             aTabValLabels.Add(pVSet->GetLabel());
             aTabValTypes.Add(STAT_PROPORTION_RATIO_TABVAL);
             break;
         case PRT_RATIO_AND_TOTAL:
             aTabValLabels.Add(sProportion);
             aTabValLabels.Add(sNumber);
             aTabValTypes.Add(STAT_PROPORTION_RATIO_TABVAL);
             aTabValTypes.Add(STAT_PROPORTION_TOTAL_TABVAL);
             break;
    }
}

///////////////////////////////////////////////////////////
//
// CTallyVarStatFmtProportion::GetTabVals
//
// copy categories for the stat into array of tab vars
///////////////////////////////////////////////////////////
void CTallyVarStatFmtProportion::GetTabVals(CArray<CTabValue*,
                                        CTabValue*>& aTabValue,
                                        int iStatIndex,
                                        const DictValueSet* pVSet,
                                        const CustSpecialValSettings* custSpecials,
                                        const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                        const FOREIGN_KEYS* pForeignKeys)
{
    UNREFERENCED_PARAMETER(pVSet);
    UNREFERENCED_PARAMETER(custSpecials);
    UNREFERENCED_PARAMETER(aNeighborStats);

    CStringArray aTabValLabels;
    CArray<TABVAL_TYPE> aTabValTypes;
    GetTabValLabelsAndTypes(aTabValLabels, aTabValTypes, pVSet, pForeignKeys);
    for (int i = 0; i < aTabValLabels.GetCount(); ++i) {
        CTabValue* pTabVal = new CTabValue();
        pTabVal->SetText(aTabValLabels.GetAt(i));
        pTabVal->SetTabValType(aTabValTypes.GetAt(i));
        pTabVal->SetStatId(iStatIndex);
        aTabValue.Add(pTabVal);
    }
}

///////////////////////////////////////////////////////////
//
// CTallyVarStatFmtProportion::ReconcileTabVals
//
// reconcile tab vals on stat or dictionary change
///////////////////////////////////////////////////////////
bool CTallyVarStatFmtProportion::ReconcileTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                                             CArray<CTabValue*,CTabValue*>& aExistingTabValue, const DictValueSet* pVSet,
                                             const CustSpecialValSettings* custSpecials,
                                             const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                             const FOREIGN_KEYS* pForeignKeys)
{
    UNREFERENCED_PARAMETER(pVSet);
    UNREFERENCED_PARAMETER(custSpecials);
    UNREFERENCED_PARAMETER(aNeighborStats);
    ASSERT(aExistingTabValue.GetSize() == 1 || aExistingTabValue.GetSize() == 2);

    bool bChanged = false;
    CStringArray aTabValLabels;
    CArray<TABVAL_TYPE> aTabValTypes;
    GetTabValLabelsAndTypes(aTabValLabels, aTabValTypes, pVSet, pForeignKeys);

    for (int i = 0; i < aTabValLabels.GetCount(); ++i) {
        CTabValue *pTabVal;
        if (i < aExistingTabValue.GetCount()) {
            // reuse existing tab val
            pTabVal = aExistingTabValue.GetAt(i);
            CDataCellFmt* pDataCellFmt =  pTabVal->GetDerFmt();
            if((pDataCellFmt && !pDataCellFmt->IsTextCustom())|| !pDataCellFmt ){
                if (pTabVal->GetText() != aTabValLabels.GetAt(i)) {
                    bChanged = true;
                    pTabVal->SetText(aTabValLabels.GetAt(i));
                }
            }
            if (pTabVal->GetTabValType() != aTabValTypes.GetAt(i)) {
                pTabVal->SetTabValType(aTabValTypes.GetAt(i));
                // fix up num decimals if we change from percent to proportion or vice versa
                if (pDataCellFmt && pDataCellFmt->GetNumDecimals() != GetDefaultNumDecimals(pTabVal)) {
                    pDataCellFmt->SetNumDecimals(GetDefaultNumDecimals(pTabVal));
                }
            }
            aExistingTabValue.SetAt(i, NULL);
        }
        else {
            // didn't have a tab val before - create a new one -
            // changing from prop w/out total to one w. total
            bChanged = true;
            pTabVal = new CTabValue();
            pTabVal->SetText(aTabValLabels.GetAt(i));
            pTabVal->SetTabValType(aTabValTypes.GetAt(i));
       }
       pTabVal->SetStatId(iStatIndex);
       aTabValue.Add(pTabVal);
    }

    return bChanged;
}

//
// CTallyVarStatFmtFactory
//

CTallyVarStatFmtFactory* CTallyVarStatFmtFactory::GetInstance()
{
    static CTallyVarStatFmtFactory theInstance;
    return &theInstance;
}

// create new stat object from TFT string
CTallyVarStatFmt* CTallyVarStatFmtFactory::Create(LPCTSTR sTFTCommand, double dVarMin, double dVarMax)
{
    if (_tcscmp(sTFTCommand, _T("Counts")) == 0) {
        return new CTallyVarStatFmtCounts;
    }
    else if (_tcscmp(sTFTCommand, _T("Total")) == 0) {
        return new CTallyVarStatFmtTotal;
    }
    else if (_tcscmp(sTFTCommand, _T("Mean")) == 0) {
        return new CTallyVarStatFmtMean;
    }
    else if (_tcscmp(sTFTCommand, _T("Percents")) == 0) {
        return new CTallyVarStatFmtPercent;
    }
    else if (_tcscmp(sTFTCommand, _T("Total Percent")) == 0) {
        return new CTallyVarStatFmtTotalPercent;
    }
    else if (_tcscmp(sTFTCommand, _T("Median")) == 0) {
        return new CTallyVarStatFmtMedian(dVarMin, dVarMax);
    }
    else if (_tcscmp(sTFTCommand, _T("Mode")) == 0) {
        return new CTallyVarStatFmtMode;
    }
    else if (_tcscmp(sTFTCommand, _T("Standard Deviation")) == 0) {
        return new CTallyVarStatFmtStdDeviation;
    }
    else if (_tcscmp(sTFTCommand, _T("Variance")) == 0) {
        return new CTallyVarStatFmtVariance;
    }
    else if (_tcscmp(sTFTCommand, _T("Minimum")) == 0) {
        return new CTallyVarStatFmtMin;
    }
    else if (_tcscmp(sTFTCommand, _T("Maximum")) == 0) {
        return new CTallyVarStatFmtMax;
    }
    else if (_tcscmp(sTFTCommand, _T("N-tiles")) == 0) {
        return new CTallyVarStatFmtNTiles(dVarMin, dVarMax);
    }
    else if (_tcscmp(sTFTCommand, _T("Proportion")) == 0) {
        return new CTallyVarStatFmtProportion;
    }
    //Savy (R) sampling app 20081208
    else if (_tcscmp(sTFTCommand, _T("R")) == 0) {
        return new CSampStatFmtR;
    }
    else if (_tcscmp(sTFTCommand, _T("SE")) == 0) {
        return new CSampStatFmtSE;
    }
    else if (_tcscmp(sTFTCommand, _T("N-UNWE")) == 0) {
        return new CSampStatFmtNUNWE;
    }
    else if (_tcscmp(sTFTCommand, _T("N-WEIG")) == 0) {
        return new CSampStatFmtNWEIG;
    }
    else if (_tcscmp(sTFTCommand, _T("SER")) == 0) {
        return new CSampStatFmtSER;
    }
    else if (_tcscmp(sTFTCommand, _T("SD")) == 0) {
        return new CSampStatFmtSD;
    }
    else if (_tcscmp(sTFTCommand, _T("DEFT")) == 0) {
        return new CSampStatFmtDEFT;
    }
    else if (_tcscmp(sTFTCommand, _T("ROH")) == 0) {
        return new CSampStatFmtROH;
    }
    else if (_tcscmp(sTFTCommand, _T("SE/R")) == 0) {
        return new CSampStatFmtSESR;
    }
    else if (_tcscmp(sTFTCommand, _T("R-2SE")) == 0) {
        return new CSampStatFmtRNSE;
    }
    else if (_tcscmp(sTFTCommand, _T("R+2SE")) == 0) {
        return new CSampStatFmtRPSE;
    }
    else if (_tcscmp(sTFTCommand, _T("SAMP_BASE")) == 0) {
        return new CSampStatFmtSampBase;
    }
    else if (_tcscmp(sTFTCommand, _T("B")) == 0) {
        return new CSampStatFmtB;
    }
    else {
        ASSERT(!_T("Invalid type for CTallyVarStatFmtFactory"));
        return NULL;
    }
}
